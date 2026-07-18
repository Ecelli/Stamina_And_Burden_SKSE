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

	enum class WeaponSlot : uint32_t
	{
		RightHand,
		LeftHand,
		TwoHanded,
		Ranged,
		Block
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

		// Weapon burden — enum-based
		virtual float GetWeaponBurden(RE::Actor* actor, WeaponSlot slot) = 0;

		// Weapon burden — convenience
		virtual float GetRightHandWeaponBurden(RE::Actor* actor) = 0;
		virtual float GetLeftHandWeaponBurden(RE::Actor* actor) = 0;
		virtual float GetTwoHandedWeaponBurden(RE::Actor* actor) = 0;
		virtual float GetRangedWeaponBurden(RE::Actor* actor) = 0;
		virtual float GetBlockWeaponBurden(RE::Actor* actor) = 0;

		// Costs — non-attack
		virtual float GetSprintDrain(RE::Actor* actor) = 0;
		virtual float GetJumpCost(RE::Actor* actor) = 0;

		// Hold penalties
		virtual float GetBlockHoldPenalty(RE::Actor* actor) = 0;
		virtual float GetBowDrawHoldPenalty(RE::Actor* actor) = 0;
		virtual float GetStaffHoldPenalty(RE::Actor* actor, bool leftHand) = 0;

		// Movement multipliers
		virtual float GetSpeedMultiplier(RE::Actor* actor) = 0;
		virtual float GetJumpHeightMultiplier(RE::Actor* actor) = 0;

		// State
		virtual bool IsExhausted(RE::Actor* actor) = 0;

		// Math utilities
		virtual float Interpolate(float min, float max, float t, float k) = 0;
		virtual float SmoothStep(float t) = 0;
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
