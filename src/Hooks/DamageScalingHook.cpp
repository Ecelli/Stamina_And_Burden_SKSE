#include "DamageScalingHook.h"
#include "Damage/DamageManager.h"

namespace
{
	float GetDamageMultiplier(const RE::HitData& hitData)
	{
		auto attacker = hitData.aggressor.get();
		if (!attacker)
			return 1.0f;

		auto* params = Damage::DamageParams::GetSingleton();
		bool isPlayer = attacker->IsPlayerRef();

		if ((isPlayer && !params->bDamageScalingPlayer.Get()) ||
			(!isPlayer && !params->bDamageScalingNPC.Get()))
			return 1.0f;

		if (!hitData.weapon && hitData.attackDataSpell)
			return 1.0f;

		return Damage::ComputeStaminaDamageMult(attacker.get());
	}
}

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
		float damageMult = GetDamageMultiplier(hitData);

		if (damageMult != 1.0f) {
			hitData.totalDamage *= damageMult;
			hitData.physicalDamage *= damageMult;

			auto attacker = hitData.aggressor.get();
			Damage::DamageLog(
				"DamageScalingHook: {:x} scaled by {:.3f} "
				"-> totalDmg={:.2f} physDmg={:.2f}",
				attacker ? attacker->GetFormID() : 0, damageMult,
				hitData.totalDamage, hitData.physicalDamage);
		}

		_func(target, hitData);
	}
}
