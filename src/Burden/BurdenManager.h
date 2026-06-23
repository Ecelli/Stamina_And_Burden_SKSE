#pragma once

namespace Burden
{

	struct ActorBurdenData
	{
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
		float weaponBurden_1h{ 0.0f };
		float weaponBurden_2h{ 0.0f };
		float weaponBurden_left{ 0.0f };
	};

	float GetEquippedWeight(RE::Actor* actor);
	float ComputeEquipmentBurden(RE::Actor* actor);
	float ComputeWeaponBurden(float weight, int skill);
	float GetBoundWeaponWeight(int conjurationSkill, bool isTwoHanded);
	ActorBurdenData UpdateBurden(RE::Actor* actor);
	ActorBurdenData UpdateBurdenLog(RE::Actor* actor);
}
