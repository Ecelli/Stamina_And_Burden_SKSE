#include "BowFireHook.h"
#include "Stamina/CostsManager.h"
#include "Common/Utils.h"
#include "Settings/Params/CostsParams.h"

namespace Hooks
{
	// Hook point discovered via exhausting-combat (Styyxus, GPL-3.0)
	// https://www.nexusmods.com/skyrimspecialedition/mods/181654
	// exhausting-combat's FireArrowHook used as reference for the cost+deny
	// pattern at this call site; cost computation is original.
	void BowFireHook::Install()
	{
		// AE: REL::ID(42859) + 0x138 inside FireArrow (or caller thereof)
		REL::Relocation<std::uintptr_t> target{ REL::ID(42859), 0x138 };
		if (!REL::make_pattern<"E8">().match(target.address())) {
			logger::error("  >BowFireHook: pattern mismatch at REL::ID(42859) + 0x138"sv);
			return;
		}
		_func = SKSE::GetTrampoline().write_call<5>(target.address(), Hook);
		logger::info("  >BowFireHook installed at REL::ID(42859) + 0x138");
	}

	void BowFireHook::Hook(RE::TESObjectWEAP* a_weapon, RE::TESObjectREFR* a_source,
		RE::TESAmmo* a_overwriteAmmo, RE::EnchantmentItem* a_ammoEnch,
		RE::AlchemyItem* a_poison)
	{
		auto* actor = a_source ? a_source->As<RE::Actor>() : nullptr;
		if (!actor)
			return _func(a_weapon, a_source, a_overwriteAmmo, a_ammoEnch, a_poison);

		bool isPlayer = actor->IsPlayerRef();
		auto* params = Costs::AttackCostParams::GetSingleton();
		bool costEnabled = isPlayer ? params->bBowCostPlayer.Get() : params->bBowCostNPC.Get();
		bool denyEnabled = isPlayer ? params->bBowDenyPlayer.Get() : params->bBowDenyNPC.Get();
		denyEnabled = costEnabled && denyEnabled;

		float cost = Costs::ComputeBowFireCost(actor);

		if (denyEnabled && !Common::CanDoStaminaAction(actor, cost)) {
			Costs::CostLog("BowFireHook: suppressed shot for {:x}", actor->GetFormID());
			return;
		}

		if (costEnabled)
			Common::ApplyStaminaCost(actor, cost);

		_func(a_weapon, a_source, a_overwriteAmmo, a_ammoEnch, a_poison);
	}
}
