#pragma once
#include "playerbot/strategy/Strategy.h"

namespace ai
{
    class TempleOfAhnQirajDungeonStrategy : public Strategy
    {
    public:
        TempleOfAhnQirajDungeonStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "temple of ahnqiraj"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Battleguard Sartura (15516) ---
    // Whirlwind (26083, 15s self-cast, threat-resets every cast) is the
    // signature mechanic. Ranged/heal stay 12y+. Sundering Cleave (25174)
    // is a frontal cone — tank eats it.
    class SarturaFightStrategy : public Strategy
    {
    public:
        SarturaFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "sartura"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
        void InitCombatMultipliers(std::list<Multiplier*>& multipliers) override;
    };

    // --- Princess Huhuran (15509) ---
    // Frenzy (26051) self-buff dispelled by Tranquilizing Shot. Noxious Poison
    // (26053) nature debuff curable by druid 'cure poison'. Wyvern Sting at low
    // HP isn't wired (boss-side mechanic, bot can't pre-empt).
    // Frost prot pot doesn't apply; Huhuran is nature damage — `.bot natres`
    // gear handles the raid-wide aura.
    class HuhuranFightStrategy : public Strategy
    {
    public:
        HuhuranFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "huhuran"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };
}
