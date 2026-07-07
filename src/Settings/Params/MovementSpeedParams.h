#pragma once

#include "Settings/Params/Parameter.h"

struct MovementSpeedParams : REX::Singleton<MovementSpeedParams>
{
	// Enable toggles
	Parameter<bool>  bEnableBurdenSpeed{ true, false, true };
	Parameter<bool>  bEnableSwimSpeed{ true, false, true };
	Parameter<bool>  bEnableExhaustionSpeed{ true, false, true };

	// Burden speed scaling: Interpolate(low, high, burdenBlend, k)
	Parameter<float> speedMultLowBurden{ 1.1f, 0.1f, 2.0f };
	Parameter<float> speedMultHighBurden{ 0.7f, 0.1f, 1.0f };
	Parameter<float> speedCurve_k{ 0.5f, 0.0f, 1.0f };

	// Swim speed scaling: Interpolate(aboveWater, submerged, submergedLevel, 1.0)
	Parameter<float> speedMultAboveWater{ 1.0f, 0.1f, 1.5f };
	Parameter<float> speedMultSubmerged{ 0.6f, 0.1f, 1.0f };

	// Exhaustion speed penalty
	Parameter<float> exhaustionSpeedMult{ 0.7f, 0.1f, 1.0f };

	// Debug
	Parameter<bool> EnableDebugLogging{ true, false, true };

	template <typename F>
	static void ForEach(F&& a_fn)
	{
		auto& s = GetSingleton();
		a_fn("bEnableBurdenSpeed"sv, s.bEnableBurdenSpeed);
		a_fn("bEnableSwimSpeed"sv, s.bEnableSwimSpeed);
		a_fn("bEnableExhaustionSpeed"sv, s.bEnableExhaustionSpeed);
		a_fn("fSpeedMultLowBurden"sv, s.speedMultLowBurden);
		a_fn("fSpeedMultHighBurden"sv, s.speedMultHighBurden);
		a_fn("fSpeedCurve_k"sv, s.speedCurve_k);
		a_fn("fSpeedMultAboveWater"sv, s.speedMultAboveWater);
		a_fn("fSpeedMultSubmerged"sv, s.speedMultSubmerged);
		a_fn("fExhaustionSpeedMult"sv, s.exhaustionSpeedMult);
		a_fn("bEnableDebugMovementLogging"sv, s.EnableDebugLogging);
	}
};
