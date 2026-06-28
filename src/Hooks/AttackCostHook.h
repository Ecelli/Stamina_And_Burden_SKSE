#pragma once

#include <RE/B/BGSAttackData.h>

namespace Hooks
{
	// Replaces the engine's attack stamina cost with our computed cost.
	// Fires for ALL actors (player and NPC).
	// AE: REL::ID(38603) + 0x171
	struct AttackCostHook
	{
		static void Install();
		static float Call(RE::ActorValueOwner* a_this, RE::BGSAttackData* a_attack);
		static inline REL::Relocation<decltype(Call)> _func;
	};
}
