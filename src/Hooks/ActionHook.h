#pragma once

namespace Hooks
{
	/*
	 * Jump stamina cost hook.
	 *
	 * write_call<5> on a GetScale call inside jump physics processing.
	 * Applies cost, returns original scale.
	 * AE: REL::ID(37257) + 0x17f  (from exhausting-combat)
	 * SSE: REL::ID(36271) + 0x190  (from StaminaNPC)
	 */
	class ActionHook
	{
	public:
		static void Install();

	private:
		static float ApplyJumpCost(RE::Actor* actor);
		static inline REL::Relocation<decltype(ApplyJumpCost)> _GetScale;
	};
}
