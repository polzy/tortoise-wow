#pragma once
#include "DungeonTriggers.h"
#include "GenericTriggers.h"

namespace ai
{
    class BlackwingLairEnterDungeonTrigger : public EnterDungeonTrigger
    {
    public:
        BlackwingLairEnterDungeonTrigger(PlayerbotAI* ai) : EnterDungeonTrigger(ai, "enter blackwing lair", "blackwing lair", 469) {}
    };

    class BlackwingLairLeaveDungeonTrigger : public LeaveDungeonTrigger
    {
    public:
        BlackwingLairLeaveDungeonTrigger(PlayerbotAI* ai) : LeaveDungeonTrigger(ai, "leave blackwing lair", "blackwing lair", 469) {}
    };

    class SuppressionDeviceNeedStealthTrigger : public Trigger
    {
    public:
        SuppressionDeviceNeedStealthTrigger(PlayerbotAI* ai) : Trigger(ai, "suppression device need stealth", 1) {}

        bool IsActive() override
        {
            if (bot->getClass() != CLASS_ROGUE)
                return false;

            if (ai->HasAura("stealth", bot))
                return false;

            std::list<GuidPosition> gos = AI_VALUE(std::list<GuidPosition>, "go usable filter::go trapped filter::entry filter::{gos in sight,suppression devices}");
            return !gos.empty();
        }
    };

    class SuppressionDeviceInSightTrigger : public Trigger
    {
    public:
        SuppressionDeviceInSightTrigger(PlayerbotAI* ai) : Trigger(ai, "suppression device in sight", 1) {}

        bool IsActive() override
        {
            if (bot->getClass() != CLASS_ROGUE)
                return false;

            std::list<GuidPosition> gosInSight = AI_VALUE(std::list<GuidPosition>, "go usable filter::go trapped filter::entry filter::{gos in sight,suppression devices}");
            std::list<GuidPosition> gosClose = AI_VALUE(std::list<GuidPosition>, "entry filter::{gos close,suppression devices}");
            
            return !gosInSight.empty() && gosClose.empty();
        }
    };

    class SuppressionDeviceCloseTrigger : public Trigger
    {
    public:
        SuppressionDeviceCloseTrigger(PlayerbotAI* ai) : Trigger(ai, "suppression device close", 1) {}

        bool IsActive() override
        {
            if (bot->getClass() != CLASS_ROGUE)
                return false;

            std::list<GuidPosition> gos = AI_VALUE(std::list<GuidPosition>, "go usable filter::go trapped filter::entry filter::{gos close,suppression devices}");
            return !gos.empty();
        }
    };

    // --- Vaelastrasz (13020) ---
    class VaelastraszStartFightTrigger : public StartBossFightTrigger
    {
    public:
        VaelastraszStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start vaelastrasz fight", "vaelastrasz", 13020) {}
    };
    class VaelastraszEndFightTrigger : public EndBossFightTrigger
    {
    public:
        VaelastraszEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end vaelastrasz fight", "vaelastrasz", 13020) {}
    };

    // Burning Adrenaline (23620/23478): bot has the aura, will explode on death.
    // Affected bot should move away from raid to minimize splash damage.
    class VaelastraszBurningAdrenalineTrigger : public Trigger
    {
    public:
        VaelastraszBurningAdrenalineTrigger(PlayerbotAI* ai) : Trigger(ai, "vaelastrasz burning adrenaline", 1) {}
        bool IsActive() override
        {
            return ai->HasAura(23620, bot) || ai->HasAura(23478, bot) || ai->HasAura(23644, bot);
        }
    };

    // --- Broodlord Lashlayer ---
    class BroodlordStartFightTrigger : public StartBossFightTrigger
    {
    public:
        BroodlordStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start broodlord fight", "broodlord", 12017) {}
    };
    class BroodlordEndFightTrigger : public EndBossFightTrigger
    {
    public:
        BroodlordEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end broodlord fight", "broodlord", 12017) {}
    };

    // --- Chromaggus ---
    class ChromaggusStartFightTrigger : public StartBossFightTrigger
    {
    public:
        ChromaggusStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start chromaggus fight", "chromaggus", 14020) {}
    };
    class ChromaggusEndFightTrigger : public EndBossFightTrigger
    {
    public:
        ChromaggusEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end chromaggus fight", "chromaggus", 14020) {}
    };

    // Brood Affliction stack of 5 = forced raid-MC via Chromatic Mutation (23174).
    // Trigger fires when the bot has 4+ of the 5 affliction debuffs so dispellers
    // know to clean a color before the lethal 5th lands.
    class ChromaggusAfflictionDangerTrigger : public Trigger
    {
    public:
        ChromaggusAfflictionDangerTrigger(PlayerbotAI* ai) : Trigger(ai, "chromaggus affliction danger", 2) {}
        bool IsActive() override
        {
            int count = 0;
            if (ai->HasAura(23153, bot)) ++count;  // Blue
            if (ai->HasAura(23154, bot)) ++count;  // Black
            if (ai->HasAura(23155, bot)) ++count;  // Red
            if (ai->HasAura(23170, bot)) ++count;  // Bronze
            if (ai->HasAura(23169, bot)) ++count;  // Green
            return count >= 4;
        }
    };
}