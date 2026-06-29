#include "DamageScalingHook.h"
#include "Damage/DamageManager.h"

namespace Hooks
{
	void DamageScalingHook::Install()
	{
		// AE: REL::ID(38627) + 0x4A8 — ProcessHit call site
		// Confirmed by ShieldOfStamina (REL::Relocate(0x3C0, 0x4A8))
		REL::Relocation<std::uintptr_t> target{ REL::ID(38627), 0x4A8 };
		_func = SKSE::GetTrampoline().write_call<5>(target.address(), ProcessHit);
		logger::info("  >DamageScalingHook installed at REL::ID(38627) + 0x4A8");
	}

	void DamageScalingHook::ProcessHit(RE::Actor* target, RE::HitData& hitData)
	{
        auto attacker = hitData.aggressor.get();

		Damage::DamageLog(
			"DamageScalingHook: attacker={:x} target={:x} "
			"totalDmg={:.1f} physDmg={:.1f} "
			"blocked={} powerAtk={} sneakAtk={} melee={} "
			"weapon={:x} flags={:#x}",
			attacker ? attacker->GetFormID() : 0,
			target ? target->GetFormID() : 0,
			hitData.totalDamage,
			hitData.physicalDamage,
			hitData.flags.any(RE::HitData::Flag::kBlocked),
			hitData.flags.any(RE::HitData::Flag::kPowerAttack),
			hitData.flags.any(RE::HitData::Flag::kSneakAttack),
			hitData.flags.any(RE::HitData::Flag::kMeleeAttack),
			hitData.weapon ? hitData.weapon->GetFormID() : 0,
			static_cast<std::underlying_type_t<RE::HitData::Flag>>(hitData.flags.get()));

		_func(target, hitData);
	}
}
