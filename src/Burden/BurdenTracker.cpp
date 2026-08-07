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
	using TrackedMap = LockedMap<RE::FormID, Burden::ActorBurdenData>;

	TrackedMap& GetTrackedMap()
	{
		static TrackedMap map;
		return map;
	}

	using TransientMap = LockedMap<RE::FormID, Burden::ActorBurdenData>;

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

	bool NeedToUpdateBurden(RE::Actor* a_actor, const Burden::ActorBurdenData& a_data)
	{
		return static_cast<int>(a_actor->GetActorValue(RE::ActorValue::kLightArmor)) != a_data.lightSkill
			|| static_cast<int>(a_actor->GetActorValue(RE::ActorValue::kHeavyArmor)) != a_data.heavySkill
			|| static_cast<int>(a_actor->GetActorValue(RE::ActorValue::kOneHanded)) != a_data.oneHandedSkill
			|| static_cast<int>(a_actor->GetActorValue(RE::ActorValue::kTwoHanded)) != a_data.twoHandedSkill
			|| static_cast<int>(a_actor->GetActorValue(RE::ActorValue::kArchery)) != a_data.marksmanSkill
			|| static_cast<int>(a_actor->GetActorValue(RE::ActorValue::kBlock)) != a_data.blockSkill
			|| static_cast<int>(a_actor->GetActorValue(RE::ActorValue::kConjuration)) != a_data.conjurationSkill
			|| static_cast<int>(a_actor->GetActorValue(RE::ActorValue::kEnchanting)) != a_data.staffSkill
			|| std::abs(a_actor->GetActorValue(RE::ActorValue::kCarryWeight) - a_data.maxCarryWeight) > 0.001f;
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

		map.insert_or_assign(formId, UpdateBurden(a_actor));
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
			Burden::ActorBurdenData data; // We don't care for data here
			if (tracked.get(formId, data)) {
				tracked.insert_or_assign(formId, UpdateBurden(actor));
				return;
			}

			// Tier 2: transient NPCs
			auto& transient = GetTransientMap();
			if (transient.get(formId, data)) {
				transient.insert_or_assign(formId, UpdateBurden(actor));
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
		Burden::ActorBurdenData data;
		if (tracked.get(formId, data)) {
			return data;
		}

		// Tier 2: transient cache
		auto& transient = GetTransientMap();
		if (transient.get(formId, data)) {
			return data;
		}

		// Compute and cache
		data = UpdateBurden(a_actor);
		transient.insert_or_assign(formId, data);
		return data;
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

		auto pollActorParams = [](RE::FormID formId, const Burden::ActorBurdenData& data) {
			auto* actor = RE::TESForm::LookupByID<RE::Actor>(formId);
			if (!actor) {
				return;
			}
			if (NeedToUpdateBurden(actor, data)) {
				Burden::Tracker::Update(actor);
			}
		};

		map.for_each(pollActorParams);
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
