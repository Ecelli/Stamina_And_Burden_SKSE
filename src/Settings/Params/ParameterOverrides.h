#pragma once

#include "Settings/Params/Parameter.h"

struct ParameterOverrides : REX::Singleton<ParameterOverrides>
{
	// ===== Combat Regen =====
	Parameter<float> CombatStaminaRegenRateMult{ 1.0f, 0.0f, 10.0f };
	Parameter<float> CombatHealthRegenRateMult{ 1.0f, 0.0f, 10.0f };
	Parameter<float> CombatMagickaRegenRateMult{ 1.0f, 0.0f, 10.0f };

	// ===== Regen Delays =====
	Parameter<float> DamagedStaminaRegenDelay{ 0.5f, 0.0f, 60.0f };
	Parameter<float> DamagedHealthRegenDelay{ 0.5f, 0.0f, 60.0f };
	Parameter<float> DamagedMagickaRegenDelay{ 0.5f, 0.0f, 60.0f };

	// ===== Sprint =====
	Parameter<float> SprintStaminaDrainMult{ 1.0f, 0.0f, 10.0f };

	// ===== Block Formula =====
	Parameter<float> fShieldBaseFactor{ 20.0f, 0.0f, 100.0f };
	Parameter<float> fShieldScalingFactor{ 0.25f, 0.0f, 2.0f };
	Parameter<float> fBlockWeaponBase{ 15.0f, 0.0f, 100.0f };
	Parameter<float> fBlockWeaponScaling{ 0.22f, 0.0f, 2.0f };
	Parameter<float> fBlockSkillMult{ 6.0f, 0.0f, 10.0f };
	Parameter<float> fBlockPowerAttackMult{ 0.66f, 0.0f, 1.0f };

	// ===== Engine Block =====
	Parameter<float> fStaminaBlockDmgMult{ 0.0f, 0.0f, 1.0f };
	Parameter<float> fStaminaBlockStaggerMult{ 0.0f, 0.0f, 20.0f };
	Parameter<float> fStaminaBlockBase{ 0.0f, 0.0f, 10.0f };

	template <typename F>
	static void ForEach(F&& a_fn)
	{
		auto* s = GetSingleton();
		a_fn("Combat Regen");
		a_fn("fCombatStaminaRegenRateMult"sv, s->CombatStaminaRegenRateMult);
		a_fn("fCombatHealthRegenRateMult"sv, s->CombatHealthRegenRateMult);
		a_fn("fCombatMagickaRegenRateMult"sv, s->CombatMagickaRegenRateMult);
		a_fn("Regen Delays");
		a_fn("fDamagedStaminaRegenDelay"sv, s->DamagedStaminaRegenDelay);
		a_fn("fDamagedHealthRegenDelay"sv, s->DamagedHealthRegenDelay);
		a_fn("fDamagedMagickaRegenDelay"sv, s->DamagedMagickaRegenDelay);
		a_fn("Sprint");
		a_fn("fSprintStaminaDrainMult"sv, s->SprintStaminaDrainMult);
		a_fn("Block Formula");
		a_fn("fShieldBaseFactor"sv, s->fShieldBaseFactor);
		a_fn("fShieldScalingFactor"sv, s->fShieldScalingFactor);
		a_fn("fBlockWeaponBase"sv, s->fBlockWeaponBase);
		a_fn("fBlockWeaponScaling"sv, s->fBlockWeaponScaling);
		a_fn("fBlockSkillMult"sv, s->fBlockSkillMult);
		a_fn("fBlockPowerAttackMult"sv, s->fBlockPowerAttackMult);
		a_fn("Engine Block");
		a_fn("fStaminaBlockDmgMult"sv, s->fStaminaBlockDmgMult);
		a_fn("fStaminaBlockStaggerMult"sv, s->fStaminaBlockStaggerMult);
		a_fn("fStaminaBlockBase"sv, s->fStaminaBlockBase);
	}
};
