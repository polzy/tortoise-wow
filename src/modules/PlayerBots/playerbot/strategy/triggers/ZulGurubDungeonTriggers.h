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

    // Aspects cast on Hakkar's tank — magic debuffs, dispelable.
    //   Jeklik  24687: 4s silence
    //   Venoxis 24688: poison (cure poison strips it; dispel magic also works)
    //   Marli   24686: 5s stun
    // We collapse the three "magic" aspects into one trigger so a single
    // dispel chain (priest/paladin) catches them all.
    class HakkarMagicAspectTrigger : public Trigger
    {
    public:
        HakkarMagicAspectTrigger(PlayerbotAI* ai) : Trigger(ai, "hakkar magic aspect", 1) {}
        bool IsActive() override
        {
            return ai->HasAura(24686, bot) || ai->HasAura(24687, bot);
        }
    };

    // Venoxis aspect (24688) is poison-typed — needs a poison cure
    // (druid 'cure poison', shaman 'cure poison', paladin 'cleanse').
    class HakkarVenoxisAspectTrigger : public Trigger
    {
    public:
        HakkarVenoxisAspectTrigger(PlayerbotAI* ai) : Trigger(ai, "hakkar venoxis aspect", 1) {}
        bool IsActive() override { return ai->HasAura(24688, bot); }
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
