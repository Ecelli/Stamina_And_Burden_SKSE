Scriptname StaminaAndBurden

Int[] Function GetVersion() Global Native

; Burden queries
Float Function GetBurden(Actor a_actor) Global Native
Float Function GetCarryBurden(Actor a_actor) Global Native
Float Function GetBurdenBlend(Actor a_actor) Global Native
Float Function GetEffectiveEquippedWeight(Actor a_actor) Global Native
Float Function GetMaxEquippedWeight(Actor a_actor) Global Native

;/
ComputeActionCost — ad-hoc burden-based stamina cost.

BurdenComponent values:
  0 = Burden           (equipment burden)
  1 = CarryBurden      (inventory/carry weight burden)
  2 = BurdenBlend      (blended: 1 - sqrt((1-burden)*(1-carryBurden)))
  3 = WeaponRightHand  (right hand weapon burden)
  4 = WeaponLeftHand   (left hand weapon burden)
  5 = WeaponTwoHanded  (two-handed weapon burden)
  6 = WeaponRanged     (ranged weapon burden)
  7 = WeaponBlock      (block/shield burden)

Base and percent curves are interpolated separately:
  baseCost  = Interpolate(baseMin, baseMax, baseComponent, baseK)
  pctCost   = Interpolate(pctMin, pctMax, pctComponent, pctK) * 1% maxStamina
  totalCost = baseCost + pctCost
/;
Float Function ComputeActionCost(Actor a_actor, Int a_baseComponent, Float a_baseMin, Float a_baseMax, Float a_baseK, Int a_pctComponent, Float a_pctMin, Float a_pctMax, Float a_pctK) Global Native