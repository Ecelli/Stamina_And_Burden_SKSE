#include "DenyHooks.h"
#include "Movement/MovementCostManager.h"
#include "Stamina/CostsManager.h"
#include "Common/Utils.h"

#include <RE/A/AttackBlockHandler.h>
#include <RE/B/ButtonEvent.h>
#include <RE/P/PlayerControls.h>
#include <RE/P/PlayerCharacter.h>

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


	void PowerAttackDenyHook::NopHasStaminaBranches()
	{
		{ // Player branch
			REL::Relocation<std::uintptr_t> target{ REL::ID(39003), 0xE1 };
			if (REL::make_pattern<"77 19">().match(target.address())) {
				REL::safe_write(target.address(), std::array<std::uint8_t, 2>{0x90, 0x90});
				logger::info("  >PowerAttackDenyHook: player branch NOPed at 39003+0xE1");
			} else {
				logger::error("  >PowerAttackDenyHook: player branch pattern fail at 39003+0xE1");
			}
		}
		{ // NPC branch
			REL::Relocation<std::uintptr_t> target{ REL::ID(49170), 0x272 };
			if (REL::make_pattern<"75 10">().match(target.address())) {
				REL::safe_write(target.address(), std::array<std::uint8_t, 2>{0x90, 0x90});
				logger::info("  >PowerAttackDenyHook: NPC branch NOPed at 49170+0x272");
			} else {
				logger::error("  >PowerAttackDenyHook: NPC branch pattern fail at 49170+0x272");
			}
		}
	}

	float PowerAttackDenyHook::HasStamina(RE::ActorValueOwner* a_this, RE::BGSAttackData*)
	{
		auto* actor = skyrim_cast<RE::Actor*>(a_this);
		if (!actor) return 0.0F;
		return actor->GetActorValue(RE::ActorValue::kStamina) > 40.0F ? 0.0F : 1.0F;
	}

	float PowerAttackDenyHook::PlayerHasStamina(RE::ActorValueOwner* a_this, RE::BGSAttackData* a_attack)
	{
		return HasStamina(a_this, a_attack);
	}

	float PowerAttackDenyHook::NPCHasStamina(RE::ActorValueOwner* a_this, RE::BGSAttackData* a_attack)
	{
		return HasStamina(a_this, a_attack);
	}

	void PowerAttackDenyHook::Install()
	{ // Based off scrambled bugs
		NopHasStaminaBranches();

		{ // Player Branch
			REL::Relocation<std::uintptr_t> target{ REL::ID(39003), 0xBB };
			if (!REL::make_pattern<"E8">().match(target.address())) {
				logger::error("  >PowerAttackDenyHook: player call mismatch at 39003+0xBB");
				return;
			}
			SKSE::GetTrampoline().write_call<5>(target.address(), PlayerHasStamina);
			logger::info("  >PowerAttackDenyHook: player installed at 39003+0xBB");
		}

		{  // NPC branch
			REL::Relocation<std::uintptr_t> target{ REL::ID(49170), 0x27A };
			if (!REL::make_pattern<"E8">().match(target.address())) {
				logger::error("  >PowerAttackDenyHook: NPC call mismatch at 49170+0x27A");
				return;
			}
			SKSE::GetTrampoline().write_call<5>(target.address(), NPCHasStamina);
			logger::info("  >PowerAttackDenyHook: NPC installed at 49170+0x27A");
		}
	}

	void PlayerNormalAttackDenyHook::Install()
	{
		auto* playerControls = RE::PlayerControls::GetSingleton();
		if (!playerControls || !playerControls->attackBlockHandler) {
			logger::error("  >PlayerNormalAttackDenyHook: AttackBlockHandler unavailable");
			return;
		}

		auto* handler = playerControls->attackBlockHandler;
		auto* vtable = *reinterpret_cast<std::uintptr_t**>(handler);
		logger::info("  >PlayerNormalAttackDenyHook: original ProcessButton at 0x{:X}", vtable[4]);

		REL::Relocation<std::uintptr_t> vtableReloc{ RE::VTABLE_AttackBlockHandler[0] };
		_original = vtableReloc.write_vfunc(4, &ProcessButtonDetour);
		logger::info("  >PlayerNormalAttackDenyHook installed on AttackBlockHandler::ProcessButton");
	}

	void PlayerNormalAttackDenyHook::ProcessButtonDetour(
		RE::AttackBlockHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data)
	{
		if (a_event && a_event->IsDown()) {
			auto* player = RE::PlayerCharacter::GetSingleton();
			if (player) {
				logger::info("PNADH: player attack press detected for {:x}", player->GetFormID());
			}
		}
		_original(a_this, a_event, a_data);
	}
}
