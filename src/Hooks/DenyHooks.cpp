#include "DenyHooks.h"
#include "Movement/MovementCostManager.h"
#include "Settings/Params/CostsParams.h"
#include "Stamina/CostsManager.h"
#include "Common/Utils.h"
#include <RE/J/JumpHandler.h>

namespace
{
	bool PlayerCanJump()
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return true;

		auto* costParams = Costs::CostsParams::GetSingleton();
		if (costParams->bJumpCostPlayer.Get()) {
			float cost = Movement::ComputeJumpCost(player);

			auto* jumpParams = JumpParams::GetSingleton();
			if (jumpParams->bJumpDenyPlayer.Get() && !Common::CanDoStaminaAction(player, cost)) {
				Deny::DenyLog("JumpInputHandler: denied (stamina={:.1f}, cost={:.1f})",
					player->GetActorValue(RE::ActorValue::kStamina), cost);
				return false;
			}
		}

		return true;
	}
}

namespace Hooks
{
	void JumpInputHandler::Install()
	{
		REL::Relocation<std::uintptr_t> vtbl{ RE::VTABLE_JumpHandler[0] };
		_ProcessButton = vtbl.write_vfunc(0x04, &ProcessButton);
		logger::info("  >JumpInputHandler installed at VTABLE index 04");
	}

	void JumpInputHandler::ProcessButton(
		RE::JumpHandler* a_this,
		RE::ButtonEvent* a_event,
		RE::PlayerControlsData* a_data)
	{
		if (a_event && a_event->IsDown() && !PlayerCanJump())
			return;

		_ProcessButton(a_this, a_event, a_data);
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

        bool isPlayer = actor->IsPlayerRef();
        auto* costParams = Costs::AttackCostParams::GetSingleton();
        auto* denyParams = Deny::DenyParams::GetSingleton();

        bool costEnabled = isPlayer ? costParams->bAttackCostPlayer.Get() : costParams->bAttackCostNPC.Get();
        if (!costEnabled)
            return _original(a_this, attackData);

        bool denyEnabled = isPlayer ? denyParams->bEnableDenyPlayer.Get() : denyParams->bEnableDenyNPC.Get();
        float cost = Costs::ComputeAttackCost(actor, attackData);

        if (!denyEnabled)
            return 0.0F;

		if (Common::CanDoStaminaAction(actor, cost))
            return 0.0f; // CAN DO

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
			_original = SKSE::GetTrampoline().write_call<5>(target.address(), PlayerHasStamina);
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
