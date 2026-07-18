#pragma once

#include <RE/A/Actor.h>

#ifdef UNICODE
#define SB_API_SOURCE L"StaminaAndBurden.dll"
#else
#define SB_API_SOURCE "StaminaAndBurden.dll"
#endif

namespace RE { class BGSAttackData; }

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

	struct CostCurve
	{
		float min{ 0.0f }; // Cost at low burden
		float max{ 0.0f }; // Cost at high burden
		float k{ 0.0f };   // curve control
		BurdenComponent burdenComponent{ BurdenComponent::BurdenBlend };
	};

	struct ActionCostConfig
	{
		CostCurve base;    // Direct cost
		CostCurve percent; // percent stamina cost
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

		// Attack costs — two APIs, different scope:
		//
		// GetBaseAttackCost(actor, bash, left, power):
		//   Returns the burden-based stamina cost for an attack. The consumer
		//   provides the attack flags directly. Does NOT include the engine's
		//   dual-wield staminaMult or perk modifications (PEPE).
		//   Use when you know the attack parameters but don't have BGSAttackData.
		//
		// GetAttackCostFromData(actor, attackData):
		//   Returns the final stamina cost as applied by S&B's hooks. Includes
		//   staminaMult (dual-wield balance, usually 1.0) and PEPE perk scaling.
		//   Use when you already have a BGSAttackData pointer (e.g. from a hook).
		//
		// Convenience wrappers delegate to GetBaseAttackCost with preset flags:
		//   GetBaseNormalAttackCost(actor) -> GetBaseAttackCost(actor, false, false, false)
		//   GetBasePowerAttackCost(actor)  -> GetBaseAttackCost(actor, false, false, true)
		//   GetBaseBashAttackCost(actor)   -> GetBaseAttackCost(actor, true,  false, false)
		//   GetBaseLeftHandAttackCost(actor) -> GetBaseAttackCost(actor, false, true,  false)
		//   GetBaseLeftPowerAttackCost(actor)  -> GetBaseAttackCost(actor, false, true,  true)
		//   GetBaseBashPowerAttackCost(actor)  -> GetBaseAttackCost(actor, true,  false, true)
		virtual float GetBaseAttackCost(RE::Actor* actor, bool bash, bool left, bool power) = 0;
		virtual float GetAttackCostFromData(RE::Actor* actor, RE::BGSAttackData* attackData) = 0;
		virtual float GetBaseNormalAttackCost(RE::Actor* actor) = 0;
		virtual float GetBasePowerAttackCost(RE::Actor* actor) = 0;
		virtual float GetBaseLeftHandAttackCost(RE::Actor* actor) = 0;
		virtual float GetBaseLeftPowerAttackCost(RE::Actor* actor) = 0;
		virtual float GetBaseBashAttackCost(RE::Actor* actor) = 0;
		virtual float GetBaseBashPowerAttackCost(RE::Actor* actor) = 0;
		virtual float GetBowFireCost(RE::Actor* actor) = 0;
		virtual float GetStaffFireCost(RE::Actor* actor, bool leftHand) = 0;

		// Custom action burden cost system — configurable cost based on burden system
		//
		// Registry-based: define once, query by name from anywhere.
		//   SetNamedActionCost("MyMod_Action", config)        — register
		//   ComputeNamedActionCost(actor, "MyMod_Action") — compute
		//   IsActionRegistered("MyMod_Action")           — check registration
		//
		// Direct: compute ad-hoc without a named entry.
		//   ComputeActionCost
		virtual void SetNamedActionCost(const char* name, const ActionCostConfig& config) = 0;
		virtual float ComputeNamedActionCost(RE::Actor* actor, const char* a_actionName) = 0;
		virtual bool IsActionRegistered(const char* name) = 0;
		virtual float ComputeActionCost(
			RE::Actor* actor,
			BurdenComponent baseComponent, float baseMin, float baseMax, float baseK,
			BurdenComponent pctComponent, float pctMin, float pctMax, float pctK) = 0;

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
