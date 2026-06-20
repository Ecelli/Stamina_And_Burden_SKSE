#include "Regen/CostsManager.h"
#include "Burden/BurdenTracker.h"
#include "Common/Utils.h"
#include "Settings/Params/CostsParams.h"

namespace Costs
{
	float CalculateSprintDrain(RE::Actor* actor)
	{
		if (!actor)
			return 0.0f;

		auto& burden = Burden::Tracker::GetOrComputeBurden(actor);
		auto* params = CostsParams::GetSingleton();

        float Stamina_1pctMax = 0.01f * actor->GetActorValueMax(RE::ActorValue::kStamina);
        float SprintBurdenFlat = Math::Interpolate(
			params->SprintDrainLowBurden.Get(),
			params->SprintDrainHighBurden.Get(),
			burden.burden,
			params->SprintDrainBurdenCurve_k.Get());
        float SprintBurdenMult = Math::Interpolate(
			params->SprintDrainLowCarryBurdenPct.Get(),
			params->SprintDrainHighCarryBurdenPct.Get(),
			burden.carryBurden,
			params->SprintDrainCarryBurdenCurve_k.Get());

        float TotalCost = SprintBurdenFlat + SprintBurdenMult* Stamina_1pctMax;
		float result = TotalCost * RE::GetSecondsSinceLastFrame();

		Regen::RegenLog("CalculateSprintDrain: BurdenCost={:.3f} BurdenStamina pct={:.3f} drain/s={:.3f} delta={:.6f} -> {:.3f}",
			SprintBurdenFlat, SprintBurdenMult, TotalCost, RE::GetSecondsSinceLastFrame(), result);
		return result;
	}
}
