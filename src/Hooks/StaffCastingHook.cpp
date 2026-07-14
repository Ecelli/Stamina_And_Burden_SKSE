#include "StaffCastingHook.h"
#include "Common/Utils.h"
#include "Stamina/CostsManager.h"
#include "Stamina/RegenManager.h"

namespace Hooks
{
	void StartCastingHook::Install()
	{
		// vtable hook on ActorMagicCaster::StartCasting (index 0x06)
		REL::Relocation<std::uintptr_t> vtbl{ RE::ActorMagicCaster::VTABLE[0] };
		_func = vtbl.write_vfunc(0x06, Thunk);
		logger::info("  >StartCastingHook installed (spell deny pipeline)");
	}

	void StartCastingHook::Thunk(RE::ActorMagicCaster* a_this)
	{
		auto* actor = a_this->GetCasterAsActor();
		if (!actor) {
			_func(a_this);
			return;
		}
		float cost = Costs::ComputeStaffFireCost(actor);
		if (!Common::CanDoStaminaAction(actor, cost)) {
			logger::info("CastDeny: {:x} stamina={:.1f} cost={:.1f} — suppressed",
				actor->GetFormID(), actor->GetActorValue(RE::ActorValue::kStamina), cost);
			return;
		}
		Common::ApplyStaminaCost(actor, cost);
		_func(a_this);
	}

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
		if (actor && a_this->state.any(RE::MagicCaster::State::kCasting)) {
			float drain = Regen::ComputeStaffHoldPenalty(actor) * a_deltaTime;
			if (drain > 0.0f) {
				if (!Common::CanDoStaminaAction(actor, drain)) {
					logger::info("ChannelDeny: {:x} stamina={:.1f} drain={:.3f} — interrupting",
						actor->GetFormID(), actor->GetActorValue(RE::ActorValue::kStamina), drain);
					a_this->InterruptCast(true);
					return;
				}
				Common::ApplyStaminaCost(actor, drain);
			}
		}
		_func(a_this, a_deltaTime);
	}
}
