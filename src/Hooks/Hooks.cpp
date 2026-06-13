#include "Hooks/hooks.h"
#include "Hooks/BurdenEventHandlers.h"

namespace Hooks {
	bool Install() {
		SECTION_SEPARATOR;
		logger::info("Installing hooks..."sv);

		auto* holder = RE::ScriptEventSourceHolder::GetSingleton();
		if (!holder) {
			logger::critical("  >Failed to get ScriptEventSourceHolder"sv);
			return false;
		}

		holder->AddEventSink(new LoadGameHandler());
		holder->AddEventSink(new EquipHandler());
		holder->AddEventSink(new ContainerHandler());

		logger::info("  >Registered burden event handlers"sv);
		return true;
	}
}
