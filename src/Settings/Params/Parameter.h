#pragma once

template <typename T>
struct Parameter
{
	T defaultValue;
	T minValue;
	T maxValue;
	T currentValue;

	constexpr Parameter(T a_def, T a_min, T a_max) noexcept :
		defaultValue(a_def),
		minValue(a_min),
		maxValue(a_max),
		currentValue(a_def)
	{}

	[[nodiscard]] constexpr T Get() const noexcept { return currentValue; }

	void Set(T new_val) noexcept
	{
		currentValue = std::clamp(new_val, minValue, maxValue);
		if (currentValue != new_val) {
			logger::warn("  >Parameter value {} out of range [{}, {}], clamping"sv, new_val, minValue, maxValue);
		}
	}

	void Reset() noexcept { currentValue = defaultValue; }

};
