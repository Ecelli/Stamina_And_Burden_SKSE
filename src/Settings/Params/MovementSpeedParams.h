#pragma once

#include "Settings/Params/Parameter.h"

struct MovementSpeedParams : REX::Singleton<MovementSpeedParams>
{
	// Burden speed per-actor toggles
	Parameter<bool>  bBurdenSpeedPlayer{ true, false, true };
	Parameter<bool>  bBurdenSpeedNPC{ true, false, true };

	// Burden speed scaling: Interpolate(low, high, burdenBlend, k)
	Parameter<float> speedMultLowBurden{ 1.1f, 0.1f, 2.0f };
	Parameter<float> speedMultHighBurden{ 0.7f, 0.1f, 1.0f };
	Parameter<float> burdenSpeedCurve_k{ 0.5f, 0.0f, 1.0f };

	// Swim speed per-actor toggles
	Parameter<bool>  bSwimSpeedPlayer{ true, false, true };
	Parameter<bool>  bSwimSpeedNPC{ true, false, true };

	// Swim speed scaling: Interpolate(aboveWater, submerged, submergedLevel, k)
	Parameter<float> speedMultAboveWater{ 1.0f, 0.1f, 1.5f };
	Parameter<float> speedMultSubmerged{ 0.6f, 0.1f, 1.0f };
	Parameter<float> submergedCurve_k{ 0.2f, 0.0f, 1.0f };

	// Exhaustion speed penalty (gated by bExhaustionPlayer/NPC)
	Parameter<float> exhaustionSpeedMult{ 0.7f, 0.1f, 1.0f };

	// Debug
	Parameter<bool> EnableDebugLogging{ true, false, true };

	// Per-actor-type master toggles
	Parameter<bool> bMovementSpeedPlayer{ true, false, true };
	Parameter<bool> bMovementSpeedNPC{ true, false, true };

	template <typename F>
	static void ForEach(F&& a_fn)
	{
		auto& s = GetSingleton();
		a_fn("bBurdenSpeedPlayer"sv, s.bBurdenSpeedPlayer);
		a_fn("bBurdenSpeedNPC"sv, s.bBurdenSpeedNPC);
		a_fn("fSpeedMultLowBurden"sv, s.speedMultLowBurden);
		a_fn("fSpeedMultHighBurden"sv, s.speedMultHighBurden);
		a_fn("fBurdenSpeedCurve_k"sv, s.burdenSpeedCurve_k);
		a_fn("bSwimSpeedPlayer"sv, s.bSwimSpeedPlayer);
		a_fn("bSwimSpeedNPC"sv, s.bSwimSpeedNPC);
		a_fn("fSpeedMultAboveWater"sv, s.speedMultAboveWater);
		a_fn("fSpeedMultSubmerged"sv, s.speedMultSubmerged);
		a_fn("fSubmergedCurve_k"sv, s.submergedCurve_k);
		a_fn("fExhaustionSpeedMult"sv, s.exhaustionSpeedMult);
		a_fn("bEnableDebugMovementLogging"sv, s.EnableDebugLogging);
		a_fn("bMovementSpeedPlayer"sv, s.bMovementSpeedPlayer);
		a_fn("bMovementSpeedNPC"sv, s.bMovementSpeedNPC);
	}
};

struct JumpParams : REX::Singleton<JumpParams>
{
	// Height scaling toggles
	Parameter<bool> bJumpHeightPlayer{ true, false, true };
	Parameter<bool> bJumpHeightNPC{ true, false, true };

	// Height curve: Interpolate(low, high, burdenBlend, k)
	Parameter<float> fJumpHeightLowBurden{ 1.0f, 0.1f, 2.0f };
	Parameter<float> fJumpHeightHighBurden{ 0.5f, 0.1f, 1.0f };
	Parameter<float> fJumpHeightCurve_k{ 0.5f, 0.0f, 1.0f };

	// Exhaustion penalty (multiplied on top of burden curve)
	Parameter<float> fJumpHeightExhaustionMult{ 0.70f, 0.1f, 1.0f };

	// Denial — player only (NPCs don't use PlayerInputHandler, rarely jump)
	Parameter<bool> bJumpDenyPlayer{ true, false, true };

	// Debug
	Parameter<bool> EnableDebugJumpLogging{ true, false, true };

	template <typename F>
	static void ForEach(F&& a_fn)
	{
		auto& s = GetSingleton();
		a_fn("bJumpHeightPlayer"sv, s.bJumpHeightPlayer);
		a_fn("bJumpHeightNPC"sv, s.bJumpHeightNPC);
		a_fn("fJumpHeightLowBurden"sv, s.fJumpHeightLowBurden);
		a_fn("fJumpHeightHighBurden"sv, s.fJumpHeightHighBurden);
		a_fn("fJumpHeightCurve_k"sv, s.fJumpHeightCurve_k);
		a_fn("fJumpHeightExhaustionMult"sv, s.fJumpHeightExhaustionMult);
		a_fn("bJumpDenyPlayer"sv, s.bJumpDenyPlayer);
		a_fn("bEnableDebugJumpLogging"sv, s.EnableDebugJumpLogging);
	}
};
