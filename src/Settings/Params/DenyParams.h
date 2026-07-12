#pragma once

#include "Settings/Params/Parameter.h"

namespace Deny
{
	struct DenyParams : REX::Singleton<DenyParams>
	{
		Parameter<bool>  EnableDebugLogging{ true, false, true };
		Parameter<bool>  bEnableDenyPlayer{ true, false, true };
		Parameter<bool>  bEnableDenyNPC{ true, false, true };
		Parameter<float> fMinStaminaCostMult{ 0.3f, 0.0f, 10.0f };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto& s = GetSingleton();
			a_fn("bEnableDebugLogging"sv, s.EnableDebugLogging);
			a_fn("bEnableDenyPlayer"sv, s.bEnableDenyPlayer);
			a_fn("bEnableDenyNPC"sv, s.bEnableDenyNPC);
			a_fn("fMinStaminaCostMult"sv, s.fMinStaminaCostMult);
		}
	};
}
