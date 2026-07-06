#include "Stamina/ExhaustionManager.h"
#include "Settings/Params/ExhaustionParams.h"
#include "Common/Utils.h"

void Exhaustion::ExhaustionManager::TriggerExhaustion(RE::Actor* a_actor)
{
	if (!a_actor)
		return;

	auto* params = ExhaustionParams::GetSingleton();
	bool isPlayer = a_actor->IsPlayerRef();
	if ((isPlayer && !params->bExhaustionPlayer.Get()) ||
		(!isPlayer && !params->bExhaustionNPC.Get()))
		return;

	auto formId = a_actor->GetFormID();
	states[formId] = { true, 0.0f };

	ExhaustionLog("ExhaustionManager: triggered for {:x}", formId);
}

bool Exhaustion::ExhaustionManager::IsExhausted(RE::Actor* a_actor)
{
	if (!a_actor)
		return false;

	auto it = states.find(a_actor->GetFormID());
	return it != states.end() && it->second.isExhausted;
}

void Exhaustion::ExhaustionManager::UpdateExhaustion(RE::Actor* a_actor, float a_deltaTime)
{
	auto it = states.find(a_actor->GetFormID());
	if (it == states.end() || !it->second.isExhausted)
		return;

	if (a_actor->IsDead()) {
		states.erase(it);
		return;
	}

	float maxStamina = a_actor->GetActorValueMax(RE::ActorValue::kStamina);
	if (maxStamina <= 0.0f) {
		states.erase(it);
		return;
	}


	float curStamina = a_actor->GetActorValue(RE::ActorValue::kStamina);
	float staminaPct = Math::Clamp01(curStamina / maxStamina);

	auto* params = ExhaustionParams::GetSingleton();
	float threshold = params->fExhaustionThresholdStamina.Get();

	if (staminaPct >= threshold) {
		ExhaustionLog("ExhaustionManager: {:x} cleared (threshold {:.0f}% >= {:.0f}%)",
			a_actor->GetFormID(), staminaPct * 100.0f, threshold * 100.0f);
		states.erase(it);
		return;
	}

	if (curStamina > 0.0f)
		it->second.safeTimer += a_deltaTime;
	else
		it->second.safeTimer = 0.0f;

	float duration = params->fExhaustionDuration.Get();
	if (it->second.safeTimer >= duration) {
		float burst = params->fExhaustionBurstStamina.Get();
		float burstAmt = burst * maxStamina;
		a_actor->RestoreActorValue(RE::ActorValue::kStamina, burstAmt);
		ExhaustionLog("ExhaustionManager: {:x} cleared (timer {:.1f}s), burst +{:.0f}",
			a_actor->GetFormID(), it->second.safeTimer, burstAmt);
		states.erase(it);
		return;
	}
}

void Exhaustion::ExhaustionManager::ClearAll()
{
	states.clear();
}

void Exhaustion::ExhaustionManager::ClearExhaustion(RE::FormID a_formId)
{
	states.erase(a_formId);
}

void Exhaustion::CheckForAndTriggerExhaustion(RE::Actor* a_actor, float a_deltaTime)
{
	if (!a_actor)
		return;

	auto* mgr = ExhaustionManager::GetSingleton();

	if (mgr->IsExhausted(a_actor)) {
		mgr->UpdateExhaustion(a_actor, a_deltaTime);
		return;
	}

	if (a_actor->GetActorValue(RE::ActorValue::kStamina) <= 0.0f) {
		mgr->TriggerExhaustion(a_actor);
	}
}

float Exhaustion::GetExhaustionDamageMultiplier(RE::Actor* a_actor)
{
	if (!a_actor)
		return 1.0f;

	if (!ExhaustionManager::GetSingleton()->IsExhausted(a_actor))
		return 1.0f;

	return ExhaustionParams::GetSingleton()->fExhaustionPenaltyDamageMult.Get();
}

float Exhaustion::GetExhaustionRegenMult(RE::Actor* a_actor, RE::ActorValue a_av)
{
	if (!a_actor)
		return 1.0f;

	if (!ExhaustionManager::GetSingleton()->IsExhausted(a_actor))
		return 1.0f;

	auto* params = ExhaustionParams::GetSingleton();
	switch (a_av) {
	case RE::ActorValue::kStamina: return params->fExhaustionPenaltyStaminaMult.Get();
	case RE::ActorValue::kHealth:  return params->fExhaustionPenaltyHealthMult.Get();
	case RE::ActorValue::kMagicka: return params->fExhaustionPenaltyMagickaMult.Get();
	default: return 1.0f;
	}
}
