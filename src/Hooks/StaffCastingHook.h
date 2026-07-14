#pragma once

namespace Hooks
{
	struct StartCastingHook
	{
		static void Install();
		static void Thunk(RE::ActorMagicCaster* a_this);
		static inline REL::Relocation<decltype(Thunk)> _func;
	};

	struct CasterUpdateHook
	{
		static void Install();
		static void Thunk(RE::ActorMagicCaster* a_this, float a_deltaTime);
		static inline REL::Relocation<decltype(Thunk)> _func;
	};
}
