#pragma once

namespace Movement
{
	// Composite speed multiplier: BurdenMult * SwimMult * ExhaustionMult
	// Used by SpeedHook to scale Actor::SetMaximumMovementSpeed.
	float ComputeSpeedMultiplier(RE::Actor* a_actor);

	// Jump height multiplier: burdenBlend curve * exhaustion penalty
	// Used by JumpHook to scale the GetScale return value.
	float ComputeJumpHeightMult(RE::Actor* a_actor);
}
