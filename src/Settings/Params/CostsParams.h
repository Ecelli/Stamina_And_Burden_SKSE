#pragma once

#include "Settings/Params/Parameter.h"

namespace Costs
{
	struct CostsParams : REX::Singleton<CostsParams>
	{
        Parameter<float> SprintDrainLowBurden{ 3.0f, 0.0f, 200.0f };
		Parameter<float> SprintDrainHighBurden{ 15.0f, 0.0f, 200.0f };
		Parameter<float> SprintDrainLowCarryBurdenPct{ 0.1f, 0.0f, 60.0f };
		Parameter<float> SprintDrainHighCarryBurdenPct{ 10.0f, 0.0f, 60.0f };
		Parameter<float> SprintDrainBurdenCurve_k{ 0.8f, 0.0f, 1.0f };
		Parameter<float> SprintDrainCarryBurdenCurve_k{ 0.9f, 0.0f, 1.0f };
	

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto& s = GetSingleton();
			a_fn("fSprintDrainLowBurden"sv, s.SprintDrainLowBurden);
			a_fn("fSprintDrainHighBurden"sv, s.SprintDrainHighBurden);
			a_fn("fSprintDrainLowCarryBurdenPct"sv, s.SprintDrainLowCarryBurdenPct);
			a_fn("fSprintDrainHighCarryBurdenPct"sv, s.SprintDrainHighCarryBurdenPct);
			a_fn("fSprintDrainBurdenCurve_k"sv, s.SprintDrainBurdenCurve_k);
			a_fn("fSprintDrainCarryBurdenCurve_k"sv, s.SprintDrainCarryBurdenCurve_k);
		}
	};

}
