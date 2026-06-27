#include "Hooks/hooks.h"
#include "Hooks/BurdenEventHandlers.h"
#include "Hooks/ActionHook.h"
#include "Hooks/RegenHooks.h"
#include "Hooks/SprintDrainHook.h"
#include "Hooks/AttackCostHook.h"
#include "Hooks/DenyHooks.h"

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
		// NpcAttackDenyHook::Install();  // NPC-only (49170), disabled — player denial TBD via vtable hook

		return true;
	}
}
