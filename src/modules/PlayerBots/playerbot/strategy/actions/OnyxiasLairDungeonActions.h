#pragma once
#include "DungeonActions.h"
#include "ChangeStrategyAction.h"

namespace ai
{
    class OnyxiasLairEnableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        OnyxiasLairEnableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable onyxia's lair strategy", "+onyxia's lair") {}
    };

    class OnyxiasLairDisableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        OnyxiasLairDisableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable onyxia's lair strategy", "-onyxia's lair") {}
    };

    class OnyxiaEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        OnyxiaEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable onyxia fight strategy", "+onyxia") {}
    };

    class OnyxiaDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        OnyxiaDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable onyxia fight strategy", "-onyxia") {}
    };

    // Move away from Onyxia (creature entry 10184) in any direction up to 40y. The base
    // MoveAwayFromCreature sweeps for a valid angle behind the bot relative to the creature,
    // so even though we don't compute a true perpendicular-to-flight-path vector, the bot
    // ends up out of the Deep Breath corridor in the majority of cases. 40y matches the
    // observed reach of the fire patch P2 cast leaves on the ground.
    class OnyxiaMoveAwayFromBreathAction : public MoveAwayFromCreature
    {
    public:
        OnyxiaMoveAwayFromBreathAction(PlayerbotAI* ai)
            : MoveAwayFromCreature(ai, "move away from onyxia breath", 10184, 40.0f) {}
    };
}
