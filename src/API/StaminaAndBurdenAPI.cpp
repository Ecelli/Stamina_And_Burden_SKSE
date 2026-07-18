#include "API/StaminaAndBurdenAPI.h"
#include "Burden/BurdenManager.h"
#include "Burden/BurdenTracker.h"

namespace
{
	class InterfaceImpl final : public StaminaAndBurdenAPI::InterfaceVersion1
	{
	public:
		StaminaAndBurdenAPI::Version GetVersion() override
		{
			return StaminaAndBurdenAPI::Version::Current;
		}

		float GetBurden(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Burden::Tracker::GetOrComputeBurden(a_actor).burden;
		}

		float GetCarryBurden(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Burden::Tracker::GetOrComputeBurden(a_actor).carryBurden;
		}

		float GetBurdenBlend(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Burden::Tracker::GetOrComputeBurden(a_actor).burdenBlend;
		}

		float GetBurdenEquippedWeight(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Burden::Tracker::GetOrComputeBurden(a_actor).equippedWeight;
		}

		float GetMaxEquippedWeight(RE::Actor* a_actor) override
		{
			if (!a_actor) return 0.0f;
			return Burden::Tracker::GetOrComputeBurden(a_actor).maxEquippedWeight;
		}
	};

	InterfaceImpl& GetImpl()
	{
		static InterfaceImpl impl;
		return impl;
	}
}

extern "C" DLLEXPORT void* __stdcall SB_RequestInterfaceImpl(StaminaAndBurdenAPI::Version a_version)
{
	switch (a_version) {
	case StaminaAndBurdenAPI::Version::Version1:
		return &GetImpl();
	default:
		return nullptr;
	}
}
