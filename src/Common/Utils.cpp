#include "Common/Utils.h"
#include "Stamina/RegenManager.h"
#include "Settings/Params/DenyParams.h"

namespace Common
{
	bool CanDoStaminaAction(RE::Actor* actor, float cost)
	{
		if (!actor || cost <= 0.0f) {
			return true;
        }

		auto* params = Deny::DenyParams::GetSingleton();
		float threshold = params->fMinStaminaCostMult.Get() * cost;
		return actor->GetActorValue(RE::ActorValue::kStamina) > threshold;
	}

	void ApplyStaminaCost(RE::Actor* actor, float cost)
	{
		if (!actor || cost <= 0.0f)
			return;

        const auto av_stamina = RE::ActorValue::kStamina;

		float oldStamina = actor->GetActorValue(av_stamina);
		actor->DamageActorValue(av_stamina, cost);

		if (actor->GetActorValue(av_stamina) <= 0.0f) {
			float overspent = cost - oldStamina;
			if (overspent > 0.0f) {
				float rate = Regen::GetEngineStaminaRate(actor);
				if (rate > 0.0f) {
					actor->UpdateRegenDelay(av_stamina, overspent / rate);
                }
			}
			if (actor->IsPlayerRef())
				RE::HUDMenu::FlashMeter(av_stamina);
		}
	}
}
