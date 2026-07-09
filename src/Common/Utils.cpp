#include "Common/Utils.h"
#include "Stamina/RegenManager.h"
#include "Settings/Params/DenyParams.h"

namespace Utils
{
	LeftHandInfo GetLeftHandInfo(RE::Actor* actor)
	{
		LeftHandInfo info;
		auto* form = actor->GetEquippedObject(true);
		if (!form)
			return info;
		if (auto* weap = form->As<RE::TESObjectWEAP>()) {
			info.type = LeftHandType::kWeapon;
			info.weapon = weap;
		} else if (auto* shield = form->As<RE::TESObjectARMO>(); shield && shield->IsShield()) {
			info.type = LeftHandType::kShield;
			info.shield = shield;
		}
		return info;
	}

	RightHandInfo GetRightHandInfo(RE::Actor* actor)
	{
		RightHandInfo info;
		auto* form = actor->GetEquippedObject(false);
		if (!form)
			return info;
		auto* weap = form->As<RE::TESObjectWEAP>();
		if (!weap)
			return info;
		auto type = weap->GetWeaponType();
		if (type == RE::WEAPON_TYPE::kHandToHandMelee)
			info.type = RightHandType::kHandToHand;
		else if (type == RE::WEAPON_TYPE::kBow || type == RE::WEAPON_TYPE::kCrossbow)
			info.type = RightHandType::kBow;
		else if (type == RE::WEAPON_TYPE::kTwoHandSword || type == RE::WEAPON_TYPE::kTwoHandAxe)
			info.type = RightHandType::kTwoHanded;
		else
			info.type = RightHandType::kOneHanded;
		info.weapon = weap;
		return info;
	}

	AttackHandInfo GetAttackHandInfo(RE::Actor* actor, bool left, bool blocking)
	{
		AttackHandInfo info;
		if (blocking) {
			auto leftInfo = Utils::GetLeftHandInfo(actor);
			if (leftInfo.HasShield()) {
				info.type = AttackHandType::BashShield;
				info.form = leftInfo.shield;
				return info;
			}
			if (Utils::GetRightHandInfo(actor).type == Utils::RightHandType::kBow) {
				info.type = AttackHandType::BashBow;
				info.form = actor->GetEquippedObject(false);
				return info;
			}
			info.type = AttackHandType::BashWeapon;
			info.form = actor->GetEquippedObject(false);
			return info;
		}
		auto* obj = actor->GetEquippedObject(left);
		if (!obj) {
			info.type = AttackHandType::Unarmed;
			return info;
		}
		auto* weap = obj->As<RE::TESObjectWEAP>();
		if (!weap) {
			info.type = AttackHandType::Unarmed;
			if (auto* armor = obj->As<RE::TESObjectARMO>(); armor && armor->IsShield())
				info.form = obj;
			return info;
		}
		auto type = weap->GetWeaponType();
		if (type == RE::WEAPON_TYPE::kHandToHandMelee) {
			info.type = AttackHandType::Unarmed;
			return info;
		}
		if (type == RE::WEAPON_TYPE::kBow || type == RE::WEAPON_TYPE::kCrossbow) {
			info.type = AttackHandType::Ranged;
			info.form = obj;
			return info;
		}
		if (!left && (type == RE::WEAPON_TYPE::kTwoHandSword || type == RE::WEAPON_TYPE::kTwoHandAxe)) {
			info.type = AttackHandType::TwoHanded;
			info.form = obj;
			return info;
		}
		info.type = AttackHandType::OneHanded;
		info.form = obj;
		return info;
	}
}

namespace Common
{
	bool CanDoStaminaAction(RE::Actor* actor, float cost)
	{
		if (!actor || cost <= 0.0f) {
			return true;
        }

		auto* params = Deny::DenyParams::GetSingleton();
		float threshold = params->fMinStaminaCostMult.Get() * cost;
		return actor->GetActorValue(RE::ActorValue::kStamina) > threshold;
	}

	void ApplyStaminaCost(RE::Actor* actor, float cost)
	{
		if (!actor || cost <= 0.0f)
			return;

        const auto av_stamina = RE::ActorValue::kStamina;

		float oldStamina = actor->GetActorValue(av_stamina);
		actor->DamageActorValue(av_stamina, cost);

		if (actor->GetActorValue(av_stamina) <= 0.0f) {
			float overspent = cost - oldStamina;
			if (overspent > 0.0f) {
				float rate = Regen::GetEngineStaminaRate(actor);
				if (rate > 0.0f) {
					actor->UpdateRegenDelay(av_stamina, overspent / rate);
                }
			}
			if (actor->IsPlayerRef())
				RE::HUDMenu::FlashMeter(av_stamina);
		}
	}
}
