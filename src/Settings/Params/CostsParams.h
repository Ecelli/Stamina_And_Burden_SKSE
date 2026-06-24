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

	struct AttackCostParams : REX::Singleton<AttackCostParams>
	{
		// ---- Shared carry burden ----
		Parameter<float> AttackLowCarryPct{ 0.2f, 0.0f, 60.0f };
		Parameter<float> AttackHighCarryPct{ 3.0f, 0.0f, 60.0f };
		Parameter<float> AttackCarryCurve_k{ 0.9f, 0.0f, 1.0f };

		// ---- 1H attack ----
		Parameter<float> Attack1hLowBurden{ 1.0f, 0.0f, 200.0f };
		Parameter<float> Attack1hHighBurden{ 6.0f, 0.0f, 200.0f };
		Parameter<float> Attack1hBurdenCurve_k{ 0.8f, 0.0f, 1.0f };
		Parameter<float> Attack1hPowerMult{ 2.0f, 1.0f, 10.0f };

		// ---- 2H attack ----
		Parameter<float> Attack2hLowBurden{ 3.0f, 0.0f, 200.0f };
		Parameter<float> Attack2hHighBurden{ 12.0f, 0.0f, 200.0f };
		Parameter<float> Attack2hBurdenCurve_k{ 0.8f, 0.0f, 1.0f };
		Parameter<float> Attack2hPowerMult{ 2.5f, 1.0f, 10.0f };

		// ---- Unarmed ----
		Parameter<float> UnarmedBaseFlat{ 1.0f, 0.0f, 50.0f };
		Parameter<float> UnarmedPowerMult{ 2.0f, 1.0f, 5.0f };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto& s = GetSingleton();
			a_fn("fAttackLowCarryPct"sv, s.AttackLowCarryPct);
			a_fn("fAttackHighCarryPct"sv, s.AttackHighCarryPct);
			a_fn("fAttackCarryCurve_k"sv, s.AttackCarryCurve_k);
			a_fn("fAttack1hLowBurden"sv, s.Attack1hLowBurden);
			a_fn("fAttack1hHighBurden"sv, s.Attack1hHighBurden);
			a_fn("fAttack1hBurdenCurve_k"sv, s.Attack1hBurdenCurve_k);
			a_fn("fAttack1hPowerMult"sv, s.Attack1hPowerMult);
			a_fn("fAttack2hLowBurden"sv, s.Attack2hLowBurden);
			a_fn("fAttack2hHighBurden"sv, s.Attack2hHighBurden);
			a_fn("fAttack2hBurdenCurve_k"sv, s.Attack2hBurdenCurve_k);
			a_fn("fAttack2hPowerMult"sv, s.Attack2hPowerMult);
			a_fn("fUnarmedBaseFlat"sv, s.UnarmedBaseFlat);
			a_fn("fUnarmedPowerMult"sv, s.UnarmedPowerMult);
		}
	};
}
