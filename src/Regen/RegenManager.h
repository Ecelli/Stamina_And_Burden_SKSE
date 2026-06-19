#pragma once

#include "Settings/Params/RegenParams.h"

namespace Regen
{
	float ComputeStaminaRegenMult(RE::Actor* actor);
	float ComputeHealthRegenMult(RE::Actor* actor);
	float ComputeMagickaRegenMult(RE::Actor* actor);
}
