#pragma once

namespace Hooks
{
	/*
	 * Intercepts Actor::SetMaximumMovementSpeed to apply burden-based
	 * speed scaling. The original return value is multiplied by a composite
	 * factor: BurdenMult * SwimMult * ExhaustionMult.
	 *
	 * AE: REL::ID(37943) + 0x51 — call to SetMaximumMovementSpeed
	 *     inside the actor movement update loop.
	 */
	class MovementSpeedHook
	{
	public:
		static void Install();

	private:
		// Thunk: calls original SetMaximumMovementSpeed, then multiplies by our factor
		static float Speed(RE::Actor* a_actor);
		static inline REL::Relocation<decltype(Speed)> _func;

		// Engine call: GetSubmergedLevel(Actor*, float zPos, TESObjectCELL*)
		// REL::ID(37448) — AE
		static float GetSubmergedLevel(RE::Actor* a_actor);

		// Core formula: BurdenMult * SwimMult * ExhaustionMult
		// Returns a_originalSpeed * composite multiplier.
		static float ComputeSpeedMultiplier(RE::Actor* a_actor);
	};
}
