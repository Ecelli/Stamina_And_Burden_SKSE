#include "BowFireHook.h"
#include "Stamina/CostsManager.h"
#include "Common/Utils.h"

namespace Hooks
{
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

		float cost = Costs::ComputeBowFireCost(actor);
		auto stamina = actor->GetActorValue(RE::ActorValue::kStamina);

		if (actor->IsPlayerRef() && !Common::CanDoStaminaAction(actor, cost)) {
			Costs::CostLog("BowFireHook: suppressed shot for {:x}, stamina={:.1f} < cost={:.1f}",
				actor->GetFormID(), stamina, cost);
			return;
		}

		Common::ApplyStaminaCost(actor, cost);
		_func(a_weapon, a_source, a_overwriteAmmo, a_ammoEnch, a_poison);
	}
}
