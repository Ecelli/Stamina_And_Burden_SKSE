#pragma once

#include "Settings/Params/Parameter.h"

namespace RE
{
	struct BGSAttackData;
}

namespace Costs
{
	float ComputeSprintDrain(RE::Actor* actor);
	float ComputeJumpCost(RE::Actor* actor);
	float ComputeAttackCost(RE::Actor* actor, RE::BGSAttackData* attackData);
}
