#pragma once
#include "DungeonActions.h"
#include "ChangeStrategyAction.h"

namespace ai
{
    class RuinsOfAhnQirajEnableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        RuinsOfAhnQirajEnableDungeonStrategyAction(PlayerbotAI* ai)
            : ChangeAllStrategyAction(ai, "enable ruins of ahnqiraj strategy", "+ruins of ahnqiraj") {}
    };

    class RuinsOfAhnQirajDisableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        RuinsOfAhnQirajDisableDungeonStrategyAction(PlayerbotAI* ai)
            : ChangeAllStrategyAction(ai, "disable ruins of ahnqiraj strategy", "-ruins of ahnqiraj") {}
    };
}
