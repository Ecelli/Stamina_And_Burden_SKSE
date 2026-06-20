#pragma once

#include "Settings/Params/RegenParams.h"
#include "Burden/BurdenManager.h"

namespace Regen
{
	enum class MovementState
	{
		kStatic,
		kWalking,
		kSneaking,
		kRunning,
		kSwimming
	};

	MovementState GetMovementState(RE::Actor* actor);
	float ComputeStateRegenFactor(const Burden::ActorBurdenData& data, MovementState state, float HMS);
	float ComputeBlockCost(const Burden::ActorBurdenData& data);
	float GetHMSStaminaMult(RE::Actor* actor, const Burden::ActorBurdenData& data);

	float ComputeStaminaRegenMult(RE::Actor* actor);
	float ComputeHealthRegenMult(RE::Actor* actor);
	float ComputeMagickaRegenMult(RE::Actor* actor);
}
