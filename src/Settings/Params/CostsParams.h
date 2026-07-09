#pragma once

#include "Settings/Params/Parameter.h"

namespace Costs
{
	struct CostsParams : REX::Singleton<CostsParams>
	{
		// Debug logging
		Parameter<bool> EnableDebugLogging{ true, false, true };

		// Per-actor-type toggles
		Parameter<bool> bAttackCostPlayer{ true, false, true };
		Parameter<bool> bAttackCostNPC{ true, false, true };
		Parameter<bool> bBowCostPlayer{ true, false, true };
		Parameter<bool> bBowCostNPC{ true, false, true };
		Parameter<bool> bBowDenyPlayer{ true, false, true };
		Parameter<bool> bBowDenyNPC{ true, false, true };
		Parameter<bool> bSprintCostPlayer{ true, false, true };
		Parameter<bool> bSprintCostNPC{ true, false, true };
		Parameter<bool> bJumpCostPlayer{ true, false, true };
		Parameter<bool> bJumpCostNPC{ true, false, true };

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

		Parameter<float> BowFireLowBurden{ 10.0f, 0.0f, 200.0f };
		Parameter<float> BowFireHighBurden{ 60.0f, 0.0f, 200.0f };
		Parameter<float> BowFireBurdenCurve_k{ 0.7f, 0.0f, 1.0f };
		Parameter<float> BowFireLowCarryPct{ 2.0f, 0.0f, 60.0f };
		Parameter<float> BowFireHighCarryPct{ 10.0f, 0.0f, 60.0f };
		Parameter<float> BowFireCarryCurve_k{ 0.7f, 0.0f, 1.0f };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto& s = GetSingleton();
			a_fn("bEnableDebugLogging"sv, s.EnableDebugLogging);
			a_fn("bAttackCostPlayer"sv, s.bAttackCostPlayer);
			a_fn("bAttackCostNPC"sv, s.bAttackCostNPC);
			a_fn("bBowCostPlayer"sv, s.bBowCostPlayer);
			a_fn("bBowCostNPC"sv, s.bBowCostNPC);
			a_fn("bBowDenyPlayer"sv, s.bBowDenyPlayer);
			a_fn("bBowDenyNPC"sv, s.bBowDenyNPC);
			a_fn("bSprintCostPlayer"sv, s.bSprintCostPlayer);
			a_fn("bSprintCostNPC"sv, s.bSprintCostNPC);
			a_fn("bJumpCostPlayer"sv, s.bJumpCostPlayer);
			a_fn("bJumpCostNPC"sv, s.bJumpCostNPC);
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
			a_fn("fBowFireLowBurden"sv, s.BowFireLowBurden);
			a_fn("fBowFireHighBurden"sv, s.BowFireHighBurden);
			a_fn("fBowFireBurdenCurve_k"sv, s.BowFireBurdenCurve_k);
			a_fn("fBowFireLowCarryPct"sv, s.BowFireLowCarryPct);
			a_fn("fBowFireHighCarryPct"sv, s.BowFireHighCarryPct);
			a_fn("fBowFireCarryCurve_k"sv, s.BowFireCarryCurve_k);
		}
	};

	struct AttackCostParams : REX::Singleton<AttackCostParams>
	{
		// ---- Shared carry burden ----
		Parameter<float> AttackLowCarryPct{ 1.0f, 0.0f, 60.0f };
		Parameter<float> AttackHighCarryPct{ 10.0f, 0.0f, 60.0f };
		Parameter<float> AttackCarryCurve_k{ 0.9f, 0.0f, 1.0f };

		// ---- 1H attack ----
		Parameter<float> Attack1hLowBurden{ 6.0f, 0.0f, 200.0f };
		Parameter<float> Attack1hHighBurden{ 50.0f, 0.0f, 200.0f };
		Parameter<float> Attack1hBurdenCurve_k{ 0.8f, 0.0f, 1.0f };
		Parameter<float> Attack1hPowerMult{ 2.5f, 1.0f, 10.0f };

		// ---- 2H attack ----
		Parameter<float> Attack2hLowBurden{ 10.0f, 0.0f, 200.0f };
		Parameter<float> Attack2hHighBurden{ 90.0f, 0.0f, 200.0f };
		Parameter<float> Attack2hBurdenCurve_k{ 0.8f, 0.0f, 1.0f };
		Parameter<float> Attack2hPowerMult{ 3.5f, 1.0f, 10.0f };

		// ---- Unarmed ----
		Parameter<float> UnarmedBaseFlat{ 3.0f, 0.0f, 50.0f };
		Parameter<float> UnarmedPowerMult{ 2.0f, 1.0f, 5.0f };

		// ---- Shield bash ----
		Parameter<float> BashShieldLowBurden{ 5.0f, 0.0f, 200.0f };
		Parameter<float> BashShieldHighBurden{ 30.0f, 0.0f, 200.0f };
		Parameter<float> BashShieldBurdenCurve_k{ 0.8f, 0.0f, 1.0f };
		Parameter<float> BashShieldPowerMult{ 2.0f, 1.0f, 10.0f };

		// ---- Bow bash ----
		Parameter<float> BashBowLowBurden{ 3.0f, 0.0f, 200.0f };
		Parameter<float> BashBowHighBurden{ 20.0f, 0.0f, 200.0f };
		Parameter<float> BashBowBurdenCurve_k{ 0.8f, 0.0f, 1.0f };
		Parameter<float> BashBowPowerMult{ 2.0f, 1.0f, 10.0f };

		// ---- Weapon bash (1H/2H/left-weapon/unarmed) ----
		Parameter<float> BashWeaponLowBurden{ 5.0f, 0.0f, 200.0f };
		Parameter<float> BashWeaponHighBurden{ 40.0f, 0.0f, 200.0f };
		Parameter<float> BashWeaponBurdenCurve_k{ 0.8f, 0.0f, 1.0f };
		Parameter<float> BashWeaponPowerMult{ 2.0f, 1.0f, 10.0f };

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
			a_fn("fBashShieldLowBurden"sv, s.BashShieldLowBurden);
			a_fn("fBashShieldHighBurden"sv, s.BashShieldHighBurden);
			a_fn("fBashShieldBurdenCurve_k"sv, s.BashShieldBurdenCurve_k);
			a_fn("fBashShieldPowerMult"sv, s.BashShieldPowerMult);
			a_fn("fBashBowLowBurden"sv, s.BashBowLowBurden);
			a_fn("fBashBowHighBurden"sv, s.BashBowHighBurden);
			a_fn("fBashBowBurdenCurve_k"sv, s.BashBowBurdenCurve_k);
			a_fn("fBashBowPowerMult"sv, s.BashBowPowerMult);
			a_fn("fBashWeaponLowBurden"sv, s.BashWeaponLowBurden);
			a_fn("fBashWeaponHighBurden"sv, s.BashWeaponHighBurden);
			a_fn("fBashWeaponBurdenCurve_k"sv, s.BashWeaponBurdenCurve_k);
			a_fn("fBashWeaponPowerMult"sv, s.BashWeaponPowerMult);
		}
	};
}
