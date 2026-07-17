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

	void DrawSectionHeader(const char* a_label, auto&& a_forEachFn)
	{
		Header(a_label);
        FUCK::PushID(a_label);
		FUCK::SameLine(600.0f);
		if (FUCK::Button("Defaults")) {
			a_forEachFn([](auto&&... a_args) {
				if constexpr (sizeof...(a_args) > 1) {
					auto [key, param] = std::tie(a_args...);
					param.Reset();
				} else {
                    // Just to avoid compilation errors
                    [](auto&&...) {}(a_args...);
				}
			});
		}
        FUCK::PopID();
	}
}

void SBMenuTool::OnClose()
{
	SBSettingsINI::Save();
}

void SBMenuTool::Draw()
{
	FUCK::Text("StaminaAndBurden");
	FUCK::Separator();

	if (!FUCK::BeginTabBar("Stamina an Burden Groups"))
		return;

	// ── Regen ──
	if (FUCK::BeginTabItem("Regen")) {
		DrawSectionHeader("RegenParams", [](auto fn) { Regen::RegenParams::ForEach(fn); });
		DrawGroup([](auto fn) { Regen::RegenParams::ForEach(fn); });

		DrawSectionHeader("RegenMovementParams", [](auto fn) { Regen::RegenMovementParams::ForEach(fn); });
		DrawGroup([](auto fn) { Regen::RegenMovementParams::ForEach(fn); });

		DrawSectionHeader("NegativeRegen", [](auto fn) { Regen::NegativeRegen::ForEach(fn); });
		DrawGroup([](auto fn) { Regen::NegativeRegen::ForEach(fn); });

		DrawSectionHeader("WeatherParams", [](auto fn) { Regen::WeatherParams::ForEach(fn); });
		DrawGroup([](auto fn) { Regen::WeatherParams::ForEach(fn); });

		FUCK::EndTabItem();
	}

	// ── Costs ──
	if (FUCK::BeginTabItem("Costs")) {
		DrawSectionHeader("CostsParams", [](auto fn) { Costs::CostsParams::ForEach(fn); });
		DrawGroup([](auto fn) { Costs::CostsParams::ForEach(fn); });

		DrawSectionHeader("AttackCostParams", [](auto fn) { Costs::AttackCostParams::ForEach(fn); });
		DrawGroup([](auto fn) { Costs::AttackCostParams::ForEach(fn); });

		FUCK::EndTabItem();
	}

	// ── Combat ──
	if (FUCK::BeginTabItem("Combat")) {
		DrawSectionHeader("DamageParams", [](auto fn) { Damage::DamageParams::ForEach(fn); });
		DrawGroup([](auto fn) { Damage::DamageParams::ForEach(fn); });

		DrawSectionHeader("BlockingParams", [](auto fn) { Blocking::BlockingParams::ForEach(fn); });
		DrawGroup([](auto fn) { Blocking::BlockingParams::ForEach(fn); });

		DrawSectionHeader("DenyParams", [](auto fn) { Deny::DenyParams::ForEach(fn); });
		DrawGroup([](auto fn) { Deny::DenyParams::ForEach(fn); });

		FUCK::EndTabItem();
	}

	// ── Burden ──
	if (FUCK::BeginTabItem("Burden")) {
		DrawSectionHeader("BurdenParams", [](auto fn) { BurdenParams::ForEach(fn); });
		DrawGroup([](auto fn) { BurdenParams::ForEach(fn); });

		DrawSectionHeader("ExhaustionParams", [](auto fn) { ExhaustionParams::ForEach(fn); });
		DrawGroup([](auto fn) { ExhaustionParams::ForEach(fn); });

		FUCK::EndTabItem();
	}

	// ── Movement ──
	if (FUCK::BeginTabItem("Movement")) {
		DrawSectionHeader("MovementSpeedParams", [](auto fn) { MovementSpeedParams::ForEach(fn); });
		DrawGroup([](auto fn) { MovementSpeedParams::ForEach(fn); });

		DrawSectionHeader("JumpParams", [](auto fn) { JumpParams::ForEach(fn); });
		DrawGroup([](auto fn) { JumpParams::ForEach(fn); });

		FUCK::EndTabItem();
	}

	// ── Overrides ──
	if (FUCK::BeginTabItem("Overrides")) {
		DrawSectionHeader("ParameterOverrides", [](auto fn) { ParameterOverrides::ForEach(fn); });
		DrawGroup([](auto fn) { ParameterOverrides::ForEach(fn); });

		FUCK::EndTabItem();
	}

	FUCK::EndTabBar();
}
