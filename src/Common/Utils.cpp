#include "Common/Utils.h"
#include "Stamina/RegenManager.h"

namespace Common
{
	bool CanDoAction(RE::Actor* actor, float cost)
	{
		if (!actor || cost <= 0.0f) {
			return true;
        }
		if (!actor->IsPlayerRef() && actor->GetActorValue(RE::ActorValue::kStaminaRateMult) *
									 actor->GetActorValue(RE::ActorValue::kStaminaRate) <
                                     0.00001f){
			return true;
        }
		auto curr_stamina = actor->GetActorValue(RE::ActorValue::kStamina);

		return curr_stamina > 0.3 * cost ;
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
