#include "Regen/RegenManager.h"
#include "Burden/BurdenTracker.h"
#include "Common/Utils.h"

namespace Regen
{
    // Returns false for any action that does prevents stamina regeneration
    bool GetCanRegenStamina(RE::Actor* actor) {
        // For now only attacking can do this
        return actor->GetAttackState() == RE::ATTACK_STATE_ENUM::kNone;
    }

    float GetBurdenStaminaMult(RE::Actor*, const Burden::ActorBurdenData& burdenData)
    {
		auto* params = RegenParams::GetSingleton();
		float burdenRatio = burdenData.burden;
		float carryBurdenRatio = burdenData.carryBurden;
        if (burdenRatio >= 1 || carryBurdenRatio >= 1) {
            return params->StaminaRegenMult_HighBurden.Get();
        }
		float burdenFactor = Math::Interpolate(
			params->StaminaRegenMult_LowBurden.Get(),
			params->StaminaRegenMult_HighBurden.Get(),
			burdenRatio,
			params->StaminaRegenCurve_kStamina.Get());

		float carryBurdenFactor = Math::Interpolate(
			params->StaminaRegenMult_LowBurden.Get(),
			params->StaminaRegenMult_HighBurden.Get(),
			carryBurdenRatio,
			params->StaminaRegenCurve_kStamina.Get());


        return burdenFactor * carryBurdenFactor;
    }

    float GetHMSStaminaMult(RE::Actor* actor, const Burden::ActorBurdenData&)
    {

		float healthPct = 1.0f;
		float staminaPct = 1.0f;
		float magickaPct = 1.0f;

        auto* params = RegenParams::GetSingleton();

		// 1. Health 
		float maxHealth = actor->GetPermanentActorValue(RE::ActorValue::kHealth);
		if (maxHealth > 0.0f) {
			healthPct = Math::Clamp01(actor->GetActorValue(RE::ActorValue::kHealth) / maxHealth);
		}
		float healthFactor = Math::Interpolate(
			params->StaminaRegenMult_LowHealth.Get(),
			params->StaminaRegenMult_HighHealth.Get(),
			healthPct,
			params->StaminaRegenCurve_kHealth.Get());

		// 2. Stamina 
        float maxStamina = actor->GetPermanentActorValue(RE::ActorValue::kStamina);
		if (maxStamina > 0.0f) {
			staminaPct = Math::Clamp01(actor->GetActorValue(RE::ActorValue::kStamina) / maxStamina);
		}
		float staminaFactor = Math::Interpolate(
			params->StaminaRegenMult_LowStamina.Get(),
			params->StaminaRegenMult_HighStamina.Get(),
			staminaPct,
			params->StaminaRegenCurve_kStamina.Get());

		// 3. Magicka 
		float maxMagicka = actor->GetPermanentActorValue(RE::ActorValue::kMagicka);
		if (maxMagicka > 0.0f) {
			magickaPct = Math::Clamp01(actor->GetActorValue(RE::ActorValue::kMagicka) / maxMagicka);
		}
		float magickaFactor = Math::Interpolate(
			params->StaminaRegenMult_LowMagicka.Get(),
			params->StaminaRegenMult_HighMagicka.Get(),
            magickaPct,
			params->StaminaRegenCurve_kMagicka.Get());

        return healthFactor*staminaFactor*magickaFactor;
    }

    float GetBlockStaminaMult(RE::Actor*, const Burden::ActorBurdenData& burdenData)
    {
        // TODO: Still incomplete formula
		auto* params = RegenPenalties::GetSingleton();
		float burdenRatio = burdenData.burden;
        return params->RegenMult_blocking.Get()*burdenRatio;
    }

    float GetActionPenaltyMult(RE::Actor* actor, const Burden::ActorBurdenData& burdenData) {
        // Incomplete, but for now is OK. We should use continued actions with
        // Cost for maintaining like blocking and bows
		if (actor->IsBlocking()) {
            return GetBlockStaminaMult(actor, burdenData);
        }
        return 1.0f;
    }

	float GetMovementPenaltyMult(RE::Actor* actor)
	{
		auto* params = RegenPenalties::GetSingleton();

		if (actor->IsSprinting()) {
			return params->RegenMult_sprinting.Get();
		}
		if (actor->IsSwimming()) {
			return params->RegenMult_swimming.Get();
		}
		if (actor->IsRunning()) {
			return params->RegenMult_running.Get();
		}
		if (actor->IsWalking()) {
			return params->RegenMult_walking.Get();
		}

		return params->RegenMult_static.Get();
	}

	float ComputeStaminaRegenMult(RE::Actor* actor)
	{
		if (!actor)
			return 1.0f;

		auto& burdenData = Burden::Tracker::GetOrComputeBurden(actor);

        float regenBonus = 0.0f;

        if (GetCanRegenStamina(actor)) {
            float burdenStaminaMult = GetBurdenStaminaMult(actor, burdenData);
            float HMS_factor = GetHMSStaminaMult(actor, burdenData);
            float gassedOutFactor = 1.0f; // Placeholder for now
            regenBonus = burdenStaminaMult * HMS_factor * gassedOutFactor;
        }

        // Penalties
		float movementPenalty = GetMovementPenaltyMult(actor); // TODO Use burden Data
		float actionPenalty = GetActionPenaltyMult(actor, burdenData);
        float weatherPenalty = 1.0f; // TODO
		return regenBonus - weatherPenalty - movementPenalty - actionPenalty;
	}

	float ComputeHealthRegenMult(RE::Actor* actor)
	{
		if (!actor)
			return 1.0f;
        float maxStamina = actor->GetPermanentActorValue(RE::ActorValue::kStamina);
		if (maxStamina <= 0.0f) {
            return 1.0f;
		}
        auto* params = RegenParams::GetSingleton();
        float staminaPct = Math::Clamp01(actor->GetActorValue(RE::ActorValue::kStamina) / maxStamina);
		return Math::Interpolate(
			params->HealthRegenMult_LowStamina.Get(),
			params->HealthRegenMult_HighStamina.Get(),
            staminaPct,
			params->HealthRegenCurve_k.Get());
	}

	float ComputeMagickaRegenMult(RE::Actor* actor)
	{
		if (!actor)
			return 1.0f;
        float maxStamina = actor->GetPermanentActorValue(RE::ActorValue::kStamina);
		if (maxStamina <= 0.0f) {
            return 1.0f;
		}
        auto* params = RegenParams::GetSingleton();
        float staminaPct = Math::Clamp01(actor->GetActorValue(RE::ActorValue::kStamina) / maxStamina);
		return Math::Interpolate(
			params->MagickaRegenMult_LowStamina.Get(),
			params->MagickaRegenMult_HighStamina.Get(),
            staminaPct,
			params->MagickaRegenCurve_k.Get());
	}
}
