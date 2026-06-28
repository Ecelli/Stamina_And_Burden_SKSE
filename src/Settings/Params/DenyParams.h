#pragma once

#include "Settings/Params/Parameter.h"

namespace Deny
{
	struct DenyParams : REX::Singleton<DenyParams>
	{
		Parameter<float> fMinStaminaCostMult{ 0.3f, 0.0f, 10.0f };
		Parameter<bool>  bPlayerAlwaysCanDoAction{ false, false, true };
		Parameter<bool>  bNpcAlwaysCanDoAction{ false, false, true };
		Parameter<float> fNpcRegenExemptionRate{ 0.0f, 0.0f, 1.0f };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto& s = GetSingleton();
			a_fn("fMinStaminaCostMult"sv, s.fMinStaminaCostMult);
			a_fn("bPlayerAlwaysCanDoAction"sv, s.bPlayerAlwaysCanDoAction);
			a_fn("bNpcAlwaysCanDoAction"sv, s.bNpcAlwaysCanDoAction);
			a_fn("fNpcRegenExemptionRate"sv, s.fNpcRegenExemptionRate);
		}
	};
}
