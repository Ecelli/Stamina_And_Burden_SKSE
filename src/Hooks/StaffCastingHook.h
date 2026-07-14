#pragma once

namespace Hooks
{
	struct StartCastingHook
	{
		static void Install();
		static void Thunk(RE::ActorMagicCaster* a_this);
		static inline REL::Relocation<decltype(Thunk)> _func;
	};
}
