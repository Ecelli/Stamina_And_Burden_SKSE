#pragma once

namespace Hooks
{
	/*
	 * Jump stamina cost — two hook points:
	 *
	 *   1. write_branch<5> detour on the actor jump function entry.
	 *      Checks current stamina vs burden-calculated cost; denies the
	 *      jump (does not call original) when stamina is too low.
	 *      NOT IMPLEMENTED — AE offset unknown (SSE ref: REL::ID(41349) + 0x114)
	 *
	 *   2. write_call<5> on a GetScale call inside jump physics processing.
	 *      Applies the stamina cost as a side effect via DamageActorValue,
	 *      then returns the original scale value.
	 *      AE: REL::ID(37257) + 0x17f  (from exhausting-combat)
	 *      SSE: REL::ID(36271) + 0x190  (from StaminaNPC)
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
