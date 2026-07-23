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

;/
Exhaustion

Papyrus scripts can react to exhaustion state changes via ModCallbackEvent.
Event name: "StaminaAndBurden_OnExhaustionChanged"

  numArg 1.0  — actor became exhausted
  numArg 0.0  — actor recovered
  sender       — the actor (Form)

Usage:
  Event OnInit()
      RegisterForModEvent("StaminaAndBurden_OnExhaustionChanged", "OnExhaustionChanged")
  EndEvent

  Event OnExhaustionChanged(string eventName, string strArg, float numArg, Form sender)
      Actor actor = sender as Actor
      if numArg == 1.0
          ; exhausted
      else
          ; recovered
      endIf
  EndEvent
/;
Bool Function IsExhausted(Actor a_actor) Global Native

; Console debug — returns formatted burden data for console display
String Function GetBurdenDebug(Actor a_actor) Global Native
