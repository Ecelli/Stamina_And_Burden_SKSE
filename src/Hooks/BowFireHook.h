#pragma once

namespace Hooks
{
	// Intercepts FireArrow call to apply stamina cost and suppress if insufficient.
	// AE: REL::ID(42859) + 0x138
	struct BowFireHook
	{
		static void Install();
		static void Hook(RE::TESObjectWEAP* a_weapon, RE::TESObjectREFR* a_source,
			RE::TESAmmo* a_overwriteAmmo, RE::EnchantmentItem* a_ammoEnch,
			RE::AlchemyItem* a_poison);
		static inline REL::Relocation<decltype(Hook)> _func;
	};
}
