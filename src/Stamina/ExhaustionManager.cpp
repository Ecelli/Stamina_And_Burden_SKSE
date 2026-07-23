#include "Stamina/ExhaustionManager.h"
#include "Settings/Params/ExhaustionParams.h"
#include "Common/Utils.h"

#include <vector>

namespace
{
	using ExhaustionListener = Exhaustion::ExhaustionManager::ExhaustionListener;
	std::vector<ExhaustionListener> s_exhaustionListeners;

	void DispatchExhaustionEvent(RE::Actor* a_actor, bool a_exhausted)
	{
		for (auto& listener : s_exhaustionListeners)
			listener(a_actor, a_exhausted);
	}

	void FireExhaustionEvent(RE::Actor* a_actor, bool a_exhausted)
	{
		if (!a_actor)
			return;

		DispatchExhaustionEvent(a_actor, a_exhausted);

		const SKSE::ModCallbackEvent event{
			.eventName = RE::BSFixedString("StaminaAndBurden_OnExhaustionChanged"),
			.numArg = a_exhausted ? 1.0f : 0.0f,
			.sender = a_actor
		};
		SKSE::GetModCallbackEventSource()->SendEvent(&event);
	}
}

void Exhaustion::ExhaustionManager::RegisterExhaustionListener(ExhaustionListener a_listener)
{
	if (a_listener)
		s_exhaustionListeners.push_back(a_listener);
}

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

	SetStaminaBarGrayIfPlayer(a_actor);
	FireExhaustionEvent(a_actor, true);

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
	if (it == states.end() || !it->second.isExhausted) {
        ResetStaminaBarColorIfPlayer(a_actor);
		return;
    }

	if (a_actor->IsDead()) {
		states.erase(it);
		FireExhaustionEvent(a_actor, false);
        ResetStaminaBarColorIfPlayer(a_actor);
		return;
	}

	float maxStamina = a_actor->GetActorValueMax(RE::ActorValue::kStamina);
	if (maxStamina <= 0.0f) {
		states.erase(it);
		FireExhaustionEvent(a_actor, false);
        ResetStaminaBarColorIfPlayer(a_actor);
		return;
	}


	SetStaminaBarGrayIfPlayer(a_actor);

	float curStamina = a_actor->GetActorValue(RE::ActorValue::kStamina);
	float staminaPct = Math::Clamp01(curStamina / maxStamina);

	auto* params = ExhaustionParams::GetSingleton();
	float threshold = params->fExhaustionThresholdStamina.Get();

	if (staminaPct >= threshold) {
		ExhaustionLog("ExhaustionManager: {:x} cleared (threshold {:.0f}% >= {:.0f}%)",
			a_actor->GetFormID(), staminaPct * 100.0f, threshold * 100.0f);
		states.erase(it);
		FireExhaustionEvent(a_actor, false);
        ResetStaminaBarColorIfPlayer(a_actor);
		return;
	}

	if (curStamina > 0.0f || !a_actor->IsPlayerRef())
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
		FireExhaustionEvent(a_actor, false);
        ResetStaminaBarColorIfPlayer(a_actor);
		return;
	}
}

void Exhaustion::SetStaminaBarGrayIfPlayer(RE::Actor* a_actor)
{
	if (!a_actor || !a_actor->IsPlayerRef())
		return;

	if (g_trueHUDAvailable && g_trueHUD) {
		g_trueHUD->OverrideBarColor(
			a_actor->GetHandle(),
			RE::ActorValue::kStamina,
			TRUEHUD_API::BarColorType::BarColor,
			0x7d7e7d);
		return;
	}

	if (auto* ui = RE::UI::GetSingleton())
		if (auto hud = ui->GetMenu<RE::HUDMenu>())
			if (hud->stamina)
				hud->stamina->root.SetColorTint(RE::GColor(128, 128, 128, 255));
}

void Exhaustion::ResetStaminaBarColorIfPlayer(RE::Actor* a_actor)
{
	if (!a_actor || !a_actor->IsPlayerRef())
		return;

	if (g_trueHUDAvailable && g_trueHUD) {
		g_trueHUD->RevertBarColor(
			a_actor->GetHandle(),
			RE::ActorValue::kStamina,
			TRUEHUD_API::BarColorType::BarColor);
		return;
	}

	if (auto* ui = RE::UI::GetSingleton()) {
		if (auto hud = ui->GetMenu<RE::HUDMenu>()) {
			if (hud->stamina) {
				hud->stamina->root.RemoveColorTint();
            }
        }
    }
}

void Exhaustion::ExhaustionManager::ClearAll()
{
	ResetStaminaBarColorIfPlayer(RE::PlayerCharacter::GetSingleton());
	states.clear();
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
