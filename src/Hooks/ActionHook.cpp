#include "ActionHook.h"
#include "Stamina/CostsManager.h"
#include "Common/Utils.h"

namespace Hooks
{
	void ActionHook::Install()
	{
		REL::Relocation<std::uintptr_t> target{ REL::ID(37257), 0x17f };
		_GetScale = SKSE::GetTrampoline().write_call<5>(target.address(), ApplyJumpCost);
		logger::info("  >ActionHook: jump cost installed at REL::ID(37257) + 0x17F"sv);
	}

	float ActionHook::ApplyJumpCost(RE::Actor* actor)
	{
		if (actor)
			Common::ApplyStaminaCost(actor, Costs::ComputeJumpCost(actor));
		return _GetScale(actor);
	}
}
