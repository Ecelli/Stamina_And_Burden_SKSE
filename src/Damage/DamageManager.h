#pragma once

#include "Settings/Params/DamageParams.h"

namespace Damage
{
	float ComputeStaminaDamageMult(RE::Actor* actor);

	template <typename... Args>
	void DamageLog(std::string_view a_fmt, Args&&... a_args)
	{
		if (DamageParams::GetSingleton()->EnableDebugLogging.Get()) {
			logger::info(fmt::runtime(a_fmt), std::forward<Args>(a_args)...);
		}
	}
}
