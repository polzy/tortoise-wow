#pragma once
#include "playerbot/strategy/Strategy.h"

namespace ai
{
    class MoltenCoreDungeonStrategy : public Strategy
    {
    public:
        MoltenCoreDungeonStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "molten core"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
    };

    class MagmadarFightStrategy : public Strategy
    {
    public:
        MagmadarFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "magmadar"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
        void InitReactionTriggers(std::list<TriggerNode*>& triggers) override;
        void InitCombatMultipliers(std::list<Multiplier*>& multipliers) override;
    };

    // --- Lucifron (12118) ---
    class LucifronFightStrategy : public Strategy
    {
    public:
        LucifronFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "lucifron"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Gehennas (12259) ---
    class GehennasFightStrategy : public Strategy
    {
    public:
        GehennasFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "gehennas"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Garr (12057) ---
    class GarrFightStrategy : public Strategy
    {
    public:
        GarrFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "garr"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Baron Geddon (12056) ---
    class BaronGeddonFightStrategy : public Strategy
    {
    public:
        BaronGeddonFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "baron geddon"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Shazzrah (12264) ---
    class ShazzrahFightStrategy : public Strategy
    {
    public:
        ShazzrahFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "shazzrah"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Sulfuron Harbinger (12098) ---
    class SulfuronFightStrategy : public Strategy
    {
    public:
        SulfuronFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "sulfuron"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Golemagg (11988) ---
    class GolemaggFightStrategy : public Strategy
    {
    public:
        GolemaggFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "golemagg"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
        void InitCombatMultipliers(std::list<Multiplier*>& multipliers) override;
    };

    // --- Majordomo Executus (12018) ---
    // Pre-Ragnaros encounter, 8 elite Healer/Elite adds need to be CC'd / killed
    // before tanking Domo himself. Spell IDs (from boss_majordomo_executus.cpp):
    //   20619 Magic Reflection, 21075 Damage Reflection (don't dispel)
    //   20229 Blastwave AoE, 20620 Aegis (self heal at <50%, interrupt target)
    //   20618 Teleport (port-out emote, ends fight)
    class MajordomoFightStrategy : public Strategy
    {
    public:
        MajordomoFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "majordomo"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
    };

    // --- Ragnaros (11502) ---
    // Spell IDs (from boss_ragnaros.cpp):
    //   20566 Wrath of Ragnaros — PBAoE knockback, melee gets bounced 40y
    //   20565 Magma Blast — ranged hit when no melee in range (keep one melee close)
    //   21154 Might of Ragnaros — targeted knockback
    //   21859 Submerge — every ~3 min he goes underground, Sons of Flame spawn
    //   20568 Emerge — comes back up after Sons phase
    class RagnarosFightStrategy : public Strategy
    {
    public:
        RagnarosFightStrategy(PlayerbotAI* ai) : Strategy(ai) {}
        std::string getName() override { return "ragnaros"; }

    private:
        void InitCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitNonCombatTriggers(std::list<TriggerNode*>& triggers) override;
        void InitDeadTriggers(std::list<TriggerNode*>& triggers) override;
        void InitReactionTriggers(std::list<TriggerNode*>& triggers) override;
        void InitCombatMultipliers(std::list<Multiplier*>& multipliers) override;
    };
}