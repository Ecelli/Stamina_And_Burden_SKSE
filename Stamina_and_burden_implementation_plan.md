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
│   └── Print/         # (existing) debug output helpers
├── Data/             # (existing) ModObjectManager, Lookup.h
├── Export/           # (existing) SKSEPlugin.cpp — entrypoint
├── Hooks/            # (existing) Hooks.h/.cpp — hook install
├── Papyrus/          # (existing) Papyrus.h/.cpp — native bindings
├── RE/               # (existing) Offset.h — REL::ID constants
├── Serialization/    # (existing) Serde.h/.cpp — serializable base
├── Settings/
│   ├── INI/          # (existing) — will be replaced by SettingsRegistry
│   ├── JSON/         # (existing) JSON settings reader
│   └── Params/       # (existing) Parameter<T> + typed structs
│
├── Config/           # NEW — replaces INI settings module
│   ├── SettingsRegistry.h
│   └── SettingsRegistry.cpp
├── Console/          # NEW — runtime debug/query commands
│   ├── ConsoleCommands.h
│   └── ConsoleCommands.cpp
├── Burden/           # (existing) burden computation + tracker
│   ├── BurdenManager.h
│   └── BurdenManager.cpp
├── Regen/            # (existing) regen formulas + costs
│   ├── RegenManager.h
│   ├── RegenManager.cpp
│   ├── CostsManager.h
│   └── CostsManager.cpp
├── Actions/          # NEW
│   ├── ActionManager.h
│   └── ActionManager.cpp
├── Blocking/         # NEW
│   ├── BlockManager.h
│   └── BlockManager.cpp
└── Combat/           # NEW
    ├── CombatManager.h
    └── CombatManager.cpp
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

### 2.6 Current state (before SettingsRegistry)

Burden parameters currently use `Parameter<T>` via `BurdenParams` singleton (`src/Settings/Params/BurdenParams.h`) with typed `ForEach()` export and the legacy `Settings::INI` reader. The `StaminaAndBurden.ini` shipped file still exists at `Data/SKSE/Plugins/`. The `SettingsRegistry` design (§2) is deferred and will replace both when introduced.

---

## 3. Module Specifications

### 3.1 BurdenManager

**Purpose:** Tracks equipped and total burden ratio per actor.

**Architecture:** Two namespaces:

- `Burden::` — burden computation functions (`UpdateBurden`, `ComputeEquipmentBurden`, `GetSlotMultiplier`, `GetWeightedArmorTypeMult`)
- `Burden::Tracker` — actor registry (`Register`, `Unregister`, `Update`, `IsTracked`, `OnGameLoad`) with a `FormID`-keyed map of `ActorBurdenData`

**Formulas:**

```
TotalBurdenRatio    = clamp(currentWeight / maxCarryWeight, 0.0, 1.0)

EquippedWeightedSum = Σ( item.weight × slotMult × armorTypeSkillMult )
MaxEquippedWeight   = fmaxEquippedWeightRatio × maxCarryWeight
EquippedBurdenRatio = clamp(EquippedWeightedSum / MaxEquippedWeight, 0.0, 1.0)
```

**Slot weight multipliers** (configurable, stored as `Parameter<float>` in `BurdenParams`):

| Slot                              | Default | Param Name              |
|-----------------------------------+---------+-------------------------|
| Body (kBody / kModChestPrimary)   | 0.70    | `fSlotBurdenMult_body`  |
| Head                              | 1.20    | `fSlotBurdenMult_head`  |
| Hands                             | 0.80    | `fSlotBurdenMult_hand`  |
| Feet                              | 1.50    | `fSlotBurdenMult_feet`  |
| All other slots                   | 1.00    | `fSlotBurdenMult_def`   |

**Armor type skill-weighted multiplier:**

For each equipped armor piece, burden is scaled by a skill-interpolated factor:

```
skillRatio  = 1.0 - clamp(skillValue / iPlayerMaxSkill, 0.0, 1.0)
skillMult   = Interpolate(minMult, maxMult, skillRatio, fSkillInterpolate)
```

`skillValue` = current Light Armor or Heavy Armor skill (depending on piece). At 0 skill → `maxMult` (highest burden), at 100 skill → `minMult` (lowest burden). Parameters: `fSkillBurdenMult_minLight`/`maxLight`, `fSkillBurdenMult_minHeavy`/`maxHeavy`.

**Steed Stone:** When the Steed Stone ability spell (`doomSteedAbility`) is active, all slot multipliers are multiplied by `fSteedStoneBurdenMult` (default 0.30). Detected via `ActiveEffect::spell == steedStoneAbility` each `UpdateBurden()` call.

**Triggering (3 triggers):**

1. **Event sinks** — `TESEquipEvent` (equip/unequip) and `TESContainerChangedEvent` (pick up/drop/transfer) call `Tracker::Update(actor)`, which defers a full `UpdateBurden()` via `AddTask` to next frame.

2. **Heartbeat polling** — A background thread wakes every 200ms and dispatches `TaskTrackBurdenParams()` via `AddTask`. For each tracked actor, reads `GetActorValue(kCarryWeight)`, `kLightArmor`, and `kHeavyArmor` and compares against cached values in `ActorBurdenData`. Uses short-circuit `||` — stops reading once a change is found.

3. **Game load** — `TESLoadGameEvent` (via `LoadGameHandler`) → `Tracker::OnGameLoad()` clears the map, re-registers the player, and starts the heartbeat thread (once via `static bool` guard).

**Data struct (`ActorBurdenData` in `BurdenManager.h`):**

```cpp
struct ActorBurdenData {
    float maxCarryWeight{ 0.0f };      // GetActorValue(kCarryWeight)
    float carryWeight{ 0.0f };          // current inventory weight
    float equippedWeight{ 0.0f };       // weighted equipped sum
    float maxEquippedWeight{ 0.0f };    // fmaxEquippedWeightRatio × maxCarryWeight
    float carryBurden{ 0.0f };          // carryWeight / maxCarryWeight (clamped)
    float burden{ 0.0f };               // equippedWeight / maxEquippedWeight (clamped)
    int   lightSkill{ -1 };             // cached for heartbeat comparison
    int   heavySkill{ -1 };             // cached for heartbeat comparison
};
```

**Computation flow (`UpdateBurden`):**

1. Read `GetActorValue(kCarryWeight)` → `maxCarryWeight`
2. Read `GetInventoryChanges()->GetInventoryWeight()` → `carryWeight`
3. Run `ComputeEquipmentBurden(actor)` → `VisitWornItems` with `BurdenEquipVisitor` → `equippedWeight`
4. `maxEquippedWeight = fmaxEquippedWeightRatio * maxCarryWeight`
5. Clamp ratios → `carryBurden`, `burden`
6. Read `GetActorValue(kLightArmor)` + `GetActorValue(kHeavyArmor)` → cache `lightSkill`, `heavySkill`

**Parameters** live in `BurdenParams` (REX::Singleton), exported via `ForEach(F&&)` with typed key names (`f`/`i` prefix convention).

### 3.2 RegenManager

**Purpose:** Modifies stamina/health/magicka regeneration rates based on burden, cross-AV levels, weather, and exhaustion.

**Stamina regen formula (per-movement-state curves):**

```
burdenBlend      = 1 - sqrt((1 - burden) × (1 - carryBurden))      [cached in ActorBurdenData]
stateFactor      = Interpolate(maxState × HMS, minState, burdenBlend, k_movement)
blockCost        = fBlockRegenCostBurdenPerc × burdenBlend
weatherPenalty   = ComputeWeatherPenalty(actor)                      [player-only, exterior-only]

result           = stateFactor - blockCost - weatherPenalty
```

Movement states (each with configurable `min`/`max`, shared curve `k`): static, walking, sneaking, running, swimming. Sprinting returns 0.0 — cost handled by SprintDrainHook.

HMS scales only the **ceiling** (maxState). Low cross-AV stats never drag the floor lower.

**Blocking** is a compound penalty (orthogonal axis — can block while walking).

**Weather** is a flat penalty, applied consistently to regen and sprint cost via `engineRate × weatherPenalty` (where `engineRate` is a clone of the game's stamina regen function).

**Health/Magicka regen:** Each depends on stamina % only (cross-AV from stamina → health/magicka), via `Interpolate(Low, High, staminaPct, k)`.

**Hook strategy:** `write_call<5>` on `REL::ID(38452) + 0x2B6` intercepts the engine's internal AVRegen rate function. The Thunk calls the original to get the base regen rate, multiplies by our formula, and returns the modified `__m128`. If the formula returns a negative rate, `DamageActorValue()` drains the actor directly and the returned rate is 0.0. A 100ms heartbeat (started in `BurdenTracker::OnGameLoad`) detects full-stamina + negative-multiplier and drains 0.1 to push below 100%, allowing regen ticks to fire.

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

**Movement stamina costs:**

| Type | Mechanism | Status |
|------|-----------|--------|
| Sprint | SprintDrainHook (ID 38022 + 0xC1/0xC9, `write_call<5>`) | ✅ Implemented |
| Run/walk/sneak/swim continuous drain | Encoded in regen curves (negative stateFactor = drain) | ✅ Implemented |
| Jump | Game-setting manipulation on burden change | ❌ Future |

**Action lockout:** Before any action (attack, jump, block), check:

```
if currentStamina < fActionLockCostFraction × actionCost → prevent action
```

**Hook points:**
- `Character::sub_140627930` (REL::ID `38603`) — zero vanilla cost, inject own
- `AttackAction` (REL::ID `49170`) — prevent attack when exhausted/locked
- Game settings `fJumpStaminaCost` — update on burden change

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

**Implementation:** Inline free function in `RegenManager.cpp`. Queries `RE::Sky::GetSingleton()->IsRaining()` / `IsSnowing()`. Parameters in `WeatherParams` struct (`WeatherRainPenalty`, `WeatherSnowPenalty`, `WeatherEnabled` toggle).

**Scope:** Player only, exteriors only (NPCs + interiors skip the check).

```
ComputeWeatherPenalty(actor):
  if not player or interior → return 0
  if !WeatherEnabled → return 0
  if snow → return WeatherSnowPenalty
  if rain → return WeatherRainPenalty
  else → 0
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

| ID                | Function                       | Module               | Technique        | Purpose                                           |
|-------------------+--------------------------------+----------------------+------------------+---------------------------------------------------|
| *(event sink)*    | `TESEquipEvent`                | BurdenManager        | Event sink       | Trigger burden recalc on equip/unequip             |
| *(event sink)*    | `TESContainerChangedEvent`     | BurdenManager        | Event sink       | Trigger burden recalc on pickup/drop/transfer      |
| *(event sink)*    | `TESLoadGameEvent`             | BurdenManager        | Event sink       | Re-register player + start heartbeat on game load  |
| *(heartbeat)*     | `TaskTrackBurdenParams`        | BurdenManager        | 200ms poll       | Detect carry weight + skill changes from any source|
| *(completed)*     | `38452 + 0x2B6`                | RegenManager         | `write_call<5>`  | Intercept AVRegen rate function × formula          |
| *(completed)*     | `TaskPlayerFullStaminaMonitor` | RegenManager         | 100ms poll       | Drain 0.1 stamina when full + negative mult        |
| *(completed)*     | `38022 + 0xC1/0xC9`            | CostsManager         | `write_call<5>` ×2| Sprint stamina drain (burden + weather)            |
| *(future)*        | `38603`                        | ActionManager        | `write_call<5>`  | Attack stamina cost → override                     |
| *(future)*        | `38627`                        | Block/Combat Manager | `write_call<5>`  | Hit processing → stamina redirect + dmg scaling     |
| *(future)*        | `49170`                        | ActionManager        | `write_call<5>`  | Prevent action when locked                          |
| *(future)*        | game settings (jump only)      | ActionManager        | Direct write     | Jump cost on burden change                          |

---

## 5. Data Flow

```
Game Event (equip / container change)
    │
    ▼
BurdenManager::UpdateBurden(actor)
    │  └── caches burdenBlend = 1 - sqrt((1-burden)*(1-carryBurden))
    │
    ├──► RegenManager: modify regen at next tick
    │       └── read burdenBlend → stateFactor × HMS - block - weather
    │
    ├──► SprintDrainHook: sprint stamina cost
    │       └── Costs::CalculateSprintDrain
    │           └── burden + weather × engineRate
    │
    ├──► ActionManager: recalc attack + jump costs
    │       ├── hook 38603: attack stamina cost
    │       └── direct write: fJumpStaminaCost
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

## 5b. HUD Integration — Optional TrueHUD Burden Widget

**Purpose:** Display real-time burden ratios on the player HUD via TrueHUD's mod API.

**Dependency status:** Optional — TrueHUD must be installed by the user. No build-time dependency. Detection via `GetModuleHandle("TrueHUD.dll")` + `GetProcAddress` at `kPostLoad`.

### Module: `src/HUD/`

| File | Purpose |
|---|---|
| `HUD/TrueHUDAPI.h` | Vendored API header (MIT, from TrueHUD repo) |
| `HUD/BurdenWidget.h` | `BurdenWidget` class declaration |
| `HUD/BurdenWidget.cpp` | API detection + special resource bar registration |

### Integration approach: Special Resource Bar

TrueHUD's `RegisterSpecialResourceFunctions` adds an extra colored bar alongside the player's health/magicka/stamina bars. No SWF required.

**Registration flow:**

1. `kPostLoad` → try `TRUEHUD_API::RequestPluginAPI(InterfaceVersion::V4)`
2. If `nullptr` → log "TrueHUD not detected", no-op
3. If valid → `RequestSpecialResourceBarsControl(handle)` → if `OK`, `RegisterSpecialResourceFunctions(handle, getCurrentBurden, getMax, ...)`
4. Callbacks read from `Burden::Tracker::GetActorBurdenData(playerID)`

**Graceful degradation:**

| Scenario | Behavior |
|---|---|
| TrueHUD not installed | No burden bar, everything else works |
| TrueHUD installed, special bar free | Burden bar displayed on player HUD |
| TrueHUD installed, special bar taken | Log warning, no bar, everything else works |

**Parameters** (configured via SettingsRegistry):
| Key | Default | Purpose |
|---|---|---|
| `iBurdenWidgetRefreshMs` | `200` | How often the burden bar value updates |
| *Color config deferred — TrueHUD allows `OverrideBarColor` in later versions* |


### Startup wiring

Add `BurdenWidget::Register()` call in `SKSEPlugin_Load` messaging listener (`kPostLoad` case), alongside existing startup steps.

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

**On hold.** No serialization of burden state is currently needed — burden data is recomputed dynamically on game load. The current serialization ID is `'TRJT'` (from the template project). A future `BurdenSerde` (`'EXHD'`) may be added if save-scoped state becomes necessary.

---

## 8. All Settings (hardcoded defaults in SettingsRegistry)

Setting keys in `Parameter<T>` structs, organized by compile-time group. No shipped INI file. Values and ranges defined in C++ defaults — subject to empirical tuning via curve plotting (planned ImGui overlay).

### Burden (BurdenParams)
  fmaxEquippedWeightRatio
  fSlotBurdenMult_def / fSlotBurdenMult_body / fSlotBurdenMult_feet / fSlotBurdenMult_head / fSlotBurdenMult_hand
  iPlayerMaxSkill
  fSkillInterpolate
  fSkillBurdenMult_minHeavy / fSkillBurdenMult_maxHeavy / fSkillBurdenMult_minLight / fSkillBurdenMult_maxLight
  fSteedStoneBurdenMult

### Regen.Cross-AV (RegenParams)
  fStaminaRegenMult_LowHealth / fStaminaRegenMult_HighHealth
  fStaminaRegenMult_LowStamina / fStaminaRegenMult_HighStamina
  fStaminaRegenMult_LowMagicka / fStaminaRegenMult_HighMagicka
  fStaminaRegenCurve_kStamina / fStaminaRegenCurve_kMagicka / fStaminaRegenCurve_kHealth
  bEnableDebugLogging

### Regen.Movement (RegenMovementParams)
  fRegenStatic_max / fRegenStatic_min
  fRegenWalking_max / fRegenWalking_min
  fRegenSneaking_max / fRegenSneaking_min
  fRegenRunning_max / fRegenRunning_min
  fRegenSwimming_max / fRegenSwimming_min
  fMovementRegenCurve_k
  fBlockRegenCostBurdenPerc

### Regen.Health (RegenParams)
  fHealthRegenMult_LowStamina / fHealthRegenMult_HighStamina
  fHealthRegenCurve_k

### Regen.Magicka (RegenParams)
  fMagickaRegenMult_LowStamina / fMagickaRegenMult_HighStamina
  fMagickaRegenCurve_k

### Weather (WeatherParams)
  fWeatherRainPenalty / fWeatherSnowPenalty / bWeatherEnabled

### Sprint Drain (CostsParams)
  fSprintDrainLowBurden / fSprintDrainHighBurden
  fSprintDrainLowCarryBurdenPct / fSprintDrainHighCarryBurdenPct
  fSprintDrainBurdenCurve_k / fSprintDrainCarryBurdenCurve_k

### Overrides.GameSettings (ParameterOverrides)
  fCombatStaminaRegenRateMult / fCombatHealthRegenRateMult / fCombatMagickaRegenRateMult
  fDamagedStaminaRegenDelay / fDamagedHealthRegenDelay / fDamagedMagickaRegenDelay

---

## 9. Implementation Phases

### Phase 1 — Burden Foundation ✅

| Task                                                                     | Files                                                  |
|--------------------------------------------------------------------------+--------------------------------------------------------|
| Implement `BurdenManager` (slot weighting, skill-weighted armor mult)     | `src/Burden/BurdenManager.h/.cpp`                     |
| Implement `BurdenTracker` (actor registry, deferred Update)               | `src/Burden/BurdenTracker.h/.cpp`                     |
| Register equip/container/gameload event sinks                             | `src/Hooks/Hooks.cpp`, `src/Hooks/BurdenEventHandlers`|
| Implement heartbeat polling (`make_heartbeat` + `TaskTrackBurdenParams`)  | `src/Common/Utils.h`, `src/Burden/BurdenTracker.cpp`  |
| Add `BurdenParams` singleton with typed `ForEach` export                  | `src/Settings/Params/BurdenParams.h`                  |
| Verify burden values in-game before proceeding                            | *(manual test)*                                        |

### Phase 1b — Weighted Burden ✅

| Task                                                 | Files                             |
|------------------------------------------------------+-----------------------------------|
| Add slot multiplier map + `GetSlotMultiplier()`      | `src/Burden/BurdenManager.cpp`    |
| Update `sb_getburden` to show weighted vs unweighted | `src/Console/ConsoleCommands.cpp` |
| Verify weighted burden in-game                       | *(manual test)*                   |

### Phase 2 — Regen ✅

| Task                                                                         | Files                           |
|------------------------------------------------------------------------------+---------------------------------|
| Implement per-movement-state regen curves (static/walk/sneak/run/swim)       | `src/Regen/RegenManager.h/.cpp` |
| Add burdenBlend to ActorBurdenData, compute once per UpdateBurden            | `src/Burden/BurdenManager.h/.cpp`|
| Implement `RegenMovementParams` with per-state min/max + shared curve k      | `src/Settings/Params/RegenParams.h`|
| Add weather penalty (player-only, exterior-only, rain/snow detection)        | `src/Regen/RegenManager.cpp`    |
| Wire weather penalty into sprint cost (engineRate × penalty, flat additive) | `src/Regen/CostsManager.cpp`    |
| Hook AVRegen rate function at `38452 + 0x2B6` (`write_call<5>`)               | `src/Hooks/RegenHooks.h/.cpp`   |
| Full-stamina monitor heartbeat (100ms) kickoff from `OnGameLoad`             | `src/Hooks/RegenHooks.cpp`      |
| Implement GetEngineStaminaRate (clone of game's regen function)              | `src/Regen/RegenManager.cpp`    |

### Phase 3 — Runtime Configuration

| Task                                                         | Files                                            |
|--------------------------------------------------------------+--------------------------------------------------|
| Add `sb_get`/`sb_set`/`sb_list`/`sb_reset` console commands  | `src/Console/ConsoleCommands.cpp`, `Papyrus.cpp` |
| Wire `SettingsRegistry::LoadFromINI()` into startup sequence | `src/Export/SKSEPlugin.cpp`                      |
| Wire `SettingsRegistry::SaveToINI()` into every `Set()`      | `src/Config/SettingsRegistry.cpp`                |

### Phase 4 — Attack Costs

| Task                                             | Files                              |
|--------------------------------------------------+------------------------------------|
| Implement `ActionManager` for attacks            | `src/Actions/ActionManager.h/.cpp` |
| Hook `38603` (attack stamina cost)               | `src/Hooks/Hooks.cpp`              |
| Hook `49170` (action prevention at low stamina)  | `src/Hooks/Hooks.cpp`              |
| Implement weapon weight + skill + burden formula | `src/Actions/ActionManager.cpp`    |

### Phase 5 — Movement Costs ⚡ (partial)

| Task                                                      | Files                               |
|-----------------------------------------------------------+-------------------------------------|
| Implement SprintDrainHook (burden + weather × engineRate) | `src/Hooks/SprintDrainHook.h/.cpp` |
| Add CostsParams with sprint drain curve params            | `src/Settings/Params/CostsParams.h`|
| Implement CalculateSprintDrain in CostsManager            | `src/Regen/CostsManager.cpp`       |
| Jump cost via game-setting manipulation (remaining)       | `src/Actions/ActionManager.cpp`    |

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
| Cross-AV regen wiring (already in Phase 2)                              | *(reuse)*                         |

### Phase 8 — Weather ✅

| Task                                                               | Files                                        |
|--------------------------------------------------------------------+----------------------------------------------|
| Implement ComputeWeatherPenalty (inline, Rain/Snow via Sky API)    | `src/Regen/RegenManager.cpp`                 |
| Add WeatherParams struct with rain/snow penalty + toggle           | `src/Settings/Params/RegenParams.h`          |
| Wire into regen formula and sprint cost (same engineRate × penalty)| `src/Regen/RegenManager.cpp`, `CostsManager.cpp` |

### Phase 9 — HUD Burden Widget (was Phase 1c)

| Task | Files |
|---|---|
| Vendor `TrueHUDAPI.h` into project | `src/HUD/TrueHUDAPI.h` |
| Implement `BurdenWidget` (API detection + registration) | `src/HUD/BurdenWidget.h/.cpp` |
| Wire into startup messaging listener | `src/Export/SKSEPlugin.cpp` |
| Expose burden data access from Tracker | `src/Burden/BurdenTracker.h/.cpp` |

### Phase 10 — Papyrus + Polish

| Task                                                                | Files                                         |
|---------------------------------------------------------------------+-----------------------------------------------|
| Bind query functions in `Papyrus.cpp`                               | `src/Papyrus/Papyrus.h/.cpp`                  |
| Update `.psc` script                                                | `Data/Source/Scripts/EC_StaminaAndBurden.psc` |
| Remove stale `UnitTest_Serialization` declarations                  | `.psc` + `Papyrus.cpp`                        |
| Fix `TestCommands.yaml` (SEA_TemplateProject → EC_StaminaAndBurden) | `TestCommands.yaml`                           |
| Remove unreferenced `TaskUpdatePlayerBurdenLog`                     | `src/Burden/BurdenManager.h/.cpp`             |
| Clean up old `Settings::INI` files                                  | `src/Settings/INI/`                           |
| Remove `StaminaAndBurden.ini` shipped file                          | `Data/SKSE/Plugins/StaminaAndBurden.ini`      |
| Final settings tuning pass                                          | C++ defaults in `Parameter<T>` structs        |

---

## 10. Scope by Actor Type

| Feature                | Player                   | NPCs                     |
|------------------------+--------------------------+--------------------------|
| Burden tracking        | ✓ (event-driven, cached) | ✓ (lazy per-hook)        |
| Regen modification     | ✓ (full formula)         | ✓ (no weather component) |
| Weather penalty        | ✓                        | ✗                        |
| Attack cost + lockout  | ✓                        | ✓                        |
| Movement cost          | ✓ (regen curves + hook)  | ✓ (regen curves, no weather) |
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
