#include "HitHook.h"

namespace Hooks
{
	void HitHook::Install()
	{
		// AE: REL::ID(38627) + 0x4A8 — ProcessHit call inside hit processing
		// Confirmed by ShieldOfStamina and Valhalla Combat
		REL::Relocation<std::uintptr_t> target{ REL::ID(38627), 0x4A8 };
		_func = SKSE::GetTrampoline().write_call<5>(target.address(), ProcessHit);
		logger::info("  >HitHook installed at REL::ID(38627) + 0x4A8");
	}

	void HitHook::ProcessHit(RE::Actor* target, RE::HitData& hitData)
	{
		auto aggressor = hitData.aggressor.get();
		bool blocked = hitData.flags.any(RE::HitData::Flag::kBlocked);
		bool blockWithWeapon = hitData.flags.any(RE::HitData::Flag::kBlockWithWeapon);

		logger::info("[HitHook] target={:x} aggressor={:x} blocked={} blockWithWeapon={}"
			" totalDmg={:.1f} physDmg={:.1f} pctBlocked={:.2f} stagger={:.2f}",
			target ? target->GetFormID() : 0,
			aggressor ? aggressor->GetFormID() : 0,
			blocked,
			blockWithWeapon,
			hitData.totalDamage,
			hitData.physicalDamage,
			hitData.percentBlocked,
			hitData.stagger);

		_func(target, hitData);
	}
}
