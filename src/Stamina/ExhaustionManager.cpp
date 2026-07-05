#include "Stamina/ExhaustionManager.h"
#include "Settings/Params/ExhaustionParams.h"
#include "Common/Utils.h"
#include <chrono>

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

void Exhaustion::ExhaustionManager::Update()
{
	static auto lastUpdate = std::chrono::steady_clock::now();
	auto now = std::chrono::steady_clock::now();
	float deltaTime = std::chrono::duration<float>(now - lastUpdate).count();
	lastUpdate = now;

	auto* params = ExhaustionParams::GetSingleton();
	float duration = params->fExhaustionDuration.Get();
	float threshold = params->fExhaustionThresholdStamina.Get();
	float burst = params->fExhaustionBurstStamina.Get();

	for (auto it = states.begin(); it != states.end(); ) {
		if (!it->second.isExhausted) {
			it = states.erase(it);
			continue;
		}

		auto* actor = RE::TESForm::LookupByID<RE::Actor>(it->first);
		if (!actor || actor->IsDead()) {
			it = states.erase(it);
			continue;
		}

		float curStamina = actor->GetActorValue(RE::ActorValue::kStamina);
		float maxStamina = actor->GetActorValueMax(RE::ActorValue::kStamina);
		if (maxStamina <= 0.0f) {
			it = states.erase(it);
			continue;
		}

		float staminaPct = Math::Clamp01(curStamina / maxStamina);

		if (staminaPct >= threshold) {
			ExhaustionLog("ExhaustionManager: {:x} cleared (threshold {:.0f}% >= {:.0f}%)",
				it->first, staminaPct * 100.0f, threshold * 100.0f);
			it = states.erase(it);
			continue;
		}

		if (curStamina > 0.0f) {
			it->second.safeTimer += deltaTime;
		} else {
			it->second.safeTimer = 0.0f;
		}

		if (it->second.safeTimer >= duration) {
			float burstAmt = burst * maxStamina;
			actor->RestoreActorValue(RE::ActorValue::kStamina, burstAmt);

			ExhaustionLog("ExhaustionManager: {:x} cleared (timer {:.1f}s), burst +{:.0f}",
				it->first, it->second.safeTimer, burstAmt);

			it = states.erase(it);
			continue;
		}

		++it;
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

void Exhaustion::TaskUpdate()
{
	SKSE::GetTaskInterface()->AddTask([]() {
		ExhaustionManager::GetSingleton()->Update();
	});
}

void Exhaustion::CheckForAndTriggerExhaustion(RE::Actor* a_actor)
{
	if (!a_actor)
		return;

	if (a_actor->GetActorValue(RE::ActorValue::kStamina) <= 0.0f) {
		auto* mgr = ExhaustionManager::GetSingleton();
		if (!mgr->IsExhausted(a_actor)) {
			mgr->TriggerExhaustion(a_actor);
		}
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
