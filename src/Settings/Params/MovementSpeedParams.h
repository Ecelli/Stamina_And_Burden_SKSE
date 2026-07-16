#pragma once

#include "Settings/Params/Parameter.h"

struct MovementSpeedParams : REX::Singleton<MovementSpeedParams>
{
	// ===== Debug =====
	Parameter<bool> EnableDebugLogging{ true, false, true };

	// ===== Burden Speed =====
	Parameter<bool>  bBurdenSpeedPlayer{ true, false, true };
	Parameter<bool>  bBurdenSpeedNPC{ true, false, true };
	Parameter<float> speedMultLowBurden{ 1.1f, 0.1f, 2.0f };
	Parameter<float> speedMultHighBurden{ 0.7f, 0.1f, 1.0f };
	Parameter<float> burdenSpeedCurve_k{ 0.5f, 0.0f, 1.0f };

	// ===== Swim Speed =====
	Parameter<bool>  bSwimSpeedPlayer{ true, false, true };
	Parameter<bool>  bSwimSpeedNPC{ true, false, true };
	Parameter<float> speedMultAboveWater{ 1.0f, 0.1f, 1.5f };
	Parameter<float> speedMultSubmerged{ 0.6f, 0.1f, 1.0f };
	Parameter<float> submergedCurve_k{ 0.2f, 0.0f, 1.0f };

	// ===== Exhaustion =====
	Parameter<float> exhaustionSpeedMult{ 0.7f, 0.1f, 1.0f };

	template <typename F>
	static void ForEach(F&& a_fn)
	{
		auto* s = GetSingleton();
		a_fn("Debug");
		a_fn("bEnableDebugMovementLogging"sv, s->EnableDebugLogging);
		a_fn("Burden Speed");
		a_fn("bBurdenSpeedPlayer"sv, s->bBurdenSpeedPlayer);
		a_fn("bBurdenSpeedNPC"sv, s->bBurdenSpeedNPC);
		a_fn("fSpeedMultLowBurden"sv, s->speedMultLowBurden);
		a_fn("fSpeedMultHighBurden"sv, s->speedMultHighBurden);
		a_fn("fBurdenSpeedCurve_k"sv, s->burdenSpeedCurve_k);
		a_fn("Swim Speed");
		a_fn("bSwimSpeedPlayer"sv, s->bSwimSpeedPlayer);
		a_fn("bSwimSpeedNPC"sv, s->bSwimSpeedNPC);
		a_fn("fSpeedMultAboveWater"sv, s->speedMultAboveWater);
		a_fn("fSpeedMultSubmerged"sv, s->speedMultSubmerged);
		a_fn("fSubmergedCurve_k"sv, s->submergedCurve_k);
		a_fn("Exhaustion");
		a_fn("fExhaustionSpeedMult"sv, s->exhaustionSpeedMult);
	}
};

struct JumpParams : REX::Singleton<JumpParams>
{
	// ===== Toggles =====
	Parameter<bool> bJumpHeightPlayer{ true, false, true };
	Parameter<bool> bJumpHeightNPC{ true, false, true };
	Parameter<bool> bJumpDenyPlayer{ true, false, true };

	// ===== Debug =====
	Parameter<bool> EnableDebugJumpLogging{ true, false, true };

	// ===== Height =====
	Parameter<float> fJumpHeightLowBurden{ 1.0f, 0.1f, 2.0f };
	Parameter<float> fJumpHeightHighBurden{ 0.5f, 0.1f, 1.0f };
	Parameter<float> fJumpHeightCurve_k{ 0.5f, 0.0f, 1.0f };
	Parameter<float> fJumpHeightExhaustionMult{ 0.70f, 0.1f, 1.0f };

	template <typename F>
	static void ForEach(F&& a_fn)
	{
		auto* s = GetSingleton();
		a_fn("Toggles");
		a_fn("bJumpHeightPlayer"sv, s->bJumpHeightPlayer);
		a_fn("bJumpHeightNPC"sv, s->bJumpHeightNPC);
		a_fn("bJumpDenyPlayer"sv, s->bJumpDenyPlayer);
		a_fn("Debug");
		a_fn("bEnableDebugJumpLogging"sv, s->EnableDebugJumpLogging);
		a_fn("Height");
		a_fn("fJumpHeightLowBurden"sv, s->fJumpHeightLowBurden);
		a_fn("fJumpHeightHighBurden"sv, s->fJumpHeightHighBurden);
		a_fn("fJumpHeightCurve_k"sv, s->fJumpHeightCurve_k);
		a_fn("fJumpHeightExhaustionMult"sv, s->fJumpHeightExhaustionMult);
	}
};
