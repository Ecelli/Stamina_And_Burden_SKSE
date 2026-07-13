#pragma once

#include <RE/B/BGSAttackData.h>

namespace Hooks
{
	// Player-only jump denial via JumpHandler::ProcessButton VTABLE hook.
	// Prevents jumping when stamina is insufficient, before any animation or
	// physics processing begins.
	// NPCs are excluded — they don't use the PlayerInputHandler system and
	// rarely jump in practice.
	struct JumpInputHandler
	{
		static void Install();
		static void ProcessButton(RE::JumpHandler* a_this, RE::ButtonEvent* a_event, RE::PlayerControlsData* a_data);
		static inline REL::Relocation<decltype(ProcessButton)> _ProcessButton;
	};

	// Implements ScrambledBugs' PowerAttackStaminaRequirement patch
	// (PowerAttackStamina.cpp in the ScrambledBugs source):
	//
	// NOPs the engine's HasStamina branches — these are the conditional
	// jumps that the engine uses to decide "does this actor have enough
	// stamina for a power attack?"
	//   - Player: REL::ID(39003) + 0xE1  (ja +0x19)
	//   - NPC:    REL::ID(49170) + 0x272 (jnz +0x10)
	//
	// Then hooks the GetAttackStamina calls that feed into those branches,
	// replacing them with our own HasStamina check:
	//   - Player: REL::ID(39003) + 0xBB
	//   - NPC:    REL::ID(49170) + 0x27A
	//
	// Return convention: 0.0F = has stamina (allow), >0.0F = no stamina (deny)
	// We are based off PowerAttackStamina::HasAttackStaminaActor functions
	struct AttackDenyHook
	{
		static void Install();
		static void NopHasStaminaBranches();
		static float HasStamina(RE::ActorValueOwner* a_this, RE::BGSAttackData* a_attack);
		static float PlayerHasStamina(RE::ActorValueOwner* a_this, RE::BGSAttackData* a_attack);
		static float NPCHasStamina(RE::ActorValueOwner* a_this, RE::BGSAttackData* a_attack);
		static inline REL::Relocation<decltype(HasStamina)> _original;
	};
}
