#pragma once

#include "Burden/BurdenManager.h"

namespace Burden::Tracker
{
	/**
	 * Registers an actor for burden tracking and computes initial burden.
	 * @param a_actor  The actor to track (must be valid).
	 */
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

	/**
	 * @param a_formId  The actor's FormID.
	 * @return true if the actor is currently tracked.
	 */
	bool IsTracked(RE::FormID a_formId);

	/**
	 * Called when a game is loaded (new save or existing save).
	 * Clears all tracked actors and re-registers the player.
	 * Burden is dynamically computed — no serialization needed.
	 */
	void OnGameLoad();
}
