#include "BurdenTracker.h"

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

		map.emplace(formId, UpdateBurden(a_actor));
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
	}
}
