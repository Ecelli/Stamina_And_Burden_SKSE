#pragma once

#include "Settings/Params/Parameter.h"

struct BurdenParams : REX::Singleton<BurdenParams>
{
	// ===== Equipment =====
	Parameter<float> maxEquippedWeightRatio{ 0.4f, 0.0f, 1.0f };
	Parameter<float> SlotBurdenMult_def{ 1.0f, 0.2f, 10.0f };
	Parameter<float> SlotBurdenMult_body{ 0.7f, 0.2f, 10.0f };
	Parameter<float> SlotBurdenMult_feet{ 1.5f, 0.2f, 10.0f };
	Parameter<float> SlotBurdenMult_head{ 1.2f, 0.2f, 10.0f };
	Parameter<float> SlotBurdenMult_hand{ 0.8f, 0.2f, 10.0f };

	// ===== Skill =====
	Parameter<int>   PlayerMaxSkill{ 100, 10, 999 };
	Parameter<float> SkillInterpolate{ 0.0f, 0.0f, 1.0f };
	Parameter<float> SkillBurdenMult_minHeavy{ 0.5f, 0.2f, 10.0f };
	Parameter<float> SkillBurdenMult_maxHeavy{ 2.5f, 0.2f, 10.0f };
	Parameter<float> SkillBurdenMult_minLight{ 0.6f, 0.2f, 10.0f };
	Parameter<float> SkillBurdenMult_maxLight{ 2.0f, 0.2f, 10.0f };

	// ===== Weapon =====
	Parameter<float> WeaponWeightMult_LowSkill{ 3.0f, 0.1f, 10.0f };
	Parameter<float> WeaponWeightMult_HighSkill{ 0.8f, 0.1f, 10.0f };
	Parameter<float> WeaponWeightMult_Curve_k{ 0.5f, 0.0f, 1.0f };

	// ===== Conjured =====
	Parameter<float> ConjuredWeightMin{ 2.0f, 0.0f, 50.0f };
	Parameter<float> ConjuredWeightMax{ 30.0f, 0.0f, 50.0f };
	Parameter<float> ConjuredWeightCurve_k{ 0.8f, 0.0f, 1.0f };

	// ===== Block =====
	Parameter<float> BlockSkillBlendFactor{ 0.5f, 0.0f, 1.0f };
	Parameter<float> BlockWeightMult_LowSkill{ 2.0f, 0.1f, 10.0f };
	Parameter<float> BlockWeightMult_HighSkill{ 0.5f, 0.1f, 10.0f };
	Parameter<float> BlockWeightMult_Curve_k{ 0.5f, 0.0f, 1.0f };
	Parameter<float> DualWieldBlockPenalty{ 1.5f, 1.0f, 3.0f };
	Parameter<float> UnarmedWeight{ 3.0f, 0.0f, 50.0f };

	// ===== Effects =====
	Parameter<float> SteedStoneBurdenMult{ 0.3f, 0.0f, 2.0f };

	template <typename F>
	static void ForEach(F&& a_fn)
	{
		auto* s = GetSingleton();
		a_fn("Equipment");
		a_fn("fmaxEquippedWeightRatio"sv, s->maxEquippedWeightRatio);
		a_fn("fSlotBurdenMult_def"sv, s->SlotBurdenMult_def);
		a_fn("fSlotBurdenMult_body"sv, s->SlotBurdenMult_body);
		a_fn("fSlotBurdenMult_feet"sv, s->SlotBurdenMult_feet);
		a_fn("fSlotBurdenMult_head"sv, s->SlotBurdenMult_head);
		a_fn("fSlotBurdenMult_hand"sv, s->SlotBurdenMult_hand);
		a_fn("Skill Multipliers");
		a_fn("iPlayerMaxSkill"sv, s->PlayerMaxSkill);
		a_fn("fSkillInterpolate"sv, s->SkillInterpolate);
		a_fn("fSkillBurdenMult_minHeavy"sv, s->SkillBurdenMult_minHeavy);
		a_fn("fSkillBurdenMult_maxHeavy"sv, s->SkillBurdenMult_maxHeavy);
		a_fn("fSkillBurdenMult_minLight"sv, s->SkillBurdenMult_minLight);
		a_fn("fSkillBurdenMult_maxLight"sv, s->SkillBurdenMult_maxLight);
		a_fn("Weapon Burden");
		a_fn("fWeaponWeightMult_LowSkill"sv, s->WeaponWeightMult_LowSkill);
		a_fn("fWeaponWeightMult_HighSkill"sv, s->WeaponWeightMult_HighSkill);
		a_fn("fWeaponWeightMult_Curve_k"sv, s->WeaponWeightMult_Curve_k);
		a_fn("Conjured Weapon Weight");
		a_fn("fConjuredWeightMin"sv, s->ConjuredWeightMin);
		a_fn("fConjuredWeightMax"sv, s->ConjuredWeightMax);
		a_fn("fConjuredWeightCurve_k"sv, s->ConjuredWeightCurve_k);
		a_fn("Block Burden");
		a_fn("fBlockSkillBlendFactor"sv, s->BlockSkillBlendFactor);
		a_fn("fBlockWeightMult_LowSkill"sv, s->BlockWeightMult_LowSkill);
		a_fn("fBlockWeightMult_HighSkill"sv, s->BlockWeightMult_HighSkill);
		a_fn("fBlockWeightMult_Curve_k"sv, s->BlockWeightMult_Curve_k);
		a_fn("fDualWieldBlockPenalty"sv, s->DualWieldBlockPenalty);
		a_fn("fUnarmedWeight"sv, s->UnarmedWeight);
		a_fn("Effects");
		a_fn("fSteedStoneBurdenMult"sv, s->SteedStoneBurdenMult);
	}
};
