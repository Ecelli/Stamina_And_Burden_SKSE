#pragma once

#include <chrono>
#include <functional>
#include <thread>
#include "Settings/Params/RegenParams.h"
#include "Settings/Params/CostsParams.h"
#include "Settings/Params/DamageParams.h"
#include "Settings/Params/BlockingParams.h"
#include "Settings/Params/ExhaustionParams.h"

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

namespace Math
{
	[[nodiscard]] constexpr inline float Clamp01(float x) noexcept
	{
		return std::clamp(x, 0.0f, 1.0f);
	}

	[[nodiscard]] constexpr inline float SmoothStep(float t) noexcept
	{
		t = Clamp01(t);
		return t * t * (3.0f - 2.0f * t);
	}

	[[nodiscard]] constexpr inline float Interpolate(float min, float max, float t, float k) noexcept
	{
		t = Clamp01(t);
		k = Clamp01(k);
		auto s = SmoothStep(t);
		return min + (max - min) * (k * s + (1.0f - k) * t);
	}
}
