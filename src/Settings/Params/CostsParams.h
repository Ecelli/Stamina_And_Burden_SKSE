#pragma once

#include "Settings/Params/Parameter.h"

namespace Costs
{
	struct CostsParams : REX::Singleton<CostsParams>
	{
		Parameter<float> SprintDrainBase{ 3.0f, 0.0f, 200.0f };
		Parameter<float> SprintDrainBurdenPenalty{ 12.0f, 0.0f, 200.0f };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto& s = GetSingleton();
			a_fn("fSprintDrainBase"sv, s.SprintDrainBase);
			a_fn("fSprintDrainBurdenPenalty"sv, s.SprintDrainBurdenPenalty);
		}
	};

}
