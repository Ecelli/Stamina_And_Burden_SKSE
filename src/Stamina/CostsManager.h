#pragma once

#include "Settings/Params/Parameter.h"

namespace RE
{
	class BGSAttackData;
}

namespace Costs
{
	float ComputeSprintDrain(RE::Actor* actor);
	float ComputeJumpCost(RE::Actor* actor);
	float ComputeAttackCost(RE::Actor* actor, RE::BGSAttackData* attackData);
	float ComputeBowFireCost(RE::Actor* actor);
}
