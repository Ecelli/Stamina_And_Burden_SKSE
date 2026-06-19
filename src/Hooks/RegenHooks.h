#pragma once

#include <xmmintrin.h>

namespace Hooks
{
	/*
	 * Intercepts the regen rate computation inside the game's per-frame AV regen
	 * function at ID 38452 + 0x2B6 (AE). Calls the original engine function to get
	 * the base regen rate, multiplies it by our formula, and returns the modified rate.
	 *
	 * The engine function returns the rate as __m128. We extract the float (lane 0),
	 * multiply it, and repack it.
	 */
	class RegenHook
	{
	public:
		static void Install();

	private:
		using AVRegen_t = __m128 (*)(RE::Actor*, std::uint32_t);
		static inline AVRegen_t _engineAVRegen = nullptr;

		static __m128 Thunk(RE::Actor* a_actor, std::uint32_t a_av);
	};

	void TaskPlayerFullStaminaMonitor();
}
