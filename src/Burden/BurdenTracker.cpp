#include "BurdenTracker.h"
#include "Common/Utils.h"

// Function-local static map avoids static-initialization-order issues:
// the map is lazily constructed on first access, safe to call at any point.
namespace
{
	using Map = std::unordered_map<RE::FormID, Burden::ActorBurdenData>;

	Map& GetMap()
	{
		static Map map;
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
		auto& map = GetMap();
		if (map.contains(formId)) {
			return;
		}

		map.emplace(formId, UpdateBurdenLog(a_actor));
	}

	void Unregister(RE::FormID a_formId)
	{
		GetMap().erase(a_formId);
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

			auto& map = GetMap();
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
		return GetMap().contains(a_formId);
	}

	// On a new game or save load, all previously-tracked actors are stale.
	// Re-register the player — other actors can be re-registered by the
	// future Papyrus API as needed.
	void OnGameLoad()
	{
		GetMap().clear();

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (player) {
			Register(player);
		}

		static bool heartbeatStarted = false; // Only ever executed once
		if (!heartbeatStarted) {
			heartbeatStarted = true;
			Common::make_heartbeat(std::chrono::milliseconds(200), TaskTrackBurdenParams);
		}
	}

	// Track Actor parameters that can change dynamically without hooks
	void TaskTrackBurdenParams()
	{
		SKSE::GetTaskInterface()->AddTask([]() {
			auto& map = GetMap();
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
