#pragma once
#include "playerbot/strategy/Strategy.h"

namespace ai
{
    class NaxxramasDungeonStrategy : public Strategy
    {
    public:
        NaxxramasDungeonStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "naxxramas"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class FourHorsemanFightStrategy : public Strategy
    {
    public:
        FourHorsemanFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "four horseman"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Patchwerk (16028) ---
    // Pure tank-and-spank with a wrinkle: Hateful Strike (28308) hits the highest-HP
    // non-tank melee in range. Solution: keep low-HP melees behind tank. We use the
    // 7-min berserk (27680) as a soft DPS race indicator.
    class PatchwerkFightStrategy : public Strategy
    {
    public:
        PatchwerkFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "patchwerk"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Loatheb (16011) ---
    // Anti-heal fight. Corrupted Mind (29201) puts a heal debuff on healers after
    // they cast (60s lockout). Spore (16286) adds drop Fungal Bloom (29232) buff
    // which counter-acts the lockout. Inevitable Doom (29204) is heavy raid AOE.
    class LoathebFightStrategy : public Strategy
    {
    public:
        LoathebFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "loatheb"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Sapphiron (15989) ---
    // Massive frost dragon. Frost Aura (28529) raid-wide, requires frost resist
    // gear (provided by `.bot frostres`). Frost Breath (28524, 7s cast) is the
    // signature mechanic — players hide behind an Ice Block created by Ice Bolt
    // (28522). Bot-side, we wire Life Drain (28542) dispel; Frost Breath hide
    // logic requires pathing to an Ice Block guid which isn't trivial without
    // a new value/finder primitive — left as TODO.
    class SapphironFightStrategy : public Strategy
    {
    public:
        SapphironFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "sapphiron"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Kel'Thuzad (15990) ---
    // Mana Detonation (27819): drain mana + AoE damage on group. Shadow Fissure
    // (27810): targeted void zone, dodge it. Frost Blast (27808): root + AoE damage.
    // Chains of Kel'Thuzad (28408): MC players, must be CC'd / killed first.
    class KelThuzadFightStrategy : public Strategy
    {
    public:
        KelThuzadFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "kel'thuzad"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };
}