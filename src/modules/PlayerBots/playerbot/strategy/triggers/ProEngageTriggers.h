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
        // bossEntry: optional gate — if non-zero, the trigger first scans for
        // a live boss creature within `bossGateRange` yards. If the boss is
        // not present, the trigger returns false WITHOUT scanning the entry
        // list. This resolves code-review #5 (2026-05-29): dungeon-level
        // pro-engage triggers were scanning every tick anywhere in the
        // dungeon, including bosses' rooms we weren't engaged with. With the
        // gate, the scan only fires when the matching boss is nearby.
        NearbyHostileCreaturesTrigger(PlayerbotAI* ai, std::string name,
                                       std::vector<uint32> entries, float rangeY,
                                       uint32 bossEntry = 0, float bossGateRange = 100.0f)
            : Trigger(ai, name)
            , m_entries(std::move(entries))
            , m_range(rangeY)
            , m_bossEntry(bossEntry)
            , m_bossGateRange(bossGateRange) {}

        bool IsActive() override
        {
            if (!bot || !bot->IsInWorld())
                return false;

            // Boss-presence gate (if set). One extra cell visit but it
            // collapses the common "wrong room" case to a single miss,
            // avoiding `entries × cell_visit` work on every tick everywhere
            // in the dungeon.
            if (m_bossEntry != 0)
            {
                std::list<Unit*> bossList;
                MaNGOS::AllCreaturesOfEntryInRangeCheck bossCheck(bot, m_bossEntry, m_bossGateRange);
                MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> bossSearcher(bossList, bossCheck);
                Cell::VisitAllObjects(bot, bossSearcher, m_bossGateRange);
                bool bossFound = false;
                for (Unit* u : bossList)
                {
                    if (u && u->IsAlive()) { bossFound = true; break; }
                }
                if (!bossFound)
                    return false;
            }

            // One scan per entry. The cost is O(entries × cell_visit) but the
            // entries list is short (typically 1-3) and the range cap on
            // VisitAllObjects keeps the cell window tight.
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
        uint32 m_bossEntry;
        float m_bossGateRange;
    };

    // --- Per-encounter instantiations ---

    // Razorgore P1 wave detection. Death Talon Dragonspawn (12422) and
    // Blackwing Mage (12420 — the wave-spawn caster) per ScriptDev2
    // boss_razorgore.cpp. 60y covers the door spawn points so the tanks
    // engage as soon as one walks in, not 20y from the orb.
    // (Previous rev included 14036 — not confirmed in any boss/instance
    // script; removed per code-review 2026-05-29 finding #2.)
    class RazorgoreAddsNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        RazorgoreAddsNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "razorgore adds nearby", { 12422, 12420 }, 60.0f) {}
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

    // Sulfuron Harbinger (12098) Flamewaker Priest adds — they cast Heal
    // (Inspire) on Sulfuron. Per molten_core.h NPC_FLAMEWAKER_PRIEST = 11662.
    // Earlier rev used 12099 (Garr's Firesworn) on a "same model = same
    // entry" assumption; that was wrong (the priests use a different DB
    // entry) and the trigger never fired. Corrected per code-review
    // 2026-05-29 (background reviewer finding #1).
    class SulfuronPriestessNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        SulfuronPriestessNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "sulfuron priestess nearby", { 11662 }, 40.0f) {}
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

    // Anub'Rekhan Crypt Guards (16573) — gated on Anub'Rekhan (15956) presence.
    class AnubrekhanCryptGuardNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        AnubrekhanCryptGuardNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "anubrekhan crypt guard nearby",
                { 16573 }, 60.0f, 15956 /* NPC_ANUB_REKHAN */) {}
    };

    // Grand Widow Faerlina Worshippers/Followers — gated on Faerlina (15953).
    class FaerlinaWorshipperNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        FaerlinaWorshipperNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "faerlina worshipper nearby",
                { 16505, 16506 }, 50.0f, 15953 /* NPC_FAERLINA */) {}
    };

    // Gluth Zombie Chows (16360) — gated on Gluth (15932).
    class GluthZombieChowNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        GluthZombieChowNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "gluth zombie chow nearby",
                { 16360 }, 50.0f, 15932 /* NPC_GLUTH */) {}
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

    // Nefarian P2 + P3 adds (gated on Nefarian 11583 alive).
    //   - Bone Construct (14605) — P2 raises killed Drakonid corpses
    //   - Corrupted Infernal (14668) — P3 Warlock-Call spawn (hostile)
    // Both spawn during the encounter and need raid AOE. ScriptDev2
    // boss_nefarian.cpp NPC_BONE_CONSTRUCT, NPC_CORRUPTED_INFERNAL.
    class NefarianAddsNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        NefarianAddsNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "nefarian adds nearby",
                { 14605, 14668 }, 80.0f, 11583 /* NPC_NEFARIAN */) {}
    };

    // Ouro Dirt Mound (15712) — spawned when Ouro burrows (SUBMERGE_VISUAL
    // aura 26063 on the boss). The raid kills mounds to make Ouro
    // re-emerge. Without pro-engage, bots stand idle while Ouro is
    // underground and the mounds free-tick raid AOE.
    // ScriptDev2 boss_ouro.cpp NPC_DIRT_MOUND = 15712, NPC_OURO = 15517.
    // Gated on Ouro alive — mounds only spawn during the encounter.
    class OuroDirtMoundNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        OuroDirtMoundNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "ouro dirt mound nearby",
                { 15712 }, 80.0f, 15517 /* NPC_OURO */) {}
    };

    // Buru Eggs (15514) — destroyable creatures placed around the room.
    // P1 strategy: Buru the Gorger (15370) chains a raid member and is
    // kited near eggs; killing an egg explodes for ~1500 damage in 8y
    // AND damages Buru. Each egg also hatches into a Hivezara Hatchling
    // (15521) if not killed first. Pro-engage so the raid focuses eggs
    // proactively. Gated on Buru the Gorger (15370) — only fires during
    // the encounter. Range 60y covers the whole arena (eggs scattered).
    // ScriptDev2 boss_buru.cpp NPC_BURU_EGG = 15514, NPC_BURU = 15370.
    class BuruEggNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        BuruEggNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "buru egg nearby",
                { 15514 }, 60.0f, 15370 /* NPC_BURU */) {}
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

    // Maexxna Spiderlings (17055) — gated on Maexxna (15952).
    class MaexxnaSpiderlingNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        MaexxnaSpiderlingNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "maexxna spiderling nearby",
                { 17055 }, 60.0f, 15952 /* NPC_MAEXXNA */) {}
    };

    // Maexxna Web Wrap (creature 16486) — every ~40s Maexxna webs a random
    // raid member to the wall. The webbed player is incapacitated until the
    // web breaks (~10s) OR the wrap NPC is killed. Ranged DPS targets the
    // wrap to free the webbed bot quickly. Without this, healers cycle to
    // try to heal a target they can't reach and the webbed bot dies.
    // ScriptDev2 boss_maexxna.cpp NPC_WEB_WRAP = 16486. Gated on Maexxna
    // alive — wraps only spawn during her fight.
    class MaexxnaWebWrapNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        MaexxnaWebWrapNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "maexxna web wrap nearby",
                { 16486 }, 80.0f, 15952 /* NPC_MAEXXNA */) {}
    };

    // Noth's Plagued adds — gated on Noth (15954).
    class NothPlaguedAddsNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        NothPlaguedAddsNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "noth plagued adds nearby",
                { 16981, 16982, 16983, 16984 }, 80.0f, 15954 /* NPC_NOTH */) {}
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

    // Thekal P1 zealots — Lor'Khan (11347) and Zath (11348) fight alongside
    // Thekal in P1. All three must die within ~6s of each other or the
    // survivors res their fallen comrades. Pro-engage helps the raid focus
    // them together (boss is single-target; the OTs grab Zath and Lor'Khan).
    class ThekalZealotsNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        ThekalZealotsNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "thekal zealots nearby",
                { 11347, 11348 }, 40.0f) {}
    };

    // Thekal P2 Tigers (15068) summoned during the tiger form phase.
    // ScriptDev2 boss_thekal.cpp NPC_TIGER.
    class ThekalTigerNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        ThekalTigerNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "thekal tiger nearby", { 15068 }, 50.0f) {}
    };

    // Arlokk Zulian Prowlers (15101) — panther adds summoned during her
    // vanish/stealth phases. ScriptDev2 boss_arlokk.cpp NPC_ZULIAN_PROWLER.
    class ArlokkProwlerNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        ArlokkProwlerNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "arlokk prowler nearby", { 15101 }, 50.0f) {}
    };

    // Venoxis Razzashi Cobras (11373) — adds in the boss room, persistent
    // and respawn-on-Venoxis-cast. ScriptDev2 boss_venoxis.cpp NPC_RAZZASHI_COBRA.
    class VenoxisCobraNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        VenoxisCobraNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "venoxis cobra nearby", { 11373 }, 50.0f) {}
    };

    // Naxx Gothik adds — 7 entry types, all engaged on spawn. Live side:
    // Unrelenting Trainee (16124), DK (16125), Rider (16126). Dead side:
    // Spectral Trainee (16127), DK (16148), Rider (16150), Horse (16149).
    // ScriptDev2 naxxramas.h NPC_UNREL_* / NPC_SPECT_*.
    class GothikAddsNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        GothikAddsNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "gothik adds nearby",
                { 16124, 16125, 16126, 16127, 16148, 16149, 16150 }, 80.0f,
                16060 /* NPC_GOTHIK */, 150.0f /* big room */) {}
    };

    // AQ40 Princess Yauj Brood (15621) summoned during the Bug Trio fight.
    // ScriptDev2 temple_of_ahnqiraj.h NPC_YAUJ_BROOD.
    class YaujBroodNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        YaujBroodNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "yauj brood nearby", { 15621 }, 50.0f) {}
    };

    // AQ40 C'Thun tentacles (P2 body fight). All 5 types covered:
    //   Eye Tentacle           15726 (small, mind-flay)
    //   Claw Tentacle          15725 (small, melee — most frequent during
    //                                 the evading-tank phase, MUST be in
    //                                 the list per code-review finding #4)
    //   Giant Claw Tentacle    15728 (high HP melee)
    //   Giant Eye Tentacle     15334 (eye beam)
    //   Flesh Tentacle         15802 (inside the stomach)
    // ScriptDev2 boss_cthun.cpp MOB_EYE_TENTACLE / MOB_CLAW_TENTACLE /
    // MOB_GIANT_*_TENTACLE / MOB_FLESH_TENTACLE.
    class CthunTentacleNearbyTrigger : public NearbyHostileCreaturesTrigger
    {
    public:
        CthunTentacleNearbyTrigger(PlayerbotAI* ai)
            : NearbyHostileCreaturesTrigger(ai, "cthun tentacle nearby",
                { 15726, 15725, 15728, 15334, 15802 }, 60.0f) {}
    };
}
