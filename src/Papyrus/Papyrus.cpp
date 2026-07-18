#include "papyrus.h"
#include "Burden/BurdenTracker.h"

namespace Papyrus
{
	std::vector<int> GetVersion(STATIC_ARGS) {
		return { Plugin::VERSION[0], Plugin::VERSION[1], Plugin::VERSION[2] };
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

	float GetEquippedWeight(STATIC_ARGS, RE::Actor* a_actor) {
		if (!a_actor) return 0.0f;
		return Burden::Tracker::GetOrComputeBurden(a_actor).equippedWeight;
	}

	float GetMaxEquippedWeight(STATIC_ARGS, RE::Actor* a_actor) {
		if (!a_actor) return 0.0f;
		return Burden::Tracker::GetOrComputeBurden(a_actor).maxEquippedWeight;
	}

	void Bind(VM& a_vm) {
		logger::info("  >Binding GetVersion..."sv);
		BIND(GetVersion);
		logger::info("  >Binding burden queries..."sv);
		BIND(GetBurden);
		BIND(GetCarryBurden);
		BIND(GetBurdenBlend);
		BIND(GetEquippedWeight);
		BIND(GetMaxEquippedWeight);
	}

	bool RegisterFunctions(VM* a_vm) {
		logger::info("Binding papyrus functions in utility script {}..."sv, script);
		Bind(*a_vm);
		logger::info("Finished binding functions."sv);
		return true;
	}
}
