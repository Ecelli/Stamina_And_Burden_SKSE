#include "BurdenTracker.h"
#include "Common/LockedMap.h"
#include "Hooks/RegenHooks.h"
#include "Stamina/ExhaustionManager.h"

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

	using MaxEquipWeightOverrideMap = LockedMap<RE::FormID, float>;

	MaxEquipWeightOverrideMap& GetMaxEquipWeightOverrideMap()
	{
		static MaxEquipWeightOverrideMap map;
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

		map.emplace(formId, UpdateBurden(a_actor));
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

		SKSE::GetTaskInterface()->AddTask([formId]() {

			auto* actor = RE::TESForm::LookupByID<RE::Actor>(formId);
			if (!actor) {
				return;
			}

			// Tier 1: tracked actors
			auto& tracked = GetTrackedMap();
			auto it = tracked.find(formId);
			if (it != tracked.end()) {
				it->second = UpdateBurden(actor);
				return;
			}

			// Tier 2: transient NPCs
			auto& transient = GetTransientMap();
			auto tIt = transient.find(formId);
			if (tIt != transient.end()) {
				tIt->second = UpdateBurden(actor);
			}
		});
	}

	bool IsTracked(RE::FormID a_formId)
	{
		return GetTrackedMap().contains(a_formId);
	}

	Burden::ActorBurdenData GetOrComputeBurden(RE::Actor* a_actor)
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
		GetMaxEquipWeightOverrideMap().clear();
		ClearTransientCache();
		Hooks::ClearRegenDrainCache();
		Exhaustion::ExhaustionManager::GetSingleton()->ClearAll();

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (player) {
			Register(player);
		}

	}

	// Track Actor parameters that can change dynamically without hooks
	void PollTrackedActorParams()
	{
		auto& map = GetTrackedMap();
		for (auto& [formId, data] : map) {
			auto* actor = RE::TESForm::LookupByID<RE::Actor>(formId);
			if (!actor) {
				continue;
			}
			if (static_cast<int>(actor->GetActorValue(RE::ActorValue::kLightArmor)) != data.lightSkill
				|| static_cast<int>(actor->GetActorValue(RE::ActorValue::kHeavyArmor)) != data.heavySkill
				|| static_cast<int>(actor->GetActorValue(RE::ActorValue::kOneHanded)) != data.oneHandedSkill
				|| static_cast<int>(actor->GetActorValue(RE::ActorValue::kTwoHanded)) != data.twoHandedSkill
				|| static_cast<int>(actor->GetActorValue(RE::ActorValue::kArchery)) != data.marksmanSkill
				|| static_cast<int>(actor->GetActorValue(RE::ActorValue::kBlock)) != data.blockSkill
				|| static_cast<int>(actor->GetActorValue(RE::ActorValue::kConjuration)) != data.conjurationSkill
				|| static_cast<int>(actor->GetActorValue(RE::ActorValue::kEnchanting)) != data.staffSkill
				|| std::abs(actor->GetActorValue(RE::ActorValue::kCarryWeight) - data.maxCarryWeight) > 0.001f) {

				Burden::Tracker::Update(actor);
			}
		}
	}

	void SetMaxEquippedWeightOverride(RE::Actor* a_actor, float a_value)
	{
		if (!a_actor) return;
		auto formId = a_actor->GetFormID();
		if (formId == 0) return;
		float clamped = std::max(a_value, 0.0f);
		GetMaxEquipWeightOverrideMap().insert_or_assign(formId, clamped);
		Update(a_actor);
	}

	float GetMaxEquippedWeightOverride(RE::Actor* a_actor)
	{
		if (!a_actor) return 0.0f;
		auto formId = a_actor->GetFormID();
		if (formId == 0) return 0.0f;
		float value;
		return GetMaxEquipWeightOverrideMap().get(formId, value) ? value : 0.0f;
	}
}
