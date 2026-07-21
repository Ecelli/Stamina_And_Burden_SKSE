#pragma once

#include "Burden/BurdenManager.h"

namespace Burden::Tracker
{
	void Register(RE::Actor* a_actor);

	/**
	 * Stops tracking the given actor.
	 * @param a_formId  The actor's FormID — same value retrieved from GetFormID().
	 */
	void Unregister(RE::FormID a_formId);

	/**
	 * Re-computes and stores the actor's burden data, then logs the result.
	 * No-op if the actor is not tracked.
	 */
	void Update(RE::Actor* a_actor);
	bool IsTracked(RE::FormID a_formId);

	/**
	 * Returns the burden data for any actor via a 2-tier cache:
	 *   Tier 1 — tracked actors (event-driven, persistent for the session).
	 *   Tier 2 — transient cache, computed once, and cleaned up periodically (worldspace).
	 * Non-tracked actors are computed on-demand and cached in the transient tier.
	 */
	const Burden::ActorBurdenData& GetOrComputeBurden(RE::Actor* a_actor);

	/**
	 * Clears the transient NPC cache. Called on worldspace change 
	 */
	void ClearTransientCache();

	/**
	 * Called when a game is loaded (new save or existing save).
	 * Clears all tracked actors and re-registers the player.
	 * Burden is dynamically computed — no serialization needed.
	 */
	void OnGameLoad();

	/**
	 * Called from the world frame hook every 6th frame (~100–240ms).
	 * Reads GetActorValue(kCarryWeight), kLightArmor, and kHeavyArmor
	 * for each tracked actor and triggers an Update() when any cached
	 * value differs from the current value.
	 */
	void PollTrackedActorParams();

	void SetMaxEquippedWeightOverride(RE::Actor* a_actor, float a_value);
	float GetMaxEquippedWeightOverride(RE::Actor* a_actor);
}
