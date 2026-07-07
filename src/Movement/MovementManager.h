#pragma once

namespace Movement
{
	// Composite speed multiplier: BurdenMult * SwimMult * ExhaustionMult
	// Used by MovementSpeedHook to scale Actor::SetMaximumMovementSpeed.
	float ComputeSpeedMultiplier(RE::Actor* a_actor);
}
