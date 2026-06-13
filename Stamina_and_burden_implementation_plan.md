# Stamina & Burden SKSE — Implementation Plan

## Overview

An SKSE plugin for Skyrim AE that overhauls stamina into a burden- and weight-driven resource. All stamina costs and regeneration rates are percentage-based (of max stamina), making burden management the primary lever rather than total stamina pool size.

**Key pillars:**

- **Burden** (equipped + total) drives stamina economy
- **Cross-AV interplay** (health/stamina/magicka affect each other's regen)
- **Smooth continuum** via `Math::Interpolate` — no discrete tiers
- **Universal** — applies to all actors (with targeted exceptions for performance)

---

## 1. Source Module Layout

```
src/
├── Common/           # (existing) PCH, Utils.h
├── Data/             # (existing) ModObjectManager, Lookup.h
├── Export/           # (existing) SKSEPlugin.cpp — entrypoint
├── Hooks/            # (existing) Hooks.h/.cpp — hook install
├── Papyrus/          # (existing) Papyrus.h/.cpp — native bindings
├── RE/               # (existing) Offset.h — REL::ID constants
├── Serialization/    # (existing) Serde.h/.cpp — serializable base
├── Settings/
│   ├── INI/          # (existing) — will be replaced by SettingsRegistry
│   └── JSON/         # (existing) JSON settings reader
│
├── Config/           # NEW — replaces INI settings module
│   ├── SettingsRegistry.h
│   └── SettingsRegistry.cpp
├── Console/          # NEW — runtime debug/query commands
│   ├── ConsoleCommands.h
│   └── ConsoleCommands.cpp
├── Burden/           # NEW
│   ├── BurdenManager.h
│   └── BurdenManager.cpp
├── Regen/            # NEW
│   ├── RegenManager.h
│   └── RegenManager.cpp
├── Actions/          # NEW
│   ├── ActionManager.h
│   └── ActionManager.cpp
├── Blocking/         # NEW
│   ├── BlockManager.h
│   └── BlockManager.cpp
├── Combat/           # NEW
│   ├── CombatManager.h
│   └── CombatManager.cpp
└── Weather/          # NEW
    ├── WeatherManager.h
    └── WeatherManager.cpp
```

---

## 2. Configuration Architecture

### 2.1 Design

Single source of truth at all times. No dual-file ambiguity.

```
C++ hardcoded defaults (static constexpr)
    │
    ▼
SettingsRegistry (in-memory, queried by all modules)
    │                        ▲
    │                        │
    ▼                        │
User INI (read at startup ───┘
          written on each Set())
```

- **Defaults** are `static constexpr` arrays in `SettingsRegistry.h` — never shipped as a file
- **Single INI** `Data/SKSE/Plugins/StaminaAndBurden_Settings.ini` — created/updated on first runtime change, read at startup
- **Console commands** and **future ImGui menu** both go through `SettingsRegistry::Set()`, which writes to INI synchronously
- **No `StaminaAndBurden.ini` file** — deleted. No `StaminaAndBurden_Custom.ini` — unnecessary. One file, one purpose.
- **Reset:** delete the INI file → next startup loads pure C++ defaults

### 2.2 SettingsRegistry class

```cpp
class SettingsRegistry : public REX::Singleton<SettingsRegistry> {
public:
    float Get(std::string_view key) const;
    void  Set(std::string_view key, float value);  // runtime + persist

    void LoadFromINI();  // called at startup
    void DumpAll() const;   // for sb_list / debug

private:
    struct Setting {
        std::string key;
        std::string section;
        float defaultValue;
        float currentValue;
    };

    std::vector<Setting> settings;
};
```

`Set()` calls `SaveToINI()` which writes only the user-changed values (sparse). The INI path is relative: `Data/SKSE/Plugins/StaminaAndBurden_Settings.ini`.

### 2.3 Console Commands (Papyrus-backed)

Add to the existing `TestCommands.yaml` infrastructure:

| Command        | Args            | Effect                                                                     |
|----------------+-----------------+----------------------------------------------------------------------------|
| `sb_get`       | `<key>`         | Prints current runtime value                                               |
| `sb_set`       | `<key> <value>` | Updates runtime + persists to INI                                          |
| `sb_list`      | —               | Dumps all settings with (default / current)                                |
| `sb_reset`     | —               | Deletes INI, resets all to C++ defaults                                    |
| `sb_getburden` | —               | Dumps current total + equipped burden ratios for debugging burden tracking |

### 2.4 Future: SKSE Menu Framework GUI (optional)

The `SettingsRegistry` is designed to be UI-agnostic. An ImGui-based settings page (using mod 120352) would:

```
ImGui slider/toggle onChange
    │
    ▼
SettingsRegistry::Set(key, value)
    │
    ├── updates currentValue in memory
    └── writes StaminaAndBurden_Settings.ini
```

No separate storage. No dual-source problem. The INI is always the persisted truth, the registry is always the live truth, and they are kept in sync on every write.

This is deferred until after the core mechanics work. The console commands provide the same capability for testing.

### 2.5 Interaction with existing INI infrastructure

The existing `Settings::INI` system (`INISettings.h/cpp`, `EXPECTED_SETTINGS`) is **replaced** by `SettingsRegistry`. The old files remain in the repo until cleanup at the end.

---

## 3. Module Specifications

### 3.1 BurdenManager

**Purpose:** Tracks equipped and total burden ratio per actor.

**Formulas:**

```
TotalBurdenRatio = clamp(currentWeight / maxCarryWeight, 0.0, 1.0)

EquippedWeightedSum = Σ( item.weight × slotMultiplier )
MaxEquippedWeight   = fEquippedMaxCarryFraction × maxCarryWeight
EquippedBurdenRatio = clamp(EquippedWeightedSum / MaxEquippedWeight, 0.0, 1.0)
```

**Slot weight multipliers** (configurable via INI, defaults in code):

| Slot (BipedObjectSlot)                                    | Default Multiplier | INI Key          |
|-----------------------------------------------------------+--------------------+------------------|
| `kBody`, `kModChestPrimary`                               |               0.80 | `fSlotMult_Body` |
| `kFeet`                                                   |               1.50 | `fSlotMult_Feet` |
| All other slots (head, hands, shield, ring, amulet, etc.) |               1.00 | *(implicit)*     |

**Triggering:** Listens to `TESEquipEvent` (for equipped) and `TESContainerChangedEvent` (for total). For NPCs, lazy-calculates inside hooks that fire per-actor. Caches the last-known `maxCarryWeight` per actor.

**Class sketch:**

```cpp
class BurdenManager : public REX::Singleton<BurdenManager> {
public:
    void UpdateBurden(RE::Actor* actor);
    float GetEquippedBurdenRatio(RE::Actor* actor) const;
    float GetTotalBurdenRatio(RE::Actor* actor) const;

    static float GetSlotMultiplier(RE::BGSBipedObjectForm::BipedObjectSlot slot);

    // Event sinks
    void OnEquip(const RE::TESEquipEvent* event);
    void OnContainerChange(const RE::TESContainerChangedEvent* event);

    // Debug query (for sb_getburden)
    struct DebugInfo {
        float totalRatio, equippedRatio, maxWeight, equippedWeight, weightedSum;
    };
    DebugInfo GetDebugInfo(RE::Actor* actor) const;

private:
    mutable std::unordered_map<RE::ActorHandle, BurdenData> cache;
    struct BurdenData {
        float equippedRatio;
        float totalRatio;
        float maxCarryWeight;
    };
};
```

### 3.2 RegenManager

**Purpose:** Modifies stamina/health/magicka regeneration rates based on burden, cross-AV levels, weather, and exhaustion.

**Formulas (all smooth via `Math::Interpolate`):**

```
staminaRegenMult = Interpolate(RegenStaminaMult_LowBurden,   // at burden=0
                               RegenStaminaMult_HighBurden,  // at burden=1
                               blendedBurdenRatio,           // (equipped+total)/2
                               kStamina)
                × Interpolate(1.0, RegenStaminaMult_LowHealth, 1 - healthRatio, k)
                × Interpolate(1.0, RegenStaminaMult_LowMagicka, 1 - magickaRatio, k)
                × (weatherManager.IsBadWeather(actor) && actor->IsPlayer() ? RegenStaminaMult_BadWeather : 1.0)
                × (isExhausted(actor) ? ExhaustedRegenMult : 1.0)

healthRegenMult  = Interpolate(HealthRegenMult_HighStamina,   // at stamina=100%
                               HealthRegenMult_LowStamina,    // at stamina=0%
                               1 - staminaRatio, kHealth)

magickaRegenMult = Interpolate(MagickaRegenMult_HighStamina,  // at stamina=100%
                               MagickaRegenMult_LowStamina,   // at stamina=0%
                               1 - staminaRatio, kMagicka)
```

**Hook strategy:** Hook `Actor::RestoreActorValue`. When the delta is positive (regen, not damage) and the AV is stamina/health/magicka, scale the delta by the computed multiplier. Also hook the regen condition function to short-circuit when burden is extremely high.

### 3.3 ActionManager

**Purpose:** Calculates and enforces stamina costs for attacks, jumping, running, sprinting, and swimming. Prevents actions when stamina is too low.

**Attack stamina cost formula:**

```
attackCost = maxStamina
           × (fAttackCostBase + weaponWeight × fAttackCostWeightMult)
           × (1 - relevantSkill / fAttackCostSkillDivisor)
           × (1 + fAttackCostBurdenPenalty × totalBurdenRatio)
```

Where `relevantSkill` is the governing skill for the equipped weapon type (OneHanded, TwoHanded, Marksman, etc.). Unarmed uses a configured default.

**Movement stamina costs:** Via game-setting manipulation (Approach B). On burden change events, recalculate and write to the relevant game settings:

```
fJumpStaminaCost         = base × (1 + burdenPenalty × ratio)
fSprintStaminaDrainMult  = base × (1 + burdenPenalty × ratio)
fSwimStaminaCost         = base × (1 + burdenPenalty × ratio)
```

Recalculation happens only on burden change events, not per frame.

*(Flagged for potential migration to per-actor hooks if game-setting approach has limitations.)*

**Action lockout:** Before any action (attack, jump, block), check:

```
if currentStamina < fActionLockCostFraction × actionCost → prevent action
```

**Hook points:**
- `Character::sub_140627930` (REL::ID `38603`) — zero vanilla cost, inject own
- `AttackAction` (REL::ID `49170`) — prevent attack when exhausted/locked
- Game settings `fJumpStaminaCost`, `fSprintStaminaDrainMult`, etc. — update on burden change

### 3.4 BlockManager

**Purpose:** Redirects blocked damage from health to stamina. Manages guard breaks.

**Damage split (fixed ratios, configurable):**

```
Weapon block:   0.30 × rawDmg → health      0.70 × rawDmg → stamina
Shield block:   0.10 × rawDmg → health      0.90 × rawDmg → stamina
```

**Stamina consumption after split:**

```
actualStaminaDmg = staminaPortion
                 × (1 + fBlockBurdenPenalty × totalBurdenRatio)
                 × (1 - blockSkill / fBlockSkillEfficiencyDivisor)
```

Higher block skill = less stamina consumed per blocked hit. The health portion passes through unchanged (player still takes chip damage).

**Guard break:** If `remainingStamina < actualStaminaDmg`:

```
excessDmg = actualStaminaDmg - remainingStamina
damageToHealth = healthPortion + excessDmg
set stamina to 0
trigger stagger animation ("staggerStart")
trigger exhaustion (see §3.7)
```

**Hook point:** `Character::sub_140628C20` (REL::ID `38627`) — inspect `HitData.flags` for `kBlocked`.

### 3.5 CombatManager

**Purpose:** Scales attack damage based on current stamina level and exhaustion state.

**Stamina-dependent damage:**

```
dmgMult = Interpolate(AttackDmgMult_HighStamina,   // at stamina=100%
                      AttackDmgMult_LowStamina,    // at stamina=0%
                      1 - staminaRatio, k)

finalDamage = baseDamage × dmgMult × (isExhausted ? ExhaustedDamageMult : 1.0)
```

**Hook point:** Same hit-processing hook as BlockManager (`38627`), applied to all melee hits (not just blocked ones). Damage is modified before being applied.

### 3.6 WeatherManager

**Purpose:** Detects if the current weather qualifies as "bad" for regen penalty purposes.

**Implementation:** Queries `RE::Sky::GetSingleton()->GetCurrentWeather()` and checks the precipitation type (rain, snow, etc.).

**Scope:** Player only (NPCs skip this check per §4).

```cpp
class WeatherManager : public REX::Singleton<WeatherManager> {
public:
    bool IsBadWeather(RE::Actor* actor) const;
    // Player-only: checks local weather
};
```

### 3.7 Exhaustion System

**Trigger:** When stamina is fully depleted (hits 0 from any source).

**State machine:**

```
Normal ──(stamina = 0)──▶ Exhausted ──(duration expires)──▶ Normal
                            │
                            ├── staminaRegenMult × fExhaustedRegenMult (0.5)
                            ├── attackDmgMult   × fExhaustedDamageMult (0.6)
                            └── duration: fExhaustionDuration seconds (default 7.0)
```

**Action prevention:** Any action where `currentStamina < fActionLockCostFraction × actionCost` is silently prevented.

**Implementation:** Per-actor exhaustion end timestamp (`GameHours`). Checked in:
- `ActionManager` (prevent attack/jump/block)
- `RegenManager` (reduce regen)
- `CombatManager` (reduce damage)

**Cleanup:** On cell unload or actor death, remove from exhaustion map.

---

## 4. Hook Summary

| ID                | Function                   | Module               | Technique       | Purpose                                         |
|-------------------+----------------------------+----------------------+-----------------+-------------------------------------------------|
| `38603`           | `Character::sub_140627930` | ActionManager        | `write_call<5>` | Attack stamina cost → override                  |
| `38452`           | `ActorState::HasFlags1`    | RegenManager         | `write_call<5>` | Modify regen condition return                   |
| vtable or detour  | `Actor::RestoreActorValue` | RegenManager         | detour          | Scale regen deltas                              |
| `38627`           | `Character::sub_140628C20` | Block/Combat Manager | `write_call<5>` | Hit processing → stamina redirect + dmg scaling |
| `49170`           | `AttackAction`             | ActionManager        | `write_call<5>` | Prevent action when locked                      |
| *(event sink)*    | `TESEquipEvent`            | BurdenManager        | Event sink      | Trigger burden recalc                           |
| *(event sink)*    | `TESContainerChangedEvent` | BurdenManager        | Event sink      | Trigger burden recalc                           |
| *(game settings)* | `fJumpStaminaCost` etc.    | ActionManager        | Direct write    | Movement costs on burden change                 |

---

## 5. Data Flow

```
Game Event (equip / container change)
    │
    ▼
BurdenManager::UpdateBurden(actor)
    │
    ├──► RegenManager: modify regen at next tick
    │       └── hook: RestoreActorValue + regen condition
    │
    ├──► ActionManager: recalc attack/movement costs
    │       ├── hook 38603: attack stamina cost
    │       ├── hook 49170: lock exhausted actions
    │       └── direct write: movement game settings
    │
    ├──► BlockManager: stamina redirect on blocked hits
    │       └── hook 38627 (blocked path)
    │           └── guard break → exhaustion
    │
    └──► CombatManager: damage scaling
            └── hook 38627 (all hits)
                └── stamina-conditional + exhaustion scaling
```

---

## 6. Papyrus Interface

Extend `Data/Source/Scripts/EC_StaminaAndBurden.psc`:

```
Scriptname EC_StaminaAndBurden

Int[] Function GetVersion() Global Native
float Function GetEquippedBurdenRatio() Global Native
float Function GetTotalBurdenRatio() Global Native
float Function GetCurrentStaminaRegenMult() Global Native
```

The existing `UnitTest_Serialization` declarations are removed (or implemented only in debug builds).

**Fix:** `Data/SKSE/CustomConsole/TestCommands.yaml` — change `SEA_TemplateProject` references to `EC_StaminaAndBurden`.

**Console commands** (via TestCommands.yaml):

| Command                | Native function         | Effect                       |
|------------------------+-------------------------+------------------------------|
| `sb_get <key>`         | `Console_GetSetting`    | Print current value          |
| `sb_set <key> <value>` | `Console_SetSetting`    | Set + persist                |
| `sb_list`              | `Console_ListSettings`  | Dump all                     |
| `sb_reset`             | `Console_ResetSettings` | Delete INI, restore defaults |
| `sb_getburden`         | `Console_GetBurden`     | Debug burden values          |

---

## 7. Serialization

Reserve ID `'EXHD'` for a `BurdenSerde` class inheriting `Serialization::Serializable`. Currently a no-op placeholder for future MCM-persistent state.

```cpp
class BurdenSerde : public Serialization::Serializable {
    bool Save(SKSE::SerializationInterface*) override { return true; }
    bool Load(SKSE::SerializationInterface*) override { return true; }
    bool Revert(SKSE::SerializationInterface*) override { return true; }
};
```

Registered in `SKSEPlugin_Load` via `Register(ID)`.

---

## 8. All Settings (hardcoded defaults in SettingsRegistry)

Default values embedded in C++ code. No shipped INI file.

```cpp
// Section: Burden
{ "fEquippedMaxCarryFraction",    "Burden",         0.4f   },
{ "fTotalBurdenCurve_k",          "Burden",         0.5f   },
{ "fEquippedBurdenCurve_k",       "Burden",         0.5f   },
{ "fSlotMult_Body",               "Burden.SlotWeights", 0.80f },
{ "fSlotMult_Feet",               "Burden.SlotWeights", 1.50f },

// Section: Regen.Stamina
{ "fStaminaRegenMult_LowBurden",  "Regen.Stamina",  1.5f   },
{ "fStaminaRegenMult_HighBurden", "Regen.Stamina",  0.5f   },
{ "fStaminaRegenMult_LowHealth",  "Regen.Stamina",  0.5f   },
{ "fStaminaRegenMult_LowMagicka", "Regen.Stamina",  0.5f   },
{ "fStaminaRegenMult_BadWeather", "Regen.Stamina",  0.75f  },
{ "fStaminaRegenCurve_k",         "Regen.Stamina",  0.5f   },

// Section: Regen.Health
{ "fHealthRegenMult_LowStamina",  "Regen.Health",   0.5f   },
{ "fHealthRegenMult_HighStamina", "Regen.Health",   1.0f   },
{ "fHealthRegenCurve_k",          "Regen.Health",   0.5f   },

// Section: Regen.Magicka
{ "fMagickaRegenMult_LowStamina", "Regen.Magicka",  0.5f   },
{ "fMagickaRegenMult_HighStamina","Regen.Magicka",  1.0f   },
{ "fMagickaRegenCurve_k",         "Regen.Magicka",  0.5f   },

// Section: Actions.Attack
{ "fAttackCostBase",              "Actions.Attack", 0.05f  },
{ "fAttackCostWeightMult",        "Actions.Attack", 0.03f  },
{ "fAttackCostSkillDivisor",      "Actions.Attack", 200.0f },
{ "fAttackCostBurdenPenalty",     "Actions.Attack", 0.5f   },
{ "fActionLockCostFraction",      "Actions.Attack", 0.2f   },

// Section: Actions.Movement
{ "fMovementCostBurdenPenalty",   "Actions.Movement", 0.3f },
{ "fJumpCostBase",                "Actions.Movement", 0.02f },
{ "fSprintCostBase",              "Actions.Movement", 0.01f },
{ "fSwimCostBase",                "Actions.Movement", 0.015f},
{ "fRunCostBase",                 "Actions.Movement", 0.005f},

// Section: Blocking
{ "fBlockStaminaSplit_Weapon",    "Blocking",       0.70f  },
{ "fBlockStaminaSplit_Shield",    "Blocking",       0.90f  },
{ "fBlockBurdenPenalty",          "Blocking",       0.3f   },
{ "fBlockSkillEfficiencyDivisor", "Blocking",       200.0f },

// Section: Exhaustion
{ "fExhaustedRegenMult",          "Exhaustion",     0.5f   },
{ "fExhaustedDamageMult",         "Exhaustion",     0.6f   },
{ "fExhaustionDuration",          "Exhaustion",     7.0f   },

// Section: Combat
{ "fAttackDmgMult_LowStamina",    "Combat",         0.5f   },
{ "fAttackDmgMult_HighStamina",   "Combat",         1.0f   },
{ "fAttackDmgCurve_k",            "Combat",         0.5f   },
```

---

## 9. Implementation Phases

### Phase 1 — Burden Foundation

| Task                                                                              | Files                                                                            |
|-----------------------------------------------------------------------------------+----------------------------------------------------------------------------------|
| Implement `SettingsRegistry` with hardcoded defaults + INI persistence            | `src/Config/SettingsRegistry.h/.cpp`                                             |
| Implement `BurdenManager` (total + equipped tracking only, no slot weighting yet) | `src/Burden/BurdenManager.h/.cpp`                                                |
| Register equip/container event sinks                                              | `src/Export/SKSEPlugin.cpp`, `src/Hooks/Hooks.cpp`                               |
| Add `sb_getburden` console command (Papyrus native + TestCommands.yaml entry)     | `src/Console/ConsoleCommands.h/.cpp`, `Papyrus/Papyrus.cpp`, `TestCommands.yaml` |
| Verify burden values in-game before proceeding                                    | *(manual test)*                                                                  |

### Phase 1b — Weighted Burden

| Task                                                 | Files                             |
|------------------------------------------------------+-----------------------------------|
| Add slot multiplier map + `GetSlotMultiplier()`      | `src/Burden/BurdenManager.cpp`    |
| Update `sb_getburden` to show weighted vs unweighted | `src/Console/ConsoleCommands.cpp` |
| Verify weighted burden in-game                       | *(manual test)*                   |

### Phase 2 — Runtime Configuration

| Task                                                         | Files                                            |
|--------------------------------------------------------------+--------------------------------------------------|
| Add `sb_get`/`sb_set`/`sb_list`/`sb_reset` console commands  | `src/Console/ConsoleCommands.cpp`, `Papyrus.cpp` |
| Wire `SettingsRegistry::LoadFromINI()` into startup sequence | `src/Export/SKSEPlugin.cpp`                      |
| Wire `SettingsRegistry::SaveToINI()` into every `Set()`      | `src/Config/SettingsRegistry.cpp`                |

### Phase 3 — Regen

| Task                                                                         | Files                           |
|------------------------------------------------------------------------------+---------------------------------|
| Implement `RegenManager` (formulas, queries SettingsRegistry for all values) | `src/Regen/RegenManager.h/.cpp` |
| Hook `Actor::RestoreActorValue`                                              | `src/Hooks/Hooks.cpp`           |
| Hook regen condition function (`38452`)                                      | `src/Hooks/Hooks.cpp`           |
| Wire burden ratios + cross-AV + weather + exhaustion into regen mult         | `src/Regen/RegenManager.cpp`    |

### Phase 4 — Attack Costs

| Task                                             | Files                              |
|--------------------------------------------------+------------------------------------|
| Implement `ActionManager` for attacks            | `src/Actions/ActionManager.h/.cpp` |
| Hook `38603` (attack stamina cost)               | `src/Hooks/Hooks.cpp`              |
| Hook `49170` (action prevention at low stamina)  | `src/Hooks/Hooks.cpp`              |
| Implement weapon weight + skill + burden formula | `src/Actions/ActionManager.cpp`    |

### Phase 5 — Movement Costs

| Task                                              | Files                           |
|---------------------------------------------------+---------------------------------|
| Add movement cost calculation to `ActionManager`  | `src/Actions/ActionManager.cpp` |
| Game-setting manipulation on burden change events | `src/Actions/ActionManager.cpp` |
| *(Marked for potential hook migration later)*     |                                 |

### Phase 6 — Blocking

| Task                                                     | Files                              |
|----------------------------------------------------------+------------------------------------|
| Implement `BlockManager` (stamina redirect, guard break) | `src/Blocking/BlockManager.h/.cpp` |
| Hook `38627` (hit processing, blocked path)              | `src/Hooks/Hooks.cpp`              |
| Wire exhaustion trigger on guard break                   | `src/Blocking/BlockManager.cpp`    |

### Phase 7 — Cross-Effects + Exhaustion

| Task                                                                     | Files                             |
|--------------------------------------------------------------------------+-----------------------------------|
| Implement `CombatManager` (stamina-conditional + exhaustion dmg scaling) | `src/Combat/CombatManager.h/.cpp` |
| Hook stamina-conditional damage scaling into `38627` (all hits)          | `src/Hooks/Hooks.cpp`             |
| Implement exhaustion state machine (timed duration)                      | `src/Blocking/BlockManager.cpp`   |
| Cross-AV regen wiring (already in `RegenManager`)                        | *(reuse Phase 3)*                 |
|                                                                          |                                   |

### Phase 8 — WeatherManager

| Task                                                          | Files                               |
|---------------------------------------------------------------+-------------------------------------|
| Implement `WeatherManager` (precipitation check, player-only) | `src/Weather/WeatherManager.h/.cpp` |
| Wire into `RegenManager`                                      | `src/Regen/RegenManager.cpp`        |

### Phase 9 — Papyrus + Polish

| Task                                                                | Files                                         |
|---------------------------------------------------------------------+-----------------------------------------------|
| Bind query functions in `Papyrus.cpp`                               | `src/Papyrus/Papyrus.h/.cpp`                  |
| Update `.psc` script                                                | `Data/Source/Scripts/EC_StaminaAndBurden.psc` |
| Remove stale `UnitTest_Serialization` declarations                  | `.psc` + `Papyrus.cpp`                        |
| Fix `TestCommands.yaml` (SEA_TemplateProject → EC_StaminaAndBurden) | `TestCommands.yaml`                           |
| Implement `BurdenSerde` serialization (placeholder)                 | `src/Serialization/Serde.h/.cpp`              |
| Clean up old `Settings::INI` files                                  | `src/Settings/INI/`                           |
| Remove `StaminaAndBurden.ini` shipped file                          | `Data/SKSE/Plugins/StaminaAndBurden.ini`      |
| Final settings tuning pass                                          | `SettingsRegistry` defaults                   |

---

## 10. Scope by Actor Type

| Feature                | Player                   | NPCs                     |
|------------------------+--------------------------+--------------------------|
| Burden tracking        | ✓ (event-driven, cached) | ✓ (lazy per-hook)        |
| Regen modification     | ✓ (full formula)         | ✓ (no weather component) |
| Weather penalty        | ✓                        | ✗                        |
| Attack cost + lockout  | ✓                        | ✓                        |
| Movement cost          | ✓ (via game settings)    | ✓ (via game settings)    |
| Block stamina redirect | ✓                        | ✓                        |
| Exhaustion             | ✓                        | ✓                        |
| Attack damage scaling  | ✓                        | ✓                        |

---

## 11. Open Items (future refinement)

1. **Movement costs**: Game-setting manipulation affects all actors equally. If per-actor differentiation is needed, migrate from game-setting writes to per-actor function hooks.

2. **Slot multiplier granularity**: Currently 2 overridable slots (Body, Feet) + default 1.0. Expandable later with more keys.

3. **Exhaustion recovery condition**: Timed (configurable). Could also trigger on full stamina recharge — revisit if timed feels wrong.

4. **Block skill → split ratio**: Currently skill only reduces stamina *consumption*, not the health/stamina split ratio. Add `fBlockSkillSplitShift` if you want expert blockers to divert even more damage to stamina.

5. **SKSE Menu Framework GUI**: Deferred. The `SettingsRegistry` + console commands provide all the runtime testing capability needed. An ImGui page can be added later without architectural changes.

6. **MCM integration**: Not in scope. The ImGui approach (SKSE Menu Framework) is preferred over MCM Helper (which requires an ESP).
