#pragma once

#include "Settings/Params/Parameter.h"

struct BurdenParams : REX::Singleton<BurdenParams>
{
	Parameter<float> maxEquippedWeightRatio{ 0.4f, 0.0f, 1.0f };
	// Equip Slot Burden Multipliers
	Parameter<float> SlotBurdenMult_def{ 1.0f, 0.2f, 10.0f };
	Parameter<float> SlotBurdenMult_body{ 0.7f, 0.2f, 10.0f };
	Parameter<float> SlotBurdenMult_feet{ 1.5f, 0.2f, 10.0f };
	Parameter<float> SlotBurdenMult_head{ 1.2f, 0.2f, 10.0f };
	Parameter<float> SlotBurdenMult_hand{ 0.8f, 0.2f, 10.0f };
	// Skill Burden Multipliers
	Parameter<int>   PlayerMaxSkill{ 100, 10, 999};
	Parameter<float> SkillInterpolate{ 0.0f, 0.0f, 1.0f};
	Parameter<float> SkillBurdenMult_minHeavy{ 0.5f, 0.2f, 10.0f };
	Parameter<float> SkillBurdenMult_maxHeavy{ 2.5f, 0.2f, 10.0f };
	Parameter<float> SkillBurdenMult_minLight{ 0.6f, 0.2f, 10.0f };
	Parameter<float> SkillBurdenMult_maxLight{ 2.0f, 0.2f, 10.0f };
	// Effect Burden 
	Parameter<float>  SteedStoneBurdenMult{ 0.3f , 0.0f, 2.0f };
	// Weapon Burden
	Parameter<float> WeaponBurden_LowSkill{ 3.0f, 0.1f, 10.0f };
	Parameter<float> WeaponBurden_HighSkill{ 0.8f, 0.1f, 10.0f };
	Parameter<float> WeaponSkillInterpolate{ 0.5f, 0.0f, 1.0f };
	// Conjured Weapon Weight
	Parameter<float> ConjuredWeightMin{ 2.0f, 0.0f, 50.0f };
	Parameter<float> ConjuredWeightMax{ 30.0f, 0.0f, 50.0f };
	Parameter<float> ConjuredWeightCurve_k{ 0.8f, 0.0f, 1.0f };

	template <typename F>
	static void ForEach(F&& a_fn)
	{
		auto& s = GetSingleton();
		a_fn("fmaxEquippedWeightRatio"sv, s.maxEquippedWeightRatio);
		a_fn("fSlotBurdenMult_def"sv, s.SlotBurdenMult_def);
		a_fn("fSlotBurdenMult_body"sv, s.SlotBurdenMult_body);
		a_fn("fSlotBurdenMult_feet"sv, s.SlotBurdenMult_feet);
		a_fn("fSlotBurdenMult_head"sv, s.SlotBurdenMult_head);
		a_fn("fSlotBurdenMult_hand"sv, s.SlotBurdenMult_hand);
		a_fn("iPlayerMaxSkill"sv, s.PlayerMaxSkill);
		a_fn("fSkillInterpolate"sv, s.SkillInterpolate);
		a_fn("fSkillBurdenMult_minHeavy"sv, s.SkillBurdenMult_minHeavy);
		a_fn("fSkillBurdenMult_maxHeavy"sv, s.SkillBurdenMult_maxHeavy);
		a_fn("fSkillBurdenMult_minLight"sv, s.SkillBurdenMult_minLight);
		a_fn("fSkillBurdenMult_maxLight"sv, s.SkillBurdenMult_maxLight);
        a_fn("fSteedStoneBurdenMult"sv, s.SteedStoneBurdenMult);
        a_fn("fWeaponBurden_LowSkill"sv, s.WeaponBurden_LowSkill);
        a_fn("fWeaponBurden_HighSkill"sv, s.WeaponBurden_HighSkill);
        a_fn("fWeaponSkillInterpolate"sv, s.WeaponSkillInterpolate);
        a_fn("fConjuredWeightMin"sv, s.ConjuredWeightMin);
        a_fn("fConjuredWeightMax"sv, s.ConjuredWeightMax);
        a_fn("fConjuredWeightCurve_k"sv, s.ConjuredWeightCurve_k);
	}
};
