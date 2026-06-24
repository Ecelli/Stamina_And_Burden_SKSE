#pragma once

#include <RE/B/BGSAttackData.h>

namespace Hooks
{
	struct AttackCostHook
	{
		static void Install();
		static float Call(RE::ActorValueOwner* a_this, RE::BGSAttackData* a_attack);
		static inline REL::Relocation<decltype(Call)> _func;
	};

	// TODO: install when action denial is ready
	// REL::ID(49170) + 0x28d
	// Blocks attacks when stamina is insufficient
	struct AttackChanceHook
	{
		static float Call(RE::Actor* a_attacker, RE::Actor* a_victim, RE::BGSAttackData* a_attack);
		static inline REL::Relocation<decltype(Call)> _func;
	};
}
