Scriptname StaminaAndBurden Hidden

Int[] Function GetVersion() Global Native

;/
GetBurdenByIndex — unified burden query using the same BurdenComponent index as ComputeActionCost.

BurdenComponent values:
  0 = Burden           (equipment burden)
  1 = CarryBurden      (inventory/carry weight burden)
  2 = BurdenBlend      (blended: 1 - sqrt((1-burden)*(1-carryBurden)))
  3 = WeaponRightHand  (right hand weapon burden)
  4 = WeaponLeftHand   (left hand weapon burden)
  5 = WeaponTwoHanded  (two-handed weapon burden)
  6 = WeaponRanged     (ranged weapon burden)
  7 = WeaponBlock      (block/shield burden)
/;
Float Function GetBurdenByIndex(Actor a_actor, Int a_index) Global Native

; Convenience wrappers
Float Function GetBurden(Actor a_actor) Global Native
Float Function GetCarryBurden(Actor a_actor) Global Native
Float Function GetBurdenBlend(Actor a_actor) Global Native
Float Function GetEffectiveEquippedWeight(Actor a_actor) Global Native
Float Function GetMaxEquippedWeight(Actor a_actor) Global Native

;/
SetMaxEquippedWeightOverride — Override the max equipped weight for an actor.
Pass 0.0 to clear the override and restore normal behavior.
Multiple mods can set this — last writer wins.
/;
Function SetMaxEquippedWeightOverride(Actor a_actor, Float a_maxEquippedWeight) Global Native

;/
ComputeActionCost — ad-hoc burden-based stamina cost.

BurdenComponent values: See GetBurdenByIndex above
Base and percent curves are interpolated separately:
  baseCost  = Interpolate(baseMin, baseMax, baseComponent, baseK)
  pctCost   = Interpolate(pctMin, pctMax, pctComponent, pctK) * 1% maxStamina
  totalCost = baseCost + pctCost
/;
Float Function ComputeActionCost(Actor a_actor, Int a_baseComponent, Float a_baseMin, Float a_baseMax, Float a_baseK, Int a_pctComponent, Float a_pctMin, Float a_pctMax, Float a_pctK) Global Native

Bool Function IsExhausted(Actor a_actor) Global Native

;/
Exhaustion events — register to receive notifications when an actor becomes exhausted or recovers.

Two registration modes:
  RegisterForExhaustionChanged(Form)         — fires for ANY actor
  RegisterForActorExhaustionChanged(Form, Actor) — fires only for the specified actor

Usage:
  Event OnInit()
      RegisterForExhaustionChanged(Self)
      ; or: RegisterForActorExhaustionChanged(Self, Game.GetPlayer())
  EndEvent

  Event OnActorExhaustionChanged(Actor akActor, bool abExhausted)
      if abExhausted
          ; actor became exhausted
      else
          ; actor recovered
      endIf
  EndEvent

  Event OnExhaustionChanged(Actor akActor, bool abExhausted)
      if abExhausted
          ; actor became exhausted
      else
          ; actor recovered
      endIf
  EndEvent
/;
;; Register for exhaustion events for all actors
Function RegisterForExhaustionChanged(Form akForm) global native
Function UnregisterForExhaustionChanged(Form akForm) global native
;; Register for exhaustion events for Specific actors (for example player and followers)
Function RegisterForActorExhaustionChanged(Form akForm, Actor akActor) global native
Function UnregisterForActorExhaustionChanged(Form akForm, Actor akActor) global native

; Console debug — returns formatted burden data for console display
String Function GetBurdenDebug(Actor a_actor) Global Native

;/
GetSpeedMultiplier — the S&B movement speed multiplier (burden × swim × exhaustion).
Returns 1.0 when nothing scales the actor's speed.
/;
Float Function GetSBSpeedMultiplier(Actor a_actor) Global Native

;/
GetTotalSpeed — engine vanilla speed × GetSpeedMultiplier, in game units.
Returns 0 if the speed hook is not active.
/;
Float Function GetSBTotalSpeed(Actor a_actor) Global Native
