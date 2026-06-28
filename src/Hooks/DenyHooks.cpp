#include "DenyHooks.h"
#include "Stamina/CostsManager.h"
#include "Common/Utils.h"

namespace Hooks
{
	void JumpDenyHook::Install()
	{
		// SSE: REL::ID(41349) + 0x114 — works in StaminaNPC
		// AE: REL::ID(42423) + 0x114 — write_branch<5> crashes on 1.6.1170
		// Needs pattern scan or Cheat Engine to find correct AE call site
		logger::info("  >JumpDenyHook: NOT INSTALLED (AE call site for jump entry unknown)");
	}

	void JumpDenyHook::JumpDetour(RE::Actor* actor)
	{
		float cost = Costs::ComputeJumpCost(actor);
		if (Common::CanDoStaminaAction(actor, cost))
			_Jump(actor);
	}


	void AttackDenyHook::Install()
	{
		// AE: REL::ID(49170) + 0x28d inside GetThisAttackChance
		// NOTE: NPC only — does NOT fire for the player.
		REL::Relocation<std::uintptr_t> target{ REL::ID(49170), 0x28d };
		_func = SKSE::GetTrampoline().write_call<5>(target.address(), Call);
		logger::info("  >AttackDenyHook installed at REL::ID(49170) + 0x28d");
	}

	float AttackDenyHook::Call(RE::Actor* a_attacker, RE::Actor* a_victim, RE::BGSAttackData* a_attack)
	{
		Costs::CostLog("AttackDenyHook: attacker={:x} victim={:x}",
			a_attacker ? a_attacker->GetFormID() : 0,
			a_victim ? a_victim->GetFormID() : 0);

		if (!a_attacker || !a_attack)
			return _func(a_attacker, a_victim, a_attack);

		float cost = Costs::ComputeAttackCost(a_attacker, a_attack);

		if (!Common::CanDoStaminaAction(a_attacker, cost)) {
			Costs::CostLog("AttackDenyHook: suppressed for {:x}, stamina={:.1f} < cost={:.1f}",
				a_attacker->GetFormID(),
				a_attacker->GetActorValue(RE::ActorValue::kStamina), cost);
			return 0.0f;
		}

		return _func(a_attacker, a_victim, a_attack);
	}
}
