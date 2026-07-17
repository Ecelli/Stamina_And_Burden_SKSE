#include "SBMenuTool.h"

static SBMenuTool s_instance;

SBMenuTool& SBMenuTool::GetSingleton()
{
	return s_instance;
}

void SBMenuTool::Draw()
{
	FUCK::Text("StaminaAndBurden v{}", Plugin::VERSION);
	FUCK::Separator();
	FUCK::Text("Settings menu coming soon");
}
