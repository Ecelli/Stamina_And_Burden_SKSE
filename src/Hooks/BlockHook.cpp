#include "BlockHook.h"
#include "Combat/BlockManager.h"
#include "Common/Utils.h"

namespace Hooks
{
	void BlockHook::Install()
	{
		// AE: REL::ID(38627) + 0x4A8 — ProcessHit call inside hit processing
		// Confirmed by ShieldOfStamina and Valhalla Combat
		REL::Relocation<std::uintptr_t> target{ REL::ID(38627), 0x4A8 };
		_func = SKSE::GetTrampoline().write_call<5>(target.address(), ProcessHit);
		logger::info("  >BlockHook installed at REL::ID(38627) + 0x4A8");
	}

	void BlockHook::ProcessHit(RE::Actor* target, RE::HitData& hitData)
	{
		auto aggressor = hitData.aggressor.get();
		bool blocked = hitData.flags.any(RE::HitData::Flag::kBlocked);
		bool blockWithWeapon = hitData.flags.any(RE::HitData::Flag::kBlockWithWeapon);

		logger::info("[BlockHook] target={:x} aggressor={:x} blocked={} blockWithWeapon={}"
			" totalDmg={:.1f} physDmg={:.1f} pctBlocked={:.2f} stagger={:.2f}",
			target ? target->GetFormID() : 0,
			aggressor ? aggressor->GetFormID() : 0,
			blocked,
			blockWithWeapon,
			hitData.totalDamage,
			hitData.physicalDamage,
			hitData.percentBlocked,
			hitData.stagger);

		// Block flow (Phase 2 stubs — no behavior change yet)
		if (blocked && target) {
			float baseCost = Blocking::ComputeBlockStaminaCost(target);
			float redirectCost = Blocking::ComputeDamageRedirectStaminaCost(target, hitData);
			float totalCost = baseCost + redirectCost;

			if (totalCost > 0.0f) {
				Blocking::ApplyBlockDamageRedirect(hitData, redirectCost);
				Common::ApplyStaminaCost(target, totalCost);
			}
		}

		_func(target, hitData);
	}
}
