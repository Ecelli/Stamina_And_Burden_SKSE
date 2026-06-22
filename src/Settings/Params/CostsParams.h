#pragma once

#include "Settings/Params/Parameter.h"

namespace Costs
{
	struct CostsParams : REX::Singleton<CostsParams>
	{
		// Debug logging
		Parameter<bool> EnableDebugLogging{ true, false, true };

        Parameter<float> SprintDrainLowBurden{ 3.0f, 0.0f, 200.0f };
		Parameter<float> SprintDrainHighBurden{ 15.0f, 0.0f, 200.0f };
		Parameter<float> SprintDrainLowCarryBurdenPct{ 0.1f, 0.0f, 60.0f };
		Parameter<float> SprintDrainHighCarryBurdenPct{ 10.0f, 0.0f, 60.0f };
		Parameter<float> SprintDrainBurdenCurve_k{ 0.8f, 0.0f, 1.0f };
		Parameter<float> SprintDrainCarryBurdenCurve_k{ 0.9f, 0.0f, 1.0f };

		Parameter<float> JumpCostLowBurden{ 3.0f, 0.0f, 200.0f };
		Parameter<float> JumpCostHighBurden{ 15.0f, 0.0f, 200.0f };
		Parameter<float> JumpCostLowCarryPct{ 0.5f, 0.0f, 60.0f };
		Parameter<float> JumpCostHighCarryPct{ 8.0f, 0.0f, 60.0f };
		Parameter<float> JumpCostBurdenCurve_k{ 0.8f, 0.0f, 1.0f };
		Parameter<float> JumpCostCarryCurve_k{ 0.9f, 0.0f, 1.0f };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto& s = GetSingleton();
			a_fn("bEnableDebugLogging"sv, s.EnableDebugLogging);
			a_fn("fSprintDrainLowBurden"sv, s.SprintDrainLowBurden);
			a_fn("fSprintDrainHighBurden"sv, s.SprintDrainHighBurden);
			a_fn("fSprintDrainLowCarryBurdenPct"sv, s.SprintDrainLowCarryBurdenPct);
			a_fn("fSprintDrainHighCarryBurdenPct"sv, s.SprintDrainHighCarryBurdenPct);
			a_fn("fSprintDrainBurdenCurve_k"sv, s.SprintDrainBurdenCurve_k);
			a_fn("fSprintDrainCarryBurdenCurve_k"sv, s.SprintDrainCarryBurdenCurve_k);
			a_fn("fJumpCostLowBurden"sv, s.JumpCostLowBurden);
			a_fn("fJumpCostHighBurden"sv, s.JumpCostHighBurden);
			a_fn("fJumpCostLowCarryPct"sv, s.JumpCostLowCarryPct);
			a_fn("fJumpCostHighCarryPct"sv, s.JumpCostHighCarryPct);
			a_fn("fJumpCostBurdenCurve_k"sv, s.JumpCostBurdenCurve_k);
			a_fn("fJumpCostCarryCurve_k"sv, s.JumpCostCarryCurve_k);
		}
	};

}
