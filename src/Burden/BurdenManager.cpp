#include "Burden/BurdenManager.h"
#include "Burden/BurdenTracker.h"
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
	float GetMaxEquippedWeight(RE::Actor* actor, float maxCarryWeight)
	{
		float override = Burden::Tracker::GetMaxEquippedWeightOverride(actor);
		if (override > 0.0f)
			return std::min(override, maxCarryWeight);

		auto* params = BurdenParams::GetSingleton();
		return std::max(params->maxEquippedWeightRatio.Get() * maxCarryWeight, 1.0f);
	}

	void ComputeRightHandBurden(RE::Actor* actor, Burden::ActorBurdenData& data)
	{
		auto info = Utils::GetRightHandInfo(actor);
		data.weaponBurden_rh = 0.0f;
		data.weaponBurden_2h = 0.0f;
		data.weaponBurden_ranged = 0.0f;

		switch (info.type) {
		case Utils::RightHandType::kTwoHanded:
			{
				float w = Burden::ResolveWeaponWeight(info.weapon, data.conjurationSkill);
				data.weaponBurden_2h = Math::Clamp01(Burden::ScaleWeaponWeight(w, data.twoHandedSkill) / data.maxEquippedWeight);
				break;
			}
		case Utils::RightHandType::kBow:
			{
				float w = Burden::ResolveWeaponWeight(info.weapon, data.conjurationSkill);
				data.weaponBurden_ranged = Math::Clamp01(Burden::ScaleWeaponWeight(w, data.marksmanSkill) / data.maxEquippedWeight);
				break;
			}
		case Utils::RightHandType::kOneHanded:
			{
				float w = Burden::ResolveWeaponWeight(info.weapon, data.conjurationSkill);
				data.weaponBurden_rh = Math::Clamp01(Burden::ScaleWeaponWeight(w, data.oneHandedSkill) / data.maxEquippedWeight);
				break;
			}
		case Utils::RightHandType::kHandToHand:
			data.weaponBurden_rh = Math::Clamp01(BurdenParams::GetSingleton()->UnarmedWeight.Get()
				/ data.maxEquippedWeight);
			break;
		case Utils::RightHandType::kStaff:
			{
				float w = Burden::ResolveWeaponWeight(info.weapon, data.conjurationSkill);
				data.weaponBurden_rh = Math::Clamp01(Burden::ScaleWeaponWeight(w, data.staffSkill) / data.maxEquippedWeight);
				break;
			}
		default:
			break;
		}
	}

	void ComputeLeftHandBurden(RE::Actor* actor, Burden::ActorBurdenData& data)
	{
		auto info = Utils::GetLeftHandInfo(actor);
		switch (info.type) {
		case Utils::LeftHandType::kStaff:
			{
				float w = Burden::ResolveWeaponWeight(info.weapon, data.conjurationSkill);
				data.weaponBurden_lh = Math::Clamp01(Burden::ScaleWeaponWeight(w, data.staffSkill) / data.maxEquippedWeight);
				break;
			}
		case Utils::LeftHandType::kWeapon:
			data.weaponBurden_lh = Math::Clamp01(Burden::ScaleWeaponWeight(
				Burden::ResolveWeaponWeight(info.weapon, data.conjurationSkill),
				data.oneHandedSkill) / data.maxEquippedWeight);
			break;
		case Utils::LeftHandType::kShield:
			data.weaponBurden_lh = 0.0f;
			break;
		default:
			data.weaponBurden_lh = Math::Clamp01(BurdenParams::GetSingleton()->UnarmedWeight.Get()
				/ data.maxEquippedWeight);
			break;
		}
	}

	float ComputeBlendedBlockSkill(int weaponSkill, int blockSkill)
	{
		auto* params = BurdenParams::GetSingleton();
		float weaponRatio = Math::Clamp01(static_cast<float>(weaponSkill) / params->PlayerMaxSkill.Get());
		float blockRatio = Math::Clamp01(static_cast<float>(blockSkill) / params->PlayerMaxSkill.Get());
		float blend = params->BlockSkillBlendFactor.Get();
		return std::pow(weaponRatio, 1.0f - blend) * std::pow(blockRatio, blend);
	}

	float ComputeBlockBurden(RE::Actor* actor, Burden::ActorBurdenData& data)
	{
		auto leftInfo = Utils::GetLeftHandInfo(actor);
		auto* params = BurdenParams::GetSingleton();

		// Shield block
		if (leftInfo.HasShield()) {
			return Math::Clamp01(Burden::ScaleWeaponWeight(leftInfo.shield->GetWeight(), data.blockSkill) / data.maxEquippedWeight);
		}

		auto blockMult = [&](int skill) {
			float blockBurden = ComputeBlendedBlockSkill(skill, data.blockSkill);
			return Math::Interpolate(
				params->BlockWeightMult_LowSkill.Get(),
				params->BlockWeightMult_HighSkill.Get(),
				blockBurden,
				params->BlockWeightMult_Curve_k.Get());
		};

		// Block for DW or only left weapon.
		if (leftInfo.HasWeapon()) {
            int skill;
			auto rightInfo = Utils::GetRightHandInfo(actor);
			if (rightInfo.HasWeapon()) {
				float dwPenalty = params->DualWieldBlockPenalty.Get();
                skill = rightInfo.type == Utils::RightHandType::kStaff ? data.staffSkill : data.oneHandedSkill;
				return Math::Clamp01(dwPenalty * 0.5f
					* (data.weaponBurden_lh + data.weaponBurden_rh)
					* blockMult(skill));
			} else {
                skill = leftInfo.type == Utils::LeftHandType::kStaff ? data.staffSkill : data.oneHandedSkill;
				return Math::Clamp01(data.weaponBurden_lh * blockMult(skill));
			}
		}

		// Block unarmored
		auto rightInfo = Utils::GetRightHandInfo(actor);
		if (!rightInfo.HasWeapon()) {
			return Math::Clamp01(params->UnarmedWeight.Get() / data.maxEquippedWeight
				* blockMult(data.blockSkill));
		}

		// Block for 2h weapons or 1h weapon
		auto h = Burden::GetWeaponHandlingInfo(data, rightInfo.type);
		return Math::Clamp01(h.weaponBurden * blockMult(h.weaponSkill));
	}
}

namespace Burden
{
	WeaponHandlingInfo GetWeaponHandlingInfo(const ActorBurdenData& d, Utils::RightHandType t)
	{
		switch (t) {
		case Utils::RightHandType::kTwoHanded:
			return { d.weaponBurden_2h, d.twoHandedSkill };
		case Utils::RightHandType::kBow:
			return { d.weaponBurden_ranged, d.marksmanSkill };
		case Utils::RightHandType::kOneHanded:
			return { d.weaponBurden_rh, d.oneHandedSkill };
		case Utils::RightHandType::kHandToHand:
			return { d.weaponBurden_rh, d.blockSkill };
		case Utils::RightHandType::kStaff:
			return { d.weaponBurden_rh, d.staffSkill };
		default:
			return { d.weaponBurden_rh, 0 };
		}
	}

	float ScaleWeaponWeight(float weight, int skill)
	{
		auto* params = BurdenParams::GetSingleton();
		float ratio = Math::Clamp01(static_cast<float>(skill) / params->PlayerMaxSkill.Get());
		float mult = Math::Interpolate(
			params->WeaponWeightMult_LowSkill.Get(),
			params->WeaponWeightMult_HighSkill.Get(),
			ratio,
			params->WeaponWeightMult_Curve_k.Get());
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

	float ResolveWeaponWeight(RE::TESObjectWEAP* weapon, int conjurationSkill)
	{
		if (!weapon->IsBound())
			return weapon->GetWeight();
		auto type = weapon->GetWeaponType();
		bool isTwoHanded = (type == RE::WEAPON_TYPE::kTwoHandSword || type == RE::WEAPON_TYPE::kTwoHandAxe);
		return GetBoundWeaponWeight(conjurationSkill, isTwoHanded);
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
		data.actor = actor;

		data.maxCarryWeight = std::max(actor->GetActorValue(RE::ActorValue::kCarryWeight), 1.0f);
		data.carryWeight = actor->GetInventoryChanges()->GetInventoryWeight();
		data.equippedWeight = ComputeEquipmentBurden(actor);
		data.maxEquippedWeight = GetMaxEquippedWeight(actor, data.maxCarryWeight);

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
		data.staffSkill = static_cast<int>(actor->GetActorValue(RE::ActorValue::kEnchanting));

		ComputeRightHandBurden(actor, data);
		ComputeLeftHandBurden(actor, data);
		data.weaponBurden_block = ComputeBlockBurden(actor, data);

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

    // This is used specifically for external APIs and Papyrus scripts
	float ResolveBurdenValue(const ActorBurdenData& data, int component)
	{
		switch (component) {
		case 0: return data.burden;
		case 1: return data.carryBurden;
		case 2: return data.burdenBlend;
		case 3: return data.weaponBurden_rh;
		case 4: return data.weaponBurden_lh;
		case 5: return data.weaponBurden_2h;
		case 6: return data.weaponBurden_ranged;
		case 7: return data.weaponBurden_block;
		default: return 0.0f;
		}
	}

}
