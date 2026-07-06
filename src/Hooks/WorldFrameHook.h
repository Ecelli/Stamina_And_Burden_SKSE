#pragma once

namespace Hooks
{
	class WorldFrameHook
	{
	public:
		static void Install();

	private:
		using OnFrame_t = std::int32_t (*)(std::int64_t);
		static inline OnFrame_t _original = nullptr;
		static inline int _frameCounter = 0;

		static std::int32_t OnFrameUpdate(std::int64_t a1);
	};
}
