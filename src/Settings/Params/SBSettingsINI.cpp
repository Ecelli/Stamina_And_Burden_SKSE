#include "SBSettingsINI.h"

#include <SimpleIni.h>
#undef max
#undef min

#include <filesystem>

#include "BurdenParams.h"
#include "CostsParams.h"
#include "DamageParams.h"
#include "DenyParams.h"
#include "ExhaustionParams.h"
#include "MovementSpeedParams.h"
#include "ParameterOverrides.h"
#include "RegenParams.h"
#include "BlockingParams.h"

namespace
{
	std::string ConfigFilePath()
	{
		return fmt::format(R"(.\Data\SKSE\Plugins\{}_Settings.ini)", Plugin::NAME);
	}

	template <typename F>
	void ForEachAll(F&& a_handler)
	{
		std::string section;

		auto run = [&](const char* a_prefix, auto&& a_forEachFn) {
			a_forEachFn([&](auto&&... a_args) {
				if constexpr (sizeof...(a_args) == 1) {
					section = a_prefix;
					section += ".";
					section += std::string_view(a_args...);
				} else {
					a_handler(section, a_args...);
				}
			});
		};

		auto withBurden = [](auto fn) { BurdenParams::ForEach(fn); };
		auto withRegen = [](auto fn) { Regen::RegenParams::ForEach(fn); };
		auto withRegenMovement = [](auto fn) { Regen::RegenMovementParams::ForEach(fn); };
		auto withNegativeRegen = [](auto fn) { Regen::NegativeRegen::ForEach(fn); };
		auto withWeather = [](auto fn) { Regen::WeatherParams::ForEach(fn); };
		auto withCosts = [](auto fn) { Costs::CostsParams::ForEach(fn); };
		auto withAttackCost = [](auto fn) { Costs::AttackCostParams::ForEach(fn); };
		auto withDamage = [](auto fn) { Damage::DamageParams::ForEach(fn); };
		auto withBlocking = [](auto fn) { Blocking::BlockingParams::ForEach(fn); };
		auto withDeny = [](auto fn) { Deny::DenyParams::ForEach(fn); };
		auto withExhaustion = [](auto fn) { ExhaustionParams::ForEach(fn); };
		auto withMovementSpeed = [](auto fn) { MovementSpeedParams::ForEach(fn); };
		auto withJump = [](auto fn) { JumpParams::ForEach(fn); };
		auto withOverrides = [](auto fn) { ParameterOverrides::ForEach(fn); };

		run("BurdenParams", withBurden);
		run("RegenParams", withRegen);
		run("RegenMovementParams", withRegenMovement);
		run("NegativeRegen", withNegativeRegen);
		run("WeatherParams", withWeather);
		run("CostsParams", withCosts);
		run("AttackCostParams", withAttackCost);
		run("DamageParams", withDamage);
		run("BlockingParams", withBlocking);
		run("DenyParams", withDeny);
		run("ExhaustionParams", withExhaustion);
		run("MovementSpeedParams", withMovementSpeed);
		run("JumpParams", withJump);
		run("ParameterOverrides", withOverrides);
	}
}

void SBSettingsINI::Load()
{
	auto path = ConfigFilePath();
	if (!std::filesystem::exists(path)) {
		logger::info("Settings file not found — using defaults");
		return;
	}

	logger::info("Loading user settings from {}", path);

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(path.c_str());

	ForEachAll([&](const std::string& a_section, std::string_view a_key, auto& a_param) {
		using VType = std::decay_t<decltype(a_param.Get())>;
		if constexpr (std::is_same_v<VType, bool>)
			a_param.Set(ini.GetBoolValue(a_section.c_str(), a_key.data(), a_param.defaultValue));
		else if constexpr (std::is_same_v<VType, float>)
			a_param.Set(static_cast<float>(ini.GetDoubleValue(a_section.c_str(), a_key.data(), a_param.defaultValue)));
		else if constexpr (std::is_same_v<VType, int>)
			a_param.Set(static_cast<int>(ini.GetLongValue(a_section.c_str(), a_key.data(), a_param.defaultValue)));
	});

	logger::info("Finished loading user settings");
}

void SBSettingsINI::Save()
{
	auto path = ConfigFilePath();

	logger::info("Saving user settings to {}", path);

	CSimpleIniA ini;
	ini.SetUnicode();

    ForEachAll([&](const std::string& a_section, std::string_view a_key, auto& a_param) {
		using VType = std::decay_t<decltype(a_param.Get())>;
		if constexpr (std::is_same_v<VType, bool>)
			ini.SetBoolValue(a_section.c_str(), a_key.data(), a_param.Get());
		else if constexpr (std::is_same_v<VType, float>) {
			char buf[64];
			snprintf(buf, sizeof(buf), "%.3f", a_param.Get());
			ini.SetValue(a_section.c_str(), a_key.data(), buf);
		} else if constexpr (std::is_same_v<VType, int>)
			ini.SetLongValue(a_section.c_str(), a_key.data(), a_param.Get());
	});

	ini.SaveFile(path.c_str());

	logger::info("Finished saving user settings");
}

void SBSettingsINI::Initialize()
{
	Load();
	if (!std::filesystem::exists(ConfigFilePath()))
		Save();
}
