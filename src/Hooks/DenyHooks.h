#pragma once

#include <RE/B/BGSAttackData.h>

namespace Hooks
{
	// NPC-only: verified at REL::ID(49170) + 0x28d fires for NPCs (formID 85452)
	// but NOT for the player (formID 14). Player denial needs a different approach
	// (e.g. vtable hook on AttackBlockHandler::ProcessButton).
	// Disabled until player-side implementation is ready.
	//
	// Findings (2026-06-24):
	//   - AttackCostHook at ID(38603)+0x171 fires for ALL actors (player + NPC)
	//   - AttackChanceHook at ID(49170)+0x28d fires for NPCs only
	//   - Player's attack denial requires a different entry point
	//   - StaminaNPC uses ID(38047)+0xBB (SE) for player denial — unverified on AE
	//   - Vtable hook on AttackBlockHandler::ProcessButton (index 04) is
	//     version-stable and doesn't need offsets — preferred approach
	struct NpcAttackDenyHook
	{
		static void Install();
		static float Call(RE::Actor* a_attacker, RE::Actor* a_victim, RE::BGSAttackData* a_attack);
		static inline REL::Relocation<decltype(Call)> _func;
	};

	// Jump denial — blocked, AE call site unknown
	// SSE ref: REL::ID(41349) + 0x114
	// AE candidate: REL::ID(42423) + 0x114 — crashes on 1.6.1170
	struct NpcJumpDenyHook
	{
		static void Install();
		static void JumpDetour(RE::Actor* actor);
		static inline REL::Relocation<decltype(JumpDetour)> _Jump;
	};

	// TODO: Player denial via AttackBlockHandler vtable hook (vtable index 04)
	// See: RE/A/AttackBlockHandler.h in CommonLibSSE-NG
	// struct PlayerAttackBlockHook { ... };
}
