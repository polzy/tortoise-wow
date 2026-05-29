#pragma once
#include "playerbot/strategy/Strategy.h"

namespace ai
{
    class RuinsOfAhnQirajDungeonStrategy : public Strategy
    {
    public:
        RuinsOfAhnQirajDungeonStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "ruins of ahnqiraj"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };
}
