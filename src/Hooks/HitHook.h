#pragma once

#include <RE/H/HitData.h>

namespace Hooks
{
	// Intercepts ProcessHit to log HitData before damage application.
	// Phase 1: Logging only — no damage modification.
	// AE: REL::ID(38627) + 0x4A8 (confirmed by ShieldOfStamina + Valhalla Combat)
	struct HitHook
	{
		static void Install();
		static void ProcessHit(RE::Actor* target, RE::HitData& hitData);
		static inline REL::Relocation<decltype(ProcessHit)> _func;
	};
}
