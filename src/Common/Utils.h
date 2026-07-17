#pragma once

#include <chrono>
#include <functional>
#include <thread>
#include <cmath>
#include "Settings/Params/RegenParams.h"
#include "Settings/Params/CostsParams.h"
#include "Settings/Params/DamageParams.h"
#include "Settings/Params/BlockingParams.h"
#include "Settings/Params/ExhaustionParams.h"
#include "Settings/Params/MovementParams.h"
#include "Settings/Params/DenyParams.h"

namespace Utils
{
	enum class LeftHandType
	{
		kEmpty,
		kWeapon,
		kShield,
		kStaff
	};

	enum class RightHandType
	{
		kEmpty,
		kOneHanded,
		kTwoHanded,
		kBow,
		kHandToHand,
		kStaff
	};

	struct LeftHandInfo
	{
		LeftHandType type{ LeftHandType::kEmpty };
		RE::TESObjectWEAP* weapon{ nullptr };
		RE::TESObjectARMO* shield{ nullptr };

		[[nodiscard]] bool HasWeapon() const { return weapon != nullptr; }
		[[nodiscard]] bool HasShield() const { return shield != nullptr; }
	};

	struct RightHandInfo
	{
		RightHandType type{ RightHandType::kEmpty };
		RE::TESObjectWEAP* weapon{ nullptr };

		[[nodiscard]] bool HasWeapon() const { return weapon != nullptr; }
	};

	enum class AttackHandType
	{
		Unarmed,
		OneHanded,
		TwoHanded,
		Ranged,
		BashShield,
		BashBow,
		BashWeapon
	};

	struct AttackHandInfo
	{
		AttackHandType type{ AttackHandType::Unarmed };
		RE::TESForm*   form{ nullptr };
	};

	LeftHandInfo   GetLeftHandInfo(RE::Actor* actor);
	RightHandInfo  GetRightHandInfo(RE::Actor* actor);
	AttackHandInfo GetAttackHandInfo(RE::Actor* actor, bool left, bool blocking);
}

namespace Common
{
	template <typename Rep, typename Period, typename F>
	void make_heartbeat(std::chrono::duration<Rep, Period> interval, F&& func)
	{
		std::thread([interval, func = std::forward<F>(func)]() mutable {
			while (true) {
				std::this_thread::sleep_for(interval);
				func();
			}
		}).detach();
	}

	bool CanDoStaminaAction(RE::Actor* actor, float cost);
	void ApplyStaminaCost(RE::Actor* actor, float cost);
}

namespace Regen
{
    template <typename... Args>
    void RegenLog(std::string_view a_fmt, Args&&... a_args)
    {
        if (RegenParams::GetSingleton()->EnableDebugLogging.Get()) {
            logger::info(fmt::runtime(a_fmt), std::forward<Args>(a_args)...);
        }
    }
}

namespace Costs
{
    template <typename... Args>
    void CostLog(std::string_view a_fmt, Args&&... a_args)
    {
        if (CostsParams::GetSingleton()->EnableDebugLogging.Get()) {
            logger::info(fmt::runtime(a_fmt), std::forward<Args>(a_args)...);
        }
    }
}


namespace Damage
{
    template <typename... Args>
    void DamageLog(std::string_view a_fmt, Args&&... a_args)
    {
        if (DamageParams::GetSingleton()->EnableDebugLogging.Get()) {
            logger::info(fmt::runtime(a_fmt), std::forward<Args>(a_args)...);
        }
    }
}

namespace Blocking
{
    template <typename... Args>
    void BlockLog(std::string_view a_fmt, Args&&... a_args)
    {
        if (BlockingParams::GetSingleton()->EnableDebugLogging.Get()) {
            logger::info(fmt::runtime(a_fmt), std::forward<Args>(a_args)...);
        }
    }
}

namespace Exhaustion
{
    template <typename... Args>
    void ExhaustionLog(std::string_view a_fmt, Args&&... a_args)
    {
        if (ExhaustionParams::GetSingleton()->bEnableDebugLogging.Get()) {
            logger::info(fmt::runtime(a_fmt), std::forward<Args>(a_args)...);
        }
    }
}

namespace Movement
{
    template <typename... Args>
    void MovementLog(std::string_view a_fmt, Args&&... a_args)
    {
        if (MovementSpeedParams::GetSingleton()->EnableDebugLogging.Get()) {
            logger::info(fmt::runtime(a_fmt), std::forward<Args>(a_args)...);
        }
    }
}

namespace Deny
{
    template <typename... Args>
    void DenyLog(std::string_view a_fmt, Args&&... a_args)
    {
        if (DenyParams::GetSingleton()->EnableDebugLogging.Get()) {
            logger::info(fmt::runtime(a_fmt), std::forward<Args>(a_args)...);
        }
    }
}

namespace Jump
{
	template <typename... Args>
	void JumpLog(std::string_view a_fmt, Args&&... a_args)
	{
		if (JumpParams::GetSingleton()->EnableDebugJumpLogging.Get()) {
			logger::info(fmt::runtime(a_fmt), std::forward<Args>(a_args)...);
		}
	}
}

namespace Math
{
	[[nodiscard]] constexpr inline float Clamp01(float x) noexcept
	{
		return std::clamp(x, 0.0f, 1.0f);
	}

	[[nodiscard]] inline float SmoothStep(float t) noexcept
	{
		static constexpr float kSigma = 0.08f;
		static constexpr int kSteps = 256;
		// Fermi-Dirac sigmoid LUT (σ=0.05, 256 entries)
		struct Table
		{
			float data[kSteps];

			Table()
			{
				for (int i = 1; i < kSteps -1; ++i) {
					float x = static_cast<float>(i) / (kSteps - 1);
					float arg = (x - 0.5f) / kSigma;
					data[i] = 1.0f / (1.0f + std::exp(-arg));
				}
				data[0] = 0.0f;
				data[kSteps - 1] = 1.0f;
			}

			[[nodiscard]] float Eval(float x) const noexcept
			{
				int i = static_cast<int>(Math::Clamp01(x) * (kSteps - 1) + 0.5f);
				if (i >= kSteps) i = kSteps - 1;
				return data[i];
			}
		};

		static const Table kTable;
		return kTable.Eval(t);
	}

	[[nodiscard]] inline float Interpolate(float min, float max, float t, float k) noexcept
	{
		t = Clamp01(t);
		k = Clamp01(k);
		auto s = SmoothStep(t);
		return min + (max - min) * (k * s + (1.0f - k) * t);
	}
}
