#pragma once

namespace Hooks
{
	/*
	 * Hooks two adjacent call sites inside the actor sprint processing function
	 * (Address Library ID 38022, AE) to replace the sprint stamina drain with our
	 * burden-based formula.
	 *
	 * Engine flow per sprinting actor every frame:
	 *   0xC1: weight = getEquippedWeight(actor)
	 *   0xC9: drain  = getSprintStaminaDrain(weight, deltaTime)
	 *   engine: stamina -= drain
	 *
	 * We hook 0xC1 to calculate our burden-based drain (using CalculateSprintDrain
	 * which includes GetSecondsSinceLastFrame()). The 0xC9 hook is a straight
	 * passthrough that returns the value as-is.
	 *
	 * IMPORTANT: write_call<5> at ID(38022) + 0xC1 replaces one specific call
	 * instruction INSIDE the sprint processing function, NOT the standalone
	 * getEquippedWeight function. Other callers of getEquippedWeight (encumbrance,
	 * animation) are completely untouched. The trampoline _getEquippedWeight
	 * preserves the original call target if needed.
	 */
	class SprintDrainHook
	{
	public:
		static void Install();

	private:
		// 0xC1: float(RE::Actor*) — replaces equipped weight with burden drain
		static float GetEquippedWeight(RE::Actor* actor);
		static inline REL::Relocation<decltype(GetEquippedWeight)> _getEquippedWeight;

		// 0xC9: float(float weight, float delta) — passthrough
		static float GetSprintStaminaDrain(float weight, float delta);
		static inline REL::Relocation<decltype(GetSprintStaminaDrain)> _getSprintStaminaDrain;
	};
}
