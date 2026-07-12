#pragma once

#include <RE/B/BGSAttackData.h>

namespace Hooks
{
	// Jump denial — blocked, AE call site unknown
	// SSE ref: REL::ID(41349) + 0x114
	// AE candidate: REL::ID(42423) + 0x114 — crashes on 1.6.1170
	struct JumpDenyHook
	{
		static void Install();
		static void JumpDetour(RE::Actor* actor);
		static inline REL::Relocation<decltype(JumpDetour)> _Jump;
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
