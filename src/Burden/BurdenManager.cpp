#include "Burden/BurdenManager.h"
#include "Settings/Params/BurdenParams.h"
#include "Common/Utils.h"

namespace
{
	float GetSlotMultiplier(RE::TESObjectARMO* a_armor)
	{
		if (!a_armor) {
			return BurdenParams::GetSingleton()->SlotBurdenMult_def.Get();
		}

		auto params = BurdenParams::GetSingleton();
		using Slot = RE::BGSBipedObjectForm::BipedObjectSlot;

		if (a_armor->HasPartOf(Slot::kBody) || a_armor->HasPartOf(Slot::kModChestPrimary)) {
			return params->SlotBurdenMult_body.Get();
		}

		if (a_armor->HasPartOf(Slot::kFeet)) {
			return params->SlotBurdenMult_feet.Get();
		}

		if (a_armor->HasPartOf(Slot::kHead)) {
			return params->SlotBurdenMult_head.Get();
		}

		if (a_armor->HasPartOf(Slot::kHands)) {
			return params->SlotBurdenMult_hand.Get();
		}

		return params->SlotBurdenMult_def.Get();
	}

	float GetWeightedArmorTypeMult(RE::Actor* a_actor, RE::TESObjectARMO* a_armor)
	{
		auto armorType = a_armor->GetArmorType();
		auto params = BurdenParams::GetSingleton();
		float minMult, maxMult;
		RE::ActorValue skill;

		switch (armorType) {
		case RE::BGSBipedObjectForm::ArmorType::kLightArmor:
			minMult = params->SkillBurdenMult_minLight.Get();
			maxMult = params->SkillBurdenMult_maxLight.Get();
			skill = RE::ActorValue::kLightArmor;
			break;
		case RE::BGSBipedObjectForm::ArmorType::kHeavyArmor:
			minMult = params->SkillBurdenMult_minHeavy.Get();
			maxMult = params->SkillBurdenMult_maxHeavy.Get();
			skill = RE::ActorValue::kHeavyArmor;
			break;
		default:
			return 1.0f;
		}

		float skillValue = a_actor->GetActorValue(skill);
		// Invert: at 0 skill → maxMult (most burden), at 100 skill → minMult (least burden)
		float skillRatio = 1.0f - Math::Clamp01(skillValue / params->PlayerMaxSkill.Get());
		float skillMultiplier = Math::Interpolate(minMult, maxMult, skillRatio, params->SkillInterpolate.Get());
		return skillMultiplier;
	}

	class BurdenEquipVisitor : public RE::InventoryChanges::IItemChangeVisitor
	{
	public:
		RE::Actor* actor;
		float total = 0.0f;

		explicit BurdenEquipVisitor(RE::Actor* a_actor) :
			actor(a_actor)
		{}

		RE::BSContainer::ForEachResult Visit(RE::InventoryEntryData* a_entry) override
		{
			auto* obj = a_entry->object;
			if (!obj) {
				return RE::BSContainer::ForEachResult::kContinue;
			}

			float weight = a_entry->GetWeight();
			float slotMult = 1.0f;
			float armorTypeMult = 1.0f;

			if (auto* armor = obj->As<RE::TESObjectARMO>()) {
				slotMult = GetSlotMultiplier(armor);
				armorTypeMult = GetWeightedArmorTypeMult(actor, armor);
			}

			total += weight * slotMult * armorTypeMult;
			return RE::BSContainer::ForEachResult::kContinue;
		}
	};
}

namespace Burden
{
	float GetEquippedWeight(RE::Actor* actor)
	{
		return actor->GetEquippedWeight();
	}

	float ComputeEquipmentBurden(RE::Actor* actor)
	{
		if (!actor || !actor->GetInventoryChanges()) {
			return 0.0f;
		}

		BurdenEquipVisitor visitor(actor);
		actor->GetInventoryChanges()->VisitWornItems(visitor);
		return visitor.total;
	}

	ActorBurdenData UpdateBurden(RE::Actor* actor)
	{
		ActorBurdenData data{};

		auto* params = BurdenParams::GetSingleton();
		data.maxCarryWeight = actor->GetPermanentActorValue(RE::ActorValue::kCarryWeight);
		data.carryWeight = actor->GetInventoryChanges()->GetInventoryWeight();
		data.equippedWeight = ComputeEquipmentBurden(actor);
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
