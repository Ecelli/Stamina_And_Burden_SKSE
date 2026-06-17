#pragma once

#include "Settings/Params/Parameter.h"

namespace Regen
{
	struct RegenParams : REX::Singleton<RegenParams>
	{
		// Burden regen curves
		Parameter<float> StaminaRegenMult_LowBurden{ 2.0f, 0.0f, 10.0f };
		Parameter<float> StaminaRegenMult_HighBurden{ 0.0f, -1.0f, 5.0f };

		// Cross-AV: health → stamina, stamina → stamina, magicka → stamina
		Parameter<float> StaminaRegenMult_LowHealth{ 0.6f, 0.0f, 10.0f };
		Parameter<float> StaminaRegenMult_HighHealth{ 1.3f, 0.0f, 10.0f };
		Parameter<float> StaminaRegenMult_LowStamina{ 0.4f, 0.0f, 10.0f };
		Parameter<float> StaminaRegenMult_HighStamina{ 1.5f, 0.0f, 10.0f };
		Parameter<float> StaminaRegenMult_LowMagicka{ 0.8f, 0.0f, 10.0f };
		Parameter<float> StaminaRegenMult_HighMagicka{ 1.2f, 0.0f, 10.0f };
		Parameter<float> StaminaRegenCurve_kStamina{ 0.8f, 0.0f, 1.0f };
		Parameter<float> StaminaRegenCurve_kMagicka{ 0.2f, 0.0f, 1.0f };
		Parameter<float> StaminaRegenCurve_kHealth{ 0.6f, 0.0f, 1.0f };

		// Health regen (cross-AV: depends on stamina %)
		Parameter<float> HealthRegenMult_LowStamina{ 0.3f, 0.0f, 10.0f };
		Parameter<float> HealthRegenMult_HighStamina{ 1.2f, 0.0f, 10.0f };
		Parameter<float> HealthRegenCurve_k{ 0.9f, 0.0f, 1.0f };

		// Magicka regen (cross-AV: depends on stamina %)
		Parameter<float> MagickaRegenMult_LowStamina{ 0.5f, 0.0f, 10.0f };
		Parameter<float> MagickaRegenMult_HighStamina{ 1.0f, 0.0f, 10.0f };
		Parameter<float> MagickaRegenCurve_k{ 0.9f, 0.0f, 1.0f };


		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto& s = GetSingleton();
			a_fn("fStaminaRegenMult_LowBurden"sv, s.StaminaRegenMult_LowBurden);
			a_fn("fStaminaRegenMult_HighBurden"sv, s.StaminaRegenMult_HighBurden);
			a_fn("fStaminaRegenMult_LowHealth"sv, s.StaminaRegenMult_LowHealth);
			a_fn("fStaminaRegenMult_HighHealth"sv, s.StaminaRegenMult_HighHealth);
			a_fn("fStaminaRegenMult_LowStamina"sv, s.StaminaRegenMult_LowStamina);
			a_fn("fStaminaRegenMult_HighStamina"sv, s.StaminaRegenMult_HighStamina);
			a_fn("fStaminaRegenMult_LowMagicka"sv, s.StaminaRegenMult_LowMagicka);
			a_fn("fStaminaRegenMult_HighMagicka"sv, s.StaminaRegenMult_HighMagicka);
			a_fn("fStaminaRegenCurve_kStamina"sv, s.StaminaRegenCurve_kStamina);
			a_fn("fStaminaRegenCurve_kMagicka"sv, s.StaminaRegenCurve_kMagicka);
			a_fn("fStaminaRegenCurve_kHealth"sv, s.StaminaRegenCurve_kHealth);
			a_fn("fHealthRegenMult_LowStamina"sv, s.HealthRegenMult_LowStamina);
			a_fn("fHealthRegenMult_HighStamina"sv, s.HealthRegenMult_HighStamina);
			a_fn("fHealthRegenCurve_k"sv, s.HealthRegenCurve_k);
			a_fn("fMagickaRegenMult_LowStamina"sv, s.MagickaRegenMult_LowStamina);
			a_fn("fMagickaRegenMult_HighStamina"sv, s.MagickaRegenMult_HighStamina);
			a_fn("fMagickaRegenCurve_k"sv, s.MagickaRegenCurve_k);
		}
	};

	struct RegenPenalties : REX::Singleton<RegenPenalties>
	{
		// Movement state multipliers
		Parameter<float> RegenMult_static{ 1.3f, 0.0f, 10.0f };
		Parameter<float> RegenMult_walking{ 1.0f, 0.0f, 10.0f };
		Parameter<float> RegenMult_running{ 0.7f, 0.0f, 10.0f };
		Parameter<float> RegenMult_sprinting{ 0.0f, 0.0f, 10.0f };
		Parameter<float> RegenMult_swimming{ 0.0f, 0.0f, 10.0f };
		Parameter<float> RegenMult_blocking{ 0.5f, 0.0f, 10.0f };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto& s = GetSingleton();
			a_fn("fRegenMult_static"sv, s.RegenMult_static);
			a_fn("fRegenMult_walking"sv, s.RegenMult_walking);
			a_fn("fRegenMult_running"sv, s.RegenMult_running);
			a_fn("fRegenMult_sprinting"sv, s.RegenMult_sprinting);
			a_fn("fRegenMult_swimming"sv, s.RegenMult_swimming);
			a_fn("fRegenMult_blocking"sv, s.RegenMult_blocking);
		}
	};
}
