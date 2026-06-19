#pragma once

#include "Burden/BurdenTracker.h"

// Each handler inherits RE::BSTEventSink<EventType>, which is a template class
// from CommonLibSSE. The EventType parameter determines:
//   - The event structure passed to ProcessEvent()
//   - The type-erased event source (always BSEventSource<EventType>*)
//    
// BSTEventSink<T> provides a single virtual ProcessEvent() that the
// ScriptEventSourceHolder dispatches to. Multiple inheritance from
// different BSTEventSink<T> specializations is allowed, but each must
// have its own ProcessEvent() override.
namespace Hooks
{
	/// Registers the player on every game load (new save or reload).
	/// Clears previous tracked-actor state via Tracker::OnGameLoad().
	class LoadGameHandler : public RE::BSTEventSink<RE::TESLoadGameEvent>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESLoadGameEvent*, RE::BSTEventSource<RE::TESLoadGameEvent>*) override;
	};

	/// Fires when any actor equips or unequips an item.
	/// Filters to tracked actors only, then recalculates burden.
	class EquipHandler : public RE::BSTEventSink<RE::TESEquipEvent>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>*) override;
	};

	/// Fires when items move between containers (pick up, drop, transfer).
	/// The event carries oldContainer and newContainer FormIDs. We check
	/// both for tracked actors and update burden if found.
	class ContainerHandler : public RE::BSTEventSink<RE::TESContainerChangedEvent>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESContainerChangedEvent* a_event, RE::BSTEventSource<RE::TESContainerChangedEvent>*) override;
	};

	/// Fires when an actor changes location (including worldspace transitions).
	/// Clears the transient NPC burden cache so stale 30-day-respawn data is
	/// re-computed on the next regen tick.
	class WorldspaceChangeHandler : public RE::BSTEventSink<RE::TESActorLocationChangeEvent>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(const RE::TESActorLocationChangeEvent* a_event, RE::BSTEventSource<RE::TESActorLocationChangeEvent>*) override;
	};
}
