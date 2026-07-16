#include "AttackCostHook.h"
#include "Stamina/CostsManager.h"
#include "Settings/Params/CostsParams.h"
#include "Common/Utils.h"

#include <RE/RTTI.h>

namespace Hooks
{
	// Hook point discovered via exhausting-combat (Styyxus, GPL-3.0)
	// https://www.nexusmods.com/skyrimspecialedition/mods/181654
	// exhausting-combat's ManageAttackStamina used as reference for hook return
	// convention (return 0.0f to zero engine cost); cost formula is original.
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

		auto* costParams = Costs::AttackCostParams::GetSingleton();
		bool costEnabled = actor->IsPlayerRef() ? costParams->bAttackCostPlayer.Get() : costParams->bAttackCostNPC.Get();
		if (!costEnabled)
			return _func(a_this, a_attack);

		float cost = Costs::ComputeAttackCost(actor, a_attack);
		Common::ApplyStaminaCost(actor, cost);
		return 0.0f;
	}
}
