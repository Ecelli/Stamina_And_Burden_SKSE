#include "Hooks/hooks.h"
#include "Hooks/BurdenEventHandlers.h"
#include "Hooks/ActionHook.h"
#include "Hooks/RegenHooks.h"
#include "Hooks/SprintDrainHook.h"
#include "Hooks/AttackCostHook.h"
#include "Hooks/BowFireHook.h"
#include "Hooks/HitHook.h"
#include "Hooks/DenyHooks.h"
#include "Hooks/DamageScalingHook.h"

namespace Hooks {
	bool Install() {
		SECTION_SEPARATOR;
		logger::info("Installing hooks..."sv);
		SKSE::AllocTrampoline(128);

		auto* holder = RE::ScriptEventSourceHolder::GetSingleton();
		if (!holder) {
			logger::critical("  >Failed to get ScriptEventSourceHolder"sv);
			return false;
		}

		holder->AddEventSink(new LoadGameHandler());
		holder->AddEventSink(new EquipHandler());
		holder->AddEventSink(new ContainerHandler());
		holder->AddEventSink(new WorldspaceChangeHandler());

		logger::info("  >Registered burden event handlers"sv);

		RegenHook::Install();
		RegenDelayHook::Install();
		SprintDrainHook::Install();
		ActionHook::Install();
		AttackCostHook::Install();
		BowFireHook::Install();
		DamageScalingHook::Install();
		HitHook::Install();
		// AttackDenyHook::Install();  // Seems to be NPC-only (49170), Deferred until a solution is found

		return true;
	}
}
