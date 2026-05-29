#pragma once
#include "DungeonTriggers.h"
#include "GenericTriggers.h"
#include <vector>

// Pro-engage triggers — fire when boss-related ADDS spawn within scan radius,
// before they have aggroed anyone. The existing CloseToCreatureTrigger only
// detects a single entry and doesn't distinguish friendly/hostile; pro-engage
// triggers carry a list of entries (a wave can have multiple types) and gate
// on alive+hostile only.
//
// Pattern: instantiate per-encounter subclass with the entry list and scan
// radius. The trigger fires the moment any one of those entries is alive
// within range. Wire it in the corresponding fight strategy to a focus/engage
// action so tanks pull and ranged retarget early.

namespace ai
{
    class NearbyHostileCreaturesTrigger : public Trigger
    {
    public:
        NearbyHostileCreaturesTrigger(PlayerbotAI* ai, std::string name, std::vector<uint32> entries, float rangeY)
            : Trigger(ai, name)
            , m_entries(std::move(entries))
            , m_range(rangeY) {}

        bool IsActive() override
        {
            if (!bot || !bot->IsInWorld())
                return false;

            // One scan per entry. The cost is O(entries × cell_visit) but the
            // entries list is short (typically 1-3) and the range cap on
            // VisitAllObjects keeps the cell window tight. Caching across
            // entries would need a single-pass anyEntry searcher we don't
            // have in the existing GridNotifiers API.
            for (uint32 entry : m_entries)
            {
                std::list<Unit*> units;
                MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, entry, m_range);
                MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
                Cell::VisitAllObjects(bot, searcher, m_range);
                for (Unit* u : units)
                {
                    if (!u || !u->IsAlive())
                        continue;
                    if (bot->IsFriendlyTo(u))
                        continue;
                    return true;
                }
            }
            return false;
        }

    protected:
        std::vector<uint32> m_entries;
        float m_range;
    };

    // --- Per-encounter instantiations ---

    // Razorgore P1 wave detection. Death Talon Dragonspawn (12422), Grethok
    // mages (12420) and the variant Death Talon Captain (14036) all spawn in
    // 8 waves during the egg phase. 60y covers the door spawn points so the
    // tanks engage as soon as one walks in, not 20y from the orb.
    class RazorgoreAddsNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        RazorgoreAddsNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "razorgore adds nearby", { 12422, 12420, 14036 }, 60.0f) {}
    };

    // Garr Firesworn (12099) pre-engage. The boss script's eruption-on-add
    // routine fires the cast on a random Firesworn when Garr hits 50% — by
    // then the cast trigger we already wire detects too late if the add was
    // standing out of the tank's threat list. Pro-engage at 50y means OTs
    // grab them on spawn.
    class GarrFireswornNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        GarrFireswornNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "garr firesworn nearby", { 12099 }, 50.0f) {}
    };

    // Onyxia P2 whelps (11262). The boss script spawns them in two waves at
    // 65% and 40% boss HP — wave radius around her position. 50y covers the
    // landing spots so OTs and ranged AOE can pick up before they hit the
    // raid frame.
    class OnyxiaWhelpsNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        OnyxiaWhelpsNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "onyxia whelps nearby", { 11262 }, 50.0f) {}
    };

    // Sulfuron Harbinger (12098) Priestess of Shahram adds (12099 — yes, the
    // same NPC entry as Garr Firesworn; they reuse the model. The encounter
    // context disambiguates: Sulfuron room vs MC bridge). Range 40y so the
    // OTs engage the priestesses before they cast Heal on Sulfuron.
    class SulfuronPriestessNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        SulfuronPriestessNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "sulfuron priestess nearby", { 12099 }, 40.0f) {}
    };

    // Baron Geddon Living Bomb is on a raid member, not an add — no entry to
    // scan. But Geddon itself has Inferno (12056 boss self-cast PBAOE). The
    // existing 'baron geddon inferno' covers the close-up case; pro-engage
    // doesn't apply here so we don't wire one.

    // Ragnaros Sons of Flame (12143) detection — they spawn at 8 fixed points
    // when Rag submerges. 60y covers the room. Combined with the existing
    // 'ragnaros submerge' aura trigger, this catches the wave.
    class RagnarosSonsNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        RagnarosSonsNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "ragnaros sons nearby", { 12143 }, 60.0f) {}
    };

    // Sartura's Royal Guard (15984) — 2-3 adds that spawn next to Battleguard
    // Sartura at fight start (boss_sartura.cpp NPC_SARTURA_S_ROYAL_GUARD).
    // Each Guard has Whirlwind too — OTs pull them away from melee bots.
    class SarturaRoyalGuardNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        SarturaRoyalGuardNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "sartura royal guard nearby", { 15984 }, 50.0f) {}
    };

    // Anub'Rekhan Crypt Guards (16573) spawn during the Locust Swarm phase —
    // 3 of them, summoned 1 per ~50% HP transition. Each has Cleave + Web +
    // Acid Spit. OTs grab on spawn so they don't reach the healers.
    // ScriptDev2 boss_anubrekhan.cpp MOB_CRYPT_GUARD.
    class AnubrekhanCryptGuardNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        AnubrekhanCryptGuardNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "anubrekhan crypt guard nearby", { 16573 }, 60.0f) {}
    };

    // Grand Widow Faerlina Worshippers (16506) + Followers (16505) — 4 are
    // chained in the room before pull. They self-detonate at 50%HP Faerlina
    // to lift her enrage. Pro-engage helps the off-tank/DPS focus them on
    // proc; ScriptDev2 boss_faerlina.cpp NPC_NaxxramasFollower/Worshipper.
    class FaerlinaWorshipperNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        FaerlinaWorshipperNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "faerlina worshipper nearby", { 16505, 16506 }, 50.0f) {}
    };

    // Gluth Zombie Chows (16360) spawn continuously during the encounter.
    // The boss EATS any within 10y of him to heal — pro-engage to OT kites
    // them away. ScriptDev2 boss_gluth.cpp NPC_ZOMBIE_CHOW.
    class GluthZombieChowNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        GluthZombieChowNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "gluth zombie chow nearby", { 16360 }, 50.0f) {}
    };

    // Fankriss summons. Spawn of Fankriss (15630) — large add with Enrage at
    // 50% HP. Vekniss Hatchling (15962) — small fast adds that swarm raid
    // members on summon. Both pro-engaged so OTs grab spawns + raid AOE on
    // hatchlings. ScriptDev2 boss_fankriss.cpp.
    class FankrissAddsNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        FankrissAddsNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "fankriss adds nearby", { 15630, 15962 }, 50.0f) {}
    };

    // --- AQ20 (Ruins of Ahn'Qiraj, map 509) ---

    // Moam Mana Fiends (15527) spawn at 35%/15% boss HP. They drain mana from
    // raid casters; OTs grab + DPS focus immediately. ScriptDev2 boss_moam.cpp
    // NPC_MANA_FIEND.
    class MoamManaFiendNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        MoamManaFiendNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "moam mana fiend nearby", { 15527 }, 60.0f) {}
    };

    // Buru Hivezara Hatchlings (15521) hatch from broken eggs and swarm the
    // raid. AOE focus required. ScriptDev2 boss_buru.cpp NPC_HIVEZARA_HATCHLING.
    class BuruHatchlingNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        BuruHatchlingNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "buru hatchling nearby", { 15521 }, 50.0f) {}
    };

    // Ayamiss summons during P1 (boss in air, raid kills adds + larvae on
    // altar). Hivezara Larva (15555) crawl to altar to be sacrificed; Hornet
    // (15934) attacks raid; Swarmer (15546) drops from Ayamiss.
    // ScriptDev2 boss_ayamiss.cpp.
    class AyamissAddsNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        AyamissAddsNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "ayamiss adds nearby", { 15555, 15934, 15546 }, 60.0f) {}
    };

    // General Rajaxx wave commanders (AQ20): 6 named officers + Rajaxx
    // himself, one per wave. Each commander leads ~10-15 anubisath/qiraji
    // soldiers. Captures the entire pull cycle. ScriptDev2 confirms entries
    // via ruins_of_ahnqiraj.h NPC_COLONEL_ZERRAN / NPC_MAJOR_YEGGETH /
    // NPC_MAJOR_PAKKON / NPC_CAPTAIN_DRENN / NPC_CAPTAIN_XURREM /
    // NPC_CAPTAIN_QEEZ / NPC_CAPTAIN_TUUBID.
    class RajaxxWaveCommanderNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        RajaxxWaveCommanderNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "rajaxx wave commander nearby",
                { 15385, 15386, 15388, 15389, 15390, 15391, 15392 }, 60.0f) {}
    };

    // Mandokir's raptor pet Ohgan (14988). The standard tactic is to kill
    // Ohgan before Mandokir — once Ohgan dies Mandokir takes 25% more dmg.
    // Plus Mandokir's Watch Out! mechanic (eye-stare death) is the bigger
    // threat: he Charges raid members at random. Pro-engage Ohgan on pull
    // so OTs grab and burn him.
    class MandokirOhganNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        MandokirOhganNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "mandokir ohgan nearby", { 14988 }, 40.0f) {}
    };

    // Naxx Maexxna Spiderlings (17055) — 12 spawn at 75%/50%/25% HP per
    // ScriptDev2 boss_maexxna.cpp NPC_SPIDERLING. AOE focus required to
    // prevent web wraps stacking on cocooned players.
    class MaexxnaSpiderlingNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        MaexxnaSpiderlingNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "maexxna spiderling nearby", { 17055 }, 60.0f) {}
    };

    // Naxx Noth's Plagued adds. Plagued Warrior (16984), Guardian (16981),
    // Champion (16983), Construct (16982). Adds spawn during the
    // teleport-to-balcony phase. ScriptDev2 boss_noth.cpp NPC_PLAGUED_*.
    class NothPlaguedAddsNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        NothPlaguedAddsNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "noth plagued adds nearby",
                { 16981, 16982, 16983, 16984 }, 80.0f) {}
    };

    // ZG High Priestess Marli summons Spawn of Marli (15041) from eggs around
    // the room. Pro-engage prevents the spawns from stacking on the raid
    // (they have a 90% poison aura that compounds quickly).
    class MarliSpawnNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        MarliSpawnNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "marli spawn nearby", { 15041 }, 50.0f) {}
    };

    // ZG Jin'do summons. Brainwash Totem (15112) MIND-CONTROLS a raid member
    // every cast — top kill priority. Powerful Healing Ward (14987) heals
    // Jin'do significantly. Burning both ASAP is fight-defining.
    // ScriptDev2 boss_jindo.cpp NPC_BRAINWASH_TOTEM / NPC_POWERFULL_HEALING_WARD.
    class JindoTotemsNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        JindoTotemsNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "jindo totems nearby",
                { 15112, 14987 }, 60.0f) {}
    };
}
