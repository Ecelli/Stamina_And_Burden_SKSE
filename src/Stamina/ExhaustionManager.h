#pragma once

#include "Common/LockedMap.h"
#include <SKSE/RegistrationSet.h>
#include "API/TrueHUDAPI.h"

namespace Exhaustion
{
	inline TRUEHUD_API::IVTrueHUD2* g_trueHUD = nullptr;
	inline bool g_trueHUDAvailable = false;
    struct ExhaustionState
	{
		bool isExhausted = false;
		float safeTimer = 0.0f;
	};

	class ExhaustionManager : public REX::Singleton<ExhaustionManager>
	{
	public:
		using ExhaustionListener = void(*)(RE::Actor* actor, bool exhausted);

		void TriggerExhaustion(RE::Actor* a_actor);
		bool IsExhausted(RE::Actor* a_actor);
		void UpdateExhaustion(RE::Actor* a_actor, float a_deltaTime);
		void ClearAll();
		void RegisterExhaustionListener(ExhaustionListener a_listener);

		// Papyrus events
		// Registration set/Map is a store <template> type for papyrus events
		// When triggering an event the "string" is the event name "OnExhaustionChanged"
		// register/unregister for event APIs add/remove consumers for the event
		// NOTE: RegistrationMap<Filter, Args...> FormID is the Key, but is not part of the return value
		//       Hence we need to return the actor
		SKSE::RegistrationSet<const RE::Actor*, bool>  exhaustionChanged{ "OnExhaustionChanged"sv };
		SKSE::RegistrationMap<RE::FormID, const RE::Actor*, bool>  actorExhaustionChanged{ "OnActorExhaustionChanged"sv };

	private:
		LockedMap<RE::FormID, ExhaustionState> states;
	};

	void CheckForAndTriggerExhaustion(RE::Actor* a_actor, float a_deltaTime);
	float GetExhaustionDamageMultiplier(RE::Actor* a_actor);
	float GetExhaustionRegenMult(RE::Actor* a_actor, RE::ActorValue a_av);
	void SetStaminaBarGrayIfPlayer(RE::Actor* a_actor);
	void ResetStaminaBarColorIfPlayer(RE::Actor* a_actor);
}
