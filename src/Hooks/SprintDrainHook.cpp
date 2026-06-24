#include "SprintDrainHook.h"
#include "Stamina/CostsManager.h"
#include "Common/Utils.h"

namespace Hooks
{
	// Taken from https://www.nexusmods.com/skyrimspecialedition/mods/128208
	void SprintDrainHook::Install()
	{
		auto base = REL::ID(38022);
		auto& trampoline = SKSE::GetTrampoline();

		// 0xC1: replaces call to getEquippedWeight(actor)
		REL::Relocation<std::uintptr_t> call1{ base, 0xC1 };
		_getEquippedWeight = trampoline.write_call<5>(
			call1.address(),
			GetEquippedWeight);

		// 0xC9: replaces call to getSprintStaminaDrain(weight, delta)
		REL::Relocation<std::uintptr_t> call2{ base, 0xC9 };
		_getSprintStaminaDrain = trampoline.write_call<5>(
			call2.address(),
			GetSprintStaminaDrain);

		logger::info("  >Installed sprint drain hook (ID 38022 + 0xC1 / 0xC9)");
	}

	float SprintDrainHook::GetEquippedWeight(RE::Actor* actor)
	{
		if (!actor)
			return 0.0f;

		float drain = Costs::ComputeSprintDrain(actor);
		Costs::CostLog("SprintDrain: {:.3f} for {:x}", drain, actor->GetFormID());
		return drain;
	}

    // In the game this function performs a computation, but we just use it as
    // passthrough because we computed for actor with getequippedweight
	float SprintDrainHook::GetSprintStaminaDrain(float weight, float)
	{
		return weight;
	}
}
