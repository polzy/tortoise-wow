#pragma once
#include "DungeonActions.h"
#include "ChangeStrategyAction.h"

namespace ai
{
    class TempleOfAhnQirajEnableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        TempleOfAhnQirajEnableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable temple of ahnqiraj strategy", "+temple of ahnqiraj") {}
    };

    class TempleOfAhnQirajDisableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        TempleOfAhnQirajDisableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable temple of ahnqiraj strategy", "-temple of ahnqiraj") {}
    };

    // Move-away from Sartura herself (15516) during Whirlwind — 12y matches the
    // 'sartura too close' trigger threshold + a small safety buffer.
    class SarturaMoveAwayAction : public MoveAwayFromCreature
    {
    public:
        SarturaMoveAwayAction(PlayerbotAI* ai) : MoveAwayFromCreature(ai, "move away from sartura", 15516, 15.0f) {}
    };

    // Fight enable/disable
    class SarturaEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        SarturaEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable sartura fight strategy", "+sartura") {}
    };
    class SarturaDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        SarturaDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable sartura fight strategy", "-sartura") {}
    };

    class HuhuranEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        HuhuranEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable huhuran fight strategy", "+huhuran") {}
    };
    class HuhuranDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        HuhuranDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable huhuran fight strategy", "-huhuran") {}
    };
}
