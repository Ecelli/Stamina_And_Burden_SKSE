#include "Regen/RegenManager.h"
#include "Burden/BurdenTracker.h"
#include "Common/Utils.h"

namespace Regen
{
	bool GetCanRegenStamina(RE::Actor* actor)
	{
		return actor->GetAttackState() == RE::ATTACK_STATE_ENUM::kNone;
	}

	MovementState GetMovementState(RE::Actor* actor)
	{
		if (actor->IsSwimming())
			return MovementState::kSwimming;
		if (actor->IsRunning())
			return MovementState::kRunning;
		if (actor->IsSneaking())
			return MovementState::kSneaking;
		if (actor->IsWalking())
			return MovementState::kWalking;
		return MovementState::kStatic;
	}

	float ComputeStateRegenFactor(const Burden::ActorBurdenData& data, MovementState state, float HMS)
	{
		auto* params = RegenMovementParams::GetSingleton();
		float maxVal, minVal;

		switch (state) {
		case MovementState::kSwimming:
			maxVal = params->RegenSwimming_max.Get();
			minVal = params->RegenSwimming_min.Get();
			break;
		case MovementState::kRunning:
			maxVal = params->RegenRunning_max.Get();
			minVal = params->RegenRunning_min.Get();
			break;
		case MovementState::kSneaking:
			maxVal = params->RegenSneaking_max.Get();
			minVal = params->RegenSneaking_min.Get();
			break;
		case MovementState::kWalking:
			maxVal = params->RegenWalking_max.Get();
			minVal = params->RegenWalking_min.Get();
			break;
		default:
			maxVal = params->RegenStatic_max.Get();
			minVal = params->RegenStatic_min.Get();
			break;
		}

		float k = params->MovementRegenCurve_k.Get();
		float blend = data.burdenBlend;
		float result = Math::Interpolate(maxVal * HMS, minVal, blend, k);
		return result;
	}

	float ComputeBlockCost(const Burden::ActorBurdenData& data)
	{
		float perc = RegenMovementParams::GetSingleton()->BlockRegenCostBurdenPerc.Get();
		float result = perc * data.burdenBlend;
		RegenLog("ComputeBlockCost: perc={:.3f} blend={:.3f} -> {:.3f}", perc, data.burdenBlend, result);
		return result;
	}

	float GetHMSStaminaMult(RE::Actor* actor)
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

		return healthFactor * staminaFactor * magickaFactor;
	}

	float ComputeStaminaRegenMult(RE::Actor* actor)
	{
		if (!actor)
			return 1.0f;

		auto& burdenData = Burden::Tracker::GetOrComputeBurden(actor);
		float regenBonus = 0.0f;

		float HMS = GetHMSStaminaMult(actor);
		if (GetCanRegenStamina(actor)) {
			MovementState state = GetMovementState(actor);
			regenBonus = ComputeStateRegenFactor(burdenData, state, HMS);
		}

		float blockCost = actor->IsBlocking() ? ComputeBlockCost(burdenData) : 0.0f;
		float weatherPenalty = 0.0f;

		float result = regenBonus - blockCost - weatherPenalty;
		RegenLog("ComputeStaminaRegenMult: MovementFactor={:.3f} HMS={:.3f} block={:.3f} weather={:.3f} -> {}",
			regenBonus, HMS, blockCost, weatherPenalty, result);
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
		float maxStamina = actor->GetActorValueMax(RE::ActorValue::kStamina);
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
