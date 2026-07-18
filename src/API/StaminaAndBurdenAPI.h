#pragma once

#include <RE/A/Actor.h>

#ifdef UNICODE
#define SB_API_SOURCE L"StaminaAndBurden.dll"
#else
#define SB_API_SOURCE "StaminaAndBurden.dll"
#endif

namespace StaminaAndBurdenAPI
{
	enum class Version
	{
		Version1,

		Current = Version1
	};

	struct InterfaceVersion1
	{
		inline static constexpr auto VERSION = Version::Version1;

		virtual ~InterfaceVersion1() = default;

		virtual Version GetVersion() = 0;

		// Burden queries
		virtual float GetBurden(RE::Actor* actor) = 0;
		virtual float GetCarryBurden(RE::Actor* actor) = 0;
		virtual float GetBurdenBlend(RE::Actor* actor) = 0;
		virtual float GetBurdenEquippedWeight(RE::Actor* actor) = 0;
		virtual float GetMaxEquippedWeight(RE::Actor* actor) = 0;
	};

	using CurrentInterface = InterfaceVersion1;

	inline CurrentInterface* Interface = nullptr;

	inline void* RequestInterface(Version version)
	{
		typedef void* (__stdcall* RequestFunction)(Version);

		static RequestFunction request_interface = nullptr;

		HINSTANCE hModule = GetModuleHandle(SB_API_SOURCE);

		if (hModule == nullptr) {
#ifdef SPDLOG_H
			spdlog::critical("StaminaAndBurden.dll not found, API will remain non functional.");
#endif
			return nullptr;
		}

		request_interface = reinterpret_cast<RequestFunction>(
			GetProcAddress(hModule, "SB_RequestInterfaceImpl"));

		if (!request_interface) {
#ifdef SPDLOG_H
			spdlog::critical("SB_RequestInterfaceImpl not found in StaminaAndBurden.dll");
#endif
			return nullptr;
		}

		return request_interface(version);
	}

	template <class InterfaceClass>
	inline InterfaceClass* RequestInterface()
	{
		static InterfaceClass* intfc = nullptr;

		if (!intfc) {
			intfc = reinterpret_cast<InterfaceClass*>(RequestInterface(InterfaceClass::VERSION));

			if constexpr (std::is_same_v<InterfaceClass, CurrentInterface>)
				Interface = intfc;
		}

		return intfc;
	}
}
