#pragma once

namespace Hooks
{
	/*
	 * Action stamina cost hooks.
	 *
	 * Each action (jump, attack, bow draw) applies a burden-calculated
	 * stamina cost on use:
	 *
	 *   Jump:
	 *     write_call<5> on a GetScale call inside jump physics processing.
	 *     Applies cost, returns original scale.
	 *     AE: REL::ID(37257) + 0x17f  (from exhausting-combat)
	 *     SSE: REL::ID(36271) + 0x190  (from StaminaNPC)
	 *
	 *     Denial (TBD): write_branch<5> on jump function entry.
	 *     SSE ref: REL::ID(41349) + 0x114
	 *     AE candidate: REL::ID(42423) + 0x114 — offset not confirmed
	 */
	class ActionHook
	{
	public:
		static void Install();

	private:
		static void JumpDetour(RE::Actor* actor);
		static inline REL::Relocation<decltype(JumpDetour)> _Jump;

		static float ApplyJumpCost(RE::Actor* actor);
		static inline REL::Relocation<decltype(ApplyJumpCost)> _GetScale;
	};
}
