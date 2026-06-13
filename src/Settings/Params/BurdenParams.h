#pragma once

#include "Settings/Params/Parameter.h"

struct BurdenParams : REX::Singleton<BurdenParams>
{
	Parameter<float> maxEquippedWeightRatio{ 0.3f, 0.0f, 1.0f };

	template <typename F>
	static void ForEach(F&& a_fn)
	{
		auto& s = GetSingleton();
		a_fn("fmaxEquippedWeightRatio"sv, s.maxEquippedWeightRatio);
	}
};
