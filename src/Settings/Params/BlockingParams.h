#pragma once

#include "Settings/Params/Parameter.h"

namespace Blocking
{
	struct BlockingParams : REX::Singleton<BlockingParams>
	{
		// ===== Debug =====
		Parameter<bool> EnableDebugLogging{ false, false, true };

		// ===== Block Cost =====
		Parameter<bool>  bBlockCostPlayer{ true, false, true };
		Parameter<bool>  bBlockCostNPC{ true, false, true };
		Parameter<float> fBlockCost_LowBlockBurden{ 2.0f, 0.0f, 50.0f };
		Parameter<float> fBlockCost_HighBlockBurden{ 30.0f, 0.0f, 100.0f };
		Parameter<float> fBlockCostCurve_k{ 0.80f, 0.0f, 1.0f };
		Parameter<float> fBlockCostPct_LowBlended{ 2.0f, 0.0f, 50.0f };
		Parameter<float> fBlockCostPct_HighBlended{ 8.0f, 0.0f, 100.0f };
		Parameter<float> fBlockCostPctCurve_k{ 0.50f, 0.0f, 1.0f };

		// ===== Redirect =====
		Parameter<bool>  bBlockRedirectPlayer{ true, false, true };
		Parameter<bool>  bBlockRedirectNPC{ true, false, true };
		Parameter<float> fBlockRedirectMult_LowBurden{ 0.8f, 0.0f, 10.0f };
		Parameter<float> fBlockRedirectMult_HighBurden{ 5.0f, 0.0f, 20.0f };
		Parameter<float> fBlockRedirectMultCurve_k{ 0.70f, 0.0f, 1.0f };
		Parameter<float> fBlockRedirectMultPct_LowBurden{ 0.1f, 0.0f, 5.0f };
		Parameter<float> fBlockRedirectMultPct_HighBurden{ 1.0f, 0.0f, 10.0f };
		Parameter<float> fBlockRedirectMultPctCurve_k{ 0.50f, 0.0f, 1.0f };

		// ===== Stagger =====
		Parameter<float> fStaggerPowerAttackMult{ 1.5f, 0.0f, 5.0f };
		Parameter<float> fStaggerInertiaFactor_LowBurden{ 1.1f, 0.0f, 2.0f };
		Parameter<float> fStaggerInertiaFactor_HighBurden{ 0.4f, 0.0f, 1.0f };
		Parameter<float> fStaggerInertiaFactorCurve_k{ 0.50f, 0.0f, 1.0f };
		Parameter<float> fStaggerMagnitudeMin{ 0.0f, 0.0f, 5.0f };
		Parameter<float> fStaggerMagnitudeMax{ 2.0f, 0.0f, 10.0f };
		Parameter<float> fStaggerMagnitudeCurve_k{ 0.50f, 0.0f, 1.0f };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto* s = GetSingleton();
			a_fn("Block Debug");
			a_fn("bEnableDebugLogging"sv, s->EnableDebugLogging);
			a_fn("Block Cost");
			a_fn("bBlockCostPlayer"sv, s->bBlockCostPlayer);
			a_fn("bBlockCostNPC"sv, s->bBlockCostNPC);
			a_fn("fBlockCost_LowBlockBurden"sv, s->fBlockCost_LowBlockBurden);
			a_fn("fBlockCost_HighBlockBurden"sv, s->fBlockCost_HighBlockBurden);
			a_fn("fBlockCostCurve_k"sv, s->fBlockCostCurve_k);
			a_fn("fBlockCostPct_LowBlended"sv, s->fBlockCostPct_LowBlended);
			a_fn("fBlockCostPct_HighBlended"sv, s->fBlockCostPct_HighBlended);
			a_fn("fBlockCostPctCurve_k"sv, s->fBlockCostPctCurve_k);
			a_fn("Block Redirect");
			a_fn("bBlockRedirectPlayer"sv, s->bBlockRedirectPlayer);
			a_fn("bBlockRedirectNPC"sv, s->bBlockRedirectNPC);
			a_fn("fBlockRedirectMult_LowBurden"sv, s->fBlockRedirectMult_LowBurden);
			a_fn("fBlockRedirectMult_HighBurden"sv, s->fBlockRedirectMult_HighBurden);
			a_fn("fBlockRedirectMultCurve_k"sv, s->fBlockRedirectMultCurve_k);
			a_fn("fBlockRedirectMultPct_LowBurden"sv, s->fBlockRedirectMultPct_LowBurden);
			a_fn("fBlockRedirectMultPct_HighBurden"sv, s->fBlockRedirectMultPct_HighBurden);
			a_fn("fBlockRedirectMultPctCurve_k"sv, s->fBlockRedirectMultPctCurve_k);
			a_fn("Break Stagger");
			a_fn("fStaggerPowerAttackMult"sv, s->fStaggerPowerAttackMult);
			a_fn("fStaggerInertiaFactor_LowBurden"sv, s->fStaggerInertiaFactor_LowBurden);
			a_fn("fStaggerInertiaFactor_HighBurden"sv, s->fStaggerInertiaFactor_HighBurden);
			a_fn("fStaggerInertiaFactorCurve_k"sv, s->fStaggerInertiaFactorCurve_k);
			a_fn("fStaggerMagnitudeMin"sv, s->fStaggerMagnitudeMin);
			a_fn("fStaggerMagnitudeMax"sv, s->fStaggerMagnitudeMax);
			a_fn("fStaggerMagnitudeCurve_k"sv, s->fStaggerMagnitudeCurve_k);
		}
	};
}
