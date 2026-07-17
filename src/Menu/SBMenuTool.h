#pragma once

#include "API/FUCK_API.h"

struct SBMenuTool : FUCK::ITool
{
	const char* Name() const override { return "StaminaAndBurden"; }

	void Draw() override;
	void OnOpen() override {}
	void OnClose() override;

	static SBMenuTool& GetSingleton();
};
