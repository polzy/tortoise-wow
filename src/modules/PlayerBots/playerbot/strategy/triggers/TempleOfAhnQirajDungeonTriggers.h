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

    // Wyvern Sting (26180) — cast on tank during Huhuran berserk phase
    // (<30% HP). Sleep, magic-school dispelable. Slept tank can't act and
    // can't self-cleanse → group-scan so a priest/paladin strips it before
    // the tank dies. ScriptDev2 boss_huhuran.cpp:39 SPELL_WYVERNSTING.
    class HuhuranWyvernStingTrigger : public PartyHasAuraBySpellIdTrigger
    {
    public:
        HuhuranWyvernStingTrigger(PlayerbotAI* ai)
            : PartyHasAuraBySpellIdTrigger(ai, "huhuran wyvern sting", 26180, 1) {}
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

    // Bug Trio — Kri Toxic Volley (25812) — POISON-school AOE on raid every
    // cycle (40y radius around the boss). Dispelable with cure poison
    // (druid/shaman) / cleanse poison (paladin). Self-aura check is fine —
    // it's raid-wide so any dispel-capable bot ticks alongside.
    // ScriptDev2 boss_bug_trio.cpp:23 SPELL_TOXIC_VOLLEY.
    class KriToxicVolleyTrigger : public Trigger
    {
    public:
        KriToxicVolleyTrigger(PlayerbotAI* ai) : Trigger(ai, "kri toxic volley", 1) {}
        bool IsActive() override { return ai->HasAura(25812, bot); }
    };

    // Bug Trio — Yauj Fear (19408 placeholder for 25807). Magic-school,
    // dispelable. Feared bot can't act → group-scan so a non-feared healer
    // fires the dispel chain. ScriptDev2 boss_bug_trio.cpp:29 SPELL_FEAR.
    class YaujFearTrigger : public PartyHasAuraBySpellIdTrigger
    {
    public:
        YaujFearTrigger(PlayerbotAI* ai)
            : PartyHasAuraBySpellIdTrigger(ai, "yauj fear", 19408, 1) {}
    };

    // Viscidus (15299) — needs 200 frost hits to freeze, then ~100 physical
    // hits to shatter. Trigger fires when bot's current target is Viscidus
    // and he's NOT frozen yet (SPELL_VISCIDUS_FREEZE 25937 absent). Caster
    // bots route to frost spam; melee bots route to normal attack.
    class ViscidusFrostPhaseTrigger : public Trigger
    {
    public:
        ViscidusFrostPhaseTrigger(PlayerbotAI* ai) : Trigger(ai, "viscidus frost phase", 1) {}
        bool IsActive() override
        {
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 15299, 60.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 60.0f);
            for (Unit* u : units)
            {
                if (!u || !u->IsAlive()) continue;
                // Frozen / shrunk / membrane states — past the freeze threshold,
                // melee shatter phase, no more frost needed.
                if (u->HasAura(25937) || u->HasAura(25893)) return false;
                return true;  // Viscidus alive and not frozen → frost phase.
            }
            return false;
        }
    };

    // Twin Emperors — Veklor's Mutate Bug (802) turns the affected player
    // into a Qiraji bug for ~8s, then they detonate (Explodebug 804) for AOE
    // damage. Mutate Bug is Magic-school, dispelable. The transformed bot is
    // polymorphed and CAN'T act on it → group-scan obligatoire so a healer
    // or paladin elsewhere strips the form before the explosion lands on the
    // raid. ScriptDev2 boss_twinemperors.cpp:47 SPELL_MUTATE_BUG.
    class TwinEmperorsMutateBugTrigger : public PartyHasAuraBySpellIdTrigger
    {
    public:
        TwinEmperorsMutateBugTrigger(PlayerbotAI* ai)
            : PartyHasAuraBySpellIdTrigger(ai, "twin emperors mutate bug", 802, 1) {}
    };

    // Bug Trio — Kri's Toxic Vapors cloud creature (15933) spawns on Kri's
    // death (boss_bug_trio.cpp:238 SPELL_SUMMON_CLOUD=25786). Anyone within
    // ~10y of the cloud takes heavy poison ticks. 12y threshold so move-out
    // completes before the next tick lands.
    class KriToxicCloudNearbyTrigger : public CloseToCreatureTrigger
    {
    public:
        KriToxicCloudNearbyTrigger(PlayerbotAI* ai)
            : CloseToCreatureTrigger(ai, "kri toxic cloud nearby", 15933, 12.0f) {}
    };
}
