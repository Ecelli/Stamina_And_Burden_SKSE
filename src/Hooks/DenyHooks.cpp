#include "DenyHooks.h"
#include "Movement/MovementCostManager.h"
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
		float cost = Movement::ComputeJumpCost(actor);
		if (Common::CanDoStaminaAction(actor, cost))
			_Jump(actor);
	}



	void AttackDenyHook::NopHasStaminaBranches()
	{
		{ // Player branch
			REL::Relocation<std::uintptr_t> target{ REL::ID(39003), 0xE1 };
			if (REL::make_pattern<"77 19">().match(target.address())) {
				REL::safe_write(target.address(), std::array<std::uint8_t, 2>{0x90, 0x90});
				logger::info("  >AttackDenyHook: player branch NOPed at 39003+0xE1");
			} else {
				logger::error("  >AttackDenyHook: player branch pattern fail at 39003+0xE1");
			}
		}
		{ // NPC branch
			REL::Relocation<std::uintptr_t> target{ REL::ID(49170), 0x272 };
			if (REL::make_pattern<"75 10">().match(target.address())) {
				REL::safe_write(target.address(), std::array<std::uint8_t, 2>{0x90, 0x90});
				logger::info("  >AttackDenyHook: NPC branch NOPed at 49170+0x272");
			} else {
				logger::error("  >AttackDenyHook: NPC branch pattern fail at 49170+0x272");
			}
		}
	}

	float AttackDenyHook::HasStamina(RE::ActorValueOwner* a_this, RE::BGSAttackData* attackData)
	{
		auto* actor = skyrim_cast<RE::Actor*>(a_this);
		if (!actor) return 0.0F;
        float cost = Costs::ComputeAttackCost(actor, attackData);
		if (Common::CanDoStaminaAction(actor, cost)) {
            return 0.0f; // CAN DO
        }

        Deny::DenyLog("AttackDeny: denied {:x}, stamina={:.1f} cost={:.1f}",
            actor->GetFormID(),
            actor->GetActorValue(RE::ActorValue::kStamina),
            cost);
        
        return cost;
	}

	float AttackDenyHook::PlayerHasStamina(RE::ActorValueOwner* a_this, RE::BGSAttackData* a_attack)
	{
		return HasStamina(a_this, a_attack);
	}

	float AttackDenyHook::NPCHasStamina(RE::ActorValueOwner* a_this, RE::BGSAttackData* a_attack)
	{
		return HasStamina(a_this, a_attack);
	}

	void AttackDenyHook::Install()
	{ // Based off scrambled bugs
		NopHasStaminaBranches();

		{ // Player Branch
			REL::Relocation<std::uintptr_t> target{ REL::ID(39003), 0xBB };
			if (!REL::make_pattern<"E8">().match(target.address())) {
				logger::error("  >AttackDenyHook: player call mismatch at 39003+0xBB");
				return;
			}
			SKSE::GetTrampoline().write_call<5>(target.address(), PlayerHasStamina);
			logger::info("  >AttackDenyHook: player installed at 39003+0xBB");
		}

		{  // NPC branch
			REL::Relocation<std::uintptr_t> target{ REL::ID(49170), 0x27A };
			if (!REL::make_pattern<"E8">().match(target.address())) {
				logger::error("  >AttackDenyHook: NPC call mismatch at 49170+0x27A");
				return;
			}
			SKSE::GetTrampoline().write_call<5>(target.address(), NPCHasStamina);
			logger::info("  >AttackDenyHook: NPC installed at 49170+0x27A");
		}
	}

}
