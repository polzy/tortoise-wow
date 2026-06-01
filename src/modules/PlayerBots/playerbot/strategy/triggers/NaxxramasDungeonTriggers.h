#pragma once
#include "DungeonTriggers.h"
#include "GenericTriggers.h"

namespace ai
{
    class NaxxramasEnterDungeonTrigger : public EnterDungeonTrigger
    {
    public:
        NaxxramasEnterDungeonTrigger(PlayerbotAI* ai) : EnterDungeonTrigger(ai, "enter naxxramas", "naxxramas", 533) {}
    };

    class NaxxramasLeaveDungeonTrigger : public LeaveDungeonTrigger
    {
    public:
        NaxxramasLeaveDungeonTrigger(PlayerbotAI* ai) : LeaveDungeonTrigger(ai, "leave naxxramas", "naxxramas", 533) {}
    };

    class FourHorsemanStartFightTrigger : public StartBossFightTrigger
    {
    public:
        FourHorsemanStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start four horseman fight", "four horseman", 16062) {}
    };

    class FourHorsemanEndFightTrigger : public EndBossFightTrigger
    {
    public:
        FourHorsemanEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end four horseman fight", "four horseman", 16062) {}
    };

    // --- Patchwerk (16028) ---
    class PatchwerkStartFightTrigger : public StartBossFightTrigger
    {
    public:
        PatchwerkStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start patchwerk fight", "patchwerk", 16028) {}
    };
    class PatchwerkEndFightTrigger : public EndBossFightTrigger
    {
    public:
        PatchwerkEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end patchwerk fight", "patchwerk", 16028) {}
    };

    // --- Loatheb (16011) ---
    class LoathebStartFightTrigger : public StartBossFightTrigger
    {
    public:
        LoathebStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start loatheb fight", "loatheb", 16011) {}
    };
    class LoathebEndFightTrigger : public EndBossFightTrigger
    {
    public:
        LoathebEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end loatheb fight", "loatheb", 16011) {}
    };

    // --- Kel'Thuzad (15990) ---
    class KelThuzadStartFightTrigger : public StartBossFightTrigger
    {
    public:
        KelThuzadStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start kelthuzad fight", "kel'thuzad", 15990) {}
    };
    class KelThuzadEndFightTrigger : public EndBossFightTrigger
    {
    public:
        KelThuzadEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end kelthuzad fight", "kel'thuzad", 15990) {}
    };

    // Mana Detonation (27819): magic debuff chained to 3 targets that drains
    // mana and explodes for AoE. Group-scan so non-affected priests/paladins
    // still trigger their dispel chain.
    class KelThuzadManaDetonationTrigger : public PartyHasAuraBySpellIdTrigger
    {
    public:
        KelThuzadManaDetonationTrigger(PlayerbotAI* ai)
            : PartyHasAuraBySpellIdTrigger(ai, "kelthuzad mana detonation", 27819, 1) {}
    };

    // --- Sapphiron (15989) ---
    class SapphironStartFightTrigger : public StartBossFightTrigger
    {
    public:
        SapphironStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start sapphiron fight", "sapphiron", 15989) {}
    };
    class SapphironEndFightTrigger : public EndBossFightTrigger
    {
    public:
        SapphironEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end sapphiron fight", "sapphiron", 15989) {}
    };

    // Life Drain (28542) — magic debuff, dispelable. Hits 5 random raid
    // members per cast. Group-scan: if the 5 don't include a priest/paladin,
    // self-aura check would skip the dispel chain entirely.
    class SapphironLifeDrainTrigger : public PartyHasAuraBySpellIdTrigger
    {
    public:
        SapphironLifeDrainTrigger(PlayerbotAI* ai)
            : PartyHasAuraBySpellIdTrigger(ai, "sapphiron life drain", 28542, 1) {}
    };

    // Anub'Rekhan Locust Swarm (28785) — self-buff on Anub that AOE-damages
    // anyone within ~20y for 20s. Cast every 80-120s. Bots within 25y must
    // move 30y+ out. Trigger fires when live Anub (15956) within 100y has
    // the aura. ScriptDev2 boss_anubrekhan.cpp:21 SPELL_LOCUSTSWARM.
    class AnubRekhanLocustSwarmTrigger : public Trigger
    {
    public:
        AnubRekhanLocustSwarmTrigger(PlayerbotAI* ai) : Trigger(ai, "anubrekhan locust swarm", 1) {}
        bool IsActive() override
        {
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 15956, 100.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 100.0f);
            for (Unit* u : units)
            {
                if (!u || !u->IsAlive()) continue;
                if (u->HasAura(28785)) return true;
            }
            return false;
        }
    };

    // Heigan Plague Fissure (533001) — eruption fissure creatures spawn in
    // a zone pattern every ~3s. Each fissure casts SPELL_ERUPTION (29371)
    // on itself dealing AOE damage in its zone, then despawns ~50ms later.
    // Reactive bot logic: any fissure within 15y → move out. Won't perfectly
    // dodge the dance (predictive timing requires script-side info) but
    // significantly improves survivability vs no avoidance.
    class HeiganFissureNearbyTrigger : public CloseToCreatureTrigger
    {
    public:
        HeiganFissureNearbyTrigger(PlayerbotAI* ai)
            : CloseToCreatureTrigger(ai, "heigan fissure nearby", 533001, 15.0f) {}
    };

    // Thaddius polarity charges (Positive 28059 / Negative 28084) applied
    // to the whole raid every ~30s via Polarity Shift (28089). Bots with the
    // same polarity must stack to absorb each other's charge tick; bots with
    // different polarity must spread or take amped damage. Trigger fires on
    // self carrying either aura → action computes same-polarity centroid.
    class ThaddiusHasPolarityTrigger : public Trigger
    {
    public:
        ThaddiusHasPolarityTrigger(PlayerbotAI* ai) : Trigger(ai, "thaddius has polarity", 1) {}
        bool IsActive() override
        {
            return ai->HasAura(28059, bot) || ai->HasAura(28084, bot);
        }
    };

    // Frost Breath (28524) — 7s cast AOE in air phase. Blocked by LOS via
    // GO_ICEBLOCK (181247) spawned where icebolted players stood. Trigger
    // fires when a live Sapphiron within 100y is casting Frost Breath.
    // ScriptDev2 boss_sapphiron.cpp:35 SPELL_FROST_BREATH.
    class SapphironFrostBreathTrigger : public Trigger
    {
    public:
        SapphironFrostBreathTrigger(PlayerbotAI* ai) : Trigger(ai, "sapphiron frost breath", 1) {}
        bool IsActive() override
        {
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 15989, 100.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 100.0f);
            for (Unit* u : units)
            {
                if (!u || !u->IsAlive()) continue;
                if (Spell* sp = u->GetCurrentSpell(CURRENT_GENERIC_SPELL))
                {
                    if (sp->m_spellInfo && sp->m_spellInfo->Id == 28524)
                        return true;
                }
            }
            return false;
        }
    };

    // --- Maexxna Necrotic Poison (28776) — poison, 90% healing reduction.
    // Single-target on the main tank (warrior — no self-cleanse). Group-scan
    // so the druid/shaman/paladin fires their cure chain. ---
    class MaexxnaNecroticPoisonTrigger : public PartyHasAuraBySpellIdTrigger
    {
    public:
        MaexxnaNecroticPoisonTrigger(PlayerbotAI* ai)
            : PartyHasAuraBySpellIdTrigger(ai, "maexxna necrotic poison", 28776, 1) {}
    };

    // --- Faerlina Poison Bolt Volley (28796) — poison, raid-wide AOE.
    // Self-aura is fine (many bots affected, dispeller ticks too). ---
    class FaerlinaPoisonBoltTrigger : public Trigger
    {
    public:
        FaerlinaPoisonBoltTrigger(PlayerbotAI* ai) : Trigger(ai, "faerlina poison bolt", 1) {}
        bool IsActive() override { return ai->HasAura(28796, bot); }
    };

    // --- Noth Curse of Plaguebringer (29213) — curse on 3 random raid
    // members per cast. Group-scan: the 3 might not include a druid/mage. ---
    class NothCursePlaguebringerTrigger : public PartyHasAuraBySpellIdTrigger
    {
    public:
        NothCursePlaguebringerTrigger(PlayerbotAI* ai)
            : PartyHasAuraBySpellIdTrigger(ai, "noth curse plaguebringer", 29213, 1) {}
    };

    // --- Four Horsemen mark stacks ---
    // Marks (28832 Korth'azz fire / 28833 Blaumeux shadow / 28834 Mograine
    // unholy / 28835 Zeliek holy) stack on every Horseman cast (~12s). At 4
    // stacks the next stack lands as 5 and the dmg is lethal — players swap to
    // the opposite Horseman to drop stacks. Bot side: detect ≥3 stacks of any
    // mark on bot and route into the dispel chain.
    // Source: ScriptDev2 boss_four_horsemen.cpp SPELL_MARK_OF_*.
    class FourHorsemenMarkDangerTrigger : public Trigger
    {
    public:
        FourHorsemenMarkDangerTrigger(PlayerbotAI* ai) : Trigger(ai, "four horsemen mark danger", 2) {}
        bool IsActive() override
        {
            // Mark auras stack; we don't have a stack-count helper exposed via
            // the bot AI surface here. Detection collapses to "has any of the
            // four mark auras" — the trigger then fires the dispel chain
            // every tick the mark is present. Dispellers naturally throttle
            // (cooldown / global) so this stays cheap and reactive.
            return ai->HasAura(28832, bot) || ai->HasAura(28833, bot)
                || ai->HasAura(28834, bot) || ai->HasAura(28835, bot);
        }
    };
}