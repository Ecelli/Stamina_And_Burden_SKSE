#include "Hooks/hooks.h"
#include "Hooks/BurdenEventHandlers.h"
#include "Hooks/RegenHooks.h"
#include "Hooks/MovementHooks.h"
#include "Hooks/AttackCostHook.h"
#include "Hooks/BowFireHook.h"
#include "Hooks/BlockHook.h"
#include "Hooks/DenyHooks.h"
#include "Hooks/DamageScalingHook.h"

namespace Hooks {
	bool Install() {
		SECTION_SEPARATOR;
		logger::info("Installing hooks..."sv);
		SKSE::AllocTrampoline(256);

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
		MovementHooks::InstallSprint();
		MovementHooks::InstallJump();
		AttackCostHook::Install();
		BowFireHook::Install();
		BlockHook::Install();
		DamageScalingHook::Install();
		MovementHooks::InstallSpeed();
		AttackDenyHook::Install();

		return true;
	}
}
