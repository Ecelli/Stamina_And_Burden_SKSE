#include "StaffCastingHook.h"
#include "Common/Utils.h"
#include "Settings/Params/CostsParams.h"
#include "Stamina/CostsManager.h"
#include "Stamina/RegenManager.h"

#include <RE/T/TESObjectWEAP.h>

namespace
{
	enum class CastStaffResult
	{
		kNotStaff,
		kLeftHand,
		kRightHand
	};

	CastStaffResult GetCastingStaffHand(RE::ActorMagicCaster* a_this)
	{
		auto* actor = a_this->GetCasterAsActor();
		if (!actor)
			return CastStaffResult::kNotStaff;

		auto source = a_this->GetCastingSource();
		if (source == RE::MagicSystem::CastingSource::kLeftHand) {
			auto* form = actor->GetEquippedObject(true);
			auto* weap = form ? form->As<RE::TESObjectWEAP>() : nullptr;
			if (weap && weap->GetWeaponType() == RE::WEAPON_TYPE::kStaff)
				return CastStaffResult::kLeftHand;
		}
		if (source == RE::MagicSystem::CastingSource::kRightHand) {
			auto* form = actor->GetEquippedObject(false);
			auto* weap = form ? form->As<RE::TESObjectWEAP>() : nullptr;
			if (weap && weap->GetWeaponType() == RE::WEAPON_TYPE::kStaff)
				return CastStaffResult::kRightHand;
		}
		return CastStaffResult::kNotStaff;
	}
}

namespace Hooks
{
	// VTABLE hook approach and cast-structure reference discovered via
	// exhausting-combat (Styyxus, GPL-3.0)
	// https://www.nexusmods.com/skyrimspecialedition/mods/181654
	// Scope narrowed to staves only; cost/deny logic is original.
	void StartCastingHook::Install()
	{
		// vtable hook on ActorMagicCaster::StartCasting (index 0x06)
		REL::Relocation<std::uintptr_t> vtbl{ RE::ActorMagicCaster::VTABLE[0] };
		_func = vtbl.write_vfunc(0x06, Thunk);
		logger::info("  >StartCastingHook installed (spell deny pipeline)");
	}

	void StartCastingHook::Thunk(RE::ActorMagicCaster* a_this)
	{
		auto result = GetCastingStaffHand(a_this);
		if (result == CastStaffResult::kNotStaff) {
			_func(a_this);
			return;
		}
		auto* actor = a_this->GetCasterAsActor();
		bool leftHand = (result == CastStaffResult::kLeftHand);

		bool isPlayer = actor->IsPlayerRef();
		auto* params = Costs::AttackCostParams::GetSingleton();
		bool costEnabled = isPlayer ? params->bStaffCostPlayer.Get() : params->bStaffCostNPC.Get();
		bool denyEnabled = isPlayer ? params->bStaffDenyPlayer.Get() : params->bStaffDenyNPC.Get();
		denyEnabled = costEnabled && denyEnabled;

		float cost = Costs::ComputeStaffFireCost(actor, leftHand);

		if (denyEnabled && !Common::CanDoStaminaAction(actor, cost)) {
			logger::info("StaffCastDeny: {:x} stamina={:.1f} cost={:.1f} — suppressed",
				actor->GetFormID(), actor->GetActorValue(RE::ActorValue::kStamina), cost);
			return;
		}

		if (costEnabled)
			Common::ApplyStaminaCost(actor, cost);

		_func(a_this);
	}

	// VTABLE hook approach discovered via exhausting-combat (Styyxus, GPL-3.0)
	// https://www.nexusmods.com/skyrimspecialedition/mods/181654
	// exhausting-combat's CasterUpdateHook used as reference for per-frame
	// channel drain; staff-only scope and penaly formula are original.
	void CasterUpdateHook::Install()
	{
		// vtable hook on ActorMagicCaster::Update (index 0x1D)
		REL::Relocation<std::uintptr_t> vtbl{ RE::ActorMagicCaster::VTABLE[0] };
		_func = vtbl.write_vfunc(0x1D, Thunk);
		logger::info("  >CasterUpdateHook installed (channel deny pipeline)");
	}

	void CasterUpdateHook::Thunk(RE::ActorMagicCaster* a_this, float a_deltaTime)
	{
		auto* actor = a_this->GetCasterAsActor();
		auto result = GetCastingStaffHand(a_this);
		if (result != CastStaffResult::kNotStaff && actor && a_this->state.any(RE::MagicCaster::State::kCasting)) {
			bool leftHand = (result == CastStaffResult::kLeftHand);

			bool isPlayer = actor->IsPlayerRef();
            auto* params = Costs::AttackCostParams::GetSingleton();
			bool costEnabled = isPlayer ? params->bStaffCostPlayer.Get() : params->bStaffCostNPC.Get();
			bool denyEnabled = isPlayer ? params->bStaffDenyPlayer.Get() : params->bStaffDenyNPC.Get();
			denyEnabled = costEnabled && denyEnabled;

			float drain = Regen::ComputeStaffHoldPenalty(actor, leftHand) * a_deltaTime;
			if (drain > 0.0f) {
				if (denyEnabled && !Common::CanDoStaminaAction(actor, drain)) {
					logger::info("StaffChannelDeny: {:x} stamina={:.1f} drain={:.3f} — interrupting",
						actor->GetFormID(), actor->GetActorValue(RE::ActorValue::kStamina), drain);
					a_this->InterruptCast(true);
					return;
				}
				if (costEnabled)
					Common::ApplyStaminaCost(actor, drain);
			}
		}
		_func(a_this, a_deltaTime);
	}
}
