#include "SBMenuTool.h"

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
		FUCK::Text("(placeholder)");

		Header("RegenMovementParams");
		FUCK::Text("(placeholder)");

		Header("NegativeRegen");
		FUCK::Text("(placeholder)");

		Header("WeatherParams");
		FUCK::Text("(placeholder)");

		FUCK::EndTabItem();
	}

	// ── Costs ──
	if (FUCK::BeginTabItem("Costs")) {
		Header("CostsParams");
		FUCK::Text("(placeholder)");

		Header("AttackCostParams");
		FUCK::Text("(placeholder)");

		FUCK::EndTabItem();
	}

	// ── Combat ──
	if (FUCK::BeginTabItem("Combat")) {
		Header("DamageParams");
		FUCK::Text("(placeholder)");

		Header("BlockingParams");
		FUCK::Text("(placeholder)");

		Header("DenyParams");
		FUCK::Text("(placeholder)");

		FUCK::EndTabItem();
	}

	// ── Burden ──
	if (FUCK::BeginTabItem("Burden")) {
		Header("BurdenParams");
		FUCK::Text("(placeholder)");

		Header("ExhaustionParams");
		FUCK::Text("(placeholder)");

		FUCK::EndTabItem();
	}

	// ── Movement ──
	if (FUCK::BeginTabItem("Movement")) {
		Header("MovementSpeedParams");
		FUCK::Text("(placeholder)");

		Header("JumpParams");
		FUCK::Text("(placeholder)");

		FUCK::EndTabItem();
	}

	// ── Overrides ──
	if (FUCK::BeginTabItem("Overrides")) {
		Header("ParameterOverrides");
		FUCK::Text("(placeholder)");

		FUCK::EndTabItem();
	}

	FUCK::EndTabBar();
}
