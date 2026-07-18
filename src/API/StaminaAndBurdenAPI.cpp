#include "API/StaminaAndBurdenAPI.h"
#include "Burden/BurdenManager.h"
#include "Burden/BurdenTracker.h"
#include "Movement/MovementCostManager.h"
#include "Movement/MovementManager.h"
#include "Stamina/RegenManager.h"
#include "Stamina/CostsManager.h"
#include "Stamina/ExhaustionManager.h"
#include "Common/Utils.h"

#include <RE/B/BGSAttackData.h>

namespace
{
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

		float GetRightHandWeaponBurden(RE::Actor* a_actor) override
		{
			return GetWeaponBurden(a_actor, StaminaAndBurdenAPI::WeaponSlot::RightHand);
		}

		float GetLeftHandWeaponBurden(RE::Actor* a_actor) override
		{
			return GetWeaponBurden(a_actor, StaminaAndBurdenAPI::WeaponSlot::LeftHand);
		}

		float GetTwoHandedWeaponBurden(RE::Actor* a_actor) override
		{
			return GetWeaponBurden(a_actor, StaminaAndBurdenAPI::WeaponSlot::TwoHanded);
		}

		float GetRangedWeaponBurden(RE::Actor* a_actor) override
		{
			return GetWeaponBurden(a_actor, StaminaAndBurdenAPI::WeaponSlot::Ranged);
		}

		float GetBlockWeaponBurden(RE::Actor* a_actor) override
		{
			return GetWeaponBurden(a_actor, StaminaAndBurdenAPI::WeaponSlot::Block);
		}

		// --- Non-attack costs ---

		float GetSprintDrain(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Movement::ComputeSprintDrain(a_actor);
		}

		float GetJumpCost(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Movement::ComputeJumpCost(a_actor);
		}

		// --- Hold penalties ---

		float GetBlockHoldPenalty(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Regen::ComputeBlockHoldPenalty(a_actor);
		}

		float GetBowDrawHoldPenalty(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Regen::ComputeBowDrawHoldPenalty(a_actor);
		}

		float GetStaffHoldPenalty(RE::Actor* a_actor, bool a_leftHand) override
		{
			if (!a_actor) return 0.0f;
			return Regen::ComputeStaffHoldPenalty(a_actor, a_leftHand);
		}

		// --- Attack costs ---

		float GetBaseAttackCost(RE::Actor* a_actor, bool a_bash, bool a_left, bool a_power) override
		{
			if (!a_actor) return 0.0f;
			return Costs::ComputeBaseAttackCost(a_actor, a_bash, a_left, a_power);
		}

		float GetAttackCostFromData(RE::Actor* a_actor, RE::BGSAttackData* a_attackData) override
		{
			if (!a_actor || !a_attackData) return 0.0f;
			return Costs::ComputeAttackCost(a_actor, a_attackData);
		}

		float GetBaseNormalAttackCost(RE::Actor* a_actor) override
		{
			return GetBaseAttackCost(a_actor, false, false, false);
		}

		float GetBasePowerAttackCost(RE::Actor* a_actor) override
		{
			return GetBaseAttackCost(a_actor, false, false, true);
		}

		float GetBaseLeftHandAttackCost(RE::Actor* a_actor) override
		{
			return GetBaseAttackCost(a_actor, false, true, false);
		}

		float GetBaseLeftPowerAttackCost(RE::Actor* a_actor) override
		{
			return GetBaseAttackCost(a_actor, false, true, true);
		}

		float GetBaseBashAttackCost(RE::Actor* a_actor) override
		{
			return GetBaseAttackCost(a_actor, true, false, false);
		}

		float GetBaseBashPowerAttackCost(RE::Actor* a_actor) override
		{
			return GetBaseAttackCost(a_actor, true, false, true);
		}

		float GetBowFireCost(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Costs::ComputeBowFireCost(a_actor);
		}

		float GetStaffFireCost(RE::Actor* a_actor, bool a_leftHand) override
		{
			if (!a_actor) return 0.0f;
			return Costs::ComputeStaffFireCost(a_actor, a_leftHand);
		}

		// --- Movement multipliers ---

		float GetSpeedMultiplier(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Movement::ComputeSpeedMultiplier(a_actor);
		}

		float GetJumpHeightMultiplier(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Movement::ComputeJumpHeightMult(a_actor);
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
