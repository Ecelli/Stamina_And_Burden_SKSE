#include "WorldFrameHook.h"
#include "Burden/BurdenTracker.h"
#include "Hooks/RegenHooks.h"

namespace Hooks
{
	void WorldFrameHook::Install()
	{
		// ID 36564 = AE world update function (Taken from Blade and Blunt)
		// offset 0x6E within that function is a `call` instruction we can detour
		// (BladeAndBlunt uses RelocationID(35565, 36564) with Relocate(0x1E, 0x6E))
		REL::Relocation<std::uintptr_t> callSite{ REL::ID(36564), 0x6E };

		_original = reinterpret_cast<OnFrame_t>(
			SKSE::GetTrampoline().write_call<5>(
				callSite.address(),
				reinterpret_cast<std::uintptr_t>(OnFrameUpdate)));

		logger::info("  >Installed world frame hook (ID 36564 + 0x6E)");
	}

	std::int32_t WorldFrameHook::OnFrameUpdate(std::int64_t a1)
	{
		// Every 6th frame (~100ms at 60fps, ~240ms at 25fps):
		if (++_frameCounter >= 6) [[unlikely]] {
			_frameCounter = 0;
		//   1) Poll player stamina — nudge below full if regen mult is negative
			Hooks::PlayerFullStaminaMonitor();
		//   2) Poll tracked actor params — trigger Update() if skills/weight changed
			Burden::Tracker::PollTrackedActorParams();
		}

		return _original(a1);
	}
}
