#include "StaffCastingHook.h"

namespace Hooks
{
	void StartCastingHook::Install()
	{
		// vtable hook on ActorMagicCaster::StartCasting (index 0x06)
		REL::Relocation<std::uintptr_t> vtbl{ RE::ActorMagicCaster::VTABLE[0] };
		_func = vtbl.write_vfunc(0x06, Thunk);
		logger::info("  >StartCastingHook installed (staff/spell deny proof-of-concept)");
	}

	void StartCastingHook::Thunk(RE::ActorMagicCaster* a_this)
	{
		auto* actor = a_this->GetCasterAsActor();
		if (actor && actor->GetActorValue(RE::ActorValue::kStamina) < 10.0f) {
			logger::info("CastDeny: {:x} stamina={:.1f} — suppressed", actor->GetFormID(),
				actor->GetActorValue(RE::ActorValue::kStamina));
			return;
		}
		_func(a_this);
	}

	void CasterUpdateHook::Install()
	{
		// vtable hook on ActorMagicCaster::Update (index 0x1D)
		REL::Relocation<std::uintptr_t> vtbl{ RE::ActorMagicCaster::VTABLE[0] };
		_func = vtbl.write_vfunc(0x1D, Thunk);
		logger::info("  >CasterUpdateHook installed (channel deny proof-of-concept)");
	}

	void CasterUpdateHook::Thunk(RE::ActorMagicCaster* a_this, float a_deltaTime)
	{
		auto* actor = a_this->GetCasterAsActor();
		if (actor && a_this->state.any(RE::MagicCaster::State::kCasting) &&
			actor->GetActorValue(RE::ActorValue::kStamina) < 10.0f)
		{
			logger::info("ChannelDeny: {:x} stamina={:.1f} — interrupting channel",
				actor->GetFormID(), actor->GetActorValue(RE::ActorValue::kStamina));
			a_this->InterruptCast(true);
			return;
		}
		_func(a_this, a_deltaTime);
	}
}
