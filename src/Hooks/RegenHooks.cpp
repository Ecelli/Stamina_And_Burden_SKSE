/*
 *  RegenHook — AV Regen Rate Interceptor
 *  =======================================
 *
 *  Hooks the call to the internal AV regen rate function at
 *  Address Library ID 38452 + 0x2B6 (AE). The engine function
 *  computes how much health/magicka/stamina an actor should
 *  regenerate per frame. We call the original to get the base
 *  rate, multiply by our formula, and return the modified rate.
 *
 *  Engine function signature:
 *    __m128 EngineAVRegen(Actor* actor, uint32_t av)
 *      actor — the actor being processed
 *      av    — 24 = Health, 25 = Magicka, 26 = Stamina
 *      return: __m128; lane 0 = regen rate (float), other lanes unused
 *
 *   __m128 details (you only need 3 operations):
 *     _mm_cvtss_f32(x)  — extract the first float from a __m128
 *     _mm_setr_ps(a,b,c,d) — create a __m128 with floats a,b,c,d
 *     _mm_setzero_ps()  — create a __m128 with all lanes = 0
 *
 *  How the hook works:
 *    1. Install() looks up ID 38452 + 0x2B6 (found via diagnostic scan)
 *    2. write_call<5> redirects that call to our Thunk
 *    3. Thunk calls the original _engineAVRegen to get the base rate
 *    4. Extracts the float, multiplies it, repacks and returns it
 */

#include "RegenHooks.h"
#include "Regen/RegenManager.h"
#include "Common/Utils.h"

namespace Hooks
{
	__m128 RegenHook::Thunk(RE::Actor* a_actor, std::uint32_t a_av)
	{
		if (!a_actor || (a_av != 24 && a_av != 25 && a_av != 26)) {
			return _engineAVRegen ? _engineAVRegen(a_actor, a_av) : _mm_setzero_ps();
		}

		// 1. Get the engine's base regen rate
		__m128 out = _engineAVRegen ? _engineAVRegen(a_actor, a_av) : _mm_setzero_ps();
		float rate = _mm_cvtss_f32(out);

		// 2. Multiply by our formula
		switch (a_av) {
		case 26:  // Stamina
			rate *= Regen::ComputeStaminaRegenMult(a_actor);
			Regen::RegenLog("Thunk: stamina rate -> {:.3f}", rate);
			break;
		case 24:  // Health
			rate *= Regen::ComputeHealthRegenMult(a_actor);
			Regen::RegenLog("Thunk: health rate -> {:.3f}", rate);
			break;
		case 25:  // Magicka
			rate *= Regen::ComputeMagickaRegenMult(a_actor);
			Regen::RegenLog("Thunk: magicka rate -> {:.3f}", rate);
			break;
		}

		// 3. If the formula returned a negative rate, drain the actor
		//    directly (RestoreActorValue won't accept negative heals)
		if (rate < 0.0f) {
			a_actor->DamageActorValue(static_cast<RE::ActorValue>(a_av), rate); // Internally it takes abs
			Regen::RegenLog("Thunk: {} drain -> {:.3f}", a_av == 26 ? "stamina" : a_av == 24 ? "health" : "magicka", -rate);
			rate = 0.0f;
		}

		// 4. Return the modified rate
		return _mm_setr_ps(rate, 0.0f, 0.0f, 0.0f);
	}

	void RegenHook::Install()
	{
		// ID 38452 + 0x2B6 (AE) — found via diagnostic analysis
		REL::Relocation<std::uintptr_t> callSite{ REL::ID(38452), 0x2B6 };

		_engineAVRegen = reinterpret_cast<AVRegen_t>(
			SKSE::GetTrampoline().write_call<5>(
				callSite.address(),
				reinterpret_cast<std::uintptr_t>(Thunk)));

		logger::info("  >Installed regen hook (ID 38452 + 0x2B6)");
	}

	// ---------------------------------------------------------------
	// Full-stamina monitor (heartbeat)
	//
	// The regen function doesn't fire when AV is at 100%, so our
	// Thunk can't drain. This heartbeat (100ms ~ 10fps) checks if
	// the player is at full stamina with a negative multiplier and
	// drains a tiny amount (0.1) to push below 100%. The next regen
	// tick will then fire normally and the Thunk handles the rest.
	//
	// Started from BurdenTracker::OnGameLoad() alongside the burden
	// parameter tracking heartbeat.
	// ---------------------------------------------------------------
	void TaskPlayerFullStaminaMonitor()
	{
		SKSE::GetTaskInterface()->AddTask([]() {
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player)
				return;

			const auto av = RE::ActorValue::kStamina;
			const float cur = player->GetActorValue(av);
			const float max = player->GetActorValueMax(av);
			if (max > 0.0f && cur >= max) {
				float mult = Regen::ComputeStaminaRegenMult(player);
				if (mult < 0.0f) {
					player->DamageActorValue(av, 0.1f);
					Regen::RegenLog("StaminaMonitor: drained 0.1 at full stamina (mult={:.3f})", mult);
				}
			}
		});
	}
}
