#include "API/StaminaAndBurdenAPI.h"
#include "Burden/BurdenManager.h"
#include "Burden/BurdenTracker.h"

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
