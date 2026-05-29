#pragma once
#include "playerbot/strategy/Strategy.h"

namespace ai
{
    class ZulGurubDungeonStrategy : public Strategy
    {
    public:
        ZulGurubDungeonStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "zulgurub"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Hakkar the Soulflayer (14834) ---
    // The five Aspects (24686-24690) come from the previously-killed High
    // Priest mini-bosses. Each persistent aspect Hakkar still has is cast
    // periodically. Bot side: dispel the magic aspects (Marli stun /
    // Jeklik silence) and cure the poison aspect (Venoxis). Strip the
    // Thekal enrage off Hakkar himself via hunter Tranquilizing Shot.
    // Cause Insanity (24327) charm is not bot-actionable without
    // raid-member-target plumbing.
    class HakkarFightStrategy : public Strategy
    {
    public:
        HakkarFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "hakkar"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };
}
