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
├── Burden/            # Burden computation + actor tracker (DONE)
├── Common/            # PCH, Utils.h — heartbeat, CanDoStaminaAction, Interpolate (DONE)
├── Data/              # ModObjectManager, Lookup.h (shell — EXPECTED_COUNT=0)
├── Export/            # SKSEPlugin.cpp — entrypoint (DONE)
├── Hooks/             # 6 code detours + 4 event sinks + 2 uninstalled denies (DONE)
├── Papyrus/           # GetVersion only bound (MINIMAL)
├── RE/                # Offset.h — placeholder; all REL::IDs inline (DONE)
├── Serialization/     # Serde.h/.cpp — infrastructure ready, nothing registered (SHELL)
├── Settings/
│   ├── INI/           # SimpleIni reader with whitelist (shell — EXPECTED_COUNT=0)
│   ├── JSON/          # JSON reader (DONE)
│   └── Params/        # Parameter<T> singletons: BurdenParams, RegenParams,
│                      #   RegenMovementParams, NegativeRegen, WeatherParams,
│                      #   CostsParams, AttackCostParams, DenyParams,
│                      #   ParameterOverrides (DONE)
└── Stamina/
    ├── RegenManager   # Regen formulas + single source of truth (DONE)
    └── CostsManager   # Sprint/jump/attack/bow cost formulas (DONE)
```

**Not created (future):**
- `src/Blocking/` — stamina redirect on blocked hits, guard break
- `src/Combat/` — stamina-conditional damage scaling
- `src/Console/` — runtime debug/query commands
- `src/HUD/` — TrueHUD burden widget

---

## 2. Configuration Architecture

**Current system:** `Parameter<T>` singletons (REX::Singleton) with typed `ForEach(F&&)` export. INI values are read by `Settings::INI::Read()` (SimpleIni with strict whitelist). Overrides via `_Custom.ini` (same stem, same dir).

**Deviations from original plan:**
- No SettingsRegistry was implemented. `Parameter<T>` + `_Custom.ini` is simpler and standard for Skyrim mods. This is the permanent system.
- INI validation is currently a shell (`EXPECTED_COUNT = 0`). The shipped `StaminaAndBurden.ini` has no key-value pairs.
- In-game settings (console commands, ImGui menu, MCM) are deferred — lowest priority.

---

## 3. Module Specifications

### 3.1 BurdenManager (DONE)

**Files:** `src/Burden/BurdenManager.h/.cpp`, `BurdenTracker.h/.cpp`

**Architecture:** Two namespaces:
- `Burden::` — burden computation functions
- `Burden::Tracker` — actor registry with 2-tier cache (tracked + transient)

**ActorBurdenData struct** (18 fields):
```
actor, maxCarryWeight, carryWeight, equippedWeight, maxEquippedWeight,
carryBurden, burden, burdenBlend,
lightSkill, heavySkill, oneHandedSkill, twoHandedSkill, marksmanSkill, blockSkill, conjurationSkill,
weaponBurden_rh, weaponBurden_lh, weaponBurden_2h, weaponBurden_ranged, weaponBurden_block
```

**burdenBlend formula:**
```
burdenBlend = 1 - sqrt((1 - burden) * (1 - carryBurden))
```
Blends equipped burden and carry burden into a single factor for cost/regen curves.

**Improvements over original plan:**
- `burdenBlend` replaces separate burden × carryBurden product — smoother interaction
- Weapon burden tracking (per-hand, 2h, ranged, block) enables per-type cost curves
- Skill-weighted weapon burden: `ScaleWeaponWeight(weight, skill)` — higher skill = less perceived weapon weight
- Conjured weapon weight computed from conjuration skill
- Block burden with shield/DW/2h/unarmed paths, dual-wield penalty, blended block skill
- `actor` field added so formulas can get maxStamina directly

**Triggers (3):**
1. Event sinks — equip/container change → `Tracker::Update(actor)` → `AddTask` deferred
2. Heartbeat polling — 200ms detached thread → `TaskTrackBurdenParams()` → compares 7 cached skills + carryWeight
3. Game load — `TESLoadGameEvent` → `OnGameLoad()` clears maps, re-registers player, starts heartbeats

### 3.2 RegenManager (DONE)

**Files:** `src/Stamina/RegenManager.h/.cpp`

**Single source of truth:** `ComputeBurdenStaminaRegenRate(actor)` — computes the effective stamina rate (regeneration or drain) in stamina per second. Used by:
- `RegenHook::InterceptAVRegen` — returns this value as the per-frame regen rate
- `TaskPlayerFullStaminaMonitor` — drains 0.1 when full stamina + negative rate

**Formula:**
```
mult           = ComputeStaminaRegenMult(actor)
regenMult      = max(mult, 0)
drainMult      = max(-mult, 0)

engineRate     = GetEngineStaminaRate(actor)   // includes kStaminaRateMult, combat mult
engineRate     = max(engineRate, 0)            // clamp before regen multiply

rate           = engineRate × regenMult
if drainMult > 0:
  burnBase     = GetBaseStaminaRate(actor)     // kStaminaRate × 0.01 × kStamina — NO rate mult
  scaler       = ComputeBurnScaler(actor)      // maps kStaminaRateMult onto [0,1]
  rate        -= burnBase × drainMult × scaler

rate          -= ComputeBlockHoldPenalty(actor)
rate          -= ComputeBowDrawHoldPenalty(actor)
```

**`ComputeStaminaRegenMult(actor)` — the regen multiplier:**
```
HMS = GetHMSStaminaMult(actor)           // health% × stamina% × magicka% triple product

if GetCanRegenStamina(actor):            // not blocking, not bow drawn/attached, attack state = kNone
  state = GetMovementState(actor)        // static/walking/sneaking/running/swimming
  regenBonus = ComputeStateRegenFactor(burdenData, state, HMS)
else:
  regenBonus = 0

mult = regenBonus - ComputeWeatherPenalty(actor)
```

**`ComputeStateRegenFactor` — per-movement-state curve:**
```
For each state: min/max pair → Interpolate(maxVal × HMS, minVal, burdenBlend, k)
```
Each movement state has its own min (high burden = worst) and max (low burden = best) parameters, blended by `burdenBlend`. The max value is multiplied by the HMS triple product, so low health/stamina/magicka reduces max regen even at zero burden.

**`ComputeBurnScaler` — negative rate mult handling:**
```
Maps kStaminaRateMult from [BurnRate_LowBound, BurnRate_HighBound] onto [0, 1]
via Interpolate(LowBonus, HighBonus, t, Curve_k).
LowBonus=2.0 (debuffed mult → amplified drain), HighBonus=0.2 (buffed mult → reduced drain)
```

**Supporting functions:**
- `GetBaseStaminaRate` — `kStaminaRate × 0.01 × kStamina` (no rate mult, no combat mult)
- `GetEngineStaminaRate` — `GetBaseStaminaRate × combatMult × kStaminaRateMult × 0.01`
- `ComputeBlockHoldPenalty` — continuous flat drain while blocking, burden-scaled
- `ComputeBowDrawHoldPenalty` — continuous flat drain while bow drawn, weapon-burden-scaled
- `ComputeWeatherPenalty` — `WeatherRainPenalty` or `WeatherSnowPenalty` from `WeatherParams`
- `GetCanRegenStamina` — false if blocking, bow drawn/attached, or in attack state
- `GetMovementState` — swimming → running → sneaking → walking → static priority
- `ComputeHealthRegenMult` / `ComputeMagickaRegenMult` — stamina% → health/magicka regen curves

**Improvements over original plan:**
- `ComputeBurdenStaminaRegenRate` as single source of truth (replaces vague "intercept rate" description)
- Per-movement-state min/max curves instead of flat scalars — more granular control
- `burdenBlend` instead of separate burden × carryBurden product
- Weather penalty + block/bow hold penalties (originally planned as "future")
- Burn scaler for kStaminaRateMult (not in original plan at all)
- `GetBaseStaminaRate` / `GetEngineStaminaRate` split (not in original plan)
- Negative rate handled via RegenDelayHook (cache + drain) instead of inline DamageActorValue in RegenHook

### 3.3 CostsManager (DONE)

**Files:** `src/Stamina/CostsManager.h/.cpp`

**Public API:**
```
float ComputeSprintDrain(actor)      — returns stamina/frame (includes GetSecondsSinceLastFrame)
float ComputeJumpCost(actor)          — returns stamina per jump
float ComputeAttackCost(actor, data)  — returns stamina per attack
float ComputeBowFireCost(actor)       — returns stamina per shot
```

**Cost formula pattern (all types use the same structure):**
```
Stamina_1pctMax = 0.01 × maxStamina
flatTerm        = Interpolate(LowBurden, HighBurden, weaponBurden, k)
pctTerm         = Interpolate(LowCarryPct, HighCarryPct, carryBurden, k)
cost            = flatTerm + pctTerm × Stamina_1pctMax
```
- `flatTerm` scales with equipped burden (or weapon-specific burden for attacks)
- `pctTerm` scales with carry burden, multiplied by 1% of max stamina
- Result is a percentage of max stamina

**Attack cost by weapon type (7 types + power attack mult):**
- 1H attack — weapon burden, 1h skill-weighted
- 2H attack — weapon burden, 2h skill-weighted
- Unarmed — flat base + carry burden component
- Shield bash — shield weight + block skill
- Bow bash — ranged weapon burden
- Weapon bash — weapon burden + blended block skill
- Ranged (bow fire) — handled by ComputeBowFireCost, not attack cost
- All types multiplied by `attackData->data.staminaMult`

**Sprint drain** is frame-time scaled (`× GetSecondsSinceLastFrame()`) and includes weather penalty integration (`engineRate × weatherPenalty`).

**Improvements over original plan:**
- Original plan described a single `attackCost = maxStamina × (fAttackCostBase + weaponWeight × fAttackCostWeightMult) × (1 - skill / fAttackCostSkillDivisor) × (1 + burdenPenalty × burdenRatio)`. Actual system uses `flatTerm + pctTerm × 1%maxStamina` with per-type params — more granular and tuneable.
- Original plan described game-setting writes for movement costs. Actual system hooks sprint drain directly (`38022 + 0xC1/0xC9`) and jump cost directly (`37257 + 0x17F`) — per-actor, no global setting manipulation.
- Bow fire cost + deny (not in original plan) built into BowFireHook.

### 3.4 ActionManager (FUTURE)

**Purpose:** Not a separate module — attack/sprint/jump costs are handled by CostsManager + their respective hooks. A future ActionManager would handle:
- Action lockout checks for blocked actions
- Any future action types not covered by existing hooks

### 3.5 BlockManager (NOT STARTED)

Stamina redirect on blocked hits + guard break + exhaustion trigger. No code exists.

### 3.6 CombatManager (NOT STARTED)

Stamina-conditional damage scaling. No code exists.

### 3.7 WeatherManager (DONE — minimal)

**Files:** Built into `src/Stamina/RegenManager.cpp` with params in `src/Settings/Params/RegenParams.h`

- `ComputeWeatherPenalty(actor)` — player-only, checks `RE::Sky` for rain/snow
- Configured via `WeatherParams` with `WeatherRainPenalty`, `WeatherSnowPenalty`, `WeatherEnabled`
- No standalone manager class — inline in RegenManager

### 3.8 Exhaustion (NOT STARTED)

State machine for depleted stamina. No code exists.

---

## 4. Hook Summary

### Installed code detours (6):

| ID | Offset | Name | Purpose |
|---|---|---|---|
| `38452` | `+0x2B6` | `RegenHook` | Intercept AV regen rate → `ComputeBurdenStaminaRegenRate` |
| `38452` | `+0x02C` | `RegenDelayHook` | Bypass regen delay when negative drain cached → `DamageActorValue` |
| `38022` | `+0xC1/C9` | `SprintDrainHook` | Replace equipped-weight with burden-based sprint drain |
| `37257` | `+0x17F` | `ActionHook` | Jump stamina cost via `ApplyStaminaCost` |
| `38603` | `+0x171` | `AttackCostHook` | Replace engine attack stamina cost → `ComputeAttackCost` |
| `42859` | `+0x138` | `BowFireHook` | Bow fire cost + deny if insufficient stamina |

### Installed event sinks (4):

| Event | Handler | Purpose |
|---|---|---|
| `TESLoadGameEvent` | `LoadGameHandler` | `OnGameLoad()` — clear maps, re-register player, start heartbeats |
| `TESEquipEvent` | `EquipHandler` | `Tracker::Update(actor)` — deferred burden recalc |
| `TESContainerChangedEvent` | `ContainerHandler` | `Tracker::Update(actor)` — deferred burden recalc |
| `TESActorLocationChangeEvent` | `WorldspaceChangeHandler` | Clear transient NPC cache on worldspace change |

### Denial hooks (NOT INSTALLED — code exists but incomplete):

| ID | Offset | Name | Status | Reason |
|---|---|---|---|---|
| `49170` | `+0x28D` | `AttackDenyHook` | Commented out | NPC-only — does not fire for player. Player denial needs vtable hook on `AttackBlockHandler::ProcessButton`. |
| `42423` | `+0x114` | `JumpDenyHook` | `Install()` logs NOT INSTALLED | AE call site crashes on 1.6.1170. SSE offset known (`41349+0x114`) but not ported. |

Both denial hooks are **implemented but not installed** — per project convention, they are considered incomplete. Deferred until player denial entry point is solved.

### Heartbeat polling:
- 200ms detached thread (`Common::make_heartbeat`) — polls tracked actor params for skill/weight changes
- 200ms detached thread — `TaskPlayerFullStaminaMonitor`, drains 0.1 if player at full stamina with negative regen rate

---

## 5. Data Flow

### Current flow:
```
Game Event (equip/container/game load)
    │
    ▼
Burden::Tracker::Update/OnGameLoad → UpdateBurden(actor)
    │
    ├── Event sinks (equip, container, worldspace, load)
    ├── Heartbeat 200ms: TaskTrackBurdenParams (skill/weight polling)
    └── Heartbeat 200ms: TaskPlayerFullStaminaMonitor

Per-frame (all actors processed by engine):
    RegenHook → ComputeBurdenStaminaRegenRate(actor)
        ├── positive → engine applies regen normally
        └── negative → cache drain rate, RegenDelayHook drains per frame

Per-action:
    SprintDrainHook (every frame while sprinting) → ComputeSprintDrain(actor)
    ActionHook (on jump) → ComputeJumpCost(actor) → ApplyStaminaCost
    AttackCostHook (on attack) → ComputeAttackCost(actor, attackData)
    BowFireHook (on bow release) → ComputeBowFireCost(actor) → ApplyStaminaCost
```

### Planned future flow (Blocking, Combat, Console, HUD):
```
Burden::Tracker::Update(actor)
    ├── BlockManager (future: stamina redirect on blocked hits)
    ├── CombatManager (future: stamina-conditional damage scaling)
    └── Console commands (future: sb_get/set/list/reset/getburden)
```

---

## 6. Denial Features

Per project convention: **any denial feature that is implemented but not installed is considered incomplete.**

| Feature | Status | Reason |
|---|---|---|
| Attack denial (NPC) | Implemented, NOT INSTALLED (`Hooks.cpp:35` commented) | Hook `49170+0x28D` only fires for NPCs. Player needs different entry point. |
| Attack denial (player) | Not implemented | Requires vtable hook on `AttackBlockHandler::ProcessButton` (vtable index 04) — approach designed, not coded. |
| Jump denial | Implemented, NOT INSTALLED | AE call site at `42423+0x114` crashes on 1.6.1170. SSE offset `41349+0x114` known but unused. |
| Bow fire deny | DONE — built into `BowFireHook` | Integrated into the cost hook — if `CanDoStaminaAction` returns false, the shot is suppressed. |

**Deferred until a solution is found for player action denial.** The BowFireHook's inline deny is the pattern to follow: check `CanDoStaminaAction` inside the cost hook, suppress the action if insufficient.

---

## 7. Papyrus Interface

**Current state:** Minimal. Only `GetVersion` bound. Script at `Data/Source/Scripts/EC_StaminaAndBurden.psc` has 3 stubs: `GetVersion` + 2 `UnitTest_Serialization` stubs.

**Planned** (deferred):
- `GetEquippedBurdenRatio` — query burden data from Papyrus
- `GetTotalBurdenRatio`
- `GetCurrentStaminaRegenMult`
- Console command natives (sb_get/set/list/reset/getburden)

---

## 8. Serialization

**Current state:** Infrastructure exists (`Serialization::Serde.h/.cpp`) — `Serializable` base class, `ObjectManager` registry, save/load/revert callbacks registered with ID `'TRJT'`. Nothing is registered.

Burden state is dynamically recomputed on game load — no serialization needed. The infrastructure remains for future use if save-scoped state becomes necessary.

---

## 9. Settings (Parameter Groups)

All settings are `Parameter<T>` structs in `src/Settings/Params/`. No shipped INI file with values — defaults are C++ `static constexpr`. `_Custom.ini` provides user overrides.

### BurdenParams (20 params)
- `fmaxEquippedWeightRatio` — ratio of carry weight used for max equipped weight
- `fSlotBurdenMult_def/body/feet/head/hand` — slot multipliers for equipped burden
- `iPlayerMaxSkill` — skill cap (default 100)
- `fSkillInterpolate` — curve shape for armor skill weighting
- `fSkillBurdenMult_minHeavy/maxHeavy/minLight/maxLight` — armor type skill multipliers
- `fSteedStoneBurdenMult` — Steed Stone factor (default 0.30)
- `fWeaponWeightMult_LowSkill/HighSkill/Curve_k` — weapon weight scaling by skill
- `fConjuredWeightMin/Max/Curve_k` — conjured weapon weight by conjuration skill
- `fBlockSkillBlendFactor` — weapon/block skill blend for block burden
- `fBlockWeightMult_LowSkill/HighSkill/Curve_k` — block weight scaling
- `fDualWieldBlockPenalty` — extra penalty for blocking while dual-wielding
- `fUnarmedWeight` — base weight for unarmed/empty hands

### RegenParams (16 params)
- `fStaminaRegenMult_LowHealth/HighHealth/LowStamina/HighStamina/LowMagicka/HighMagicka` — cross-AV curves
- `fStaminaRegenCurve_kStamina/kMagicka/kHealth` — curve shapes
- `fHealthRegenMult_LowStamina/HighStamina/k` — stamina → health regen curve
- `fMagickaRegenMult_LowStamina/HighStamina/k` — stamina → magicka regen curve
- `bEnableDebugLogging`

### RegenMovementParams (17 params)
- `fRegenStatic_max/min`, `fRegenWalking_max/min`, `fRegenSneaking_max/min`, `fRegenRunning_max/min`, `fRegenSwimming_max/min` — per-state curves
- `fMovementRegenCurve_k` — shared curve shape
- `fBowDrawLowBurden/HighBurden/Curve_k` — bow draw hold penalty
- `fBlockHoldLowBurden/HighBurden/Curve_k` — block hold penalty

### NegativeRegen (5 params)
- `fBurnRate_LowBonus/HighBonus/Curve_k/LowBound/HighBound` — burn scaler mapping

### WeatherParams (3 params)
- `fWeatherRainPenalty`, `fWeatherSnowPenalty`, `bWeatherEnabled`

### CostsParams (16 params)
- `fSprintDrainLowBurden/HighBurden/LowCarryBurdenPct/HighCarryBurdenPct/BurdenCurve_k/CarryBurdenCurve_k`
- `fJumpCostLowBurden/HighBurden/LowCarryPct/HighCarryPct/BurdenCurve_k/CarryCurve_k`
- `fBowFireLowBurden/HighBurden/BurdenCurve_k/LowCarryPct/HighCarryPct/CarryCurve_k`

### AttackCostParams (26 params)
- `fAttackLowCarryPct/HighCarryPct/CarryCurve_k` — shared carry burden component
- `fAttack1hLowBurden/HighBurden/BurdenCurve_k/PowerMult`
- `fAttack2hLowBurden/HighBurden/BurdenCurve_k/PowerMult`
- `fUnarmedBaseFlat/PowerMult`
- `fBashShieldLowBurden/HighBurden/BurdenCurve_k/PowerMult`
- `fBashBowLowBurden/HighBurden/BurdenCurve_k/PowerMult`
- `fBashWeaponLowBurden/HighBurden/BurdenCurve_k/PowerMult`

### DenyParams (4 params)
- `fMinStaminaCostMult` — stamina threshold fraction for action denial
- `bPlayerAlwaysCanDoAction` — bypass for player (debug)
- `bNpcAlwaysCanDoAction` — bypass for NPCs (debug)
- `fNpcRegenExemptionRate` — regen threshold for NPC exemption

### ParameterOverrides (6 params)
- `fCombatStaminaRegenRateMult/Health/Magicka` — overrides for GMST combat regen mults
- `fDamagedStaminaRegenDelay/Health/Magicka` — overrides for GMST damaged regen delays

### Settings future work:
- INI whitelist population (currently `EXPECTED_COUNT = 0`)
- Shipped `StaminaAndBurden.ini` with documented defaults
- In-game console commands (sb_get/set/list/reset/getburden)
- ImGui settings menu (lowest priority)

---

## 10. Implementation Phases

### Phase 1 — Burden Foundation ✅
| Task | Status |
|---|---|
| Slot weighting, skill-weighted armor mult | DONE |
| Actor registry with 2-tier cache | DONE |
| Equip/container/gameload event sinks | DONE |
| Weapon burden tracking (rh/lh/2h/ranged/block) | DONE |
| Block burden (shield/DW/2h/unarmed/paths) | DONE |
| Conjured weapon weight | DONE |
| Heartbeat polling for skill/weight changes | DONE |
| BurdenParams singleton | DONE |

### Phase 2 — Regen ✅
| Task | Status |
|---|---|
| `ComputeBurdenStaminaRegenRate` — single source of truth | DONE |
| Per-movement-state min/max curves | DONE |
| Cross-AV triple product (health% × stamina% × magicka%) | DONE |
| Block/bow draw hold penalties | DONE |
| Weather penalty | DONE |
| Burn scaler for kStaminaRateMult | DONE |
| `GetBaseStaminaRate` / `GetEngineStaminaRate` | DONE |
| Health/magicka regen from stamina% | DONE |
| RegenHook at 38452+0x2B6 | DONE |
| RegenDelayHook at 38452+0x02C | DONE |
| Full-stamina monitor heartbeat | DONE |
| RegenMovementParams, WeatherParams, NegativeRegen | DONE |

### Phase 3 — Attack Costs ✅
| Task | Status |
|---|---|
| `ComputeAttackCost` with 7 weapon types | DONE |
| Power attack multiplier | DONE |
| `ComputeBowFireCost` | DONE |
| `ComputeJumpCost` | DONE |
| `ComputeSprintDrain` (frame-time scaled) | DONE |
| AttackCostHook at 38603+0x171 | DONE |
| SprintDrainHook at 38022+0xC1/0xC9 | DONE |
| ActionHook at 37257+0x17F | DONE |
| BowFireHook at 42859+0x138 | DONE |
| CostsParams, AttackCostParams | DONE |

### Phase 4 — Denial (DEFERRED)
| Task | Status |
|---|---|
| AttackDenyHook at 49170+0x28D | NOT INSTALLED — NPC only |
| Player attack denial | NOT STARTED — needs vtable hook on AttackBlockHandler |
| JumpDenyHook at 42423+0x114 | NOT INSTALLED — AE call site crashes |

### Phase 5 — Blocking (NOT STARTED)
| Task | Status |
|---|---|
| BlockManager — stamina redirect on blocked hits | NOT STARTED |
| Guard break mechanic | NOT STARTED |
| Block skill influence on stamina consumption | NOT STARTED |
| Hook 38627 (hit processing) | NOT STARTED |

### Phase 6 — Combat Damage Scaling (NOT STARTED)
| Task | Status |
|---|---|
| CombatManager — stamina-conditional damage | NOT STARTED |
| Exhaustion state machine | NOT STARTED |
| Damage scaling on 38627 | NOT STARTED |

### Phase 7 — Settings & Console (DEFERRED — lowest priority)
| Task | Status |
|---|---|
| Populate INI whitelist with all params | NOT STARTED |
| Ship `StaminaAndBurden.ini` with defaults | NOT STARTED |
| Console commands (sb_get/set/list/reset) | NOT STARTED |
| sb_getburden debug command | NOT STARTED |
| Fix TestCommands.yaml (SEA_TemplateProject → EC_StaminaAndBurden) | NOT STARTED |

### Phase 8 — Papyrus & Polish (NOT STARTED)
| Task | Status |
|---|---|
| Bind query functions | NOT STARTED |
| Clean up UnitTest_Serialization stubs | NOT STARTED |
| Clean up stale Settings::INI files | NOT STARTED |
| Clean up unreferenced `TaskUpdatePlayerBurdenLog` | NOT STARTED |

### Phase 9 — HUD Burden Widget (NOT STARTED — optional)
| Task | Status |
|---|---|
| TrueHUD API integration | NOT STARTED |
| Burden special resource bar | NOT STARTED |

---

## 11. Deviations from Original Plan (Improvements)

1. **No SettingsRegistry** — `Parameter<T>` + `_Custom.ini` is simpler and standard for Skyrim. Not a gap.
2. **Per-movement-state min/max curves** instead of flat scalars — finer granularity for regen tuning.
3. **burdenBlend** instead of separate burden × carryBurden product — smoother blend between equipped and carry burden.
4. **Weapon burden tracking** (per-hand, 2h, ranged, block) — enables per-type attack cost curves.
5. **Block burden** (shield/DW/2h/unarmed paths) — not in original plan at all.
6. **Conjured weapon weight** — not in original plan.
7. **Burn scaler** for kStaminaRateMult — not in original plan.
8. **Weather penalty** implemented as part of RegenManager, not a separate WeatherManager.
9. **Block/bow draw hold penalties** — continuous flat drain, not in original plan.
10. **RegenDelayHook** — caches negative rate, drains per frame. Improves over inline DamageActorValue in RegenHook.
11. **Heartbeat via `std::thread` + `make_heartbeat`** instead of WorldFrameHook — no hook needed, works on all AE versions.
12. **`ComputeBurdenStaminaRegenRate`** as single source of truth — improves clarity and maintainability.

## 12. Scope by Actor Type

| Feature | Player | NPCs |
|---|---|---|
| Burden tracking | ✓ (event-driven, cached, skill-polled) | ✓ (lazy per-hook, transient cache) |
| Regen modification | ✓ (full formula) | ✓ (no weather component) |
| Weather penalty | ✓ | ✗ |
| Attack cost | ✓ (all weapon types + power) | ✓ |
| Jump cost | ✓ | ✓ |
| Sprint drain | ✓ | ✓ |
| Bow fire cost + deny | ✓ | ✓ |
| Block stamina redirect | ✗ (future) | ✗ (future) |
| Exhaustion | ✗ (future) | ✗ (future) |
| Attack damage scaling | ✗ (future) | ✗ (future) |
| Action denial | AttackDenyHook NPC-only, JumpDenyHook broken AE | AttackDenyHook NPC-only |

---

## 13. Key Open Items

1. **Player attack denial** — vtable hook on `AttackBlockHandler::ProcessButton` (vtable index 04). AE-stable per CommonLibSSE-NG.
2. **Jump denial AE** — need working call site for AE 1.6.1170. SSE `41349+0x114` known working.
3. **Block redirect** — hook `38627` for hit processing, split damage to stamina on blocked hits.
4. **Exhaustion** — state machine with timed duration, regen/damage penalties.
5. **Settings INI** — populate whitelist with all active params.
6. **Console commands** — sb_get/set/list/reset/getburden via Papyrus + TestCommands.yaml.
