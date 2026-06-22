#pragma once

namespace Hooks
{
	/*
	 * Jump stamina cost — two hook points:
	 *
	 *   1. write_branch<5> detour on the actor jump function entry.
	 *      Checks current stamina vs burden-calculated cost; denies the
	 *      jump (does not call original) when stamina is too low.
	 *
	 *   2. write_call<5> on a GetScale call inside jump physics processing.
	 *      Applies the stamina cost as a side effect via RestoreActorValue,
	 *      then returns the original scale value.
	 *
	 * TODO: AE offsets TBD, have SSE references from Fenix
	 *   SSE refs: REL::ID(41349) + 0x114  (denial)
	 *             REL::ID(36271) + 0x190  (cost)
	 */
	class JumpHook
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
