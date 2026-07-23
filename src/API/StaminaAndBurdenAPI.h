#pragma once

#include <RE/A/Actor.h>
#include <RE/B/BGSAttackData.h>

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

	enum class BurdenComponent : uint32_t
	{
		Burden,       // Equipment burden
		CarryBurden,  // Inventory Burden (scales with maximum carry weight)
		BurdenBlend,  // Weighted mix of burden and carry burden
		WeaponRightHand, // Right hand weapon burden 
        // NOTE: Staves use weapon left/right hand
		WeaponLeftHand,  // Left hand weapon burden 
		WeaponTwoHanded, // 2handed weapon burden 
		WeaponRanged,    // ranged weapon burden 
		WeaponBlock      // Block/shield burden 
	};

	struct InterfaceVersion1
	{
		inline static constexpr auto VERSION = Version::Version1;

		virtual ~InterfaceVersion1() = default;

		virtual Version GetVersion() = 0;

		// Burden data
		virtual float GetBurden(RE::Actor* actor) = 0;
		virtual float GetCarryBurden(RE::Actor* actor) = 0;
		virtual float GetBurdenBlend(RE::Actor* actor) = 0;
		virtual float GetBurdenEquippedWeight(RE::Actor* actor) = 0;
		virtual float GetMaxEquippedWeight(RE::Actor* actor) = 0;

		// Max equipped weight override
		virtual void SetMaxEquippedWeightOverride(RE::Actor* actor, float maxEquippedWeight) = 0;

		// Weapon burden — enum-based
		virtual float GetWeaponBurden(RE::Actor* actor, WeaponSlot slot) = 0;

		// Attack costs — two APIs, different scope:
		// GetAttackCostFromData(actor, attackData):
		//   Returns the final stamina cost as applied by S&B's hooks. Includes
		//   staminaMult (dual-wield balance, usually 1.0) and PEPE perk scaling.
		//   Use when you already have a BGSAttackData pointer (e.g. from a hook).
		virtual float GetAttackCostFromData(RE::Actor* actor, RE::BGSAttackData* attackData) = 0;
	
		// Custom action burden cost system — configurable cost based on burden system
		//
		// Direct: compute ad-hoc without a named entry.
		//   ComputeActionCost
		virtual float ComputeActionCost(
			RE::Actor* actor,
			BurdenComponent baseComponent, float baseMin, float baseMax, float baseK,
			BurdenComponent pctComponent, float pctMin, float pctMax, float pctK) = 0;

		// State
		virtual bool IsExhausted(RE::Actor* actor) = 0;

		// Math utilities
		virtual float Interpolate(float min, float max, float t, float k) = 0;
		virtual float SmoothStep(float t) = 0;

		// Exhaustion listener:
		// Exhaustion is a temporary state, it does not live through saves.
		// This listener gives the actor that changed state and the new state
		// You could also register for the mod event using "StaminaAndBurden_OnExhaustionChanged"
		using ExhaustionListener = void(*)(RE::Actor* actor, bool exhausted);
		virtual void RegisterExhaustionListener(ExhaustionListener listener) = 0;
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
