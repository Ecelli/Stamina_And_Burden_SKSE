#pragma once

#include "Settings/Params/Parameter.h"

struct MovementSpeedParams : REX::Singleton<MovementSpeedParams>
{
	// ===== Debug =====
	Parameter<bool> EnableDebugLogging{ false, false, true };

	// ===== Burden Speed =====
	Parameter<bool>  bBurdenSpeedPlayer{ true, false, true };
	Parameter<bool>  bBurdenSpeedNPC{ true, false, true };
	Parameter<float> SpeedMultLowBurden{ 1.2f, 0.1f, 2.0f };
	Parameter<float> SpeedMultHighBurden{ 0.7f, 0.1f, 1.0f };
	Parameter<float> BurdenSpeedCurve_k{ 0.5f, 0.0f, 1.0f };

	// ===== Swim Speed =====
	Parameter<bool>  bSwimSpeedPlayer{ true, false, true };
	Parameter<bool>  bSwimSpeedNPC{ true, false, true };
	Parameter<float> SpeedMultNotSubmerged{ 1.0f, 0.1f, 1.5f };
	Parameter<float> SpeedMultSubmerged{ 0.6f, 0.1f, 1.0f };
	Parameter<float> SpeedSubmergedCurve_k{ 0.2f, 0.0f, 1.0f };

	// ===== Exhaustion =====
	Parameter<float> ExhaustionSpeedMult{ 0.7f, 0.1f, 1.0f };

	template <typename F>
	static void ForEach(F&& a_fn)
	{
		auto* s = GetSingleton();
		a_fn("Speed Debug");
		a_fn("bEnableDebugMovementLogging"sv, s->EnableDebugLogging);
		a_fn("Burden Speed");
		a_fn("bBurdenSpeedPlayer"sv, s->bBurdenSpeedPlayer);
		a_fn("bBurdenSpeedNPC"sv, s->bBurdenSpeedNPC);
		a_fn("fSpeedMultLowBurden"sv, s->SpeedMultLowBurden);
		a_fn("fSpeedMultHighBurden"sv, s->SpeedMultHighBurden);
		a_fn("fBurdenSpeedCurve_k"sv, s->BurdenSpeedCurve_k);
		a_fn("Swim Speed");
		a_fn("bSwimSpeedPlayer"sv, s->bSwimSpeedPlayer);
		a_fn("bSwimSpeedNPC"sv, s->bSwimSpeedNPC);
		a_fn("fSpeedMultNotSubmerged"sv, s->SpeedMultNotSubmerged);
		a_fn("fSpeedMultSubmerged"sv, s->SpeedMultSubmerged);
		a_fn("fSpeedSubmergedCurve_k"sv, s->SpeedSubmergedCurve_k);
		a_fn("Exhaustion");
		a_fn("fExhaustionSpeedMult"sv, s->ExhaustionSpeedMult);
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
	Parameter<float> fJumpHeightLowBurden{ 1.2f, 0.1f, 2.0f };
	Parameter<float> fJumpHeightHighBurden{ 0.6f, 0.1f, 1.0f };
	Parameter<float> fJumpHeightCurve_k{ 0.7f, 0.0f, 1.0f };
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
