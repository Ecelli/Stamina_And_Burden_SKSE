#pragma once

#include "Settings/Params/Parameter.h"

struct ExhaustionParams : REX::Singleton<ExhaustionParams>
{
	// ===== Debug =====
	Parameter<bool> bEnableDebugLogging{ true, false, true };

	// ===== Toggles =====
	Parameter<bool> bExhaustionPlayer{ true, false, true };
	Parameter<bool> bExhaustionNPC{ false, false, true };

	// ===== Recovery =====
	Parameter<float> fExhaustionDuration{ 8.0f, 1.0f, 30.0f };
	Parameter<float> fExhaustionBurstStamina{ 0.25f, 0.0f, 1.0f };
	Parameter<float> fExhaustionThresholdStamina{ 0.30f, 0.0f, 1.0f };

	// ===== Penalties =====
	Parameter<float> fExhaustionPenaltyDamageMult{ 0.50f, 0.0f, 1.0f };
	Parameter<float> fExhaustionPenaltyStaminaMult{ 0.30f, 0.0f, 1.0f };
	Parameter<float> fExhaustionPenaltyHealthMult{ 0.0f, 0.0f, 1.0f };
	Parameter<float> fExhaustionPenaltyMagickaMult{ 0.0f, 0.0f, 1.0f };

	template <typename F>
	static void ForEach(F&& a_fn)
	{
		auto& s = GetSingleton();
		a_fn("Debug");
		a_fn("bEnableDebugLogging"sv, s.bEnableDebugLogging);
		a_fn("Toggles");
		a_fn("bExhaustionPlayer"sv, s.bExhaustionPlayer);
		a_fn("bExhaustionNPC"sv, s.bExhaustionNPC);
		a_fn("Recovery");
		a_fn("fExhaustionDuration"sv, s.fExhaustionDuration);
		a_fn("fExhaustionBurstStamina"sv, s.fExhaustionBurstStamina);
		a_fn("fExhaustionThresholdStamina"sv, s.fExhaustionThresholdStamina);
		a_fn("Penalties");
		a_fn("fExhaustionPenaltyDamageMult"sv, s.fExhaustionPenaltyDamageMult);
		a_fn("fExhaustionPenaltyStaminaMult"sv, s.fExhaustionPenaltyStaminaMult);
		a_fn("fExhaustionPenaltyHealthMult"sv, s.fExhaustionPenaltyHealthMult);
		a_fn("fExhaustionPenaltyMagickaMult"sv, s.fExhaustionPenaltyMagickaMult);
	}
};
