#pragma once
#include "DungeonTriggers.h"
#include "GenericTriggers.h"

// Zul'Gurub (ZG, map 309). Spell + entry IDs lifted from
// `src/scripts/dungeons/zulgurub/boss_*.cpp`. Only Hakkar has bot-actionable
// mechanics wired here for now (Aspects + Thekal enrage on boss).
// High Priests (Venoxis/Jeklik/Marli/Thekal/Arlokk) and Mandokir / Jin'do
// require multi-phase or single-target-pull frameworks not yet exposed.

namespace ai
{
    class ZulGurubEnterDungeonTrigger : public EnterDungeonTrigger
    {
    public:
        ZulGurubEnterDungeonTrigger(PlayerbotAI* ai) : EnterDungeonTrigger(ai, "enter zulgurub", "zulgurub", 309) {}
    };

    class ZulGurubLeaveDungeonTrigger : public LeaveDungeonTrigger
    {
    public:
        ZulGurubLeaveDungeonTrigger(PlayerbotAI* ai) : LeaveDungeonTrigger(ai, "leave zulgurub", "zulgurub", 309) {}
    };

    // --- Hakkar the Soulflayer (14834) ---
    class HakkarStartFightTrigger : public StartBossFightTrigger
    {
    public:
        HakkarStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start hakkar fight", "hakkar", 14834) {}
    };
    class HakkarEndFightTrigger : public EndBossFightTrigger
    {
    public:
        HakkarEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end hakkar fight", "hakkar", 14834) {}
    };

    // Aspects cast on Hakkar's CURRENT VICTIM (the tank) — magic debuffs.
    //   Marli   24686: 5s stun  (tank can't act)
    //   Jeklik  24687: 4s silence (tank can't cast — and warriors can't
    //                  dispel themselves anyway)
    //   Venoxis 24688: poison (cure poison strips it; dispel magic also works)
    // Single-target on tank → group-scan so a priest/paladin elsewhere fires
    // their dispel chain.
    class HakkarMagicAspectTrigger : public Trigger
    {
    public:
        HakkarMagicAspectTrigger(PlayerbotAI* ai) : Trigger(ai, "hakkar magic aspect", 1) {}
        bool IsActive() override
        {
            Player* bot = ai->GetBot();
            if (!bot) return false;
            if (ai->HasAura(24686, bot) || ai->HasAura(24687, bot)) return true;

            Group* group = bot->GetGroup();
            if (!group) return false;

            for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
            {
                Player* member = ref->getSource();
                if (!member || member == bot) continue;
                if (member->GetMapId() != bot->GetMapId()) continue;
                if (!member->IsAlive()) continue;
                if (ai->HasAura(24686, member) || ai->HasAura(24687, member)) return true;
            }
            return false;
        }
    };

    // Venoxis aspect (24688) is poison-typed — needs a poison cure
    // (druid/shaman 'cure poison', paladin 'cleanse poison'). Cast on the
    // tank → group-scan.
    class HakkarVenoxisAspectTrigger : public PartyHasAuraBySpellIdTrigger
    {
    public:
        HakkarVenoxisAspectTrigger(PlayerbotAI* ai)
            : PartyHasAuraBySpellIdTrigger(ai, "hakkar venoxis aspect", 24688, 1) {}
    };

    // Aspect of Thekal (24689) is cast on Hakkar himself — short enrage that
    // hunters can strip via Tranquilizing Shot. Detection: a live Hakkar
    // within 40y has the aura.
    // High Priest Thekal mini-boss Hazzarah Sleep (24664) — magic, dispelable,
    // single-target. The slept bot can't act (incapacitated) so the trigger
    // must group-scan: a healer or paladin elsewhere fires the dispel chain.
    // ScriptDev2 boss_hazzarah.cpp.
    class HazzarahSleepTrigger : public PartyHasAuraBySpellIdTrigger
    {
    public:
        HazzarahSleepTrigger(PlayerbotAI* ai)
            : PartyHasAuraBySpellIdTrigger(ai, "hazzarah sleep", 24664, 1) {}
    };

    class HakkarThekalEnrageTrigger : public Trigger
    {
    public:
        HakkarThekalEnrageTrigger(PlayerbotAI* ai) : Trigger(ai, "hakkar thekal enrage") {}
        bool IsActive() override
        {
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 14834, 40.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 40.0f);
            for (Unit* u : units)
            {
                if (!u || !u->IsAlive()) continue;
                if (u->HasAura(24689))
                    return true;
            }
            return false;
        }
    };
}
