#include "Movement/MovementManager.h"
#include "Burden/BurdenTracker.h"
#include "Stamina/ExhaustionManager.h"
#include "Settings/Params/MovementSpeedParams.h"
#include "Common/Utils.h"

namespace
{
	float GetSubmergedLevel(RE::Actor* a_actor)
	{
		using func_t = float(*)(RE::Actor*, float, RE::TESObjectCELL*);
		REL::Relocation<func_t> func{ REL::ID(37448) };
		return func(a_actor, a_actor->GetPositionZ(), a_actor->GetParentCell());
	}
}

namespace Movement
{
	float ComputeSpeedMultiplier(RE::Actor* a_actor)
	{
		auto* params = MovementSpeedParams::GetSingleton();
		float burdenMult = 1.0f;
		float swimMult = 1.0f;
		float exhaustMult = 1.0f;

		// Burden speed scaling
		if (params->bEnableBurdenSpeed.Get()) {
			auto& data = Burden::Tracker::GetOrComputeBurden(a_actor);
			burdenMult = Math::Interpolate(
				params->speedMultLowBurden.Get(),
				params->speedMultHighBurden.Get(),
				data.burdenBlend,
				params->speedCurve_k.Get());
		}

		// Swim speed scaling
		if (params->bEnableSwimSpeed.Get()) {
			float submerged = GetSubmergedLevel(a_actor);
			swimMult = Math::Interpolate(
				params->speedMultAboveWater.Get(),
				params->speedMultSubmerged.Get(),
				submerged,
				1.0f);
		}

		// Exhaustion speed penalty
		if (params->bEnableExhaustionSpeed.Get()) {
			if (Exhaustion::ExhaustionManager::GetSingleton()->IsExhausted(a_actor)) {
				exhaustMult = params->exhaustionSpeedMult.Get();
			}
		}

		float result = burdenMult * swimMult * exhaustMult;

		if (result != 1.0f) {
			Movement::MovementLog(
				"MovementSpeedHook: {:x} -> SpeedMult = {:.2f} "
				"(burden={:.3f} swim={:.3f} exhaust={:.3f})",
				a_actor->GetFormID(), result,
				burdenMult, swimMult, exhaustMult);
		}

		return result;
	}
}
