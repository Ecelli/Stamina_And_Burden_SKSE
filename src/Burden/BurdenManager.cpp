#include "Burden/BurdenManager.h"
#include "Settings/Params/BurdenParams.h"

namespace Burden
{
	float GetEquippedWeight(RE::Actor* actor)
	{
		return actor->GetEquippedWeight();
	}

	ActorBurdenData UpdateBurden(RE::Actor* actor)
	{
		ActorBurdenData data{};

		auto* params = BurdenParams::GetSingleton();
		data.maxCarryWeight = actor->GetPermanentActorValue(RE::ActorValue::kCarryWeight);
		data.carryWeight = actor->GetInventoryChanges()->GetInventoryWeight();
		data.equippedWeight = GetEquippedWeight(actor);
		data.maxEquippedWeight = params->maxEquippedWeightRatio.Get() * data.maxCarryWeight;

		data.carryBurden = std::clamp(data.carryWeight / data.maxCarryWeight, 0.0f, 1.0f);
		data.burden = std::clamp(data.equippedWeight / data.maxEquippedWeight, 0.0f, 1.0f);

		return data;
	}

	ActorBurdenData UpdateBurdenLog(RE::Actor* actor)
	{
		auto data = UpdateBurden(actor);
		logger::info("Burden | Carry: {:.2f}% ({:.1f}/{}), Equipped: {:.2f}% ({:.1f}/{:.1f})",
			data.carryBurden * 100.0f,
			data.carryWeight,
			data.maxCarryWeight,
			data.burden * 100.0f,
			data.equippedWeight,
			data.maxEquippedWeight);
        return data;
	}

	void TaskUpdatePlayerBurdenLog()
	{
        SKSE::GetTaskInterface()->AddTask([]() {
            auto* player = RE::PlayerCharacter::GetSingleton();
            if (player && player->Get3D()) {
                UpdateBurdenLog(player);
            }
        });
	}
}
