#include "API/StaminaAndBurdenAPI.h"
#include "Burden/BurdenTracker.h"
#include "Stamina/CostsManager.h"
#include "Stamina/ExhaustionManager.h"
#include "Common/Utils.h"

namespace
{
	float ResolveBurdenValue(const Burden::ActorBurdenData& data, StaminaAndBurdenAPI::BurdenComponent component)
	{
		return Burden::ResolveBurdenValue(data, static_cast<int>(component));
	}


	class InterfaceImpl final : public StaminaAndBurdenAPI::InterfaceVersion1
	{
	public:
		StaminaAndBurdenAPI::Version GetVersion() override
		{
			return StaminaAndBurdenAPI::Version::Current;
		}

		float GetBurden(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Burden::Tracker::GetOrComputeBurden(a_actor).burden;
		}

		float GetCarryBurden(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Burden::Tracker::GetOrComputeBurden(a_actor).carryBurden;
		}

		float GetBurdenBlend(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Burden::Tracker::GetOrComputeBurden(a_actor).burdenBlend;
		}

		float GetBurdenEquippedWeight(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Burden::Tracker::GetOrComputeBurden(a_actor).equippedWeight;
		}

		float GetMaxEquippedWeight(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Burden::Tracker::GetOrComputeBurden(a_actor).maxEquippedWeight;
		}

		// --- Max equipped weight override ---

		void SetMaxEquippedWeightOverride(RE::Actor* a_actor, float a_maxEquippedWeight) override
		{
			Burden::Tracker::SetMaxEquippedWeightOverride(a_actor, a_maxEquippedWeight);
		}

		// --- Weapon burden ---

		float GetWeaponBurden(RE::Actor* a_actor, StaminaAndBurdenAPI::WeaponSlot a_slot) override
		{
			if (!a_actor) return 0.0f;
			const auto& data = Burden::Tracker::GetOrComputeBurden(a_actor);
			switch (a_slot) {
			case StaminaAndBurdenAPI::WeaponSlot::RightHand:  return data.weaponBurden_rh;
			case StaminaAndBurdenAPI::WeaponSlot::LeftHand:   return data.weaponBurden_lh;
			case StaminaAndBurdenAPI::WeaponSlot::TwoHanded:  return data.weaponBurden_2h;
			case StaminaAndBurdenAPI::WeaponSlot::Ranged:     return data.weaponBurden_ranged;
			case StaminaAndBurdenAPI::WeaponSlot::Block:      return data.weaponBurden_block;
			default: return 0.0f;
			}
		}

		// --- Attack costs ---
		float GetAttackCostFromData(RE::Actor* a_actor, RE::BGSAttackData* a_attackData) override
		{
			if (!a_actor || !a_attackData) return 0.0f;
			return Costs::ComputeAttackCost(a_actor, a_attackData);
		}

		float ComputeActionCost(
			RE::Actor* a_actor,
			StaminaAndBurdenAPI::BurdenComponent a_baseComponent,
			float a_baseMin, float a_baseMax, float a_baseK,
			StaminaAndBurdenAPI::BurdenComponent a_pctComponent,
			float a_pctMin, float a_pctMax, float a_pctK) override
		{
            if (!a_actor) return 0.0f;
            
            auto& burden = Burden::Tracker::GetOrComputeBurden(a_actor);
            float Stamina1pct = 0.01f * static_cast<float>(a_actor->GetActorValueMax(RE::ActorValue::kStamina));
            
            float baseT = ResolveBurdenValue(burden, a_baseComponent);
            float pctT = ResolveBurdenValue(burden, a_pctComponent);
            
            float base = Math::Interpolate(a_baseMin, a_baseMax, baseT, a_baseK);
            float pct = Stamina1pct * Math::Interpolate(a_pctMin, a_pctMax, pctT, a_pctK);
            
            return base + pct;

		}

		// --- State ---

		bool IsExhausted(RE::Actor* a_actor) override
		{
			if (!a_actor) return false;
			return Exhaustion::ExhaustionManager::GetSingleton()->IsExhausted(a_actor);
		}

		// --- Math utilities ---

		float Interpolate(float a_min, float a_max, float a_t, float a_k) override
		{
			return Math::Interpolate(a_min, a_max, a_t, a_k);
		}

		float SmoothStep(float a_t) override
		{
			return Math::SmoothStep(a_t);
		}

		void RegisterExhaustionListener(ExhaustionListener a_listener) override
		{
			Exhaustion::ExhaustionManager::GetSingleton()->RegisterExhaustionListener(a_listener);
		}
	};

	InterfaceImpl& GetImpl()
	{
		static InterfaceImpl impl;
		return impl;
	}
}

extern "C" DLLEXPORT void* __stdcall SB_RequestInterfaceImpl(StaminaAndBurdenAPI::Version a_version)
{
	switch (a_version) {
	case StaminaAndBurdenAPI::Version::Version1:
		return &GetImpl();
	default:
		return nullptr;
	}
}
