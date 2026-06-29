#include "Damage/DamageManager.h"

namespace Damage
{
	float ComputeStaminaDamageMult(RE::Actor* actor)
	{
        if (! actor)
            logger::info(" Invalid Actor for stamia damage mult"sv);
		return 1.0f;
	}
}
