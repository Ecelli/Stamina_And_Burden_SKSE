#pragma once

#include "Common/Utils.h"

namespace Burden
{

	struct WeaponHandlingInfo
	{
		float weaponBurden{ 0.0f };
		int   weaponSkill{ 0 };
	};

	struct ActorBurdenData
	{
		RE::Actor* actor{ nullptr };
		float maxCarryWeight{ 0.0f };
		float carryWeight{ 0.0f };
		float equippedWeight{ 0.0f };     //  Weighted
		float maxEquippedWeight{ 0.0f };  //  Max Weighted equipped weight/ max carry weight
		float carryBurden{ 0.0f };        //  Carry weight/Max Carry Weight
		float burden{ 0.0f };             //  weighted equipped weight/max equiped weight
		float burdenBlend{ 0.0f };        //  1 - sqrt((1-burden)*(1-carryBurden))
		int lightSkill{ -1 };
		int heavySkill{ -1 };
		int oneHandedSkill{ -1 };
		int twoHandedSkill{ -1 };
		int marksmanSkill{ -1 };
		int blockSkill{ -1 };
		int conjurationSkill{ -1 };
		float weaponBurden_rh{ 0.0f };
		float weaponBurden_lh{ 0.0f };
		float weaponBurden_2h{ 0.0f };
		float weaponBurden_ranged{ 0.0f };
		float weaponBurden_block{ 0.0f };
	};

	WeaponHandlingInfo GetWeaponHandlingInfo(const ActorBurdenData& data, Utils::RightHandType type);

	float GetEquippedWeight(RE::Actor* actor);
	float ComputeEquipmentBurden(RE::Actor* actor);
	float ScaleWeaponWeight(float weight, int skill);
	float GetBoundWeaponWeight(int conjurationSkill, bool isTwoHanded);
	float ResolveWeaponWeight(RE::TESObjectWEAP* weapon, int conjurationSkill);
	ActorBurdenData UpdateBurden(RE::Actor* actor);
	ActorBurdenData UpdateBurdenLog(RE::Actor* actor);
}
