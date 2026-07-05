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
 *    2. write_call<5> redirects that call to our InterceptAVRegen
 *    3. InterceptAVRegen calls the original _engineAVRegen to get the base rate
 *    4. Extracts the float, multiplies it, repacks and returns it
 */

#include "RegenHooks.h"
#include "Stamina/RegenManager.h"
#include "Stamina/ExhaustionManager.h"
#include "Common/Utils.h"
#include <unordered_map>

namespace
{
	std::unordered_map<RE::Actor*, float> s_cachedDrainRate;
}

namespace Hooks
{
	__m128 RegenHook::InterceptAVRegen(RE::Actor* a_actor, std::uint32_t a_av)
	{
		if (!a_actor || (a_av != 24 && a_av != 25 && a_av != 26)) {
			return _engineAVRegen ? _engineAVRegen(a_actor, a_av) : _mm_setzero_ps();
		}

		// 1. Get the engine's base regen rate
		__m128 out = _engineAVRegen ? _engineAVRegen(a_actor, a_av) : _mm_setzero_ps();
		float rate = _mm_cvtss_f32(out);

		// 2. Multiply by our formula
		switch (a_av) {
		case 26: // Stamina
			rate = Regen::ComputeBurdenStaminaRegenRate(a_actor);
			break;
		case 24:  // Health
			rate *= Regen::ComputeHealthRegenMult(a_actor);
			break;
		case 25:  // Magicka
			rate *= Regen::ComputeMagickaRegenMult(a_actor);
			break;
		}

		// 3. If negative, cache for RegenDelayHook's bypass check
		//    and return 0 (engine ignores negative rates at +0xE1).
		if (rate < 0.0f) {
			s_cachedDrainRate[a_actor] = rate;
			rate = 0.0f;
		}
		else if (a_av == 26) {
			s_cachedDrainRate.erase(a_actor);
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
				reinterpret_cast<std::uintptr_t>(InterceptAVRegen)));

		logger::info("  >Installed regen hook (ID 38452 + 0x2B6)");
	}

	bool RegenDelayHook::InterceptUpdateRegenDelay(
		RE::Actor* a_actor, RE::ActorValue a_av, float a_passedTime)
	{
		if (a_av == RE::ActorValue::kStamina) {
			Exhaustion::CheckForAndTriggerExhaustion(a_actor);

			auto it = s_cachedDrainRate.find(a_actor);
			if (it != s_cachedDrainRate.end() && it->second < 0.0f) {
				// Bypass the regen delay check — drain stamina
				// directly and return false so the engine proceeds
				// to +0x2B6 (rate recomputation and cache update).
				float drain = -it->second * a_passedTime;
				a_actor->DamageActorValue(a_av, drain);
				return false;
			}
		}
		return _original(a_actor, a_av, a_passedTime);
	}

	void RegenDelayHook::Install()
	{
		REL::Relocation<std::uintptr_t> callSite{ REL::ID(38452), 0x02C };
		_original = reinterpret_cast<UpdateRegenDelay_t>(
			SKSE::GetTrampoline().write_call<5>(
				callSite.address(),
				reinterpret_cast<std::uintptr_t>(InterceptUpdateRegenDelay)));
		logger::info("  >Installed regen delay hook (ID 38452 + 0x02C) — bypass on negative rate");
	}

	void ClearRegenDrainCache()
	{
		s_cachedDrainRate.clear();
	}

	// ---------------------------------------------------------------
	// Full-stamina monitor (heartbeat)
	//
	// The regen function doesn't fire when AV is at 100%, so our
	// InterceptAVRegen can't drain. This heartbeat computes what the
	// effective regen rate *would* be via ComputeBurdenStaminaRegenRate
	// and drains 0.1 if negative, pushing stamina below 100% so the
	// next regen tick fires normally.
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
				float rate = Regen::ComputeBurdenStaminaRegenRate(player);
				if (rate < 0.0f) {
					player->DamageActorValue(av, 0.1f);
					Regen::RegenLog("StaminaMonitor: drained 0.1 at full stamina (rate={:.3f}/s)", rate);
				}
			}
		});
	}
}
