#include "Regen/RegenManager.h"
#include "Burden/BurdenTracker.h"
#include "Common/Utils.h"

namespace Regen
{
    // Returns false for any action that does prevents stamina regeneration
    bool GetCanRegenStamina(RE::Actor* actor) {
        // For now only attacking can do this
        bool result = actor->GetAttackState() == RE::ATTACK_STATE_ENUM::kNone;
        RegenLog("GetCanRegenStamina: attackState={} -> {}", (int)actor->GetAttackState(), result);
        return result;
    }

    float GetBurdenStaminaMult(RE::Actor*, const Burden::ActorBurdenData& burdenData)
    {
		auto* params = RegenParams::GetSingleton();
		float burdenRatio = burdenData.burden;
		float carryBurdenRatio = burdenData.carryBurden;
        if (burdenRatio >= 1 || carryBurdenRatio >= 1) {
            RegenLog("GetBurdenStaminaMult: burden={:.3f} carry={:.3f} -> HighBurden={:.3f} (clamped)", burdenRatio, carryBurdenRatio, params->StaminaRegenMult_HighBurden.Get());
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

        float result = burdenFactor * carryBurdenFactor;
        RegenLog("GetBurdenStaminaMult: burden={:.3f}->f={:.3f} carry={:.3f}->f={:.3f} -> product={:.3f}", burdenRatio, burdenFactor, carryBurdenRatio, carryBurdenFactor, result);
        return result;
    }

    float GetHMSStaminaMult(RE::Actor* actor, const Burden::ActorBurdenData&)
    {
		float healthPct = 1.0f;
		float staminaPct = 1.0f;
		float magickaPct = 1.0f;

        auto* params = RegenParams::GetSingleton();

		// 1. Health 
		float maxHealth = actor->GetActorValueMax(RE::ActorValue::kHealth);
		if (maxHealth > 0.0f) {
			healthPct = Math::Clamp01(actor->GetActorValue(RE::ActorValue::kHealth) / maxHealth);
		}
		float healthFactor = Math::Interpolate(
			params->StaminaRegenMult_LowHealth.Get(),
			params->StaminaRegenMult_HighHealth.Get(),
			healthPct,
			params->StaminaRegenCurve_kHealth.Get());

		// 2. Stamina 
        float maxStamina = actor->GetActorValueMax(RE::ActorValue::kStamina);
		if (maxStamina > 0.0f) {
			staminaPct = Math::Clamp01(actor->GetActorValue(RE::ActorValue::kStamina) / maxStamina);
		}
		float staminaFactor = Math::Interpolate(
			params->StaminaRegenMult_LowStamina.Get(),
			params->StaminaRegenMult_HighStamina.Get(),
			staminaPct,
			params->StaminaRegenCurve_kStamina.Get());

		// 3. Magicka 
		float maxMagicka = actor->GetActorValueMax(RE::ActorValue::kMagicka);
		if (maxMagicka > 0.0f) {
			magickaPct = Math::Clamp01(actor->GetActorValue(RE::ActorValue::kMagicka) / maxMagicka);
		}
		float magickaFactor = Math::Interpolate(
			params->StaminaRegenMult_LowMagicka.Get(),
			params->StaminaRegenMult_HighMagicka.Get(),
            magickaPct,
			params->StaminaRegenCurve_kMagicka.Get());

        float result = healthFactor * staminaFactor * magickaFactor;
        RegenLog("GetHMSStaminaMult: hPct={:.3f}->f={:.3f} sPct={:.3f}->f={:.3f} mPct={:.3f}->f={:.3f} -> {}", healthPct, healthFactor, staminaPct, staminaFactor, magickaPct, magickaFactor, result);
        return result;
    }

    float GetBlockStaminaMult(RE::Actor*, const Burden::ActorBurdenData& burdenData)
    {
        // TODO: Still incomplete formula
		auto* params = RegenPenalties::GetSingleton();
		float burdenRatio = burdenData.burden;
        float result = params->RegenMult_blocking.Get() * burdenRatio;
        RegenLog("GetBlockStaminaMult: burdenRatio={:.3f} blockMult={:.3f} -> {}", burdenRatio, params->RegenMult_blocking.Get(), result);
        return result;
    }

    float GetActionPenaltyMult(RE::Actor* actor, const Burden::ActorBurdenData& burdenData) {
        // Incomplete, but for now is OK. We should use continued actions with
        // Cost for maintaining like blocking and bows
		if (actor->IsBlocking()) {
            float result = GetBlockStaminaMult(actor, burdenData);
            RegenLog("GetActionPenaltyMult: blocking=true -> {}", result);
            return result;
        }
        return 1.0f;
    }

	float GetMovementPenaltyMult(RE::Actor* actor)
	{
		auto* params = RegenPenalties::GetSingleton();

		if (actor->IsSprinting()) {
            RegenLog("GetMovementPenaltyMult: sprinting -> {}", params->RegenMult_sprinting.Get());
			return params->RegenMult_sprinting.Get();
		}
		if (actor->IsSwimming()) {
            RegenLog("GetMovementPenaltyMult: swimming -> {}", params->RegenMult_swimming.Get());
			return params->RegenMult_swimming.Get();
		}
		if (actor->IsRunning()) {
            RegenLog("GetMovementPenaltyMult: running -> {}", params->RegenMult_running.Get());
			return params->RegenMult_running.Get();
		}
		if (actor->IsWalking()) {
            RegenLog("GetMovementPenaltyMult: walking -> {}", params->RegenMult_walking.Get());
			return params->RegenMult_walking.Get();
		}

        RegenLog("GetMovementPenaltyMult: static -> {}", params->RegenMult_static.Get());
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
		float movementPenalty = GetMovementPenaltyMult(actor);
		float actionPenalty = GetActionPenaltyMult(actor, burdenData);
        float weatherPenalty = 0.0f;
        float result = regenBonus - weatherPenalty - movementPenalty - actionPenalty;
        RegenLog("ComputeStaminaRegenMult: bonus={:.3f} movePen={:.3f} actPen={:.3f} weatherPen={:.3f} -> {}", regenBonus, movementPenalty, actionPenalty, weatherPenalty, result);
        return result;
	}

	float ComputeHealthRegenMult(RE::Actor* actor)
	{
		if (!actor)
			return 1.0f;
        float maxStamina = actor->GetActorValueMax(RE::ActorValue::kStamina);
		if (maxStamina <= 0.0f) {
            return 1.0f;
		}
        auto* params = RegenParams::GetSingleton();
        float staminaPct = Math::Clamp01(actor->GetActorValue(RE::ActorValue::kStamina) / maxStamina);
        float result = Math::Interpolate(
			params->HealthRegenMult_LowStamina.Get(),
			params->HealthRegenMult_HighStamina.Get(),
            staminaPct,
			params->HealthRegenCurve_k.Get());
        RegenLog("ComputeHealthRegenMult: staminaPct={:.3f} low={:.3f} high={:.3f} k={:.3f} -> {}", staminaPct, params->HealthRegenMult_LowStamina.Get(), params->HealthRegenMult_HighStamina.Get(), params->HealthRegenCurve_k.Get(), result);
        return result;
	}

	float ComputeMagickaRegenMult(RE::Actor* actor)
	{
		if (!actor)
			return 1.0f;
        float maxStamina = actor->GetActorValueMax(RE::ActorValue::kStamina);
		if (maxStamina <= 0.0f) {
            return 1.0f;
		}
        auto* params = RegenParams::GetSingleton();
        float staminaPct = Math::Clamp01(actor->GetActorValue(RE::ActorValue::kStamina) / maxStamina);
        float result = Math::Interpolate(
			params->MagickaRegenMult_LowStamina.Get(),
			params->MagickaRegenMult_HighStamina.Get(),
            staminaPct,
			params->MagickaRegenCurve_k.Get());
        RegenLog("ComputeMagickaRegenMult: staminaPct={:.3f} low={:.3f} high={:.3f} k={:.3f} -> {}", staminaPct, params->MagickaRegenMult_LowStamina.Get(), params->MagickaRegenMult_HighStamina.Get(), params->MagickaRegenCurve_k.Get(), result);
        return result;
	}
}
