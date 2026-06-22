#include "ActionHook.h"
#include "Stamina/CostsManager.h"
#include "Common/Utils.h"

namespace Hooks
{
	void ActionHook::Install()
	{
		// Denial hook — TBD
		//   SSE ref: REL::ID(41349) + 0x114 (StaminaNPC)
		//   AE candidate: REL::ID(42423) + 0x114
		//   Discovered via: Address Library 1.6.318 offsets + cross-match table
		//   Match score: 0.944, but offset NOT preserved on AE:
		//     - Cost hook offset changed 0x190->0x17f between versions
		//     - write_branch<5> at REL::ID(42423)+0x114 crashes on 1.6.1170
		//   Needs pattern scan or Cheat Engine to find correct AE call site
		// _Jump = SKSE::GetTrampoline().write_branch<5>(target.address(), JumpDetour);
		logger::info("  >ActionHook: denial NOT INSTALLED (AE call site for jump entry unknown)"sv);

		// Cost hook — GetScale call inside jump physics processing
		REL::Relocation<std::uintptr_t> target{ REL::ID(37257), 0x17f };
		_GetScale = SKSE::GetTrampoline().write_call<5>(target.address(), ApplyJumpCost);
		logger::info("  >ActionHook: jump cost installed at REL::ID(37257) + 0x17F"sv);
	}

	void ActionHook::JumpDetour(RE::Actor* actor)
	{
		float cost = Costs::CalculateJumpCost(actor);
		if (Common::CanDoAction(actor, cost))
			_Jump(actor);
	}

	float ActionHook::ApplyJumpCost(RE::Actor* actor)
	{
		if (actor)
			Common::ApplyStaminaCost(actor, Costs::CalculateJumpCost(actor));
		return _GetScale(actor);
	}
}
