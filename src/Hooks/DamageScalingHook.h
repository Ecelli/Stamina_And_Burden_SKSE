#pragma once

namespace Hooks
{
	// Intercepts ProcessHit to log hit data and (Phase 2) scale damage
	// based on attacker stamina%.
	// AE: REL::ID(38627) + 0x4A8
	struct DamageScalingHook
	{
		static void Install();
		static void ProcessHit(RE::Actor* target, RE::HitData& hitData);
		static inline REL::Relocation<decltype(ProcessHit)> _func;
	};
}
