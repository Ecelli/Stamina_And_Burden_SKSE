#include "SettingsOverride.h"
#include "ParameterOverrides.h"

namespace Regen
{
	void OverrideGameSettings()
	{
		auto* gmst = RE::GameSettingCollection::GetSingleton();
		if (!gmst) {
			logger::error("  >Failed to get GameSettingCollection");
			return;
		}

		auto* params = ParameterOverrides::GetSingleton();

		auto overrideFloat = [&](const char* a_name, float a_value) {
			auto* setting = gmst->GetSetting(a_name);
			if (setting) {
				setting->data.f = a_value;
				logger::info("  >Overrode {} to {}", a_name, a_value);
			} else {
				logger::warn("  >{} not found", a_name);
			}
		};

		overrideFloat("fCombatStaminaRegenRateMult", params->CombatStaminaRegenRateMult.Get());
		overrideFloat("fCombatHealthRegenRateMult", params->CombatHealthRegenRateMult.Get());
		overrideFloat("fCombatMagickaRegenRateMult", params->CombatMagickaRegenRateMult.Get());
		overrideFloat("fDamagedStaminaRegenDelay", params->DamagedStaminaRegenDelay.Get());
		overrideFloat("fDamagedHealthRegenDelay", params->DamagedHealthRegenDelay.Get());
		overrideFloat("fDamagedMagickaRegenDelay", params->DamagedMagickaRegenDelay.Get());
		overrideFloat("fSprintStaminaDrainMult", params->SprintStaminaDrainMult.Get());

		// Block formula GMSTs
		overrideFloat("fShieldBaseFactor", params->fShieldBaseFactor.Get());
		overrideFloat("fShieldScalingFactor", params->fShieldScalingFactor.Get());
		overrideFloat("fBlockWeaponBase", params->fBlockWeaponBase.Get());
		overrideFloat("fBlockWeaponScaling", params->fBlockWeaponScaling.Get());
		overrideFloat("fBlockSkillMult", params->fBlockSkillMult.Get());
		overrideFloat("fBlockPowerAttackMult", params->fBlockPowerAttackMult.Get());

		// Engine block stamina drain GMSTs
		overrideFloat("fStaminaBlockDmgMult", params->fStaminaBlockDmgMult.Get());
		overrideFloat("fStaminaBlockStaggerMult", params->fStaminaBlockStaggerMult.Get());
		overrideFloat("fStaminaBlockBase", params->fStaminaBlockBase.Get());
	}
}
