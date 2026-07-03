#include "Combat/BlockManager.h"
#include "Settings/Params/BlockingParams.h"
#include "Common/Utils.h"
#include "Burden/BurdenTracker.h"

namespace Blocking
{
	float ComputeBlockStaminaCost(RE::Actor* actor)
	{
		if (!actor)
			return 0.0f;

		auto* params = BlockingParams::GetSingleton();
		bool isPlayer = actor->IsPlayerRef();
		if ((isPlayer && !params->bBlockCostPlayer.Get()) ||
			(!isPlayer && !params->bBlockCostNPC.Get()))
			return 0.0f;

		auto& burden = Burden::Tracker::GetOrComputeBurden(actor);
		float stamina1pct = 0.01f * actor->GetActorValueMax(RE::ActorValue::kStamina);

		float flatCost = Math::Interpolate(
			params->fBlockCost_LowBlockBurden.Get(),
			params->fBlockCost_HighBlockBurden.Get(),
			burden.weaponBurden_block,
			params->fBlockCostCurve_k.Get());

		float pctCost = stamina1pct * Math::Interpolate(
			params->fBlockCostPct_LowBlended.Get(),
			params->fBlockCostPct_HighBlended.Get(),
			burden.burdenBlend,
			params->fBlockCostPctCurve_k.Get()) ;

		float totalCost = flatCost + pctCost;

		BlockLog("ComputeBlockStaminaCost: {:x} blockBurden={:.2f} burdenBlend={:.2f}"
			" flat={:.2f} pct={:.2f} total={:.2f}"sv,
			actor->GetFormID(), burden.weaponBurden_block, burden.burdenBlend,
			flatCost, pctCost, totalCost);

		return totalCost;
	}

	float ComputeDamageRedirectStaminaCost(RE::Actor* actor, const RE::HitData& hitData)
	{
		if (!actor)
			return 0.0f;

		auto* params = BlockingParams::GetSingleton();
		bool isPlayer = actor->IsPlayerRef();
		if ((isPlayer && !params->bBlockRedirectPlayer.Get()) ||
			(!isPlayer && !params->bBlockRedirectNPC.Get()))
			return 0.0f;

		auto& burden = Burden::Tracker::GetOrComputeBurden(actor);
		float stamina1pct = 0.01f * actor->GetActorValueMax(RE::ActorValue::kStamina);

		float redirectMult = Math::Interpolate(
			params->fBlockRedirectMult_LowBurden.Get(),
			params->fBlockRedirectMult_HighBurden.Get(),
			burden.weaponBurden_block,
			params->fBlockRedirectMultCurve_k.Get());

        float pctCost = stamina1pct * Math::Interpolate(
			params->fBlockRedirectMultPct_LowBurden.Get(),
			params->fBlockRedirectMultPct_HighBurden.Get(),
			burden.burdenBlend,
			params->fBlockRedirectMultPctCurve_k.Get());

		float redirectCost = hitData.totalDamage * (redirectMult + pctCost);

		BlockLog("ComputeDamageRedirectStaminaCost: {:x} blockBurden={:.2f}"
			" totalDmg={:.1f} mult={:.3f} redirectCost={:.2f}"sv,
			actor->GetFormID(), burden.weaponBurden_block,
			hitData.totalDamage, redirectMult, redirectCost);

		return redirectCost;
	}

	void ApplyBlockDamageRedirect(RE::HitData& hitData, float redirectAmount)
	{
		float original = hitData.totalDamage;
		hitData.totalDamage = std::max(0.0f, hitData.totalDamage - redirectAmount);

		BlockLog("ApplyBlockDamageRedirect: {:.1f} -> {:.1f} (redirected {:.1f})"sv,
			original, hitData.totalDamage, redirectAmount);
	}
}
