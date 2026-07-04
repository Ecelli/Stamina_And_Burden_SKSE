#pragma once

#include "Settings/Params/Parameter.h"

namespace Damage
{
	struct DamageParams : REX::Singleton<DamageParams>
	{
		Parameter<bool>  EnableDebugLogging{ true, false, true };
		Parameter<bool>  bDamageScalingPlayer{ true, false, true };
		Parameter<bool>  bDamageScalingNPC{ true, false, true };
		Parameter<float> fDamageScaleLow{ 0.50f, 0.0f, 1.0f };
		Parameter<float> fDamageScaleHigh{ 1.20f, 0.8f, 3.0f };
		Parameter<float> fDamageScaleCurve_k{ 0.80f, 0.0f, 1.0f };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto& s = GetSingleton();
			a_fn("bEnableDebugLogging"sv, s.EnableDebugLogging);
			a_fn("bDamageScalingPlayer"sv, s.bDamageScalingPlayer);
			a_fn("bDamageScalingNPC"sv, s.bDamageScalingNPC);
			a_fn("fDamageScaleLow"sv, s.fDamageScaleLow);
			a_fn("fDamageScaleHigh"sv, s.fDamageScaleHigh);
			a_fn("fDamageScaleCurve_k"sv, s.fDamageScaleCurve_k);
		}
	};
}
