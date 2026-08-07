#include "Movement/MovementCostManager.h"
#include "Stamina/RegenManager.h"
#include "Burden/BurdenTracker.h"
#include "Common/Utils.h"
#include "Common/PerkCategories.h"
#include "Settings/Params/CostsParams.h"

namespace Movement
{
	float ComputeSprintDrain(RE::Actor* actor)
	{
		if (!actor)
			return 0.0f;

		const auto burden = Burden::Tracker::GetOrComputeBurden(actor);
		auto* params = Costs::CostsParams::GetSingleton();

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

		float TotalCost = SprintBurdenFlat + SprintBurdenMult * Stamina_1pctMax;

		float weatherPenalty = Regen::ComputeWeatherPenalty(actor);
		if (weatherPenalty > 0.0f) {
			float engineRate = Regen::GetEngineStaminaRate(actor);
			TotalCost += engineRate * weatherPenalty;
		}

		RE::HandleEntryPoint(PEPE_STAMINA_ENTRY_POINT, actor, TotalCost, PEPE::Group::SprintStamina,
			Utils::GetAttackHandInfo(actor, false, false).form);

		return TotalCost * RE::GetSecondsSinceLastFrame();
	}

	float ComputeJumpCost(RE::Actor* actor)
	{
		if (!actor)
			return 0.0f;

		const auto burden = Burden::Tracker::GetOrComputeBurden(actor);
		auto* params = Costs::CostsParams::GetSingleton();

		float Stamina_1pctMax = 0.01f * actor->GetActorValueMax(RE::ActorValue::kStamina);
		float JumpBurdenFlat = Math::Interpolate(
			params->JumpCostLowBurden.Get(),
			params->JumpCostHighBurden.Get(),
			burden.burden,
			params->JumpCostBurdenCurve_k.Get());
		float JumpCarryPct = Math::Interpolate(
			params->JumpCostLowCarryPct.Get(),
			params->JumpCostHighCarryPct.Get(),
			burden.carryBurden,
			params->JumpCostCarryCurve_k.Get());

		float TotalCost = JumpBurdenFlat + JumpCarryPct * Stamina_1pctMax;

		RE::HandleEntryPoint(PEPE_STAMINA_ENTRY_POINT, actor, TotalCost, PEPE::Group::JumpStamina,
			Utils::GetAttackHandInfo(actor, false, false).form);

		Movement::MovementLog(
			"ComputeJumpCost: burden={:.3f} carry={:.3f} -> {:.3f} for {:x}",
			burden.burden, burden.carryBurden, TotalCost, actor->GetFormID());

		return TotalCost;
	}
}
