#pragma once

#include <unordered_map>

namespace Exhaustion
{
    struct ExhaustionState
	{
		bool isExhausted = false;
		float safeTimer = 0.0f;
	};

	class ExhaustionManager : public REX::Singleton<ExhaustionManager>
	{
	public:
		void TriggerExhaustion(RE::Actor* a_actor);
		bool IsExhausted(RE::Actor* a_actor);
		void UpdateExhaustion(RE::Actor* a_actor, float a_deltaTime);
		void ClearAll();

	private:
		std::unordered_map<RE::FormID, ExhaustionState> states;

		void ClearExhaustion(RE::FormID a_formId);
	};

	void CheckForAndTriggerExhaustion(RE::Actor* a_actor, float a_deltaTime);
	float GetExhaustionDamageMultiplier(RE::Actor* a_actor);
	float GetExhaustionRegenMult(RE::Actor* a_actor, RE::ActorValue a_av);
}
