#include "Combat/BlockManager.h"
#include "Settings/Params/BlockingParams.h"
#include "Settings/Params/ParameterOverrides.h"
#include "Common/Utils.h"
#include "Common/PerkCategories.h"
#include "Burden/BurdenTracker.h"
#include <RE/N/NiMath.h>

namespace Blocking
{
	float getEngineBlockStaminaCost(const RE::HitData& hitData)
	{
		auto* params = ParameterOverrides::GetSingleton();
		return ((hitData.percentBlocked * hitData.physicalDamage) * params->fStaminaBlockDmgMult.Get())
			 + (params->fStaminaBlockStaggerMult.Get() * hitData.stagger + params->fStaminaBlockBase.Get());
	}

	float ComputeBlockStaminaCost(RE::Actor* actor, const RE::HitData& hitData)
	{
		if (!actor)
			return 0.0f;

		auto* params = BlockingParams::GetSingleton();
		bool isPlayer = actor->IsPlayerRef();
		if ((isPlayer && !params->bBlockCostPlayer.Get()) ||
			(!isPlayer && !params->bBlockCostNPC.Get()))
			return 0.0f;

		const auto burden = Burden::Tracker::GetOrComputeBurden(actor);
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

		float burdenCost = flatCost + pctCost;
		float engineCost = getEngineBlockStaminaCost(hitData);
		float totalCost = std::max(0.0f, burdenCost - engineCost);

		RE::HandleEntryPoint(PEPE_STAMINA_ENTRY_POINT, actor, totalCost, PEPE::Group::BlockStamina,
			Utils::GetAttackHandInfo(actor, false, true).form);

		BlockLog("ComputeBlockStaminaCost: {:x} blockBurden={:.2f} burdenBlend={:.2f}"
			" flat={:.2f} pct={:.2f} engine={:.2f} total={:.2f}"sv,
			actor->GetFormID(), burden.weaponBurden_block, burden.burdenBlend,
			flatCost, pctCost, engineCost, totalCost);

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

		const auto burden = Burden::Tracker::GetOrComputeBurden(actor);
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

		RE::HandleEntryPoint(PEPE_STAMINA_ENTRY_POINT, actor, redirectCost, PEPE::Group::BlockStamina,
			Utils::GetAttackHandInfo(actor, false, true).form);

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

	float ComputeStaggerMagnitude(RE::Actor* actor, const RE::HitData& hitData)
	{
		if (!actor)
			return 0.0f;

		auto* params = BlockingParams::GetSingleton();
		bool isPlayer = actor->IsPlayerRef();
		if ((isPlayer && !params->bBlockCostPlayer.Get()) ||
			(!isPlayer && !params->bBlockCostNPC.Get()))
			return 0.0f;

		const auto burden = Burden::Tracker::GetOrComputeBurden(actor);

		float effectiveDamage = hitData.totalDamage;
		float currentHealth = actor->GetActorValue(RE::ActorValue::kHealth);
		if (effectiveDamage <= 0.0f || currentHealth <= 0.0f)
			return 0.0f;

		if (hitData.flags.any(RE::HitData::Flag::kPowerAttack))
			effectiveDamage = effectiveDamage * params->fStaggerPowerAttackMult.Get();

		float damageBurden = Math::Clamp01(effectiveDamage / currentHealth);

		float inertiaFactor = Math::Interpolate(
			params->fStaggerInertiaFactor_LowBurden.Get(),
			params->fStaggerInertiaFactor_HighBurden.Get(),
			burden.burdenBlend,
			params->fStaggerInertiaFactorCurve_k.Get());

		float unblockedBurden = damageBurden * inertiaFactor;

		BlockLog("ComputeStaggerMagnitude: {:x} effective={:.1f}"
			" damageBurd={:.2f} inertiaF={:.2f} unblockedBurd={:.2f}"sv,
			actor->GetFormID(), effectiveDamage,
			damageBurden, inertiaFactor, unblockedBurden);

		return Math::Interpolate(
			params->fStaggerMagnitudeMin.Get(),
			params->fStaggerMagnitudeMax.Get(),
			unblockedBurden,
			params->fStaggerMagnitudeCurve_k.Get());
	}

	float ComputeStaggerDirection(RE::Actor* target, const RE::HitData& hitData)
	{
		float theta = RE::NiFastATan2(hitData.hitDirection.x, hitData.hitDirection.y);
		float heading = RE::rad_to_deg(theta - target->GetAngleZ());
		if (heading < -180.0f) heading += 360.0f;
		if (heading > 180.0f) heading -= 360.0f;
		return (heading >= 0.0f) ? heading / 360.0f : (360.0f + heading) / 360.0f;
	}
}
