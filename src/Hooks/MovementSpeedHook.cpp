#include "Hooks/MovementSpeedHook.h"
#include "Burden/BurdenTracker.h"
#include "Stamina/ExhaustionManager.h"
#include "Settings/Params/MovementSpeedParams.h"
#include "Common/Utils.h"

namespace Hooks
{
	void MovementSpeedHook::Install()
	{
		// AE: REL::ID(37943) + 0x51 — call to SetMaximumMovementSpeed
		//     inside the actor movement update loop.
		//     Confirmed by WadeInWaterRedux reference mod.
		REL::Relocation<std::uintptr_t> target{ REL::ID(37943), 0x51 };

		if (!REL::make_pattern<"E8">().match(target.address())) {
			logger::error("  >MovementSpeedHook: pattern mismatch at REL::ID(37943) + 0x51"sv);
			return;
		}

		_func = SKSE::GetTrampoline().write_call<5>(target.address(), Speed);
		logger::info("  >MovementSpeedHook installed at REL::ID(37943) + 0x51");
	}

	float MovementSpeedHook::Speed(RE::Actor* a_actor)
	{
		float original_speed = _func(a_actor);

		if (!a_actor)
			return original_speed;

		return original_speed * ComputeSpeedMultiplier(a_actor);
	}

	float MovementSpeedHook::GetSubmergedLevel(RE::Actor* a_actor)
	{
		using func_t = float(*)(RE::Actor*, float, RE::TESObjectCELL*);
        REL::Relocation<func_t> func{ REL::ID(37448) };
		return func(a_actor, a_actor->GetPositionZ(), a_actor->GetParentCell());
	}

	float MovementSpeedHook::ComputeSpeedMultiplier(RE::Actor* a_actor)
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
			Regen::RegenLog(
				"MovementSpeedHook: {:x} -> SpeedMult = {:.2f} "
				"(burden={:.3f} swim={:.3f} exhaust={:.3f})",
				a_actor->GetFormID(), result,
				burdenMult, swimMult, exhaustMult);
		}

		return result;
	}
}
