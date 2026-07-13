#include "Hooks/MovementHooks.h"
#include "Movement/MovementManager.h"
#include "Movement/MovementCostManager.h"
#include "Common/Utils.h"
#include "Settings/Params/CostsParams.h"
#include "Settings/Params/MovementSpeedParams.h"

namespace
{
	// ── Speed: REL::ID(37943) + 0x51 ──
	struct SpeedHook
	{
		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::ID(37943), 0x51 };

			if (!REL::make_pattern<"E8">().match(target.address())) {
				logger::error("  >SpeedHook: pattern mismatch at REL::ID(37943) + 0x51"sv);
				return;
			}

			_func = SKSE::GetTrampoline().write_call<5>(target.address(), Speed);
			logger::info("  >SpeedHook installed at REL::ID(37943) + 0x51");
		}

		static float Speed(RE::Actor* a_actor)
		{
			float original_speed = _func(a_actor);

			if (!a_actor)
				return original_speed;

			auto* params = MovementSpeedParams::GetSingleton();
			bool enabled = a_actor->IsPlayerRef() ? params->bMovementSpeedPlayer.Get() : params->bMovementSpeedNPC.Get();
			if (!enabled)
				return original_speed;

			return original_speed * Movement::ComputeSpeedMultiplier(a_actor);
		}

		static inline REL::Relocation<decltype(Speed)> _func;
	};

	// ── Sprint: REL::ID(38022) + 0xC1 / +0xC9 ──
	struct SprintHook
	{
		static void Install()
		{
			auto base = REL::ID(38022);
			auto& trampoline = SKSE::GetTrampoline();

			// 0xC1: replaces call to getEquippedWeight(actor)
			REL::Relocation<std::uintptr_t> call1{ base, 0xC1 };
			_getEquippedWeight = trampoline.write_call<5>(
				call1.address(),
				GetEquippedWeight);

			// 0xC9: replaces call to getSprintStaminaDrain(weight, delta)
			REL::Relocation<std::uintptr_t> call2{ base, 0xC9 };
			_getSprintStaminaDrain = trampoline.write_call<5>(
				call2.address(),
				GetSprintStaminaDrain);

			logger::info("  >SprintHook installed at REL::ID(38022) + 0xC1 / 0xC9");
		}

		static float GetEquippedWeight(RE::Actor* actor)
		{
			if (!actor)
				return 0.0f;

			auto* params = Costs::CostsParams::GetSingleton();
			bool enabled = actor->IsPlayerRef() ? params->bSprintCostPlayer.Get() : params->bSprintCostNPC.Get();
			if (!enabled)
				return _getEquippedWeight(actor);

			float drain = Movement::ComputeSprintDrain(actor);
			Costs::CostLog("SprintDrain: {:.3f} for {:x}", drain, actor->GetFormID());
			return drain;
		}

		static float GetSprintStaminaDrain(float weight, float)
		{
			return weight;
		}

		static inline REL::Relocation<decltype(GetEquippedWeight)> _getEquippedWeight;
		static inline REL::Relocation<decltype(GetSprintStaminaDrain)> _getSprintStaminaDrain;
	};

	// ── Jump: REL::ID(37257) + 0x17F ──
	struct JumpHook
	{
		static void Install()
		{
			REL::Relocation<std::uintptr_t> target{ REL::ID(37257), 0x17f };
			_GetScale = SKSE::GetTrampoline().write_call<5>(target.address(), ApplyJumpCost);
			logger::info("  >JumpHook installed at REL::ID(37257) + 0x17F");
		}

		static float ApplyJumpCost(RE::Actor* actor)
		{
			if (actor) {
				auto* params = Costs::CostsParams::GetSingleton();
				bool enabled = actor->IsPlayerRef() ? params->bJumpCostPlayer.Get() : params->bJumpCostNPC.Get();
				if (enabled)
					Common::ApplyStaminaCost(actor, Movement::ComputeJumpCost(actor));
			}

			float scale = _GetScale(actor);
			if (actor) {
				scale *= Movement::ComputeJumpHeightMult(actor);
			}
			return scale;
		}

		static inline REL::Relocation<decltype(ApplyJumpCost)> _GetScale;
	};
}

namespace Hooks::MovementHooks
{
	void InstallSpeed()  { SpeedHook::Install(); }
	void InstallSprint() { SprintHook::Install(); }
	void InstallJump()   { JumpHook::Install(); }
}
