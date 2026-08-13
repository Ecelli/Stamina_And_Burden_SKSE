#pragma once

namespace Hooks::MovementHooks
{
	void InstallSpeed();
	void InstallSprint();
	void InstallJump();

	// Returns the unmodified engine movement speed (the value the SpeedHook
	// intercepts before applying the S&B multiplier). Used by Papyrus/API queries.
	float GetEngineSpeed(RE::Actor* a_actor);
}
