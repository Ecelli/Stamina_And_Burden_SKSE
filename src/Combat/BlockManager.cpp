#include "Combat/BlockManager.h"
#include "Settings/Params/BlockingParams.h"
#include "Common/Utils.h"

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

		BlockLog("ComputeBlockStaminaCost: {:x} -> 0 (stub)"sv,
			actor->GetFormID());

		// TODO: flat cost from blockBurden + % max stamina from burdenBlend
		return 0.0f;
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

		BlockLog("ComputeDamageRedirectStaminaCost: {:x} totalDmg={:.1f} -> 0 (stub)"sv,
			actor->GetFormID(), hitData.totalDamage);

		// TODO: redirectMultiplier(blockBurden) * hitData.totalDamage
		return 0.0f;
	}

	void ApplyBlockDamageRedirect(RE::HitData& hitData, float redirectAmount)
	{
		BlockLog("ApplyBlockDamageRedirect: redirectAmount={:.1f}  from={:.1f}-> no-op (stub)"sv,
			redirectAmount, hitData.totalDamage);

		// TODO: hitData.totalDamage -= redirectAmount
	}
}
