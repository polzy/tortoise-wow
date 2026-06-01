#pragma once
// MCWoW Eluna compat shim — vanilla 1.18.1 doesn't have UnitAI as a
// distinct base class. Eluna upstream uses UnitAI for its compat layer;
// in vanilla all creature AI inherits from CreatureAI directly.
// Re-include the canonical CreatureAI; UnitAI references become aliases.
#include "../../../AI/CreatureAI.h"

// Backwards compat alias. Eluna uses UnitAI as the parent class for
// ScriptedAI / CombatAI; vanilla uses CreatureAI everywhere.
typedef CreatureAI UnitAI;
