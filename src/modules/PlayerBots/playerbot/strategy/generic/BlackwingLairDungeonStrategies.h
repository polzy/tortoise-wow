#pragma once
#include "playerbot/strategy/Strategy.h"

namespace ai
{
    class BlackwingLairDungeonStrategy : public Strategy
    {
    public:
        BlackwingLairDungeonStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "blackwing lair"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class SuppressionRoomStrategy : public Strategy
    {
    public:
        SuppressionRoomStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "suppression room"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitCombatMultipliers(std::list<Multiplier*>& multipliers) override;
        void InitNonCombatMultipliers(std::list<Multiplier*>& multipliers) override;
        void OnStrategyAdded(BotState state) override;
    };

    // --- Vaelastrasz the Corrupt (13020) ---
    // The Essence of the Red buff (23513) on raid start gives infinite resources for
    // 3 minutes — straight DPS race. Burning Adrenaline (23620 mana / 23478 melee /
    // 23644 instakill) is the key mechanic: affected target gains 200% damage but
    // dies in ~30s and explodes on death dealing AoE fire to raid.
    class VaelastraszFightStrategy : public Strategy
    {
    public:
        VaelastraszFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "vaelastrasz"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Broodlord Lashlayer ---
    // Tank-and-spank with two tricky abilities:
    //   - Knock Away (18670): big threat drop on tank, requires immediate taunt.
    //   - Mortal Strike (24573): 50% healing reduction on tank — healers must
    //     adapt (already handled by class healer logic).
    //   - Blast Wave (23331): AoE fire, ranged classes flee.
    class BroodlordFightStrategy : public Strategy
    {
    public:
        BroodlordFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "broodlord"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Chromaggus ---
    // Brood Affliction (23153/54/55/70/69 = blue/black/red/bronze/green) applied
    // randomly to raid members; if a player has all 5 simultaneously, Chromatic
    // Mutation (23174) MCs them. Each color needs the right dispel type.
    // Frenzy (23128) needs hunter Tranquilizing Shot. At 20% he Enrages (28747).
    class ChromaggusFightStrategy : public Strategy
    {
    public:
        ChromaggusFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "chromaggus"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };
}