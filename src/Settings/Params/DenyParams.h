#pragma once

#include "Settings/Params/Parameter.h"

namespace Deny
{
	struct DenyParams : REX::Singleton<DenyParams>
	{
		// ===== Debug =====
		Parameter<bool> EnableDebugLogging{ true, false, true };

		// ===== Threshold =====
		Parameter<float> fMinStaminaCostMult{ 0.3f, 0.0f, 10.0f };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto* s = GetSingleton();
			a_fn("Deny Debug");
			a_fn("bEnableDebugLogging"sv, s->EnableDebugLogging);
			a_fn("Stamina Threshold");
			a_fn("fMinStaminaCostMult"sv, s->fMinStaminaCostMult);
		}
	};
}
