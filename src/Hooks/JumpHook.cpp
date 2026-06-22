#include "JumpHook.h"
#include "Stamina/CostsManager.h"
#include "Common/Utils.h"

namespace Hooks
{
	void JumpHook::Install()
	{
		// TODO: AE offsets TBD, Reference Fenix Stamina
		//   Denial: SSR ref REL::ID(41349) + 0x114 → write_branch<5>
		//   Cost:   SSR ref REL::ID(36271) + 0x190 → write_call<5>
		// Once known:
		//   REL::Relocation<std::uintptr_t> target1{ REL::ID(TBD), 0xTBD };
		//   _Jump = SKSE::GetTrampoline().write_branch<5>(target1.address(), JumpDetour);
		//   REL::Relocation<std::uintptr_t> target2{ REL::ID(TBD), 0xTBD };
		//   _GetScale = SKSE::GetTrampoline().write_call<5>(target2.address(), ApplyJumpCost);
		logger::info("  >JumpHook: NOT INSTALLED (AE offsets TBD)"sv);
	}

	void JumpHook::JumpDetour(RE::Actor* actor)
	{
		float cost = Costs::CalculateJumpCost(actor);
		if (Common::CanDoAction(actor, cost))
			_Jump(actor);
	}

	float JumpHook::ApplyJumpCost(RE::Actor* actor)
	{
		if (actor)
			Common::ApplyStaminaCost(actor, Costs::CalculateJumpCost(actor));
		return _GetScale(actor);
	}
}
