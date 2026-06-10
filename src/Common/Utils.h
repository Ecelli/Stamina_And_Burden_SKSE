#pragma once

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
