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

    // --- Razorgore the Untamed (12435) ---
    // Two-phase fight. P1: Razorgore is mind-controlled (aura 23014) by a player
    // standing on the Possess orb; the boss attacks his own eggs. Adds spawn
    // (Death Talon Dragonspawn 12422 / 14036, Grethok mages 12420) and assault
    // the orb-bound player. Raid kills adds, ignores boss. P2 (after all 30 eggs
    // smashed): Razorgore reverts and is tank-and-spanked with Fireball Volley
    // (22425), Conflagration (23023), War Stomp (24375).
    //
    // Bot side: in P1 the "razorgore phase 1" trigger fires; we route ranged/melee
    // to "attack least hp target" so they pick the adds (lowest-HP attacker = add,
    // boss is a tank with millions HP and isn't attacking the bot anyway). When the
    // aura drops we're back to normal target selection in P2.
    class RazorgoreFightStrategy : public Strategy
    {
    public:
        RazorgoreFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "razorgore"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
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

    // --- Firemaw (11983) ---
    // Tank-and-spank with frontal cone Shadow Flame (22539, 30y), Wing Buffet
    // (23339 -50% threat on hit target) and Flame Buffet (23341, periodic).
    // Bot-side: fire prot pot only — tank-swap on Flame Buffet stacks requires
    // a multi-tank coord framework we don't have yet; ranged are already
    // out-of-cone via class strategy 'enemy too close for spell'.
    class FiremawFightStrategy : public Strategy
    {
    public:
        FiremawFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "firemaw"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Ebonroc (14601) ---
    // Shadow of Ebonroc (23340) is a self-heal aura the boss applies to himself
    // on melee hit — burst-through; no bot-side action helps here.
    // Same cone + Wing Buffet as Firemaw.
    class EbonrocFightStrategy : public Strategy
    {
    public:
        EbonrocFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "ebonroc"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Flamegor (11981) ---
    // Frenzy (23342) self-buff is dispelable via Hunter Tranquilizing Shot (19801).
    // The hunter class action 'tranquilizing shot' is registered in
    // HunterActions.h; the trigger drives it priority 90.
    class FlamegorFightStrategy : public Strategy
    {
    public:
        FlamegorFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "flamegor"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Nefarian (11583) ---
    // P2-only strategy (P1 is the channelling-drake phase where Nefarian isn't
    // attackable). Mechanics: Bellowing Roar (22686, 8s fear), Veil of Shadow
    // (22687, -100% healing on tank), Shadow Flame (22539, 30y frontal cone),
    // Cleave (20691, frontal cone), Tail Lash (23364, rear cone) + Class Calls
    // (23397/8/10/14/18/25/27/36/01 by class). Bot-side we wire fire prot and
    // Bellowing Roar — Class Calls require per-class fear/MC handling beyond
    // this strategy.
    class NefarianFightStrategy : public Strategy
    {
    public:
        NefarianFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "nefarian"; }

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