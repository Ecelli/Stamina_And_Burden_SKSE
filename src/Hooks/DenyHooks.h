#pragma once

#include <RE/B/BGSAttackData.h>

namespace Hooks
{
	// NPC-only: verified at REL::ID(49170) + 0x28d fires for NPCs (formID 85452)
	// but NOT for the player (formID 14). Player denial needs a different approach
	// (e.g. vtable hook on AttackBlockHandler::ProcessButton).
	//
	// Findings (2026-06-24):
	//   - AttackCostHook at ID(38603)+0x171 fires for ALL actors (player + NPC)
	//   - AttackChanceHook at ID(49170)+0x28d fires for NPCs only
	//   - Player's attack denial requires a different entry point
	//   - StaminaNPC uses ID(38047)+0xBB (SE) for player denial — unverified on AE
	//   - Vtable hook on AttackBlockHandler::ProcessButton (index 04) is
	//     version-stable and doesn't need offsets — preferred approach
	struct AttackDenyHook
	{
		static void Install();
		static float Call(RE::Actor* a_attacker, RE::Actor* a_victim, RE::BGSAttackData* a_attack);
		static inline REL::Relocation<decltype(Call)> _func;
	};

	// Jump denial — blocked, AE call site unknown
	// SSE ref: REL::ID(41349) + 0x114
	// AE candidate: REL::ID(42423) + 0x114 — crashes on 1.6.1170
	struct JumpDenyHook
	{
		static void Install();
		static void JumpDetour(RE::Actor* actor);
		static inline REL::Relocation<decltype(JumpDetour)> _Jump;
	};

	// Power attack denial via ScrambledBugs pattern:
	//   - NOP HasStamina branches (39003+0xE1, 49170+0x272)
	//   - Hook GetAttackStamina calls (39003+0xBB, 49170+0x27A)
	//   - No call to original — directly return 0.0F (allow) or 1.0F (deny)
	struct PowerAttackDenyHook
	{
		static void Install();
		static void NopHasStaminaBranches();
		static float HasStamina(RE::ActorValueOwner* a_this, RE::BGSAttackData* a_attack);
		static float PlayerHasStamina(RE::ActorValueOwner* a_this, RE::BGSAttackData* a_attack);
		static float NPCHasStamina(RE::ActorValueOwner* a_this, RE::BGSAttackData* a_attack);
	};

	// Player normal attack denial via AttackBlockHandler::ProcessButton vtable hook.
	// VTABLE_AttackBlockHandler[0] vtable index 04. Step 1: pass-through + logging.
	struct PlayerNormalAttackDenyHook
	{
		static void Install();
		static void ProcessButtonDetour(RE::AttackBlockHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data);
		static inline REL::Relocation<decltype(ProcessButtonDetour)> _original;
	};
}
