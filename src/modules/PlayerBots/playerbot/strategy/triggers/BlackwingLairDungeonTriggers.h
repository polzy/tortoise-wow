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

    // --- Razorgore the Untamed (12435) ---
    class RazorgoreStartFightTrigger : public StartBossFightTrigger
    {
    public:
        RazorgoreStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start razorgore fight", "razorgore", 12435) {}
    };
    class RazorgoreEndFightTrigger : public EndBossFightTrigger
    {
    public:
        RazorgoreEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end razorgore fight", "razorgore", 12435) {}
    };

    // Phase 1 — Razorgore is mind-controlled by a player via the Possess Orb.
    // ScriptDev2 SPELL_POSSESS_ORB = 19832 is the aura actually applied to
    // Razorgore (see boss_razorgore.cpp:44, instance_blackwing_lair.cpp:622/659
    // — `pCreature->HasAura(...)` checks this id). SPELL_POSSESS (23014) is the
    // CHANNEL_SPELL value used cosmetically on the trigger creature, NOT a
    // real aura on Razorgore. Earlier rev used 23014 → trigger never fired,
    // bots DPS'd Razorgore in P1 → wipe. Corrected to 19832.
    class RazorgorePhase1Trigger : public Trigger
    {
    public:
        RazorgorePhase1Trigger(PlayerbotAI* ai) : Trigger(ai, "razorgore phase 1") {}
        bool IsActive() override
        {
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 12435, 80.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 80.0f);
            for (Unit* u : units)
            {
                if (!u || !u->IsAlive()) continue;
                if (u->HasAura(19832))
                    return true;
            }
            return false;
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

    // --- Firemaw (11983) ---
    class FiremawStartFightTrigger : public StartBossFightTrigger
    {
    public:
        FiremawStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start firemaw fight", "firemaw", 11983) {}
    };
    class FiremawEndFightTrigger : public EndBossFightTrigger
    {
    public:
        FiremawEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end firemaw fight", "firemaw", 11983) {}
    };

    // --- Ebonroc (14601) ---
    class EbonrocStartFightTrigger : public StartBossFightTrigger
    {
    public:
        EbonrocStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start ebonroc fight", "ebonroc", 14601) {}
    };
    class EbonrocEndFightTrigger : public EndBossFightTrigger
    {
    public:
        EbonrocEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end ebonroc fight", "ebonroc", 14601) {}
    };

    // --- Flamegor (11981) ---
    class FlamegorStartFightTrigger : public StartBossFightTrigger
    {
    public:
        FlamegorStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start flamegor fight", "flamegor", 11981) {}
    };
    class FlamegorEndFightTrigger : public EndBossFightTrigger
    {
    public:
        FlamegorEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end flamegor fight", "flamegor", 11981) {}
    };

    // Flamegor casts Frenzy (23342) on himself — a self-buff +50% attack speed +
    // damage. Interruptible by hunter Tranquilizing Shot (19801). Trigger fires
    // when a live Flamegor within 40y has the aura.
    class FlamegorFrenzyTrigger : public Trigger
    {
    public:
        FlamegorFrenzyTrigger(PlayerbotAI* ai) : Trigger(ai, "flamegor frenzy") {}
        bool IsActive() override
        {
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 11981, 40.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 40.0f);
            for (Unit* u : units)
            {
                if (!u || !u->IsAlive()) continue;
                if (u->HasAura(23342))
                    return true;
            }
            return false;
        }
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

    // --- Nefarian (11583) ---
    // Strategy activates only in P2 (Nefarian himself spawns after the 40-drake
    // channelling phase ends). The Start/End fight triggers therefore implicitly
    // gate on Nefarian's presence in world.
    class NefarianStartFightTrigger : public StartBossFightTrigger
    {
    public:
        NefarianStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start nefarian fight", "nefarian", 11583) {}
    };
    class NefarianEndFightTrigger : public EndBossFightTrigger
    {
    public:
        NefarianEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end nefarian fight", "nefarian", 11583) {}
    };

    // Bellowing Roar (22686) — 8s fear AoE, same mechanic as Onyxia P3. Detection
    // is bot-side via the aura on bot (already fear-debuffed; the cure has 0
    // effect at that point, but the trigger drives the same Tremor Totem / Will
    // of the Forsaken / Berserker Rage breaks the class strategies already wire).
    class NefarianBellowingRoarTrigger : public Trigger
    {
    public:
        NefarianBellowingRoarTrigger(PlayerbotAI* ai) : Trigger(ai, "nefarian bellowing roar") {}
        bool IsActive() override { return ai->HasAura(22686, bot); }
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