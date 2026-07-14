#include "Stamina/RegenManager.h"
#include "Stamina/ExhaustionManager.h"
#include "Burden/BurdenTracker.h"
#include "Common/Utils.h"
#include "Common/PerkCategories.h"

namespace Regen
{
	bool GetCanRegenStamina(RE::Actor* actor)
	{
		if (actor->IsBlocking())
			return false;
		auto state = actor->GetAttackState();
		if (state == RE::ATTACK_STATE_ENUM::kBowDrawn ||
			state == RE::ATTACK_STATE_ENUM::kBowAttached)
			return false;
		return state == RE::ATTACK_STATE_ENUM::kNone;
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

	float ComputeBlockHoldPenalty(RE::Actor* actor)
	{
		if (!actor || !actor->IsBlocking())
			return 0.0f;
		auto& data = Burden::Tracker::GetOrComputeBurden(actor);
		auto* params = RegenMovementParams::GetSingleton();
		float penalty = Math::Interpolate(
			params->BlockHoldLowBurden.Get(),
			params->BlockHoldHighBurden.Get(),
			data.weaponBurden_block,
			params->BlockHoldCurve_k.Get());

		float maxStamina = actor->GetActorValueMax(RE::ActorValue::kStamina);
		penalty += maxStamina * 0.01f * Math::Interpolate(
			params->HoldDrainLowBlended.Get(),
			params->HoldDrainHighBlended.Get(),
			data.burdenBlend,
			params->HoldBlendedCurve_k.Get());

		RE::HandleEntryPoint(PEPE_STAMINA_ENTRY_POINT, actor, penalty, PEPE::Group::BlockHoldStamina,
			Utils::GetAttackHandInfo(actor, false, true).form);

		RegenLog("ComputeBlockHoldPenalty: blockBurden={:.3f} blend={:.3f} penalty={:.3f}/s",
			data.weaponBurden_block, data.burdenBlend, penalty);
		return penalty;
	}

	float ComputeStaffHoldPenalty(RE::Actor* actor)
	{
		(void)actor;
		return 1.0f;
	}

	float ComputeBowDrawHoldPenalty(RE::Actor* actor)
	{
		if (!actor)
			return 0.0f;
		auto state = actor->GetAttackState();
		if (state != RE::ATTACK_STATE_ENUM::kBowDrawn &&
			state != RE::ATTACK_STATE_ENUM::kBowAttached)
			return 0.0f;
		auto* obj = actor->GetEquippedObject(false);
		auto* weap = obj ? obj->As<RE::TESObjectWEAP>() : nullptr;
		if (!weap)
			return 0.0f;
		auto type = weap->GetWeaponType();
		if (type != RE::WEAPON_TYPE::kBow && type != RE::WEAPON_TYPE::kCrossbow)
			return 0.0f;
		auto& data = Burden::Tracker::GetOrComputeBurden(actor);
		auto* params = RegenMovementParams::GetSingleton();
		float penalty = Math::Interpolate(
			params->BowDrawLowBurden.Get(),
			params->BowDrawHighBurden.Get(),
			data.weaponBurden_ranged,
			params->BowDrawCurve_k.Get());

		float maxStamina = actor->GetActorValueMax(RE::ActorValue::kStamina);
		penalty += maxStamina * 0.01f * Math::Interpolate(
			params->HoldDrainLowBlended.Get(),
			params->HoldDrainHighBlended.Get(),
			data.burdenBlend,
			params->HoldBlendedCurve_k.Get());

		RE::HandleEntryPoint(PEPE_STAMINA_ENTRY_POINT, actor, penalty, PEPE::Group::BowDrawHoldStamina, weap);

		RegenLog("ComputeBowDrawHoldPenalty: weapBurden={:.3f} blend={:.3f} penalty={:.3f}/s",
			data.weaponBurden_ranged, data.burdenBlend, penalty);
		return penalty;
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

	float GetEngineStaminaRate(RE::Actor* actor)
	{
		float rate = GetBaseStaminaRate(actor);
		if (rate > 0.0f) {
			if (actor->IsInCombat()) {
				rate *= RE::GameSettingCollection::GetSingleton()->GetSetting("fCombatStaminaRegenRateMult")->GetFloat();
			}
			rate *= actor->GetActorValue(RE::ActorValue::kStaminaRateMult) * 0.01f;
		}
		return rate;
	}

	float GetBaseStaminaRate(RE::Actor* actor)
	{
		float rate = actor->GetActorValue(RE::ActorValue::kStaminaRate) * 0.01f;
		if (rate <= 0.0f)
			return 0.0f;

		rate = rate * actor->GetActorValueMax(RE::ActorValue::kStamina);
		return rate;
	}

	float ComputeWeatherPenalty(RE::Actor* actor)
	{
		if (!actor || actor != RE::PlayerCharacter::GetSingleton())
			return 0.0f;

		auto* sky = RE::Sky::GetSingleton();
		if (!sky || sky->mode == RE::Sky::Mode::kInterior)
			return 0.0f;

		auto* wParams = WeatherParams::GetSingleton();
		if (!wParams->WeatherEnabled.Get())
			return 0.0f;

		if (sky->IsSnowing())
			return wParams->WeatherSnowPenalty.Get();
		if (sky->IsRaining())
			return wParams->WeatherRainPenalty.Get();

		return 0.0f;
	}

	float ComputeBurnScaler(RE::Actor* actor)
	{
		if (!actor)
			return 1.0f;

		auto* params = NegativeRegen::GetSingleton();
		float rateMult = actor->GetActorValue(RE::ActorValue::kStaminaRateMult);
		float lowBound = params->BurnRate_LowBound.Get();
		float highBound = params->BurnRate_HighBound.Get();
		float range = highBound - lowBound;
		if (range <= 0.0f)
			return 1.0f;

		float t = (rateMult - lowBound) / range;
		t = Math::Clamp01(t);

		float low = params->BurnRate_LowBonus.Get();
		float high = params->BurnRate_HighBonus.Get();
		float k = params->BurnRate_Curve_k.Get();

		float scaler = Math::Interpolate(low, high, t, k);
		RegenLog("ComputeBurnScaler: kStaminaRateMult={:.0f} t={:.3f} scaler={:.3f}", rateMult, t, scaler);
		return scaler;
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

		float weatherPenalty = ComputeWeatherPenalty(actor);

		float result = regenBonus - weatherPenalty;
		RegenLog("ComputeStaminaRegenMult: MovementFactor={:.3f} HMS={:.3f} weather={:.3f} -> {}",
			regenBonus, HMS, weatherPenalty, result);
		return result;
	}

	float ComputeBurdenStaminaRegenRate(RE::Actor* actor)
	{
		if (!actor)
			return 0.0f;

		float mult = ComputeStaminaRegenMult(actor);
		float regenMult = mult >= 0.0f ? mult : 0.0f;
		float drainMult = mult < 0.0f ? -mult : 0.0f;

		float engineRate = GetEngineStaminaRate(actor);
		if (engineRate < 0.0f)
			engineRate = 0.0f;

		engineRate *= Exhaustion::GetExhaustionRegenMult(actor, RE::ActorValue::kStamina);

		float rate = engineRate * regenMult;
		if (drainMult > 0.0f) {
			float burnBase = GetBaseStaminaRate(actor);
			float scaler = ComputeBurnScaler(actor);
			rate -= burnBase * drainMult * scaler;
		}
		rate -= ComputeBlockHoldPenalty(actor);
		rate -= ComputeBowDrawHoldPenalty(actor);

		RegenLog("ComputeBurdenStaminaRegenRate: engineRate={:.3f} mult={:.3f} -> rate={:.3f}/s", engineRate, mult, rate);
		return rate;
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
			params->HealthRegenCurve_k.Get())
			* Exhaustion::GetExhaustionRegenMult(actor, RE::ActorValue::kHealth);
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
			params->MagickaRegenCurve_k.Get())
			* Exhaustion::GetExhaustionRegenMult(actor, RE::ActorValue::kMagicka);
	}
}
