#include "Regen/CostsManager.h"
#include "Burden/BurdenTracker.h"
#include "Common/Utils.h"
#include "Settings/Params/CostsParams.h"

namespace Costs
{
	float CalculateSprintDrain(RE::Actor* actor)
	{
		if (!actor)
			return 0.0f;

		auto& burden = Burden::Tracker::GetOrComputeBurden(actor);
		auto* params = CostsParams::GetSingleton();

		float base = params->SprintDrainBase.Get();
		float burdenPenalty = params->SprintDrainBurdenPenalty.Get() * burden.burden;
		float drainPerSecond = base + burdenPenalty;
		float result = drainPerSecond * RE::GetSecondsSinceLastFrame();

		Regen::RegenLog("CalculateSprintDrain: base={:.3f} burdenPen={:.3f} drain/s={:.3f} delta={:.6f} -> {:.3f}",
			base, burdenPenalty, drainPerSecond, RE::GetSecondsSinceLastFrame(), result);
		return result;
	}
}
