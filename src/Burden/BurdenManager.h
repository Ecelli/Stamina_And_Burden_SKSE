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
	};

	float GetEquippedWeight(RE::Actor* actor);
	ActorBurdenData UpdateBurden(RE::Actor* actor);
	void UpdateBurdenLog(RE::Actor* actor);
	void TaskUpdatePlayerBurdenLog();
}
