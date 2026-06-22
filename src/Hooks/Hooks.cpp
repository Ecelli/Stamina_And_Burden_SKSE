#include "Hooks/hooks.h"
#include "Hooks/BurdenEventHandlers.h"
#include "Hooks/JumpHook.h"
#include "Hooks/RegenHooks.h"
#include "Hooks/SprintDrainHook.h"

namespace Hooks {
	bool Install() {
		SECTION_SEPARATOR;
		logger::info("Installing hooks..."sv);
		SKSE::AllocTrampoline(64);

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
		SprintDrainHook::Install();
		JumpHook::Install();

		return true;
	}
}
