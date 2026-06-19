#include "BurdenTracker.h"
#include "Common/Utils.h"
#include "Hooks/RegenHooks.h"

// Tier 1: tracked actors (event-driven, persistent for the session)
// Tier 2: transient NPC cache — grows freely within a worldspace,
// cleared on worldspace change or game load.
// Function-local static maps avoid static-initialization-order issues.
namespace
{
	using TrackedMap = std::unordered_map<RE::FormID, Burden::ActorBurdenData>;

	TrackedMap& GetTrackedMap()
	{
		static TrackedMap map;
		return map;
	}

	using TransientMap = std::unordered_map<RE::FormID, Burden::ActorBurdenData>;

	TransientMap& GetTransientMap()
	{
		static TransientMap map;
		return map;
	}
}

namespace Burden::Tracker
{
	void Register(RE::Actor* a_actor)
	{
		if (!a_actor) {
			return;
		}

		auto formId = a_actor->GetFormID();
		auto& map = GetTrackedMap();
		if (map.contains(formId)) {
			return;
		}

		map.emplace(formId, UpdateBurdenLog(a_actor));
	}

	void Unregister(RE::FormID a_formId)
	{
		GetTrackedMap().erase(a_formId);
	}

	void Update(RE::Actor* a_actor)
	{
		if (!a_actor) {
			return;
		}

		auto formId = a_actor->GetFormID();
		if (!IsTracked(formId)) {
			return;
		}

		SKSE::GetTaskInterface()->AddTask([formId]() {

			auto& map = GetTrackedMap();
			auto it = map.find(formId);
			if (it == map.end()) {
				return;
			}

			auto* actor = RE::TESForm::LookupByID<RE::Actor>(formId);
			if (!actor) {
				return;
			}

			it->second = UpdateBurden(actor);
			UpdateBurdenLog(actor);
		});
	}

	bool IsTracked(RE::FormID a_formId)
	{
		return GetTrackedMap().contains(a_formId);
	}

	const Burden::ActorBurdenData& GetOrComputeBurden(RE::Actor* a_actor)
	{
		auto formId = a_actor->GetFormID();

		// Tier 1: tracked map
		auto& tracked = GetTrackedMap();
		auto it = tracked.find(formId);
		if (it != tracked.end()) {
			return it->second;
		}

		// Tier 2: transient cache
		auto& transient = GetTransientMap();
		auto tIt = transient.find(formId);
		if (tIt != transient.end()) {
			return tIt->second;
		}

		// Compute and cache
		auto [newIt, _] = transient.emplace(formId, UpdateBurden(a_actor));
		return newIt->second;
	}

	void ClearTransientCache()
	{
        // We also call this back from other places like worldspace transition
		GetTransientMap().clear();
	}

	// On a new game or save load, all previously-tracked actors are stale.
	// Re-register the player — other actors can be re-registered by the
	// future Papyrus API as needed.
	void OnGameLoad()
	{
		GetTrackedMap().clear();
		ClearTransientCache();

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (player) {
			Register(player);
		}

		static bool heartbeatStarted = false; // Only ever executed once
		if (!heartbeatStarted) {
			heartbeatStarted = true;
			Common::make_heartbeat(std::chrono::milliseconds(200), TaskTrackBurdenParams);
			Common::make_heartbeat(std::chrono::milliseconds(200), Hooks::TaskPlayerFullStaminaMonitor);
		}
	}

	// Track Actor parameters that can change dynamically without hooks
	void TaskTrackBurdenParams()
	{
		SKSE::GetTaskInterface()->AddTask([]() {
			auto& map = GetTrackedMap();
			for (auto& [formId, data] : map) {
				auto* actor = RE::TESForm::LookupByID<RE::Actor>(formId);
				if (!actor) {
					continue;
				}

				if (static_cast<int>(actor->GetActorValue(RE::ActorValue::kLightArmor)) != data.lightSkill
					|| static_cast<int>(actor->GetActorValue(RE::ActorValue::kHeavyArmor)) != data.heavySkill
					|| std::abs(actor->GetActorValue(RE::ActorValue::kCarryWeight) - data.maxCarryWeight) > 0.001f) {

					Burden::Tracker::Update(actor);
				}
			}
		});
	}
}
