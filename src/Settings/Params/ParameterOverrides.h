#pragma once

#include "Settings/Params/Parameter.h"

namespace Regen
{
	struct ParameterOverrides : REX::Singleton<ParameterOverrides>
	{
		// Combat regen multipliers
		Parameter<float> CombatStaminaRegenRateMult{ 1.0f, 0.0f, 10.0f };
		Parameter<float> CombatHealthRegenRateMult{ 1.0f, 0.0f, 10.0f };
		Parameter<float> CombatMagickaRegenRateMult{ 1.0f, 0.0f, 10.0f };

		// Damage regen delays
		Parameter<float> DamagedStaminaRegenDelay{ 0.5f, 0.0f, 60.0f };
		Parameter<float> DamagedHealthRegenDelay{ 0.5f, 0.0f, 60.0f };
		Parameter<float> DamagedMagickaRegenDelay{ 0.5f, 0.0f, 60.0f };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto& s = GetSingleton();
			a_fn("fCombatStaminaRegenRateMult"sv, s.CombatStaminaRegenRateMult);
			a_fn("fCombatHealthRegenRateMult"sv, s.CombatHealthRegenRateMult);
			a_fn("fCombatMagickaRegenRateMult"sv, s.CombatMagickaRegenRateMult);
			a_fn("fDamagedStaminaRegenDelay"sv, s.DamagedStaminaRegenDelay);
			a_fn("fDamagedHealthRegenDelay"sv, s.DamagedHealthRegenDelay);
			a_fn("fDamagedMagickaRegenDelay"sv, s.DamagedMagickaRegenDelay);
		}
	};
}
