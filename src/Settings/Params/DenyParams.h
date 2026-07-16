#pragma once

#include "Settings/Params/Parameter.h"

namespace Deny
{
	struct DenyParams : REX::Singleton<DenyParams>
	{
		// ===== Debug =====
		Parameter<bool> EnableDebugLogging{ true, false, true };

		// ===== Toggles =====
		Parameter<bool> bEnableDenyPlayer{ true, false, true };
		Parameter<bool> bEnableDenyNPC{ true, false, true };

		// ===== Threshold =====
		Parameter<float> fMinStaminaCostMult{ 0.3f, 0.0f, 10.0f };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto* s = GetSingleton();
			a_fn("Debug");
			a_fn("bEnableDebugLogging"sv, s->EnableDebugLogging);
			a_fn("Toggles");
			a_fn("bEnableDenyPlayer"sv, s->bEnableDenyPlayer);
			a_fn("bEnableDenyNPC"sv, s->bEnableDenyNPC);
			a_fn("Threshold");
			a_fn("fMinStaminaCostMult"sv, s->fMinStaminaCostMult);
		}
	};
}
