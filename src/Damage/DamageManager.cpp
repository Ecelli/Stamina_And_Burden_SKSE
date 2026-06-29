#include "Damage/DamageManager.h"
#include "Common/Utils.h"

namespace Damage
{
	float ComputeStaminaDamageMult(RE::Actor* actor)
	{
		if (!actor)
			return 1.0f;

		float maxStamina = actor->GetActorValueMax(RE::ActorValue::kStamina);
		if (maxStamina <= 0.0f)
			return 1.0f;

		float staminaPct = Math::Clamp01(
			actor->GetActorValue(RE::ActorValue::kStamina) / maxStamina);

		auto* params = DamageParams::GetSingleton();
		float result = Math::Interpolate(
			params->fDamageScaleLow.Get(),
			params->fDamageScaleHigh.Get(),
			staminaPct,
			params->fDamageScaleCurve_k.Get());

		DamageLog("ComputeStaminaDamageMult: {:x} staminaPct={:.2f} mult={:.3f}",
			actor->GetFormID(), staminaPct, result);

		return result;
	}
}
