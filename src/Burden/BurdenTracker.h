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
	 * Called when a game is loaded (new save or existing save).
	 * Clears all tracked actors and re-registers the player.
	 * Burden is dynamically computed — no serialization needed.
	 */
	void OnGameLoad();
}
