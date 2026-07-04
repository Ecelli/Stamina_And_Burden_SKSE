#include "BlockHook.h"
#include "Combat/BlockManager.h"
#include "Common/Utils.h"
#include "Settings/Params/BlockingParams.h"

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
		auto aggressor = hitData.aggressor.get().get();
		bool blocked = hitData.flags.any(RE::HitData::Flag::kBlocked);

		Blocking::BlockLog("[BlockHook] target={:x} aggressor={:x} blocked={}"
			" totalDmg={:.1f} physDmg={:.1f} pctBlocked={:.2f}"sv,
			target ? target->GetFormID() : 0,
			aggressor ? aggressor->GetFormID() : 0,
			blocked,
			hitData.totalDamage,
			hitData.physicalDamage,
			hitData.percentBlocked);

		// Block flow
		if (blocked && target) {
			float baseCost = Blocking::ComputeBlockStaminaCost(target);
			float redirectCost = Blocking::ComputeDamageRedirectStaminaCost(target, hitData);
			float totalCost = baseCost + redirectCost;

			if (totalCost > 0.0f) {
				float currentStamina = target->GetActorValue(RE::ActorValue::kStamina);
				float redirectAmount = hitData.totalDamage;

				if (currentStamina >= totalCost) {
					Blocking::ApplyBlockDamageRedirect(hitData, redirectAmount);
					Common::ApplyStaminaCost(target, totalCost);
				} else {
					float staminaBudget = std::max(0.0f, currentStamina - baseCost);

					if (staminaBudget > 0.0f && redirectCost > 0.0f) {
						redirectAmount = redirectAmount * (staminaBudget / redirectCost);
						Blocking::ApplyBlockDamageRedirect(hitData, redirectAmount);
					}
					Common::ApplyStaminaCost(target, currentStamina);

					if (Blocking::BlockingParams::GetSingleton()->bGuardBreakEnabled.Get()) {
						float magnitude = Blocking::ComputeStaggerMagnitude(target, hitData);
						float direction = Blocking::ComputeStaggerDirection(target, aggressor);
						target->SetGraphVariableFloat("staggerDirection", direction);
						target->SetGraphVariableFloat("StaggerMagnitude", magnitude);
						target->NotifyAnimationGraph("staggerStart");
					}
				}
			}
		}

		_func(target, hitData);
	}
}
