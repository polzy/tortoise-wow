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

    // Ouro Sweep cone — when Ouro (15517) is alive AND above ground
    // (NOT carrying SUBMERGE_VISUAL aura 26063), Sweep is a frontal
    // melee cone. Non-tank melee bots within 8y need to stay BEHIND
    // him. We detect "above ground" by inverting the boss-aura check;
    // when the trigger fires, the bot is in danger zone.
    //   Boss alive ✓
    //   Bot is melee (current target is Ouro, in melee range)
    //   Bot is NOT a tank (tanks hold position to keep boss facing)
    //   Ouro does NOT have aura 26063 (= above ground = Sweep active)
    // Action: flee from boss for a tick to get out of frontal cone.
    // SD2 boss_ouro.cpp:45 SPELL_SUBMERGE_VISUAL = 26063.
    class OuroSweepConeNonTankTrigger : public Trigger
    {
    public:
        OuroSweepConeNonTankTrigger(PlayerbotAI* ai) : Trigger(ai, "ouro sweep cone non tank", 2) {}
        bool IsActive() override
        {
            if (!bot || ai->IsTank(bot)) return false;
            std::list<Unit*> bosses;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 15517 /* NPC_OURO */, 10.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(bosses, check);
            Cell::VisitAllObjects(bot, searcher, 10.0f);
            for (Unit* b : bosses)
            {
                if (!b || !b->IsAlive()) continue;
                // Submerged → no Sweep, safe.
                if (b->HasAura(26063)) return false;
                // Above ground AND we're in melee range → at risk of Sweep.
                return true;
            }
            return false;
        }
    };

    // Framework #8 wire — Twin Emperors teleport swap.
    //
    // Vek'lor (15276) and Vek'nilash (15275) teleport-swap every ~30s.
    // SD2 cross-ref (boss_twinemperors.cpp:37-39):
    //   SPELL_TWIN_TELEPORT_SCRIPT  = 799  (script effect, no-op)
    //   SPELL_TWIN_TELEPORT_MSG     = 800  (CTRA watches for this)
    //   SPELL_TWIN_TELEPORT_VISUAL  = 26638
    // The MSG (800) fires ~5s before the actual swap, giving tanks the
    // best heads-up. The VISUAL (26638) is concurrent with the swap.
    // We check BOTH so we still catch the swap if 800 isn't in the
    // Turtle DB. EITHER trigger fires → pair-swap action.
    class TwinEmperorsTeleportCastTrigger : public Trigger
    {
    public:
        TwinEmperorsTeleportCastTrigger(PlayerbotAI* ai) : Trigger(ai, "twin emperors teleport cast", 1) {}
        bool IsActive() override
        {
            // Try the early-warning MSG first; fall back to visual on either twin.
            return AI_VALUE2(bool, "boss is casting", "15275:800")
                || AI_VALUE2(bool, "boss is casting", "15276:800")
                || AI_VALUE2(bool, "boss is casting", "15275:26638")
                || AI_VALUE2(bool, "boss is casting", "15276:26638");
        }
    };

    // The Prophet Skeram split phase — at 75%/50%/25% HP Skeram teleports
    // and spawns 2 illusionary clones (entry 15263 same as boss; clones
    // share the entry but get full HP on spawn). Bots stuck on a clone
    // when the real boss is elsewhere just hit a sponge that vanishes
    // on the next split. Strategy: when ANY 15263 has HP%<33, retarget
    // to whichever 15263 is closest (the bot's existing closest-enemy
    // logic naturally picks up). The trigger fires the retarget chain
    // every tick during the low-HP window. Framework #3 BossHpPctValue
    // primitive — entry 15263 scan within 100y, HP% gate.
    // ScriptDev2 boss_skeram.cpp NPC_THE_PROPHET_SKERAM = 15263.
    class SkeramSplitPhaseTrigger : public Trigger
    {
    public:
        SkeramSplitPhaseTrigger(PlayerbotAI* ai) : Trigger(ai, "skeram split phase", 5) {}
        bool IsActive() override
        {
            // Find any 15263 with HP%<33 — that's the real Skeram in low
            // phase. Clones share the entry but are spawned alongside the
            // boss so multiple 15263 instances mean we're mid-split.
            std::list<Unit*> matches;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 15263, 100.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(matches, check);
            Cell::VisitAllObjects(bot, searcher, 100.0f);

            int liveCount = 0;
            bool lowHp = false;
            for (Unit* u : matches)
            {
                if (!u || !u->IsAlive()) continue;
                ++liveCount;
                if (u->GetMaxHealth() > 0 &&
                    (float)u->GetHealth() / (float)u->GetMaxHealth() < 0.33f)
                    lowHp = true;
            }
            // Trigger during the split (>=2 entities) AND when at least one
            // is low — the bot's current target may be a clone that's
            // already at full HP and a different one needs focus.
            return liveCount >= 2 && lowHp;
        }
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

    // Twin Emperors Unbalancing Strike (26613) — Veknilash melee debuff, strips
    // armor stacks on MT. At 3+ stacks the MT takes too much physical, OT
    // taunts. Framework #4 pattern.
    class TwinEmperorsUnbalancingStrikeSwapTrigger : public PartyOtherTankHasAuraStacksTrigger
    {
    public:
        TwinEmperorsUnbalancingStrikeSwapTrigger(PlayerbotAI* ai)
            : PartyOtherTankHasAuraStacksTrigger(ai, "twin emperors unbalancing strike swap", 26613, 3, 1) {}
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
