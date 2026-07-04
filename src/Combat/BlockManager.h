#pragma once

#include <RE/H/HitData.h>

namespace Blocking
{
	float ComputeBlockStaminaCost(RE::Actor* actor);
	float ComputeDamageRedirectStaminaCost(RE::Actor* actor, const RE::HitData& hitData);
	void ApplyBlockDamageRedirect(RE::HitData& hitData, float redirectAmount);
	float ComputeStaggerMagnitude(RE::Actor* actor, const RE::HitData& hitData);
	float ComputeStaggerDirection(RE::Actor* target, const RE::HitData& hitData);
}
