#pragma once

#include <xmmintrin.h>

namespace Hooks
{
	/*
	 * Intercepts the regen rate computation inside the game's per-frame AV regen
	 * function at ID 38452 + 0x2B6 (AE). Calls the original engine function to get
	 * the base regen rate, multiplies it by our formula, and returns the modified rate.
	 *
	 * When the result is negative for stamina, the rate is cached for the
	 * RegenDelayHook to drain per-frame, and 0 is returned (the engine ignores
	 * negative rates at the RestoreActorValue call in +0xE1).
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

		static __m128 InterceptAVRegen(RE::Actor* a_actor, std::uint32_t a_av);
	};

	/*
	 * Intercepts the update_RegenDelay call inside function 38452 at +0x02C (AE).
	 * When the RegenHook has cached a negative drain rate, we drain stamina via
	 * DamageActorValue and return false to bypass the regen delay check, so the
	 * engine proceeds to +0x2B6 for the next rate recomputation. The RegenHook
	 * then returns 0 (engine ignores negative rates at the RestoreActorValue
	 * call at +0xE1) and caches the new negative rate for the next frame.
	 */
	class RegenDelayHook
	{
	public:
		static void Install();

	private:
		using UpdateRegenDelay_t = bool (*)(RE::Actor*, RE::ActorValue, float);
		static inline UpdateRegenDelay_t _original = nullptr;

		static bool InterceptUpdateRegenDelay(RE::Actor* a_actor, RE::ActorValue a_av, float a_passedTime);
	};

	void PlayerFullStaminaMonitor();
	void ClearRegenDrainCache();
}
