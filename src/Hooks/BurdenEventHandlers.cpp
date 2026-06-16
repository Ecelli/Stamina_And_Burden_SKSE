#include "BurdenEventHandlers.h"

// ProcessEvent is called synchronously by the game's event dispatch.
// Return kContinue to allow other sinks to process the event.
namespace
{
	void CheckWorldspaceChange()
	{
		static RE::FormID lastWorldspace = 0;
		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player)
			return;

		auto* worldspace = player->GetWorldspace();
		RE::FormID current = worldspace ? worldspace->GetFormID() : 0;
		if (current != lastWorldspace) {
			lastWorldspace = current;
			Burden::Tracker::ClearTransientCache();
			logger::info("  >Cleared transient burden cache (worldspace change)");
		}
	}
}

namespace Hooks
{
	RE::BSEventNotifyControl WorldspaceChangeHandler::ProcessEvent(const RE::TESActorLocationChangeEvent*, RE::BSTEventSource<RE::TESActorLocationChangeEvent>*)
	{
		SKSE::GetTaskInterface()->AddTask(CheckWorldspaceChange);
		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl LoadGameHandler::ProcessEvent(const RE::TESLoadGameEvent*, RE::BSTEventSource<RE::TESLoadGameEvent>*)
	{
		Burden::Tracker::OnGameLoad();
		return RE::BSEventNotifyControl::kContinue;
	}

	RE::BSEventNotifyControl EquipHandler::ProcessEvent(const RE::TESEquipEvent* a_event, RE::BSTEventSource<RE::TESEquipEvent>*)
	{
		if (!a_event || !a_event->actor) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* actor = a_event->actor->As<RE::Actor>();
		if (actor) {
			Burden::Tracker::Update(actor);
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	// The event carries the FormIDs of both source and destination
	// containers. We check which side is an actor — that's the one
	// whose inventory actually changed (pickup → newContainer is actor,
	// drop → oldContainer is actor, transfer → could be either).
	RE::BSEventNotifyControl ContainerHandler::ProcessEvent(const RE::TESContainerChangedEvent* a_event, RE::BSTEventSource<RE::TESContainerChangedEvent>*)
	{
		if (!a_event) {
			return RE::BSEventNotifyControl::kContinue;
		}

		auto formId = a_event->newContainer;
		if (!RE::TESForm::LookupByID<RE::Actor>(formId)) {
			formId = a_event->oldContainer;
			if (!RE::TESForm::LookupByID<RE::Actor>(formId)) {
				return RE::BSEventNotifyControl::kContinue;
			}
		}

		auto* actor = RE::TESForm::LookupByID<RE::Actor>(formId);
		if (actor) {
			Burden::Tracker::Update(actor);
		}

		return RE::BSEventNotifyControl::kContinue;
	}
}
