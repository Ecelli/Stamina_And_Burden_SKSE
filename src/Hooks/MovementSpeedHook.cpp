#include "Hooks/MovementSpeedHook.h"
#include "Movement/MovementManager.h"

namespace Hooks
{
	void MovementSpeedHook::Install()
	{
		// AE: REL::ID(37943) + 0x51 — call to SetMaximumMovementSpeed
		//     inside the actor movement update loop.
		//     Confirmed by WadeInWaterRedux reference mod.
		REL::Relocation<std::uintptr_t> target{ REL::ID(37943), 0x51 };

		if (!REL::make_pattern<"E8">().match(target.address())) {
			logger::error("  >MovementSpeedHook: pattern mismatch at REL::ID(37943) + 0x51"sv);
			return;
		}

		_func = SKSE::GetTrampoline().write_call<5>(target.address(), Speed);
		logger::info("  >MovementSpeedHook installed at REL::ID(37943) + 0x51");
	}

	float MovementSpeedHook::Speed(RE::Actor* a_actor)
	{
		float original_speed = _func(a_actor);

		if (!a_actor)
			return original_speed;

		return original_speed * Movement::ComputeSpeedMultiplier(a_actor);
	}
}
