#include "Stamina/CostsManager.h"
#include "Burden/BurdenManager.h"
#include "Burden/BurdenTracker.h"
#include "Common/Utils.h"
#include "Common/PerkCategories.h"
#include "Settings/Params/CostsParams.h"

#include <RE/B/BGSAttackData.h>
#include <RE/T/TESObjectWEAP.h>

namespace
{
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
	float ComputeBaseAttackCost(RE::Actor* actor, bool bash, bool left, bool power)
	{
		if (!actor)
			return 0.0f;

		auto& burden = Burden::Tracker::GetOrComputeBurden(actor);
		auto handInfo = Utils::GetAttackHandInfo(actor, left, bash);
		float baseCost = 0;

		switch (handInfo.type) {
		case Utils::AttackHandType::BashShield:
			baseCost = ComputeShieldBash(burden, burden.weaponBurden_block, power);
			break;
		case Utils::AttackHandType::BashBow:
			baseCost = ComputeBowBash(burden, burden.weaponBurden_ranged, power);
			break;
		case Utils::AttackHandType::BashWeapon:
			{
				if (Utils::GetLeftHandInfo(actor).HasWeapon()) {
                    baseCost += ComputeWeaponBash(burden, burden.weaponBurden_lh, power);
                    if (Utils::GetRightHandInfo(actor).type != Utils::RightHandType::kHandToHand) {
                        baseCost += ComputeWeaponBash(burden, burden.weaponBurden_rh, power);
                    }
                }
				else if (Utils::GetRightHandInfo(actor).type == Utils::RightHandType::kTwoHanded)
                    baseCost = ComputeWeaponBash(burden, burden.weaponBurden_2h, power);
				else
                    baseCost += ComputeWeaponBash(burden, burden.weaponBurden_rh, power);
				break;
			}
		case Utils::AttackHandType::Unarmed:
			baseCost = ComputeUnarmedAttack(burden, power);
			break;
		case Utils::AttackHandType::TwoHanded:
			baseCost = Compute2hAttack(burden, burden.weaponBurden_2h, power);
			break;
		case Utils::AttackHandType::OneHanded:
			baseCost = left ?
				Compute1hAttack(burden, burden.weaponBurden_lh, power) :
				Compute1hAttack(burden, burden.weaponBurden_rh, power);
			break;
		case Utils::AttackHandType::Ranged:
		default:
			baseCost = 0.0f;
			break;
		}

		return baseCost;
	}

	float ComputeAttackCost(RE::Actor* actor, RE::BGSAttackData* attackData)
	{
		if (!actor || !attackData)
			return 0.0f;

		bool power = attackData->data.flags.any(RE::AttackData::AttackFlag::kPowerAttack);
		bool left = attackData->IsLeftAttack();
		bool bash = attackData->data.flags.any(RE::AttackData::AttackFlag::kBashAttack);

		float cost = ComputeBaseAttackCost(actor, bash, left, power);

        // Engine multiplier, basically 1, but for dual wield stamina balance
		cost *= attackData->data.staminaMult;

		auto handInfo = Utils::GetAttackHandInfo(actor, left, bash);
		RE::HandleEntryPoint(PEPE_STAMINA_ENTRY_POINT, actor, cost, PEPE::Group::AttackStamina, handInfo.form);

		Costs::CostLog("ComputeAttackCost: power={} left={} bash={} staminaMult={:.2f} -> {:.3f} for {:x}",
			power, left, bash, attackData->data.staminaMult, cost, actor->GetFormID());

		return cost;
	}

	float ComputeStaffFireCost(RE::Actor* actor, bool leftHand)
	{
		if (!actor)
			return 0.0f;

		auto& burden = Burden::Tracker::GetOrComputeBurden(actor);
		auto* params = AttackCostParams::GetSingleton();

		float Stamina_1pctMax = 0.01f * actor->GetActorValueMax(RE::ActorValue::kStamina);
		float weaponBurden = leftHand ? burden.weaponBurden_lh : burden.weaponBurden_rh;
		float StaffFireBurdenFlat = Math::Interpolate(
			params->StaffFireLowBurden.Get(),
			params->StaffFireHighBurden.Get(),
			weaponBurden,
			params->StaffFireBurdenCurve_k.Get());
		float StaffFireCarryPct = Math::Interpolate(
			params->StaffFireLowCarryPct.Get(),
			params->StaffFireHighCarryPct.Get(),
			burden.burdenBlend,
			params->StaffFireCarryCurve_k.Get());

		float TotalCost = StaffFireBurdenFlat + StaffFireCarryPct * Stamina_1pctMax;

		RE::HandleEntryPoint(PEPE_STAMINA_ENTRY_POINT, actor, TotalCost, PEPE::Group::StaffFireStamina,
			Utils::GetAttackHandInfo(actor, leftHand, false).form);

		Costs::CostLog("ComputeStaffFireCost: hand={} burden={:.3f} carry={:.3f} -> {:.3f} for {:x}",
			leftHand ? 'L' : 'R', weaponBurden, burden.burdenBlend, TotalCost, actor->GetFormID());

		return TotalCost;
	}

	float ComputeBowFireCost(RE::Actor* actor)
	{
		if (!actor)
			return 0.0f;

		auto& burden = Burden::Tracker::GetOrComputeBurden(actor);
		auto* params = AttackCostParams::GetSingleton();

		float Stamina_1pctMax = 0.01f * actor->GetActorValueMax(RE::ActorValue::kStamina);
		float BowFireBurdenFlat = Math::Interpolate(
			params->BowFireLowBurden.Get(),
			params->BowFireHighBurden.Get(),
			burden.weaponBurden_ranged,
			params->BowFireBurdenCurve_k.Get());
		float BowFireCarryPct = Math::Interpolate(
			params->BowFireLowCarryPct.Get(),
			params->BowFireHighCarryPct.Get(),
			burden.burdenBlend,
			params->BowFireCarryCurve_k.Get());

		float TotalCost = BowFireBurdenFlat + BowFireCarryPct * Stamina_1pctMax;

		RE::HandleEntryPoint(PEPE_STAMINA_ENTRY_POINT, actor, TotalCost, PEPE::Group::BowFireStamina,
			Utils::GetAttackHandInfo(actor, false, false).form);

		Costs::CostLog("ComputeBowFireCost: burden={:.3f} carry={:.3f} -> {:.3f} for {:x}",
			burden.burden, burden.burdenBlend, TotalCost, actor->GetFormID());

		return TotalCost;
	}
}
