#pragma once

#include "Settings/Params/Parameter.h"

namespace Deny
{
	struct DenyParams : REX::Singleton<DenyParams>
	{
		Parameter<float> fMinStaminaCostMult{ 0.3f, 0.0f, 10.0f };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto& s = GetSingleton();
			a_fn("fMinStaminaCostMult"sv, s.fMinStaminaCostMult);
		}
	};
}
