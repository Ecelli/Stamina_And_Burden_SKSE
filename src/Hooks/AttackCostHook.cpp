#include "AttackCostHook.h"
#include "Stamina/CostsManager.h"
#include "Settings/Params/CostsParams.h"

#include <RE/RTTI.h>

namespace Hooks
{
	void AttackCostHook::Install()
	{
		// AE: REL::ID(38603) + 0x171 inside ActorValueOwner::GetAttackStaminaCost
		REL::Relocation<std::uintptr_t> target{ REL::ID(38603), 0x171 };
		_func = SKSE::GetTrampoline().write_call<5>(target.address(), Call);
		logger::info("  >AttackCostHook installed at REL::ID(38603) + 0x171");
	}

	float AttackCostHook::Call(RE::ActorValueOwner* a_this, RE::BGSAttackData* a_attack)
	{
		if (!a_this || !a_attack)
			return _func(a_this, a_attack);

		auto* actor = skyrim_cast<RE::Actor*>(a_this);
		if (!actor)
			return _func(a_this, a_attack);

		auto* params = Costs::CostsParams::GetSingleton();
		bool enabled = actor->IsPlayerRef() ? params->bAttackCostPlayer.Get() : params->bAttackCostNPC.Get();
		if (!enabled)
			return _func(a_this, a_attack);

		// NOTE: cost already drained in AttackDenyHook — return 0 to prevent engine double-drain
		// return Costs::ComputeAttackCost(actor, a_attack); // If we need to fix it
		return 0.0f;
	}
}
