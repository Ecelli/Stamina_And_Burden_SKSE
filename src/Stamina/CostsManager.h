#pragma once

#include "Settings/Params/Parameter.h"

namespace RE
{
	class BGSAttackData;
}

namespace Costs
{
	float ComputeBaseAttackCost(RE::Actor* actor, bool bash, bool left, bool power);
	float ComputeAttackCost(RE::Actor* actor, RE::BGSAttackData* attackData);
	float ComputeBowFireCost(RE::Actor* actor);
	float ComputeStaffFireCost(RE::Actor* actor, bool leftHand);
}
