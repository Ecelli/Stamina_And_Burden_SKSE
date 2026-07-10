#pragma once

#include <RE/B/BGSPerkEntry.h>
#include "API/PerkEntryPointExtenderAPI.h"

namespace PEPE::Group
{
	inline constexpr auto AttackStamina        = "SB_AttackStamina";
	inline constexpr auto BowFireStamina       = "SB_BowFireStamina";
	inline constexpr auto SprintStamina        = "SB_SprintStamina";
	inline constexpr auto JumpStamina          = "SB_JumpStamina";
	inline constexpr auto BlockStamina         = "SB_BlockStamina";
	inline constexpr auto BowDrawHoldStamina   = "SB_BowDrawHoldStamina";
	inline constexpr auto BlockHoldStamina     = "SB_BlockHoldStamina";
}

inline constexpr auto PEPE_STAMINA_ENTRY_POINT = RE::PerkEntryPoint::kModPowerAttackStamina;
