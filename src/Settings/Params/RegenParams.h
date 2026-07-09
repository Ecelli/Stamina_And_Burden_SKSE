#pragma once

#include "Settings/Params/Parameter.h"

namespace Regen
{
	struct RegenParams : REX::Singleton<RegenParams>
	{
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

		// Debug logging
		Parameter<bool> EnableDebugLogging{ true, false, true };

		// Per-actor-type toggles
		Parameter<bool> bRegenPlayer{ true, false, true };
		Parameter<bool> bRegenNPC{ true, false, true };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto& s = GetSingleton();
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
			a_fn("bEnableDebugLogging"sv, s.EnableDebugLogging);
			a_fn("bRegenPlayer"sv, s.bRegenPlayer);
			a_fn("bRegenNPC"sv, s.bRegenNPC);
		}
	};

	struct RegenMovementParams : REX::Singleton<RegenMovementParams>
	{
		// Movement state curves — each has max (low burden / best regen) and min (high burden / worst regen)
		// Static
		Parameter<float> RegenStatic_max{ 2.0f, 0.0f, 5.0f };
		Parameter<float> RegenStatic_min{ 0.0f, -5.0f, 5.0f };
		// Walking
		Parameter<float> RegenWalking_max{ 1.2f, 0.0f, 5.0f };
		Parameter<float> RegenWalking_min{ -0.2f, -2.0f, 5.0f };
		// Sneaking
		Parameter<float> RegenSneaking_max{ 1.4f, 0.0f, 5.0f };
		Parameter<float> RegenSneaking_min{ -0.3f, -2.0f, 5.0f };
		// Running
		Parameter<float> RegenRunning_max{ 0.6f, 0.0f, 5.0f };
		Parameter<float> RegenRunning_min{ -0.8f, -2.0f, 5.0f };
		// Swimming
		Parameter<float> RegenSwimming_max{ 0.2f, 0.0f, 5.0f };
		Parameter<float> RegenSwimming_min{ -1.5f, -2.0f, 5.0f };
		// Shared curve shape for all states
		Parameter<float> MovementRegenCurve_k{ 0.8f, 0.0f, 1.0f };

		// Bow-draw hold penalty (flat stamina/sec, weapon burden-scaled)
		Parameter<float> BowDrawLowBurden{ 1.0f, 0.0f, 50.0f };
		Parameter<float> BowDrawHighBurden{ 10.0f, 0.0f, 50.0f };
		Parameter<float> BowDrawCurve_k{ 0.8f, 0.0f, 1.0f };

		// Block-hold penalty (flat stamina/sec, burden-scaled)
		Parameter<float> BlockHoldLowBurden{ 2.0f, 0.0f, 50.0f };
		Parameter<float> BlockHoldHighBurden{ 30.0f, 0.0f, 50.0f };
		Parameter<float> BlockHoldCurve_k{ 0.8f, 0.0f, 1.0f };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto& s = GetSingleton();
			a_fn("fRegenStatic_max"sv, s.RegenStatic_max);
			a_fn("fRegenStatic_min"sv, s.RegenStatic_min);
			a_fn("fRegenWalking_max"sv, s.RegenWalking_max);
			a_fn("fRegenWalking_min"sv, s.RegenWalking_min);
			a_fn("fRegenSneaking_max"sv, s.RegenSneaking_max);
			a_fn("fRegenSneaking_min"sv, s.RegenSneaking_min);
			a_fn("fRegenRunning_max"sv, s.RegenRunning_max);
			a_fn("fRegenRunning_min"sv, s.RegenRunning_min);
			a_fn("fRegenSwimming_max"sv, s.RegenSwimming_max);
			a_fn("fRegenSwimming_min"sv, s.RegenSwimming_min);
			a_fn("fMovementRegenCurve_k"sv, s.MovementRegenCurve_k);
			a_fn("fBowDrawLowBurden"sv, s.BowDrawLowBurden);
			a_fn("fBowDrawHighBurden"sv, s.BowDrawHighBurden);
			a_fn("fBowDrawCurve_k"sv, s.BowDrawCurve_k);
			a_fn("fBlockHoldLowBurden"sv, s.BlockHoldLowBurden);
			a_fn("fBlockHoldHighBurden"sv, s.BlockHoldHighBurden);
			a_fn("fBlockHoldCurve_k"sv, s.BlockHoldCurve_k);
		}
	};

	struct NegativeRegen : REX::Singleton<NegativeRegen>
	{
		// Burn rate scaler: rescales the drain portion when our regen multiplier is
		// negative. Maps kStaminaRateMult from [LowBound, HighBound] onto [0, 1]
		// and interpolates between LowBonus (weak regen → amplified drain) and
		// HighBonus (strong regen → reduced drain).
		Parameter<float> BurnRate_LowBonus{ 2.0f, 0.0f, 10.0f };
		Parameter<float> BurnRate_HighBonus{ 0.2f, 0.0f, 10.0f };
		Parameter<float> BurnRate_Curve_k{ 0.5f, 0.0f, 1.0f };
		Parameter<float> BurnRate_LowBound{ -200.0f, -1000.0f, 0.0f };
		Parameter<float> BurnRate_HighBound{ 500.0f, 100.0f, 1000.0f };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto& s = GetSingleton();
			a_fn("fBurnRate_LowBonus"sv, s.BurnRate_LowBonus);
			a_fn("fBurnRate_HighBonus"sv, s.BurnRate_HighBonus);
			a_fn("fBurnRate_Curve_k"sv, s.BurnRate_Curve_k);
			a_fn("fBurnRate_LowBound"sv, s.BurnRate_LowBound);
			a_fn("fBurnRate_HighBound"sv, s.BurnRate_HighBound);
		}
	};

	struct WeatherParams : REX::Singleton<WeatherParams>
	{
		Parameter<float> WeatherRainPenalty{ 0.5f, 0.0f, 10.0f };
		Parameter<float> WeatherSnowPenalty{ 1.5f, 0.0f, 10.0f };
		Parameter<bool>  WeatherEnabled{ true, false, true };

		template <typename F>
		static void ForEach(F&& a_fn)
		{
			auto& s = GetSingleton();
			a_fn("fWeatherRainPenalty"sv, s.WeatherRainPenalty);
			a_fn("fWeatherSnowPenalty"sv, s.WeatherSnowPenalty);
			a_fn("bWeatherEnabled"sv, s.WeatherEnabled);
		}
	};
}
