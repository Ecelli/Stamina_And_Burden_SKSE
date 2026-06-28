#include "Stamina/CostsManager.h"
#include "Stamina/RegenManager.h"
#include "Burden/BurdenManager.h"
#include "Burden/BurdenTracker.h"
#include "Common/Utils.h"
#include "Settings/Params/CostsParams.h"
#include "Settings/Params/RegenParams.h"

#include <RE/B/BGSAttackData.h>
#include <RE/T/TESObjectWEAP.h>

namespace
{
	enum class AttackHandType
	{
		Unarmed,
		OneHanded,
		TwoHanded,
		Ranged,
		BashShield,
		BashBow,
		BashWeapon
	};

	AttackHandType GetAttackHandType(RE::Actor* actor, bool left, bool bash)
	{
		if (bash) {
			auto leftInfo = Burden::GetLeftHandInfo(actor);
			if (leftInfo.HasShield())
				return AttackHandType::BashShield;
			if (Burden::GetRightHandInfo(actor).type == Burden::RightHandType::kBow)
				return AttackHandType::BashBow;
			return AttackHandType::BashWeapon;
		}
		auto* obj = actor->GetEquippedObject(left);
		if (!obj)
			return AttackHandType::Unarmed;
		auto* weap = obj->As<RE::TESObjectWEAP>();
		if (!weap)
			return AttackHandType::Unarmed;
		auto type = weap->GetWeaponType();
		if (type == RE::WEAPON_TYPE::kHandToHandMelee)
			return AttackHandType::Unarmed;
		if (type == RE::WEAPON_TYPE::kBow || type == RE::WEAPON_TYPE::kCrossbow)
			return AttackHandType::Ranged;
		if (!left && (type == RE::WEAPON_TYPE::kTwoHandSword || type == RE::WEAPON_TYPE::kTwoHandAxe))
			return AttackHandType::TwoHanded;
		return AttackHandType::OneHanded;
	}

	float ComputeBurdenAttackCost(const Burden::ActorBurdenData& burden)
	{
		auto* params = Costs::AttackCostParams::GetSingleton();
		float maxStamina = burden.actor ? burden.actor->GetActorValueMax(RE::ActorValue::kStamina) : 100.0f;
		return maxStamina * 0.01f * Math::Interpolate(
			params->AttackLowCarryPct.Get(),
			params->AttackHighCarryPct.Get(),
			burden.burdenBlend,
			params->AttackCarryCurve_k.Get());
	}

	float Compute1hAttack(const Burden::ActorBurdenData& burden, float weaponBurden, bool power)
	{
		auto* params = Costs::AttackCostParams::GetSingleton();
		float burdenTerm = Math::Interpolate(
			params->Attack1hLowBurden.Get(),
			params->Attack1hHighBurden.Get(),
			weaponBurden,
			params->Attack1hBurdenCurve_k.Get());
		float cost = burdenTerm + ComputeBurdenAttackCost(burden);
		if (power)
			cost *= params->Attack1hPowerMult.Get();
		return cost;
	}

	float Compute2hAttack(const Burden::ActorBurdenData& burden, float weaponBurden, bool power)
	{
		auto* params = Costs::AttackCostParams::GetSingleton();
		float burdenTerm = Math::Interpolate(
			params->Attack2hLowBurden.Get(),
			params->Attack2hHighBurden.Get(),
			weaponBurden,
			params->Attack2hBurdenCurve_k.Get());
		float cost = burdenTerm + ComputeBurdenAttackCost(burden);
		if (power)
			cost *= params->Attack2hPowerMult.Get();
		return cost;
	}

	float ComputeUnarmedAttack(const Burden::ActorBurdenData& burden, bool power)
	{
		auto* params = Costs::AttackCostParams::GetSingleton();
		float cost = params->UnarmedBaseFlat.Get() + ComputeBurdenAttackCost(burden);
		if (power)
			cost *= params->UnarmedPowerMult.Get();
		return cost;
	}

	float ComputeWeaponBash(const Burden::ActorBurdenData& burden, float weight, bool power)
	{
		auto* params = Costs::AttackCostParams::GetSingleton();
		float burdenTerm = Math::Interpolate(
			params->BashWeaponLowBurden.Get(),
			params->BashWeaponHighBurden.Get(),
			weight,
			params->BashWeaponBurdenCurve_k.Get());
		float cost = burdenTerm + ComputeBurdenAttackCost(burden);
		if (power)
			cost *= params->BashWeaponPowerMult.Get();
		return cost;
	}

	float ComputeBowBash(const Burden::ActorBurdenData& burden, float weight, bool power)
	{
		auto* params = Costs::AttackCostParams::GetSingleton();
		float burdenTerm = Math::Interpolate(
			params->BashBowLowBurden.Get(),
			params->BashBowHighBurden.Get(),
			weight,
			params->BashBowBurdenCurve_k.Get());
		float cost = burdenTerm + ComputeBurdenAttackCost(burden);
		if (power)
			cost *= params->BashBowPowerMult.Get();
		return cost;
	}

	float ComputeShieldBash(const Burden::ActorBurdenData& burden, float weight, bool power)
	{
		auto* params = Costs::AttackCostParams::GetSingleton();
		float burdenTerm = Math::Interpolate(
			params->BashShieldLowBurden.Get(),
			params->BashShieldHighBurden.Get(),
			weight,
			params->BashShieldBurdenCurve_k.Get());
		float cost = burdenTerm + ComputeBurdenAttackCost(burden);
		if (power)
			cost *= params->BashShieldPowerMult.Get();
		return cost;
	}
}

namespace Costs
{
	float ComputeSprintDrain(RE::Actor* actor)
	{
		if (!actor)
			return 0.0f;

		auto& burden = Burden::Tracker::GetOrComputeBurden(actor);
		auto* params = CostsParams::GetSingleton();

        float Stamina_1pctMax = 0.01f * actor->GetActorValueMax(RE::ActorValue::kStamina);
        float SprintBurdenFlat = Math::Interpolate(
			params->SprintDrainLowBurden.Get(),
			params->SprintDrainHighBurden.Get(),
			burden.burden,
			params->SprintDrainBurdenCurve_k.Get());
        float SprintBurdenMult = Math::Interpolate(
			params->SprintDrainLowCarryBurdenPct.Get(),
			params->SprintDrainHighCarryBurdenPct.Get(),
			burden.carryBurden,
			params->SprintDrainCarryBurdenCurve_k.Get());

        float TotalCost = SprintBurdenFlat + SprintBurdenMult* Stamina_1pctMax;

		float weatherPenalty = Regen::ComputeWeatherPenalty(actor);
		if (weatherPenalty > 0.0f) {
			float engineRate = Regen::GetEngineStaminaRate(actor);
			TotalCost += engineRate * weatherPenalty;
		}

		return TotalCost * RE::GetSecondsSinceLastFrame();
	}

	float ComputeJumpCost(RE::Actor* actor)
	{
		if (!actor)
			return 0.0f;

		auto& burden = Burden::Tracker::GetOrComputeBurden(actor);
		auto* params = CostsParams::GetSingleton();

		float Stamina_1pctMax = 0.01f * actor->GetActorValueMax(RE::ActorValue::kStamina);
		float JumpBurdenFlat = Math::Interpolate(
			params->JumpCostLowBurden.Get(),
			params->JumpCostHighBurden.Get(),
			burden.burden,
			params->JumpCostBurdenCurve_k.Get());
		float JumpCarryPct = Math::Interpolate(
			params->JumpCostLowCarryPct.Get(),
			params->JumpCostHighCarryPct.Get(),
			burden.carryBurden,
			params->JumpCostCarryCurve_k.Get());

		float TotalCost = JumpBurdenFlat + JumpCarryPct * Stamina_1pctMax;

		Costs::CostLog("ComputeJumpCost: burden={:.3f} carry={:.3f} -> {:.3f} for {:x}",
			burden.burden, burden.carryBurden, TotalCost, actor->GetFormID());

		return TotalCost;
	}

	float ComputeAttackCost(RE::Actor* actor, RE::BGSAttackData* attackData)
	{
		if (!actor || !attackData)
			return 0.0f;

		auto& burden = Burden::Tracker::GetOrComputeBurden(actor);
		bool power = attackData->data.flags.any(RE::AttackData::AttackFlag::kPowerAttack);
		bool left = attackData->IsLeftAttack();
		bool bash = attackData->data.flags.any(RE::AttackData::AttackFlag::kBashAttack);

		float baseCost = 0;

		switch (GetAttackHandType(actor, left, bash)) {
		case AttackHandType::BashShield:
			baseCost = ComputeShieldBash(burden, burden.weaponBurden_block, power);
			break;
		case AttackHandType::BashBow:
			baseCost = ComputeBowBash(burden, burden.weaponBurden_ranged, power);
			break;
		case AttackHandType::BashWeapon:
			{
				if (Burden::GetLeftHandInfo(actor).HasWeapon()) {
                    baseCost += ComputeWeaponBash(burden, burden.weaponBurden_lh, power);
                    if (Burden::GetRightHandInfo(actor).type != Burden::RightHandType::kHandToHand) {
                        baseCost += ComputeWeaponBash(burden, burden.weaponBurden_rh, power);
                    }
                }
				else if (Burden::GetRightHandInfo(actor).type == Burden::RightHandType::kTwoHanded)
                    baseCost = ComputeWeaponBash(burden, burden.weaponBurden_2h, power);
				else
                    baseCost += ComputeWeaponBash(burden, burden.weaponBurden_rh, power);
				break;
			}
		case AttackHandType::Unarmed:
			baseCost = ComputeUnarmedAttack(burden, power);
			break;
		case AttackHandType::TwoHanded:
			baseCost = Compute2hAttack(burden, burden.weaponBurden_2h, power);
			break;
		case AttackHandType::OneHanded:
			baseCost = left ?
				Compute1hAttack(burden, burden.weaponBurden_lh, power) :
				Compute1hAttack(burden, burden.weaponBurden_rh, power);
			break;
		case AttackHandType::Ranged:
		default:
			baseCost = 0.0f;
			break;
		}

		baseCost *= attackData->data.staminaMult;

		Costs::CostLog("ComputeAttackCost: power={} left={} bash={} staminaMult={:.2f} -> {:.3f} for {:x}",
			power, left, bash, attackData->data.staminaMult, baseCost, actor->GetFormID());

		return baseCost;
	}

	float ComputeBowFireCost(RE::Actor* actor)
	{
		if (!actor)
			return 0.0f;

		auto& burden = Burden::Tracker::GetOrComputeBurden(actor);
		auto* params = CostsParams::GetSingleton();

		float Stamina_1pctMax = 0.01f * actor->GetActorValueMax(RE::ActorValue::kStamina);
		float BowFireBurdenFlat = Math::Interpolate(
			params->BowFireLowBurden.Get(),
			params->BowFireHighBurden.Get(),
			burden.weaponBurden_ranged,
			params->BowFireBurdenCurve_k.Get());
		float BowFireCarryPct = Math::Interpolate(
			params->BowFireLowCarryPct.Get(),
			params->BowFireHighCarryPct.Get(),
			burden.carryBurden,
			params->BowFireCarryCurve_k.Get());

		float TotalCost = BowFireBurdenFlat + BowFireCarryPct * Stamina_1pctMax;

		Costs::CostLog("ComputeBowFireCost: burden={:.3f} carry={:.3f} -> {:.3f} for {:x}",
			burden.burden, burden.carryBurden, TotalCost, actor->GetFormID());

		return TotalCost;
	}
}
