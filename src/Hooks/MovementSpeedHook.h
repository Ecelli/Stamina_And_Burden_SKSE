#pragma once

namespace Hooks
{
	/*
	 * Intercepts Actor::SetMaximumMovementSpeed to apply burden-based
	 * speed scaling. The original return value is multiplied by a composite
	 * factor computed by Movement::ComputeSpeedMultiplier.
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
	};
}
