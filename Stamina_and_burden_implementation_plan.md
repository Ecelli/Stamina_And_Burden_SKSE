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
├── API/               # Vendored PerkEntryPointExtenderAPI.h (PEPE v3) (DONE)
├── Burden/            # Burden computation + actor tracker (DONE)
├── Combat/            # BlockManager + DamageManager (DONE)
│   ├── BlockManager   # Block stamina cost, damage redirect, guard break
│   └── DamageManager  # Stamina-conditional damage scaling
├── Common/            # PCH, Utils.h, PerkCategories.h (DONE)
│   ├── Utils.h/.cpp   # Hand detection types (LeftHandInfo, RightHandInfo, AttackHandInfo),
│   │                  #   CanDoStaminaAction, ApplyStaminaCost, heartbeat, debug log macros
│   └── PerkCategories.h # 7 PEPE category constants (SB_*) + PEPE_STAMINA_ENTRY_POINT
├── Data/              # ModObjectManager, Lookup.h (shell — EXPECTED_COUNT=0)
├── Export/            # SKSEPlugin.cpp — entrypoint, PEPE RequestInterface() at kDataLoaded (DONE)
├── Hooks/             # 10 code detours + 4 event sinks + 1 uninstalled deny (DONE)
├── Movement/          # Movement speed (burden/swim/exhaustion) + sprint/jump cost functions (DONE)
│   ├── MovementManager        # ComputeSpeedMultiplier — burden, swim depth, exhaustion speed scaling
│   └── MovementCostManager    # ComputeSprintDrain, ComputeJumpCost + PEPE calls
├── Papyrus/           # GetVersion only bound (MINIMAL)
├── RE/                # Offset.h — placeholder; all REL::IDs inline (DONE)
├── Serialization/     # Serde.h/.cpp — infrastructure ready, nothing registered (SHELL)
├── Settings/
│   ├── INI/           # SimpleIni reader with whitelist (shell — EXPECTED_COUNT=0)
│   ├── JSON/          # JSON reader (DONE)
│   └── Params/        # Parameter<T> singletons: BurdenParams, RegenParams,
│                      #   RegenMovementParams, NegativeRegen, WeatherParams,
│                      #   CostsParams, AttackCostParams, DenyParams,
│                      #   BlockingParams, ParameterOverrides, DamageParams,
│                      #   ExhaustionParams, MovementSpeedParams (DONE)
└── Stamina/
    ├── RegenManager      # Regen formulas + PEPE calls on hold penalties (DONE)
    ├── CostsManager      # Attack/bow cost formulas + PEPE calls (DONE)
    └── ExhaustionManager # State machine: trigger on stamina=0, penalties, safe-timer clear (DONE)

**Not created (future):**
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

**ActorBurdenData struct** (20 fields):
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

rate          -= ComputeBlockHoldPenalty(actor)    // PEPE: SB_BlockHoldStamina
rate          -= ComputeBowDrawHoldPenalty(actor)  // PEPE: SB_BowDrawHoldStamina
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
- `ComputeBlockHoldPenalty` — continuous flat drain while blocking, burden-scaled. PEPE: `SB_BlockHoldStamina` applied to penalty before returning.
- `ComputeBowDrawHoldPenalty` — continuous flat drain while bow drawn, weapon-burden-scaled. PEPE: `SB_BowDrawHoldStamina` applied to penalty before returning.
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
- PEPE: `SB_AttackStamina` applied to cost after staminaMult multiply, before return

**Sprint drain** is frame-time scaled (`× GetSecondsSinceLastFrame()`) and includes weather penalty integration (`engineRate × weatherPenalty`).

**Improvements over original plan:**
- Original plan described a single `attackCost = maxStamina × (fAttackCostBase + weaponWeight × fAttackCostWeightMult) × (1 - skill / fAttackCostSkillDivisor) × (1 + burdenPenalty × burdenRatio)`. Actual system uses `flatTerm + pctTerm × 1%maxStamina` with per-type params — more granular and tuneable.
- Original plan described game-setting writes for movement costs. Actual system hooks sprint drain directly (`38022 + 0xC1/0xC9`) and jump cost directly (`37257 + 0x17F`) — per-actor, no global setting manipulation.
- Bow fire cost + deny (not in original plan) built into BowFireHook.
- PEPE: `SB_BowFireStamina` applied to cost after computation, before return.

### 3.4 Movement (DONE)

**Files:** `src/Movement/MovementManager.h/.cpp`, `src/Movement/MovementCostManager.h/.cpp`, `src/Hooks/MovementHooks.h/.cpp`, `src/Settings/Params/MovementSpeedParams.h`

**Three concerns in one directory:**

1. **MovementSpeed** — scales actor walk/run speed based on burden, swim depth, and exhaustion
2. **MovementCosts** — sprint drain and jump cost formulas (delegated from CostsManager's original scope)
3. **JumpHeightScaling** — scales jump height by burden blend and exhaustion status

#### 3.4.1 SpeedHook — Movement Speed Scaling

**Hook:** `REL::ID(37943) + 0x51` — intercepts the engine's speed computation for all actors.

**`Movement::ComputeSpeedMultiplier(actor)`** — composite multiplier:
```
burdenMult  = Interpolate(speedMultLowBurden, speedMultHighBurden, burdenBlend, burdenSpeedCurve_k)
              (only if bEnableBurdenSpeed)

swimMult    = Interpolate(speedMultAboveWater, speedMultSubmerged, submergedLevel, submergedCurve_k)
              (only if bEnableSwimSpeed; submergedLevel via REL::ID(37448))

exhaustMult = exhaustionSpeedMult
              (only if bEnableExhaustionSpeed AND actor is exhausted)

result      = burdenMult × swimMult × exhaustMult
```

**Parameters (12):**
| Key | Type | Default | Range | Purpose |
|---|---|---|---|---|
| `bEnableBurdenSpeed` | bool | true | — | Master toggle for burden speed scaling |
| `bEnableSwimSpeed` | bool | true | — | Master toggle for swim speed scaling |
| `bEnableExhaustionSpeed` | bool | true | — | Master toggle for exhaustion speed penalty |
| `fSpeedMultLowBurden` | float | 1.10 | 0.1–2.0 | Speed mult at zero burden (slight bonus) |
| `fSpeedMultHighBurden` | float | 0.70 | 0.1–1.0 | Speed mult at full burden |
| `fBurdenSpeedCurve_k` | float | 0.50 | 0.0–1.0 | Burden speed curve shape |
| `fSpeedMultAboveWater` | float | 1.00 | 0.1–1.5 | Speed mult when not submerged |
| `fSpeedMultSubmerged` | float | 0.60 | 0.1–1.0 | Speed mult when fully submerged |
| `fSubmergedCurve_k` | float | 0.20 | 0.0–1.0 | Swim speed curve shape |
| `fExhaustionSpeedMult` | float | 0.70 | 0.1–1.0 | Speed mult while exhausted |
| `bEnableDebugMovementLogging` | bool | true | — | Debug toggle |

#### 3.4.2 Sprint Drain and Jump Cost

Sprint and jump cost formulas were originally planned under CostsManager (§3.3). They now live in the `Movement::` namespace and are implemented in `MovementCostManager.cpp`. The hooks (SprintHook, JumpHook) are installed via `MovementHooks` in `src/Hooks/MovementHooks.cpp`.

**`Movement::ComputeSprintDrain(actor)`** — returns stamina/frame:
```
SprintBurdenFlat  = Interpolate(SprintDrainLowBurden, SprintDrainHighBurden, burden, SprintDrainBurdenCurve_k)
SprintBurdenMult  = Interpolate(SprintDrainLowCarryBurdenPct, SprintDrainHighCarryBurdenPct, carryBurden, SprintDrainCarryBurdenCurve_k)
TotalCost         = SprintBurdenFlat + SprintBurdenMult × 1% maxStamina
TotalCost        += engineRate × weatherPenalty    (if weather penalty active)
PEPE: SB_SprintStamina  applied to TotalCost before delta-time multiplication
TotalCost        *= GetSecondsSinceLastFrame()      (frame-time scaled)
```

**`Movement::ComputeJumpCost(actor)`** — returns stamina per jump:
```
JumpBurdenFlat = Interpolate(JumpCostLowBurden, JumpCostHighBurden, burden, JumpCostBurdenCurve_k)
JumpCarryPct   = Interpolate(JumpCostLowCarryPct, JumpCostHighCarryPct, carryBurden, JumpCostCarryCurve_k)
TotalCost      = JumpBurdenFlat + JumpCarryPct × 1% maxStamina
PEPE: SB_JumpStamina  applied to TotalCost before return
```

**Hooks:**
| ID | Offset | Name | Purpose |
|---|---|---|---|
| `38022` | `+0xC1/0xC9` | `SprintHook` | Replaces equipped-weight with `Movement::ComputeSprintDrain` |
| `37257` | `+0x17F` | `JumpHook` | Applies `Movement::ComputeJumpCost` via `ApplyStaminaCost` + scales height via `ComputeJumpHeightMult` |
| `37943` | `+0x51` | `SpeedHook` | Scales movement speed by `Movement::ComputeSpeedMultiplier` |

**Parameters** for sprint/jump are in `CostsParams` (§9, `CostsParams.h`). Speed parameters are in `MovementSpeedParams` (§9). Jump height parameters are in `JumpParams` (§9).

#### 3.4.3 Jump Height Scaling

**`Movement::ComputeJumpHeightMult(actor)`** — scales jump height by burden and exhaustion:
```
burdenMult  = Interpolate(fJumpHeightLowBurden, fJumpHeightHighBurden, burdenBlend, fJumpHeightCurve_k)
              (only if bJumpHeightPlayer/NPC toggle allows)

exhaustMult = fJumpHeightExhaustionMult
              (only if actor is exhausted)

result      = burdenMult × exhaustMult
```

Implemented as part of `JumpHook::ApplyJumpCost` — the engine's `GetScale` return value is multiplied by the height mult, so jump animation scaling, physics arc, and landing timing all respond naturally.

**JumpParams** (8 params in `MovementSpeedParams.h`) control height curves, exhaustion penalty, and player-only jump deny. See §9.

### 3.5 BlockManager (DONE)

**Files:** `src/Combat/BlockManager.h/.cpp`, `src/Hooks/BlockHook.h/.cpp`, `src/Settings/Params/BlockingParams.h`

**Purpose:** On blocked hits, redirects remaining (post-block) damage from health to stamina cost. Running out of stamina triggers guard break stagger. Engine block formula is reimplemented as an offset to avoid double-counting.

**Engine context:** `HitData.totalDamage` at the hook point is already post-block:
```
totalDamage = physicalDamage × (1 - pctBlocked) × (1 - resistanceFactor)
```

`pctBlocked` varies depending on block skill, shield AR/weapon damage, and perk investment. Our redirect does NOT reduce damage further — it converts the already-reduced damage from health to stamina. Requires SSE Engine Fixes for correct weapon blocking (uses blocker's weapon, not attacker's).

**Engine block formula (GMST-controlled):**
```
Shield: pctBlocked = (fShieldBaseFactor + fShieldScalingFactor × ShieldAR
                      × (1 + BlockSkill × fBlockSkillMult × 0.0075))
                     × perks × fBlockPowerAttackMult

Weapon: pctBlocked = (fBlockWeaponBase + fBlockWeaponScaling × WeaponDamage
                      × (1 + BlockSkill × fBlockSkillMult × 0.0075))
                     × perks × fBlockPowerAttackMult
```

**Hook:** `REL::ID(38627) + 0x4A8` — same site as `DamageScalingHook`, chained via trampoline. DamageScalingHook executes first (scales damage), then BlockHook processes block mechanics on the scaled values.

**Engine drain formula** (reimplemented from Shield of Stamina):
```
engineCost = (percentBlocked × physicalDamage × fStaminaBlockDmgMult)
           + (fStaminaBlockStaggerMult × stagger + fStaminaBlockBase)
```
GMSTs: `fStaminaBlockDmgMult` (0.0), `fStaminaBlockStaggerMult` (0.0), `fStaminaBlockBase` (0.0) — configurable via `ParameterOverrides.h`.

**Burden block cost formula:**
```
burdenData = GetOrComputeBurden(actor)

flatCost = Interpolate(
    fBlockCost_LowBlockBurden,
    fBlockCost_HighBlockBurden,
    weaponBurden_block,
    fBlockCostCurve_k)

pctCost = 1% maxStamina × Interpolate(
    fBlockCostPct_LowBlended,
    fBlockCostPct_HighBlended,
    burdenBlend,
    fBlockCostPctCurve_k)

burdenCost = flatCost + pctCost
totalCost  = max(0, burdenCost - engineCost)
PEPE: SB_BlockStamina  applied to totalCost before log/return
```
**Per-actor-type toggle:** `bBlockCostPlayer` (true), `bBlockCostNPC` (false).

**Damage redirect cost formula:**
```
redirectMult = Interpolate(
    fBlockRedirectMult_LowBurden,
    fBlockRedirectMult_HighBurden,
    weaponBurden_block,
    fBlockRedirectMultCurve_k)

pctCost = 1% maxStamina × Interpolate(
    fBlockRedirectMultPct_LowBurden,
    fBlockRedirectMultPct_HighBurden,
    burdenBlend,
    fBlockRedirectMultPctCurve_k)

redirectCost = totalDamage × (redirectMult + pctCost)
PEPE: SB_BlockStamina  applied to redirectCost before log/return
```
**Per-actor-type toggle:** `bBlockRedirectPlayer` (true), `bBlockRedirectNPC` (false).

**Block flow in `BlockHook::ProcessHit`:**
```
if (hit blocked && target):
    baseCost     = ComputeBlockStaminaCost(target, hitData)
    redirectCost = ComputeDamageRedirectStaminaCost(target, hitData)
    totalCost    = baseCost + redirectCost

    if (totalCost > 0):
        currentStamina = target->GetActorValue(kStamina)

        if (currentStamina >= totalCost):
            // Full redirect: zero totalDamage, drain totalCost stamina
            ApplyBlockDamageRedirect(hitData, totalDamage)
            ApplyStaminaCost(target, totalCost)
        else:
            // Guard break: partial redirect + drain all stamina
            staminaBudget = max(0, currentStamina - baseCost)

            if (staminaBudget > 0 && redirectCost > 0):
                redirectAmount = totalDamage × (staminaBudget / redirectCost)
                ApplyBlockDamageRedirect(hitData, redirectAmount)

            ApplyStaminaCost(target, currentStamina)

            if (bGuardBreakEnabled):
                magnitude = ComputeStaggerMagnitude(target, hitData)
                direction = ComputeStaggerDirection(target, hitData)
                target->SetGraphVariableFloat("staggerDirection", direction)
                target->SetGraphVariableFloat("StaggerMagnitude", magnitude)
                target->NotifyAnimationGraph("staggerStart")
```

**Stagger magnitude:**
```
effectiveDamage = totalDamage
if (kPowerAttack): effectiveDamage *= fStaggerPowerAttackMult

damageBurden  = Clamp01(effectiveDamage / currentHealth)
inertiaFactor = Interpolate(
    fStaggerInertiaFactor_LowBurden,
    fStaggerInertiaFactor_HighBurden,
    burdenBlend,
    fStaggerInertiaFactorCurve_k)

unblockedBurden = damageBurden × inertiaFactor

magnitude = Interpolate(
    fStaggerMagnitudeMin,
    fStaggerMagnitudeMax,
    unblockedBurden,
    fStaggerMagnitudeCurve_k)
```

**Stagger direction:** Uses `hitData.hitDirection` (NiPoint3). Computes horizontal angle via `NiFastATan2`, relative to target's heading via `GetAngleZ()`, normalizes to 0–1.

**Upstream:** `ComputeBlockBurden()` in `BurdenManager.cpp:198` — shield/DW/2h/unarmed paths with blended block skill. Result stored in `weaponBurden_block`. Block skill is embedded upstream, not as a separate multiplier.

**Deferred items:**
- Exhaustion debuff — stamina-0 feature, not block-specific. See §3.8. (DONE)
- Block commitment — only makes sense with timed block.
- Timed block — significant future feature: timed block window, commitment, perfect block, window penalty system. Dependencies: input hooks, state machine.
- Perk integration — PEPE entry point (`kModPowerAttackStamina`) wired via `RE::HandleEntryPoint` on both block sub-costs using category `SB_BlockStamina`. Modded perks can scale base block stamina cost and damage redirect cost independently via the same category.

### 3.6 CombatManager (DEFERRED)

Future umbrella for combat-related features not covered by existing managers. Currently no standalone module needed — combat logic lives in `BlockManager`, `DamageManager`, and `CostsManager`.

### 3.7 WeatherManager (DONE — minimal)

**Files:** Built into `src/Stamina/RegenManager.cpp` with params in `src/Settings/Params/RegenParams.h`

- `ComputeWeatherPenalty(actor)` — player-only, checks `RE::Sky` for rain/snow
- Configured via `WeatherParams` with `WeatherRainPenalty`, `WeatherSnowPenalty`, `WeatherEnabled`
- No standalone manager class — inline in RegenManager

### 3.8 Exhaustion (DONE)

**Files:** `src/Stamina/ExhaustionManager.h/.cpp`, `src/Settings/Params/ExhaustionParams.h`

**Purpose:** State machine that triggers when any actor's stamina hits ≤ 0. While exhausted, the actor suffers penalties to damage output and all regen rates. Applies regardless of what caused the stamina drain (sprint, power attack, block, etc.).

**Trigger:** `Exhaustion::CheckForAndTriggerExhaustion(actor, deltaTime)` is called every frame from `RegenDelayHook::InterceptUpdateRegenDelay` (stamina AV updates only). If the actor is not already exhausted and their stamina ≤ 0, exhaustion is triggered (if toggles allow).

**While exhausted (`UpdateExhaustion`):**
```
if actor is dead → clear exhaustion
else if stamina >= fExhaustionThresholdStamina (default 0.30) → clear immediately
else if stamina > 0 → accumulate safeTimer += deltaTime
     if safeTimer >= fExhaustionDuration (default 8.0s):
         clear exhaustion
         restore fExhaustionBurstStamina (default 0.25) × maxStamina
else (stamina <= 0) → reset safeTimer to 0
```

**Penalties applied while exhausted:**
| Penalty | Param | Default | Applied via |
|---|---|---|---|
| Damage output | `fExhaustionPenaltyDamageMult` | 0.50 (50% damage) | `DamageScalingHook::ProcessHit` |
| Stamina regen | `fExhaustionPenaltyStaminaMult` | 0.30 (30% regen) | `RegenManager::ComputeBurdenStaminaRegenRate` |
| Health regen | `fExhaustionPenaltyHealthMult` | 0.0 (no health regen) | `RegenManager::ComputeHealthRegenMult` |
| Magicka regen | `fExhaustionPenaltyMagickaMult` | 0.0 (no magicka regen) | `RegenManager::ComputeMagickaRegenMult` |

**Parameters (10):**
| Key | Type | Default | Range | Purpose |
|---|---|---|---|---|
| `bExhaustionPlayer` | bool | true | — | Master toggle: player can become exhausted |
| `bExhaustionNPC` | bool | false | — | Master toggle: NPCs can become exhausted |
| `fExhaustionDuration` | float | 8.0 | 1.0–30.0 | Seconds of safe time required to clear exhaustion |
| `fExhaustionBurstStamina` | float | 0.25 | 0.0–1.0 | Fraction of max stamina restored as burst on clear |
| `fExhaustionThresholdStamina` | float | 0.30 | 0.0–1.0 | Stamina % threshold for immediate clear |
| `fExhaustionPenaltyDamageMult` | float | 0.50 | 0.0–1.0 | Damage output multiplier while exhausted |
| `fExhaustionPenaltyStaminaMult` | float | 0.30 | 0.0–1.0 | Stamina regen multiplier while exhausted |
| `fExhaustionPenaltyHealthMult` | float | 0.0 | 0.0–1.0 | Health regen multiplier while exhausted |
| `fExhaustionPenaltyMagickaMult` | float | 0.0 | 0.0–1.0 | Magicka regen multiplier while exhausted |
| `bEnableDebugLogging` | bool | true | — | Debug toggle |

**State management:**
- Per-actor state stored in `unordered_map<RE::FormID, ExhaustionState>` inside singleton `ExhaustionManager`
- `ClearAll()` called on game load via `BurdenTracker::OnGameLoad()` — wipes all exhaustion states
- `ClearExhaustion(FormID)` — erases a single actor's state

**Not included:**
- Action denial — not part of exhaustion scope (per project decision)
- Visual/audio feedback — purely mechanical stat penalty
- Papyrus bindings — state not exposed to scripts (see §8)
- Save/load serialization — exhaustion is transient, wiped on game load (see §8)
- INI configuration entries — defaults from `ExhaustionParams.h` always apply; INI whitelist not populated (see §7)

### 3.9 DamageManager (DONE)

**Files:** `src/Combat/DamageManager.h/.cpp`, `src/Settings/Params/DamageParams.h`

**Purpose:** Scales outgoing physical damage based on attacker stamina percentage. Reinforces stamina management by making low stamina directly reduce combat effectiveness.

**Hook:** `DamageScalingHook` at `REL::ID(38627) + 0x4A8` (ProcessHit call site)

```
ProcessHit(target, hitData) → attacker = hitData.aggressor.get()
    → GetDamageMultiplier(hitData)
        → skip if null, toggle off, or spell-only
        → ComputeStaminaDamageMult(attacker)
            → staminaPct = curStamina / maxStamina
            → Interpolate(fDamageScaleLow, fDamageScaleHigh, staminaPct, k)
    → scale both totalDamage and physicalDamage
```

**Formula:**
```
staminaPct      = currentStamina / maxStamina
damageMult      = Interpolate(fDamageScaleLow, fDamageScaleHigh, staminaPct, fDamageScaleCurve_k)
```

**Defaults:**
- At 0% stamina: 50% damage (`fDamageScaleLow = 0.50`)
- At 100% stamina: 120% damage (`fDamageScaleHigh = 1.20`)
- Curve shape: smooth (`fDamageScaleCurve_k = 0.80`)

**Parameters (DamageParams singleton, 6 params):**
| Key | Default | Description |
|---|---|---|
| `bDamageScalingPlayer` | true | Apply scaling to player attacks |
| `bDamageScalingNPC` | true | Apply scaling to NPC attacks |
| `fDamageScaleLow` | 0.50 | Damage mult at 0% stamina |
| `fDamageScaleHigh` | 1.20 | Damage mult at 100% stamina |
| `fDamageScaleCurve_k` | 0.80 | Curve shape (0=linear, 1=smoothstep) |
| `bEnableDebugLogging` | true | Debug toggle |

**Design decisions:**
- Burden NOT included in damage formula — burden affects stamina economy (costs/regen), stamina% affects damage. No double-counting.
- ProcessHit hook chosen over `get_damage` inside Populate — see Hook Summary note for tradeoffs.
- Both `totalDamage` and `physicalDamage` scaled uniformly — matches StaminaNPC behavior where crit bonus is derived from scaled `physicalDamage`.
- Spell-only attacks excluded via `!hitData.weapon && hitData.attackDataSpell` — prevents scaling pure magic damage.

---

## 4. Hook Summary

### Installed code detours (10 + 1 VTABLE hook):

| ID | Offset | Name | Purpose |
|---|---|---|---|
| `38452` | `+0x2B6` | `RegenHook` | Intercept AV regen rate → `ComputeBurdenStaminaRegenRate` |
| `38452` | `+0x02C` | `RegenDelayHook` | Bypass regen delay when negative drain cached → `DamageActorValue` |
| `38022` | `+0xC1/C9` | `SprintDrainHook` | Replace equipped-weight with `Movement::ComputeSprintDrain` |
| `37257` | `+0x17F` | `ActionHook` | Jump stamina cost via `Movement::ComputeJumpCost` → `ApplyStaminaCost` |
| `38603` | `+0x171` | `AttackCostHook` | Replace engine attack stamina cost → `ComputeAttackCost` |
| `42859` | `+0x138` | `BowFireHook` | Bow fire cost + deny if insufficient stamina |
| `38627` | `+0x4A8` | `DamageScalingHook` | Scale outgoing damage by attacker stamina% |
| `38627` | `+0x4A8` | `BlockHook` | Block stamina cost, damage redirect, guard break stagger |
| `37943` | `+0x51` | `SpeedHook` | Scale movement speed by `Movement::ComputeSpeedMultiplier` (burden/swim/exhaustion) |
| `39003` / `49170` | `+0xBB` / `+0x27A` (+ NOP branches `0xE1`/`0x272`) | `AttackDenyHook` | Replaces engine HasStamina check — denies attacks when stamina insufficient (player + NPC) |
| VTABLE `JumpHandler[0]` | index `0x04` | `JumpInputHandler` | Player-only jump cost + denial via `ProcessButton` VTABLE detour |

Note: `DamageScalingHook` and `BlockHook` share the same hook point (`38627+0x4A8`). They chain via trampoline — DamageScalingHook scales damage first, then BlockHook processes block mechanics on the scaled values.

### Installed event sinks (4):

| Event | Handler | Purpose |
|---|---|---|
| `TESLoadGameEvent` | `LoadGameHandler` | `OnGameLoad()` — clear maps, re-register player, start heartbeats |
| `TESEquipEvent` | `EquipHandler` | `Tracker::Update(actor)` — deferred burden recalc |
| `TESContainerChangedEvent` | `ContainerHandler` | `Tracker::Update(actor)` — deferred burden recalc |
| `TESActorLocationChangeEvent` | `WorldspaceChangeHandler` | Clear transient NPC cache on worldspace change |

### Denial hooks:

| ID | Offset | Name | Status | Notes |
|---|---|---|---|---|---|
| `39003` / `49170` | `+0xBB` / `+0x27A` (+ NOP branches `0xE1`/`0x272`) | `AttackDenyHook` | INSTALLED | Both player and NPC. Shared `HasStamina()` dispatches on per-actor-type toggles from `DenyParams`. Follows ScrambledBugs' `PowerAttackStaminaRequirement` approach. |
| VTABLE `JumpHandler[0]` | index `0x04` | `JumpInputHandler` | INSTALLED | Player-only VTABLE hook on `PlayerControls::JumpHandler::ProcessButton`. Applies jump cost + denies jump if stamina insufficient (bypasses animation/graph). No AE call-site issue. |

`AttackDenyHook` is fully implemented, tested, and active. `JumpInputHandler` provides player-only jump denial via VTABLE detour, replacing the dead `JumpDenyHook` code at `42423+0x114` that crashed on AE.

### Potential hook migration — `get_damage` inside Populate

An alternative hook point exists at `RELOCATION_ID(42832, 44001) + 0x1A5` — the `get_damage` function called inside `HitData::Populate`. StaminaNPC uses this hook for damage scaling. **Advantages:** scaled value propagates through all downstream Populate computations (crit bonus, etc.) automatically. **Disadvantages:** no access to HitData flags, target, or spell-attack detection (`attackDataSpell`); AE offset unverified; weapon pointer is opaque (`void*`). Current ProcessHit hook is preferred because it provides full HitData context and is confirmed by multiple mods on AE. Migration is possible if future formula needs simplify (no attack-type conditioning, no spell exclusion needed) and AE offset is verified via pattern scan.

### PEPE integration (not a hook — Perk Entry Point Extender):

PEPE `HandleEntryPoint` calls are placed inside 8 compute functions, using `PEPE_STAMINA_ENTRY_POINT` (= `kModPowerAttackStamina`) and 7 category strings:

| Category | Compute function | Variable scaled | Form arg |
|---|---|---|---|
| `SB_AttackStamina` | `ComputeAttackCost` | `baseCost` | `AttackHandInfo::form` (weapon/shield) |
| `SB_BowFireStamina` | `ComputeBowFireCost` | `TotalCost` | Right-hand weapon |
| `SB_SprintStamina` | `ComputeSprintDrain` | `TotalCost` | Right-hand weapon |
| `SB_JumpStamina` | `ComputeJumpCost` | `TotalCost` | Right-hand weapon |
| `SB_BlockStamina` | `ComputeBlockStaminaCost` | `totalCost` | Shield or blocking weapon |
| `SB_BlockStamina` | `ComputeDamageRedirectStaminaCost` | `redirectCost` | Shield or blocking weapon |
| `SB_BlockHoldStamina` | `ComputeBlockHoldPenalty` | `penalty` | Shield or blocking weapon |
| `SB_BowDrawHoldStamina` | `ComputeBowDrawHoldPenalty` | `penalty` | Bow weapon |

`SB_BlockStamina` is applied to both block sub-costs (base + redirect) independently so proration math stays correct. `RequestInterface()` called at `kDataLoaded` with nullptr guard logging.

> **Usage note:** For a perk entry to affect any of these stamina costs, it must be
> assigned to entry point `kModPowerAttackStamina` (index 27) in the CK, and its
> **category** (PEPE field) must match the `SB_*` string exactly (case-sensitive).
> Without a matching category, PEPE skips the entry. The condition target form
> (weapon/shield) is passed so perks can condition on equipment type.

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
    ├── Heartbeat 200ms: TaskPlayerFullStaminaMonitor
    └── Game load → ExhaustionManager::ClearAll()

Per-frame (all actors processed by engine):
    RegenHook → ComputeBurdenStaminaRegenRate(actor)
        ├── positive → engine applies regen normally
        └── negative → cache drain rate, RegenDelayHook drains per frame
            └── Exhaustion::CheckForAndTriggerExhaustion(actor, deltaTime) — every frame via RegenDelayHook
    SpeedHook → Movement::ComputeSpeedMultiplier(actor) → burdenMult × swimMult × exhaustMult

Per-action (each cost passes through PEPE HandleEntryPoint before consumption):
    SprintDrainHook (every frame while sprinting) → Movement::ComputeSprintDrain(actor) → [PEPE SB_SprintStamina] → ApplyStaminaCost
    ActionHook (on jump) → Movement::ComputeJumpCost(actor) → [PEPE SB_JumpStamina] → ApplyStaminaCost
    AttackCostHook (on attack) → ComputeAttackCost(actor, attackData) → [PEPE SB_AttackStamina] → ApplyStaminaCost / staminaMult
    AttackDenyHook (on attack) → CanDoStaminaAction(actor, cost) → 0.0F (allow) or cost (deny — engine penalizes)
    BowFireHook (on bow release) → ComputeBowFireCost(actor) → [PEPE SB_BowFireStamina] → ApplyStaminaCost
    DamageScalingHook (on hit) → ComputeStaminaDamageMult(attacker) → scale HitData
    BlockHook (on blocked hit) → [PEPE SB_BlockStamina × 2] on base cost + redirect cost → redirect or guard break
```

### Future flow (Console, HUD):
```
Burden::Tracker::Update(actor)
    └── Console commands (future: sb_get/set/list/reset/getburden)
```

---

## 6. Denial Features

| Feature | Status | Implementation |
|---|---|---|
| Attack denial (player) | **INSTALLED** | `AttackDenyHook` at `REL::ID(39003)` + `0xBB` (call detour) + `0xE1` (NOP branch). NOPs engine's player HasStamina conditional jump and replaces the GetAttackStamina call. |
| Attack denial (NPC) | **INSTALLED** | `AttackDenyHook` at `REL::ID(49170)` + `0x27A` (call detour) + `0x272` (NOP branch). Same approach as player. |
| Jump denial | **INSTALLED** — via `JumpInputHandler` VTABLE hook | `VTABLE_JumpHandler[0]` index 0x04. Player-only — intercepts `JumpHandler::ProcessButton` before animation/physics. Checks stamina against `Movement::ComputeJumpCost` + `bJumpDenyPlayer` toggle. No AE compatibility issue (VTABLE hook, not code-detour at `42423+0x114`). |
| Bow fire deny | **DONE** — built into `BowFireHook` | Integrated into the cost hook. Per-actor toggles (`bBowDenyPlayer/NPC`) control denial independently of cost. |

Both player and NPC attack denial share the same `HasStamina()` logic which checks per-actor-type toggles from `DenyParams` (`bEnableDenyPlayer`/`bEnableDenyNPC`). Return convention: `0.0F` = has stamina (allow), `>0.0F` = no stamina (deny — engine handles the penalty). Jump denial is installed via VTABLE (player-only) 

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

### BurdenParams (25 params)
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

### RegenParams (18 params)
- `bRegenPlayer/NPC` — per-actor toggles for regen modification
- `fStaminaRegenMult_LowHealth/HighHealth/LowStamina/HighStamina/LowMagicka/HighMagicka` — cross-AV curves
- `fStaminaRegenCurve_kStamina/kMagicka/kHealth` — curve shapes
- `fHealthRegenMult_LowStamina/HighStamina/k` — stamina → health regen curve
- `fMagickaRegenMult_LowStamina/HighStamina/k` — stamina → magicka regen curve
- `bEnableDebugLogging`

### RegenMovementParams (20 params)
- `fRegenStatic_max/min`, `fRegenWalking_max/min`, `fRegenSneaking_max/min`, `fRegenRunning_max/min`, `fRegenSwimming_max/min` — per-state curves
- `fMovementRegenCurve_k` — shared curve shape
- `fBowDrawLowBurden/HighBurden/Curve_k` — bow draw hold penalty (weapon-burden component)
- `fBlockHoldLowBurden/HighBurden/Curve_k` — block hold penalty (weapon-burden component)
- `fHoldDrainLowBlended/HighBlended` + `fHoldBlendedCurve_k` — shared blended-burden component (pct of max stamina/sec) added to both hold penalties

### NegativeRegen (5 params)
- `fBurnRate_LowBonus/HighBonus/Curve_k/LowBound/HighBound` — burn scaler mapping

### WeatherParams (3 params)
- `fWeatherRainPenalty`, `fWeatherSnowPenalty`, `bWeatherEnabled`

### CostsParams (29 params)
- `bAttackCostPlayer/NPC` — per-actor toggles for attack stamina cost
- `bBowCostPlayer/NPC` — per-actor toggles for bow fire stamina cost
- `bBowDenyPlayer/NPC` — per-actor toggles for bow fire denial on insufficient stamina
- `bSprintCostPlayer/NPC` — per-actor toggles for sprint drain
- `bJumpCostPlayer/NPC` — per-actor toggles for jump cost
- `bEnableDebugLogging`
- `fSprintDrainLowBurden/HighBurden/LowCarryBurdenPct/HighCarryBurdenPct/BurdenCurve_k/CarryBurdenCurve_k`
- `fJumpCostLowBurden/HighBurden/LowCarryPct/HighCarryPct/BurdenCurve_k/CarryCurve_k`
- `fBowFireLowBurden/HighBurden/BurdenCurve_k/LowCarryPct/HighCarryPct/CarryCurve_k`

### AttackCostParams (25 params)
- `fAttackLowCarryPct/HighCarryPct/CarryCurve_k` — shared carry burden component
- `fAttack1hLowBurden/HighBurden/BurdenCurve_k/PowerMult`
- `fAttack2hLowBurden/HighBurden/BurdenCurve_k/PowerMult`
- `fUnarmedBaseFlat/PowerMult`
- `fBashShieldLowBurden/HighBurden/BurdenCurve_k/PowerMult`
- `fBashBowLowBurden/HighBurden/BurdenCurve_k/PowerMult`
- `fBashWeaponLowBurden/HighBurden/BurdenCurve_k/PowerMult`

### DenyParams (4 params)
- `bEnableDenyPlayer` — enable attack denial for player (default true)
- `bEnableDenyNPC` — enable attack denial for NPCs (default true)
- `fMinStaminaCostMult` — stamina threshold fraction for action denial; actor must have stamina > `fMinStaminaCostMult × cost` to be allowed to act (default 0.30)
- `bEnableDebugLogging` (field `EnableDebugLogging`) — enable `DenyLog()` output (default true)

Removed: `bPlayerAlwaysCanDoAction`, `bNpcAlwaysCanDoAction`, `fNpcRegenExemptionRate` — unused, deprecated in favor of per-feature toggles.

### ExhaustionParams (10 params)
- `bExhaustionPlayer` — master toggle: player can become exhausted (default true)
- `bExhaustionNPC` — master toggle: NPCs can become exhausted (default false)
- `fExhaustionDuration` — safe time to clear exhaustion in seconds (default 8.0, range 1.0–30.0)
- `fExhaustionBurstStamina` — fraction of max stamina restored as burst on clear (default 0.25)
- `fExhaustionThresholdStamina` — stamina % for immediate clear (default 0.30)
- `fExhaustionPenaltyDamageMult` — damage output multiplier while exhausted (default 0.50)
- `fExhaustionPenaltyStaminaMult` — stamina regen multiplier while exhausted (default 0.30)
- `fExhaustionPenaltyHealthMult` — health regen multiplier while exhausted (default 0.0)
- `fExhaustionPenaltyMagickaMult` — magicka regen multiplier while exhausted (default 0.0)
- `bEnableDebugLogging` — shared debug toggle

### BlockingParams (25 params)

| Key | Default | Description |
|---|---|---|
| `bEnableDebugLogging` | true | Guard debug log output |
| `bBlockCostPlayer` | true | Apply stamina cost to player |
| `bBlockCostNPC` | false | Apply stamina cost to NPCs |
| `bBlockRedirectPlayer` | true | Apply damage redirect to player |
| `bBlockRedirectNPC` | false | Apply damage redirect to NPCs |
| `bGuardBreakEnabled` | true | Enable guard break stagger |
| `fBlockCost_LowBlockBurden` | 2.0 | Min flat stamina cost at zero block burden |
| `fBlockCost_HighBlockBurden` | 30.0 | Max flat stamina cost at full block burden |
| `fBlockCostCurve_k` | 0.80 | Flat cost curve shape |
| `fBlockCostPct_LowBlended` | 2.0 | Min % maxStamina cost at zero burden |
| `fBlockCostPct_HighBlended` | 8.0 | Max % maxStamina cost at full burden |
| `fBlockCostPctCurve_k` | 0.50 | % maxStamina cost curve shape |
| `fBlockRedirectMult_LowBurden` | 0.8 | Min redirect stamina mult at zero block burden |
| `fBlockRedirectMult_HighBurden` | 5.0 | Max redirect stamina mult at full block burden |
| `fBlockRedirectMultCurve_k` | 0.70 | Redirect mult curve shape |
| `fBlockRedirectMultPct_LowBurden` | 0.1 | Min redirect % maxStamina at zero burden |
| `fBlockRedirectMultPct_HighBurden` | 1.0 | Max redirect % maxStamina at full burden |
| `fBlockRedirectMultPctCurve_k` | 0.50 | Redirect % curve shape |
| `fStaggerPowerAttackMult` | 1.5 | Power attack damage multiplier for stagger |
| `fStaggerInertiaFactor_LowBurden` | 1.0 | Inertia at zero burden (more stagger) |
| `fStaggerInertiaFactor_HighBurden` | 0.3 | Inertia at full burden (less stagger) |
| `fStaggerInertiaFactorCurve_k` | 0.50 | Inertia curve shape |
| `fStaggerMagnitudeMin` | 0.0 | Min stagger magnitude |
| `fStaggerMagnitudeMax` | 2.0 | Max stagger magnitude |
| `fStaggerMagnitudeCurve_k` | 0.50 | Stagger magnitude curve shape |

### ParameterOverrides (16 params)
- `fCombatStaminaRegenRateMult/Health/Magicka` — overrides for GMST combat regen mults
- `fDamagedStaminaRegenDelay/Health/Magicka` — overrides for GMST damaged regen delays
- `fSprintStaminaDrainMult` — override for sprint drain GMST
- `fShieldBaseFactor` (20.0), `fShieldScalingFactor` (0.25) — shield block %
- `fBlockWeaponBase` (15.0), `fBlockWeaponScaling` (0.22) — weapon block %
- `fBlockSkillMult` (6.0), `fBlockPowerAttackMult` (0.66) — block skill + power attack
- `fStaminaBlockDmgMult` (0.0), `fStaminaBlockStaggerMult` (0.0), `fStaminaBlockBase` (0.0) — engine block stamina drain

### MovementSpeedParams (13 params)

| Key | Type | Default | Range | Purpose |
|---|---|---|---|---|
| `bEnableBurdenSpeed` | bool | true | — | Master toggle for burden speed scaling |
| `bEnableSwimSpeed` | bool | true | — | Master toggle for swim speed scaling |
| `bEnableExhaustionSpeed` | bool | true | — | Master toggle for exhaustion speed penalty |
| `fSpeedMultLowBurden` | float | 1.10 | 0.1–2.0 | Speed mult at zero burden (slight bonus) |
| `fSpeedMultHighBurden` | float | 0.70 | 0.1–1.0 | Speed mult at full burden |
| `fBurdenSpeedCurve_k` | float | 0.50 | 0.0–1.0 | Burden speed curve shape |
| `fSpeedMultAboveWater` | float | 1.00 | 0.1–1.5 | Speed mult when not submerged |
| `fSpeedMultSubmerged` | float | 0.60 | 0.1–1.0 | Speed mult when fully submerged |
| `fSubmergedCurve_k` | float | 0.20 | 0.0–1.0 | Swim speed curve shape |
| `fExhaustionSpeedMult` | float | 0.70 | 0.1–1.0 | Speed mult while exhausted |
| `bEnableDebugMovementLogging` | bool | true | — | Debug toggle |
| `bMovementSpeedPlayer` | bool | true | — | Per-actor toggle for player speed scaling |
| `bMovementSpeedNPC` | bool | true | — | Per-actor toggle for NPC speed scaling |

### JumpParams (8 params)

| Key | Type | Default | Range | Purpose |
|---|---|---|---|---|
| `bJumpHeightPlayer` | bool | true | — | Enable burden-based jump height scaling for player |
| `bJumpHeightNPC` | bool | true | — | Enable burden-based jump height scaling for NPCs |
| `fJumpHeightLowBurden` | float | 1.00 | 0.1–2.0 | Jump height mult at zero burden blend (normal height) |
| `fJumpHeightHighBurden` | float | 0.50 | 0.1–1.0 | Jump height mult at full burden blend |
| `fJumpHeightCurve_k` | float | 0.50 | 0.0–1.0 | Jump height curve shape |
| `fJumpHeightExhaustionMult` | float | 0.70 | 0.1–1.0 | Jump height multiplier while exhausted |
| `bJumpDenyPlayer` | bool | true | — | Deny jump when insufficient stamina (player-only) |
| `bEnableDebugJumpLogging` | bool | true | — | Debug logging for jump height + deny |

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
| AttackCostHook at 38603+0x171 | DONE |
| BowFireHook at 42859+0x138 | DONE |
| CostsParams, AttackCostParams | DONE |

### Phase 3b — Movement ✅
| Task | Status |
|---|---|
| `Movement::ComputeSprintDrain` (frame-time scaled) | DONE |
| `Movement::ComputeJumpCost` | DONE |
| `Movement::ComputeSpeedMultiplier` — burden/swim/exhaustion | DONE |
| `Movement::ComputeJumpHeightMult` — burden + exhaustion height scaling | DONE |
| SprintDrainHook at 38022+0xC1/0xC9 | DONE |
| ActionHook at 37257+0x17F | DONE |
| SpeedHook at 37943+0x51 | DONE |
| MovementSpeedParams (13 params) | DONE |
| JumpParams (8 params) | DONE |
| MovementHooks (3 hooks in single file) | DONE |

### Phase 3c — Perk Entry Point Extender Integration ✅
| Task | Status |
|---|---|
| Vendored PEPE API v3 at `src/API/PerkEntryPointExtenderAPI.h` | DONE |
| `PEPE::Group` namespace with 7 `SB_*` category constants | DONE |
| `PEPE_STAMINA_ENTRY_POINT` (= `kModPowerAttackStamina`) | DONE |
| `RequestInterface()` at `kDataLoaded` with nullptr guard | DONE |
| `RE::HandleEntryPoint` on `ComputeAttackCost` — `SB_AttackStamina` | DONE |
| `RE::HandleEntryPoint` on `ComputeBowFireCost` — `SB_BowFireStamina` | DONE |
| `RE::HandleEntryPoint` on `ComputeSprintDrain` — `SB_SprintStamina` | DONE |
| `RE::HandleEntryPoint` on `ComputeJumpCost` — `SB_JumpStamina` | DONE |
| `RE::HandleEntryPoint` on `ComputeBlockStaminaCost` + `ComputeDamageRedirectStaminaCost` — `SB_BlockStamina` (2 calls) | DONE |
| `RE::HandleEntryPoint` on `ComputeBlockHoldPenalty` — `SB_BlockHoldStamina` | DONE |
| `RE::HandleEntryPoint` on `ComputeBowDrawHoldPenalty` — `SB_BowDrawHoldStamina` | DONE |
| Hand detection refactored into `Utils::` namespace (`GetAttackHandInfo`, `AttackHandInfo`) for PEPE form args | DONE |

### Phase 4 — Denial ✅
| Task | Status |
|---|---|
| AttackDenyHook — player (39003+0xBB + NOP 0xE1) | **INSTALLED** |
| AttackDenyHook — NPC (49170+0x27A + NOP 0x272) | **INSTALLED** |
| Bow fire deny (built into BowFireHook) | DONE |
| JumpInputHandler — VTABLE JumpHandler[0] index 0x04 (player-only jump cost + deny) | **DONE** (replaces dead JumpDenyHook at 42423+0x114) |

### Phase 5 — Blocking ✅ (Timed Block deferred)
| Task | Status |
|---|---|
| BlockHook at 38627+0x4A8 (chained with DamageScalingHook) | DONE |
| `ComputeBlockStaminaCost` — burden-based flat + pct cost | DONE |
| Engine block drain offset (`getEngineBlockStaminaCost`) | DONE |
| `ComputeDamageRedirectStaminaCost` — health→stamina redirect | DONE |
| `ApplyBlockDamageRedirect` — zero totalDamage on full redirect | DONE |
| Guard break stagger — partial redirect + drain all stamina | DONE |
| Stagger magnitude formula (damageBurden × inertiaFactor) | DONE |
| Stagger direction formula (NiFastATan2, heading-relative) | DONE |
| BlockingParams singleton (23 params) | DONE |
| Engine GMST overrides (9 block-related) | DONE |
| NPC toggles (`bBlockCostNPC`, `bBlockRedirectNPC`) | DONE |
| Timed block (Valhalla-style) | DEFERRED — future consideration |
| Perk integration | DONE — PEPE HandleEntryPoint on both block sub-costs (SB_BlockStamina) |

### Phase 6 — Damage Scaling ✅
| Task | Status |
|---|---|
| DamageManager — stamina-conditional damage scaling | DONE |
| DamageScalingHook at 38627+0x4A8 | DONE |
| DamageParams singleton (6 params) | DONE |
| Per-actor-type toggles (player/NPC) | DONE |
| Spell-only attack exclusion | DONE |
| Physical-only scaling (preserves crit bonus) | DONE |

### Phase 7 — Exhaustion ✅
| Task | Status |
|---|---|
| ExhaustionManager — state machine (trigger, update, clear with threshold/duration/burst) | DONE |
| 10 ExhaustionParams (toggles, duration, burst, 4 penalties) | DONE |
| Regen integration (stamina/health/magicka regen penalties) | DONE |
| Damage scaling integration (damage penalty) | DONE |
| Game-load cleanup | DONE |
| Debug logging | DONE |
| Papyrus bindings | NOT STARTED |
| Save serialization | NOT STARTED |
| INI configuration entries | NOT STARTED |
| Visual/audio feedback | NOT STARTED |

### Phase 8 — Settings & Console (DEFERRED — lowest priority)
| Task | Status |
|---|---|
| Populate INI whitelist with all params | NOT STARTED |
| Ship `StaminaAndBurden.ini` with defaults | NOT STARTED |
| Console commands (sb_get/set/list/reset) | NOT STARTED |
| sb_getburden debug command | NOT STARTED |
| Fix TestCommands.yaml (SEA_TemplateProject → EC_StaminaAndBurden) | NOT STARTED |

### Phase 9 — Papyrus & Polish (NOT STARTED)
| Task | Status |
|---|---|
| Bind query functions | NOT STARTED |
| Clean up UnitTest_Serialization stubs | NOT STARTED |
| Clean up stale Settings::INI files | NOT STARTED |
| Clean up unreferenced `TaskUpdatePlayerBurdenLog` | NOT STARTED |

### Phase 10 — HUD Burden Widget (NOT STARTED — optional)
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
13. **Damage scaling via ProcessHit** instead of `get_damage` inside Populate — ProcessHit provides full HitData context (flags, target, spell detection) at the cost of needing to manually scale `totalDamage` and `physicalDamage`. `get_damage` propagates automatically but lacks context and has unverified AE offset.
14. **Uniform scaling of totalDamage and physicalDamage** — both fields multiplied by same `damageMult`, preserving crit bonus proportionally. Matches StaminaNPC behavior where crit bonus is derived from scaled `physicalDamage`.
15. **BlockHook + DamageScalingHook share hook point** — both detour at `38627+0x4A8`. Chained via trampoline: DamageScalingHook scales damage first, then BlockHook processes block mechanics. No conflict — they execute sequentially.
16. **BlockManager in `src/Combat/`** — not a separate `src/Blocking/` directory. Keeps combat-related modules (`BlockManager`, `DamageManager`) together.
17. **Engine block GMST overrides** — `fShieldBaseFactor`, `fBlockSkillMult`, etc. overridden via `ParameterOverrides.h` at startup. Requires SSE Engine Fixes for correct weapon blocking.
18. **Block skill embedded upstream** — `ComputeBlockBurden()` in BurdenManager produces `weaponBurden_block`, not a separate multiplier in BlockManager. Avoids double-counting.
19. **Exhaustion implemented** — originally deferred as a block-specific feature, now implemented as a general stamina-0 state machine triggered from the regen delay hook. Applies cross-AV regen penalties and damage scaling. No action denial.
20. **SpeedHook** — movement speed scaling by burden blend, swim depth, and exhaustion status. Not in original plan. Composite multiplier applied via dedicated hook at `37943+0x51`. Sprint/jump cost functions delegated from CostsManager to `Movement::` namespace.
21. **Per-actor-type toggles** — every user-facing feature (regen, attack cost, bow cost, bow deny, sprint, jump, movement speed) has individual Player/NPC boolean toggle pairs in its respective param group. Replaces the removed global bypass params (`bPlayerAlwaysCanDoAction`, `bNpcAlwaysCanDoAction`, `fNpcRegenExemptionRate`). DenyParams contains four params: `bEnableDenyPlayer`, `bEnableDenyNPC`, `fMinStaminaCostMult`, and `EnableDebugLogging`.
22. **PEPE integration** — all 8 stamina cost/penalty functions wired to `kModPowerAttackStamina` entry point via `RE::HandleEntryPoint`. Uses 7 `SB_*` category strings for per-cost-type targeting. Block uses same category on both sub-costs for correct proration. Hand detection refactored into `Utils::` namespace to supply condition form arguments. Vendored PEPE API v3, no external dependency.
23. **Hold penalty blended-burden component** — block/bow draw hold penalties originally scaled by weapon-specific burden only. Added a percentage-of-max-stamina `burdenBlend` term matching the attack cost pattern, so total carry burden also contributes to hold drain. Uses 3 shared params (`HoldDrainLowBlended`, `HoldDrainHighBlended`, `HoldBlendedCurve_k`) — one curve shape, separate range for both hold types.

## 12. Scope by Actor Type

| Feature | Player | NPCs |
|---|---|---|---|
| Burden tracking | ✓ (event-driven, cached, skill-polled) | ✓ (lazy per-hook, transient cache) |
| Regen modification | ✓ (full formula) | ✓ (no weather component) |
| Weather penalty | ✓ | ✗ |
| Attack cost | ✓ (all weapon types + power) | ✓ |
| Jump cost | ✓ | ✓ |
| Jump height scaling | ✓ (burden + exhaustion curve) | ✓ (burden + exhaustion curve) |
| Sprint drain | ✓ | ✓ |
| Bow fire cost + deny | ✓ | ✓ |
| Block stamina redirect | ✓ (burden cost + redirect + guard break) | ✗ (default off, toggled via `bBlockCostNPC`/`bBlockRedirectNPC`) |
| Exhaustion | ✓ (default on) | ✓ (default off, toggle via `bExhaustionNPC`) |
| Attack damage scaling | ✓ (stamina-based curve) | ✓ (stamina-based curve) |
| Movement speed | ✓ (burden + swim + exhaustion) | ✓ (burden + exhaustion) |
| Attack denial | ✓ (via `AttackDenyHook` at `39003+0xBB`/`0xE1`) | ✓ (via `AttackDenyHook` at `49170+0x27A`/`0x272`) |
| Jump denial | ✓ (via `JumpInputHandler` VTABLE hook) | ✗ (NPCs don't use PlayerInputHandler, rarely jump) |

---

## 13. Key Open Items

1. **Timed block** — Valhalla Combat-style timed block window, commitment, perfect block, window penalty system. Dependencies: input hooks, state machine. Deferrable to separate plan.
2. **Settings INI** — populate whitelist with all active params.
3. **Console commands** — sb_get/set/list/reset/getburden via Papyrus + TestCommands.yaml.
4. ~~Perk integration~~ → **DONE** (PEPE, §3c). All 8 stamina cost/penalty functions wired to `kModPowerAttackStamina` entry point via PEPE `HandleEntryPoint` with `SB_*` categories. Modded perks can scale any cost by adding perk entries targeting the relevant category. The single entry point covers attack, bow, sprint, jump, block, and hold penalty costs. Future perk-specific mechanics (timed block windows, stamina refunds, conditional bonuses) would need additional perk entry points or custom hook logic beyond PEPE's scope.
5. **Exhaustion regen formula** — consider replacing the current HMS triple-product multiplier structure with a formula that can dip below zero flat (i.e. amplify/drain beyond the maximum-multiplier boundary). Currently exhaustion applies a multiplicative penalty to the engine rate, but never pushes regen into negative territory. A deeper stamina-drain gameplay loop could trigger exhaustion-to-drain cascades.
---



## 14. Resolved Questions

### 14.1 `get_damage` hook inside HitData::Populate
**Status:** CANCELLED — retained ProcessHit hook.

**Considered:** Moving damage scaling from `ProcessHit` (`REL::ID(38627) + 0x4A8`) to the `get_damage` call inside `HitData::Populate` (`RELOCATION_ID(42832, 44001) + 0x1A5`). This would let scaled `physicalDamage` propagate automatically through crit bonus, sneak attack, and other downstream Populate computations.

**Rejected because:**
- `get_damage` receives an opaque `void*` weapon — no access to the aggressor actor (needed for `GetActorValue(kStamina)`), `HitData` flags, or `attackDataSpell`
- Spell-exclusion (`!hitData.weapon && hitData.attackDataSpell`) would be impossible
- The AE offset is unverified and pattern-scanning is the only reliable approach
- The existing ProcessHit hook at the shared `38627+0x4A8` chains correctly with other mods (PreludeToPurgatory confirmed) via `write_call<5>` trampoline ordering, so there's no compatibility pressure to move
- The marginal improvement in crit-bonus precision doesn't justify losing HitData context

### 14.2 Hold penalty scaling — blended-burden component
**Status:** RESOLVED — added blended-burden component.

**Considered:** Block/bow draw hold penalties originally scaled by weapon-specific burden only (`weaponBurden_block` / `weaponBurden_ranged`). Regular attack costs include a `burdenBlend` term (equipped + carry burden composite) via `ComputeBurdenAttackCost`. The question was whether hold penalties should match this pattern for consistency.

**Decision:** Both hold penalties now include a `maxStamina × 1% × Interpolate(HoldDrainLowBlended, HoldDrainHighBlended, burdenBlend, HoldBlendedCurve_k)` term, matching the attack cost structure. 3 shared params (`HoldDrainLowBlended`, `HoldDrainHighBlended`, `HoldBlendedCurve_k`) added to `RegenMovementParams`. The curve shape is shared (`HoldBlendedCurve_k`) while the range is tunable via the low/high bounds.
