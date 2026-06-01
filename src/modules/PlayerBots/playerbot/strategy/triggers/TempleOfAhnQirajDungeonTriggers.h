#pragma once
#include "DungeonTriggers.h"
#include "GenericTriggers.h"

// AQ40 (Temple of Ahn'Qiraj, map 531). Boss + spell IDs are lifted verbatim
// from `src/scripts/dungeons/temple_of_ahnqiraj/boss_*.cpp`. Only the bosses
// with bot-actionable mechanics are wired here — Skeram / Twin Emperors /
// Ouro / C'Thun / Viscidus / Bug Trio have phase mechanics (mind control
// retake, mutate swap, burrow/dirt mound, eye phase, frozen/shatter,
// healer-MC) that require frameworks (raid-member-target pinning, multi-tank
// swap, GO interaction) we don't currently expose to the strategy layer.

namespace ai
{
    class TempleOfAhnQirajEnterDungeonTrigger : public EnterDungeonTrigger
    {
    public:
        TempleOfAhnQirajEnterDungeonTrigger(PlayerbotAI* ai) : EnterDungeonTrigger(ai, "enter temple of ahnqiraj", "temple of ahnqiraj", 531) {}
    };

    class TempleOfAhnQirajLeaveDungeonTrigger : public LeaveDungeonTrigger
    {
    public:
        TempleOfAhnQirajLeaveDungeonTrigger(PlayerbotAI* ai) : LeaveDungeonTrigger(ai, "leave temple of ahnqiraj", "temple of ahnqiraj", 531) {}
    };

    // --- Battleguard Sartura (15516) ---
    class SarturaStartFightTrigger : public StartBossFightTrigger
    {
    public:
        SarturaStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start sartura fight", "sartura", 15516) {}
    };
    class SarturaEndFightTrigger : public EndBossFightTrigger
    {
    public:
        SarturaEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end sartura fight", "sartura", 15516) {}
    };

    // Whirlwind (26083) self-cast 15s, threat resets every cast and Sartura
    // picks random targets in melee range while it spins. Ranged/heal stay
    // 12y+ from her to avoid being eligible for the random-threat swap.
    class SarturaTooCloseTrigger : public CloseToCreatureTrigger
    {
    public:
        SarturaTooCloseTrigger(PlayerbotAI* ai) : CloseToCreatureTrigger(ai, "sartura too close", 15516, 12.0f) {}
    };

    // --- Princess Huhuran (15509) ---
    class HuhuranStartFightTrigger : public StartBossFightTrigger
    {
    public:
        HuhuranStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start huhuran fight", "huhuran", 15509) {}
    };
    class HuhuranEndFightTrigger : public EndBossFightTrigger
    {
    public:
        HuhuranEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end huhuran fight", "huhuran", 15509) {}
    };

    // Huhuran Frenzy (26051) self-buff — interruptible by hunter Tranquilizing
    // Shot (19801). Same pattern as Flamegor Frenzy. Detects a live Huhuran
    // within 40y carrying the aura.
    class HuhuranFrenzyTrigger : public Trigger
    {
    public:
        HuhuranFrenzyTrigger(PlayerbotAI* ai) : Trigger(ai, "huhuran frenzy") {}
        bool IsActive() override
        {
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 15509, 40.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 40.0f);
            for (Unit* u : units)
            {
                if (!u || !u->IsAlive()) continue;
                if (u->HasAura(26051))
                    return true;
            }
            return false;
        }
    };

    // Noxious Poison (26053) — nature debuff on a random raid member. Druids
    // 'remove poison' (or shamans 'cure poison') strip it. Bot-side detection
    // Single-target on the current victim (tank — typically warrior with no
    // self-cleanse). Group-scan so druid/shaman/paladin fires the cure chain.
    class HuhuranNoxiousPoisonTrigger : public PartyHasAuraBySpellIdTrigger
    {
    public:
        HuhuranNoxiousPoisonTrigger(PlayerbotAI* ai)
            : PartyHasAuraBySpellIdTrigger(ai, "huhuran noxious poison", 26053, 1) {}
    };

    // The Prophet Skeram True Fulfillment (785) — MIND-CONTROLS the closest
    // raid member every cycle. Spell is Magic-school, dispelable. Quick kick
    // from any priest 'dispel magic' / paladin 'cleanse magic' returns the
    // MC'd player to the raid before they hit anyone.
    // ScriptDev2 boss_skeram.cpp:21 SPELL_TRUE_FULFILLMENT = 785.
    //
    // Note: scans the WHOLE raid for the aura — the MC'd bot is charmed and
    // can't run its own dispel logic, so the trigger must fire for OTHER
    // (un-MC'd) bots in the group so a healer/paladin can react.
    class SkeramTrueFulfillmentTrigger : public PartyHasAuraBySpellIdTrigger
    {
    public:
        SkeramTrueFulfillmentTrigger(PlayerbotAI* ai)
            : PartyHasAuraBySpellIdTrigger(ai, "skeram true fulfillment", 785, 1) {}
    };
}
