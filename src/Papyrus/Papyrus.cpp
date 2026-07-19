#include "papyrus.h"
#include "Burden/BurdenTracker.h"
#include "Stamina/ExhaustionManager.h"
#include "API/StaminaAndBurdenAPI.h"

namespace Papyrus
{
	std::vector<int> GetVersion(STATIC_ARGS) {
		return { Plugin::VERSION[0], Plugin::VERSION[1], Plugin::VERSION[2] };
	}

	float GetBurdenByIndex(STATIC_ARGS, RE::Actor* a_actor, int a_index) {
		if (!a_actor) return 0.0f;
		return Burden::ResolveBurdenValue(Burden::Tracker::GetOrComputeBurden(a_actor), a_index);
	}

	float GetBurden(STATIC_ARGS, RE::Actor* a_actor) {
		if (!a_actor) return 0.0f;
		return Burden::Tracker::GetOrComputeBurden(a_actor).burden;
	}

	float GetCarryBurden(STATIC_ARGS, RE::Actor* a_actor) {
		if (!a_actor) return 0.0f;
		return Burden::Tracker::GetOrComputeBurden(a_actor).carryBurden;
	}

	float GetBurdenBlend(STATIC_ARGS, RE::Actor* a_actor) {
		if (!a_actor) return 0.0f;
		return Burden::Tracker::GetOrComputeBurden(a_actor).burdenBlend;
	}

	float GetEffectiveEquippedWeight(STATIC_ARGS, RE::Actor* a_actor) {
		if (!a_actor) return 0.0f;
		return Burden::Tracker::GetOrComputeBurden(a_actor).equippedWeight;
	}

	float GetMaxEquippedWeight(STATIC_ARGS, RE::Actor* a_actor) {
		if (!a_actor) return 0.0f;
		return Burden::Tracker::GetOrComputeBurden(a_actor).maxEquippedWeight;
	}

	float ComputeActionCost(STATIC_ARGS, RE::Actor* a_actor,
		int a_baseComponent, float a_baseMin, float a_baseMax, float a_baseK,
		int a_pctComponent, float a_pctMin, float a_pctMax, float a_pctK)
	{
		if (!a_actor || !StaminaAndBurdenAPI::Interface) return 0.0f;
		return StaminaAndBurdenAPI::Interface->ComputeActionCost(a_actor,
			static_cast<StaminaAndBurdenAPI::BurdenComponent>(a_baseComponent), a_baseMin, a_baseMax, a_baseK,
			static_cast<StaminaAndBurdenAPI::BurdenComponent>(a_pctComponent), a_pctMin, a_pctMax, a_pctK);
	}

	bool IsExhausted(STATIC_ARGS, RE::Actor* a_actor) {
		if (!a_actor) return false;
		return Exhaustion::ExhaustionManager::GetSingleton()->IsExhausted(a_actor);
	}

	void Bind(VM& a_vm) {
		logger::info("  >Binding GetVersion..."sv);
		BIND(GetVersion);
		logger::info("  >Binding burden queries..."sv);
		BIND(GetBurdenByIndex);
		BIND(GetBurden);
		BIND(GetCarryBurden);
		BIND(GetBurdenBlend);
		BIND(GetEffectiveEquippedWeight);
		BIND(GetMaxEquippedWeight);
		logger::info("  >Binding ComputeActionCost..."sv);
		BIND(ComputeActionCost);
		logger::info("  >Binding IsExhausted..."sv);
		BIND(IsExhausted);
	}

	bool RegisterFunctions(VM* a_vm) {
		logger::info("Binding papyrus functions in utility script {}..."sv, script);
		Bind(*a_vm);
		logger::info("Finished binding functions."sv);
		return true;
	}
}
