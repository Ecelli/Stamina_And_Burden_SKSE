#pragma once

#include <RE/H/HitData.h>

namespace Hooks
{
	// Intercepts ProcessHit for block mechanics (stamina drain, damage redirect, guard break).
	// Phase 1: Logging only — no modifications.
	// AE: REL::ID(38627) + 0x4A8 (confirmed by ShieldOfStamina + Valhalla Combat)
	struct BlockHook
	{
		static void Install();
		static void ProcessHit(RE::Actor* target, RE::HitData& hitData);
		static inline REL::Relocation<decltype(ProcessHit)> _func;
	};
}
