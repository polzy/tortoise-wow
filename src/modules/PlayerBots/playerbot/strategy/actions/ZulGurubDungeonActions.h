#pragma once
#include "DungeonActions.h"
#include "ChangeStrategyAction.h"

namespace ai
{
    class ZulGurubEnableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        ZulGurubEnableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable zulgurub strategy", "+zulgurub") {}
    };

    class ZulGurubDisableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        ZulGurubDisableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable zulgurub strategy", "-zulgurub") {}
    };

    class HakkarEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        HakkarEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable hakkar fight strategy", "+hakkar") {}
    };
    class HakkarDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        HakkarDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable hakkar fight strategy", "-hakkar") {}
    };
}
