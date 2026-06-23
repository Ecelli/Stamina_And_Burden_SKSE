#include "Burden/BurdenManager.h"
#include "Settings/Params/BurdenParams.h"
#include "Common/Utils.h"

namespace
{
	bool HasSteedStoneBlessing(RE::Actor* a_actor)
	{
		static RE::SpellItem* steedStoneAbility = nullptr;
		static bool initialized = false;
		if (!initialized) {
			steedStoneAbility = RE::TESForm::LookupByEditorID<RE::SpellItem>("doomSteedAbility");
			initialized = true;
			if (steedStoneAbility) {
				logger::info("  >Steed Stone ability found: {:X}", steedStoneAbility->GetFormID());
			} else {
				logger::warn("  >Steed Stone ability NOT found by EditorID 'doomSteedAbility'");
			}
		}
		if (!steedStoneAbility) {
			return false;
		}

		auto* effectList = a_actor->GetActiveEffectList();
		if (!effectList) {
			return false;
		}

		for (auto* effect : *effectList) {
			if (effect && effect->spell == steedStoneAbility) {
				return true;
			}
		}
		return false;
	}

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
		float SteedMult;
		bool steedStoneActive;

		explicit BurdenEquipVisitor(RE::Actor* a_actor) :
			actor(a_actor),
			steedStoneActive(HasSteedStoneBlessing(a_actor)),
			SteedMult(BurdenParams::GetSingleton()->SteedStoneBurdenMult.Get())
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
				if (steedStoneActive) {
					slotMult *= SteedMult;
				}
			}

			total += weight * slotMult * armorTypeMult;
			return RE::BSContainer::ForEachResult::kContinue;
		}
	};
}

namespace
{
	void ComputeRightHandBurden(RE::Actor* actor, Burden::ActorBurdenData& data)
	{
		auto* form = actor->GetEquippedObject(false);
		auto* weap = (form && form->IsWeapon()) ? form->As<RE::TESObjectWEAP>() : nullptr;

		if (!weap) {
			data.weaponBurden_1h = 0.0f;
			data.weaponBurden_2h = 0.0f;
			data.weaponBurden_ranged = 0.0f;
			return;
		}

		auto type = weap->GetWeaponType();
		bool bound = weap->IsBound();
		float w;

		switch (type) {
		case RE::WEAPON_TYPE::kTwoHandSword:
		case RE::WEAPON_TYPE::kTwoHandAxe:
			w = bound ? Burden::GetBoundWeaponWeight(data.conjurationSkill, true) : weap->GetWeight();
			data.weaponBurden_2h = Burden::ComputeWeaponBurden(w, data.twoHandedSkill);
			data.weaponBurden_1h = 0.0f;
			data.weaponBurden_ranged = 0.0f;
			break;

		case RE::WEAPON_TYPE::kBow:
		case RE::WEAPON_TYPE::kCrossbow:
			w = bound ? Burden::GetBoundWeaponWeight(data.conjurationSkill, false) : weap->GetWeight();
			data.weaponBurden_ranged = Burden::ComputeWeaponBurden(w, data.marksmanSkill);
			data.weaponBurden_1h = 0.0f;
			data.weaponBurden_2h = 0.0f;
			break;

		default:
			w = bound ? Burden::GetBoundWeaponWeight(data.conjurationSkill, false) : weap->GetWeight();
			data.weaponBurden_1h = Burden::ComputeWeaponBurden(w, data.oneHandedSkill);
			data.weaponBurden_2h = 0.0f;
			data.weaponBurden_ranged = 0.0f;
			break;
		}
	}

	void ComputeLeftHandBurden(RE::Actor* actor, Burden::ActorBurdenData& data)
	{
		auto* form = actor->GetEquippedObject(true);
		auto* weap = (form && form->IsWeapon()) ? form->As<RE::TESObjectWEAP>() : nullptr;
		auto* shield = (form && !weap) ? form->As<RE::TESObjectARMO>() : nullptr;

		if (weap) {
			float w = weap->IsBound() ? Burden::GetBoundWeaponWeight(data.conjurationSkill, false) : weap->GetWeight();
			data.weaponBurden_left = Burden::ComputeWeaponBurden(w, data.oneHandedSkill);
			data.weaponBurden_block = 0.0f;
		} else if (shield && shield->IsShield()) {
			data.weaponBurden_left = 0.0f;
			data.weaponBurden_block = Burden::ComputeWeaponBurden(shield->GetWeight(), data.blockSkill);
		} else {
			data.weaponBurden_left = 0.0f;
			data.weaponBurden_block = 0.0f;
		}
	}
}

namespace Burden
{
	float ComputeWeaponBurden(float weight, int skill)
	{
		auto* params = BurdenParams::GetSingleton();
		float ratio = Math::Clamp01(static_cast<float>(skill) / params->PlayerMaxSkill.Get());
		float mult = Math::Interpolate(
			params->WeaponBurden_LowSkill.Get(),
			params->WeaponBurden_HighSkill.Get(),
			ratio,
			params->WeaponSkillInterpolate.Get());
		return weight * mult;
	}

	float GetBoundWeaponWeight(int conjurationSkill, bool isTwoHanded)
	{
		auto* params = BurdenParams::GetSingleton();
		float ratio = Math::Clamp01(static_cast<float>(conjurationSkill) / params->PlayerMaxSkill.Get());
		float weight = Math::Interpolate(
			params->ConjuredWeightMin.Get(),
			params->ConjuredWeightMax.Get(),
			ratio,
			params->ConjuredWeightCurve_k.Get());
		return isTwoHanded ? weight * 2.0f : weight;
	}

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
		data.maxCarryWeight = actor->GetActorValue(RE::ActorValue::kCarryWeight);
		data.carryWeight = actor->GetInventoryChanges()->GetInventoryWeight();
		data.equippedWeight = ComputeEquipmentBurden(actor);
		data.maxEquippedWeight = params->maxEquippedWeightRatio.Get() * data.maxCarryWeight;

		data.carryBurden = std::clamp(data.carryWeight / data.maxCarryWeight, 0.0f, 1.0f);
		data.burden = std::clamp(data.equippedWeight / data.maxEquippedWeight, 0.0f, 1.0f);
		data.burdenBlend = 1.0f - std::sqrt((1.0f - data.burden) * (1.0f - data.carryBurden));

		data.lightSkill = static_cast<int>(actor->GetActorValue(RE::ActorValue::kLightArmor));
		data.heavySkill = static_cast<int>(actor->GetActorValue(RE::ActorValue::kHeavyArmor));
		data.oneHandedSkill = static_cast<int>(actor->GetActorValue(RE::ActorValue::kOneHanded));
		data.twoHandedSkill = static_cast<int>(actor->GetActorValue(RE::ActorValue::kTwoHanded));
		data.marksmanSkill = static_cast<int>(actor->GetActorValue(RE::ActorValue::kArchery));
		data.blockSkill = static_cast<int>(actor->GetActorValue(RE::ActorValue::kBlock));
		data.conjurationSkill = static_cast<int>(actor->GetActorValue(RE::ActorValue::kConjuration));

		ComputeRightHandBurden(actor, data);
		ComputeLeftHandBurden(actor, data);

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

}
