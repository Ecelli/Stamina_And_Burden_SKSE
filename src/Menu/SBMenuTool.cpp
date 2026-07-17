#include "SBMenuTool.h"

#include "Settings/Params/BurdenParams.h"
#include "Settings/Params/CostsParams.h"
#include "Settings/Params/DamageParams.h"
#include "Settings/Params/DenyParams.h"
#include "Settings/Params/ExhaustionParams.h"
#include "Settings/Params/MovementSpeedParams.h"
#include "Settings/Params/ParameterOverrides.h"
#include "Settings/Params/RegenParams.h"
#include "Settings/Params/BlockingParams.h"
#include "Settings/Params/SBSettingsINI.h"

static SBMenuTool s_instance;

SBMenuTool& SBMenuTool::GetSingleton()
{
	return s_instance;
}

namespace
{
	void Header(const char* a_label)
	{
		FUCK::Separator();
		FUCK::TextColored(ImVec4(0.7f, 0.8f, 1.0f, 1.0f), a_label);
	}

	template <typename F>
	void DrawGroup(F&& a_forEachFn)
	{
		bool prevOpen = false;
		bool hasSections = false;
        // Call on the iterator given as parameter using an anonymous function
		a_forEachFn([&](auto&&... a_args) {
            // On the full parameter list we have headers (args == 1)
			if constexpr (sizeof...(a_args) == 1) {
				hasSections = true;
				if (prevOpen)
					FUCK::TreePop();
				prevOpen = FUCK::TreeNode(std::string(a_args...).c_str());
            // OR [parameter_name(key), parameter]
			} else if (!hasSections || prevOpen) {
				auto [key, param] = std::tie(a_args...);
                // We check the type of the parameter to check what to do with it
				using VType = std::decay_t<decltype(param.Get())>;
				if constexpr (std::is_same_v<VType, bool>)
					FUCK::Checkbox(key.data(), &param.currentValue);
				else if constexpr (std::is_same_v<VType, float>)
					FUCK::SliderFloat(key.data(), &param.currentValue, param.minValue, param.maxValue);
				else if constexpr (std::is_same_v<VType, int>)
					FUCK::SliderInt(key.data(), &param.currentValue, param.minValue, param.maxValue);
			}
		});
		if (prevOpen)
			FUCK::TreePop();
	}
}

void SBMenuTool::OnClose()
{
	SBSettingsINI::Save();
}

void SBMenuTool::Draw()
{
	FUCK::Text("StaminaAndBurden v{}", Plugin::VERSION);
	FUCK::Separator();

	if (!FUCK::BeginTabBar("SBGroups"))
		return;

	// ── Regen ──
	if (FUCK::BeginTabItem("Regen")) {
		Header("RegenParams");
		DrawGroup([](auto fn) { Regen::RegenParams::ForEach(fn); });

		Header("RegenMovementParams");
		DrawGroup([](auto fn) { Regen::RegenMovementParams::ForEach(fn); });

		Header("NegativeRegen");
		DrawGroup([](auto fn) { Regen::NegativeRegen::ForEach(fn); });

		Header("WeatherParams");
		DrawGroup([](auto fn) { Regen::WeatherParams::ForEach(fn); });

		FUCK::EndTabItem();
	}

	// ── Costs ──
	if (FUCK::BeginTabItem("Costs")) {
		Header("CostsParams");
		DrawGroup([](auto fn) { Costs::CostsParams::ForEach(fn); });

		Header("AttackCostParams");
		DrawGroup([](auto fn) { Costs::AttackCostParams::ForEach(fn); });

		FUCK::EndTabItem();
	}

	// ── Combat ──
	if (FUCK::BeginTabItem("Combat")) {
		Header("DamageParams");
		DrawGroup([](auto fn) { Damage::DamageParams::ForEach(fn); });

		Header("BlockingParams");
		DrawGroup([](auto fn) { Blocking::BlockingParams::ForEach(fn); });

		Header("DenyParams");
		DrawGroup([](auto fn) { Deny::DenyParams::ForEach(fn); });

		FUCK::EndTabItem();
	}

	// ── Burden ──
	if (FUCK::BeginTabItem("Burden")) {
		Header("BurdenParams");
		DrawGroup([](auto fn) { BurdenParams::ForEach(fn); });

		Header("ExhaustionParams");
		DrawGroup([](auto fn) { ExhaustionParams::ForEach(fn); });

		FUCK::EndTabItem();
	}

	// ── Movement ──
	if (FUCK::BeginTabItem("Movement")) {
		Header("MovementSpeedParams");
		DrawGroup([](auto fn) { MovementSpeedParams::ForEach(fn); });

		Header("JumpParams");
		DrawGroup([](auto fn) { JumpParams::ForEach(fn); });

		FUCK::EndTabItem();
	}

	// ── Overrides ──
	if (FUCK::BeginTabItem("Overrides")) {
		Header("ParameterOverrides");
		DrawGroup([](auto fn) { ParameterOverrides::ForEach(fn); });

		FUCK::EndTabItem();
	}

	FUCK::EndTabBar();
}
