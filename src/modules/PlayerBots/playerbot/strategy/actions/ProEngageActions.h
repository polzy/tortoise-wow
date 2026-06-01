#pragma once
#include "AttackAction.h"
#include <vector>

// Pro-engage actions — pick the closest hostile creature of a given entry
// list within range and aggressively engage it. Used by tank/DPS strategies
// to grab adds the MOMENT a wave spawns, before the boss-side aggro routine
// kicks in and randomly distributes threat (e.g. on the orb-bound player
// during Razorgore P1).
//
// Pattern mirrors AttackOnyxiaAction: scan via AllCreaturesOfEntryInRangeCheck,
// pin "attack target" via SET_AI_VALUE, then route through the proven
// AttackAction::Attack pipeline (selection / facing / pet / OnCombatStarted).

namespace ai
{
    class EngageNearbyAddAction : public AttackAction
    {
    public:
        EngageNearbyAddAction(PlayerbotAI* ai, std::string name, std::vector<uint32> entries, float rangeY)
            : AttackAction(ai, name)
            , m_entries(std::move(entries))
            , m_range(rangeY) {}

        bool Execute(Event& event) override
        {
            Unit* target = FindClosest();
            if (!target)
                return false;

            SET_AI_VALUE(ObjectGuid, "attack target", target->GetObjectGuid());

            Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
            return Attack(requester, target);
        }

        bool isUseful() override
        {
            return FindClosest() != nullptr;
        }

    protected:
        Unit* FindClosest() const
        {
            if (!bot || !bot->IsInWorld())
                return nullptr;

            Unit* closest = nullptr;
            float closestDistSq = m_range * m_range + 1.0f;

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
                    // GetDistance() (3D) is slightly more expensive than
                    // GetDistance2d() but Razorgore room has multiple Z layers
                    // (orb platform vs egg floor) — using 3D avoids picking
                    // an unreachable target.
                    float d = bot->GetDistance(u);
                    float dsq = d * d;
                    if (dsq < closestDistSq)
                    {
                        closestDistSq = dsq;
                        closest = u;
                    }
                }
            }
            return closest;
        }

        std::vector<uint32> m_entries;
        float m_range;
    };

    // --- Per-encounter instantiations ---

    class EngageRazorgoreAddAction : public EngageNearbyAddAction
    {
    public:
        EngageRazorgoreAddAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage razorgore add", { 12422, 12420 }, 60.0f) {}
    };

    class EngageGarrFireswornAction : public EngageNearbyAddAction
    {
    public:
        EngageGarrFireswornAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage garr firesworn", { 12099 }, 50.0f) {}
    };

    class EngageOnyxiaWhelpAction : public EngageNearbyAddAction
    {
    public:
        EngageOnyxiaWhelpAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage onyxia whelp", { 11262 }, 50.0f) {}
    };

    class EngageSulfuronPriestessAction : public EngageNearbyAddAction
    {
    public:
        EngageSulfuronPriestessAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage sulfuron priestess", { 11662 }, 40.0f) {}
    };

    class EngageRagnarosSonAction : public EngageNearbyAddAction
    {
    public:
        EngageRagnarosSonAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage ragnaros son", { 12143 }, 60.0f) {}
    };

    class EngageSarturaRoyalGuardAction : public EngageNearbyAddAction
    {
    public:
        EngageSarturaRoyalGuardAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage sartura royal guard", { 15984 }, 50.0f) {}
    };

    class EngageAnubrekhanCryptGuardAction : public EngageNearbyAddAction
    {
    public:
        EngageAnubrekhanCryptGuardAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage anubrekhan crypt guard", { 16573 }, 60.0f) {}
    };

    class EngageFaerlinaWorshipperAction : public EngageNearbyAddAction
    {
    public:
        EngageFaerlinaWorshipperAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage faerlina worshipper", { 16505, 16506 }, 50.0f) {}
    };

    class EngageGluthZombieChowAction : public EngageNearbyAddAction
    {
    public:
        EngageGluthZombieChowAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage gluth zombie chow", { 16360 }, 50.0f) {}
    };

    class EngageFankrissAddAction : public EngageNearbyAddAction
    {
    public:
        EngageFankrissAddAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage fankriss add", { 15630, 15962 }, 50.0f) {}
    };

    // --- AQ20 ---
    class EngageMoamManaFiendAction : public EngageNearbyAddAction
    {
    public:
        EngageMoamManaFiendAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage moam mana fiend", { 15527 }, 60.0f) {}
    };

    class EngageBuruHatchlingAction : public EngageNearbyAddAction
    {
    public:
        EngageBuruHatchlingAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage buru hatchling", { 15521 }, 50.0f) {}
    };

    // Buru Eggs (15514) — kill before they hatch. Wider scan range (60y)
    // covers the whole arena because eggs are scattered.
    class EngageBuruEggAction : public EngageNearbyAddAction
    {
    public:
        EngageBuruEggAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage buru egg", { 15514 }, 60.0f) {}
    };

    // Skeram split-phase retarget — clones and real boss share entry
    // 15263. When 2+ are alive AND one is at low HP, the bot's current
    // target may be a clone the raid already burned through. This action
    // re-targets the CLOSEST live 15263 — if the real boss is closer the
    // raid converges on him; if a clone is closer they finish it fast and
    // then the trigger fires again next tick on the next closest.
    // Trigger gate: SkeramSplitPhaseTrigger fires only during split.
    class EngageNearestSkeramAction : public EngageNearbyAddAction
    {
    public:
        EngageNearestSkeramAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage nearest skeram", { 15263 }, 100.0f) {}
    };

    // Framework #8 — Twin Emperors pair-swap.
    //
    // Vek'lor (15276) and Vek'nilash (15275) teleport-swap positions every
    // ~30s. Each tank needs to retarget the OTHER twin after the swap.
    // EngageOtherTwinAction inspects the bot's current target; if it's
    // one of the twins, retarget to the OTHER one. If bot has no twin
    // target yet (just arrived at the fight), retarget to the closest
    // live twin. Idempotent — re-fires harmlessly if bot already on
    // the right twin (closest-pick converges back).
    class EngageOtherTwinAction : public EngageNearbyAddAction
    {
    public:
        EngageOtherTwinAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage other twin emperor", { 15275, 15276 }, 100.0f) {}

        bool Execute(Event& event) override
        {
            Unit* current = AI_VALUE(Unit*, "current target");
            uint32 myTwin = (current && current->IsAlive()) ? current->GetEntry() : 0;

            // Pick the entry that is NOT my current twin. If I have no
            // twin target, the closest-pick in base class handles it.
            std::vector<uint32> wanted;
            if (myTwin == 15275) wanted.push_back(15276);
            else if (myTwin == 15276) wanted.push_back(15275);
            else { wanted.push_back(15275); wanted.push_back(15276); }

            // Temporarily swap entries, run base scan, then restore.
            std::vector<uint32> saved = m_entries;
            m_entries = wanted;
            bool ok = EngageNearbyAddAction::Execute(event);
            m_entries = saved;
            return ok;
        }
    };

    class EngageAyamissAddAction : public EngageNearbyAddAction
    {
    public:
        EngageAyamissAddAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage ayamiss add", { 15555, 15934, 15546 }, 60.0f) {}
    };

    class EngageRajaxxWaveCommanderAction : public EngageNearbyAddAction
    {
    public:
        EngageRajaxxWaveCommanderAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage rajaxx wave commander",
                { 15385, 15386, 15388, 15389, 15390, 15391, 15392 }, 60.0f) {}
    };

    class EngageMandokirOhganAction : public EngageNearbyAddAction
    {
    public:
        EngageMandokirOhganAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage mandokir ohgan", { 14988 }, 40.0f) {}
    };

    class EngageMaexxnaSpiderlingAction : public EngageNearbyAddAction
    {
    public:
        EngageMaexxnaSpiderlingAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage maexxna spiderling", { 17055 }, 60.0f) {}
    };

    // Maexxna Web Wrap (creature 16486) — kill the wrap to free the
    // webbed bot. Wider scan range (80y) than spiderlings because the
    // wrap spawns at the wall (away from raid center).
    class EngageMaexxnaWebWrapAction : public EngageNearbyAddAction
    {
    public:
        EngageMaexxnaWebWrapAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage maexxna web wrap", { 16486 }, 80.0f) {}
    };

    class EngageNothPlaguedAddAction : public EngageNearbyAddAction
    {
    public:
        EngageNothPlaguedAddAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage noth plagued add",
                { 16981, 16982, 16983, 16984 }, 80.0f) {}
    };

    class EngageMarliSpawnAction : public EngageNearbyAddAction
    {
    public:
        EngageMarliSpawnAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage marli spawn", { 15041 }, 50.0f) {}
    };

    class EngageJindoTotemAction : public EngageNearbyAddAction
    {
    public:
        EngageJindoTotemAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage jindo totem",
                { 15112, 14987 }, 60.0f) {}
    };

    class EngageThekalZealotAction : public EngageNearbyAddAction
    {
    public:
        EngageThekalZealotAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage thekal zealot", { 11347, 11348 }, 40.0f) {}
    };

    class EngageThekalTigerAction : public EngageNearbyAddAction
    {
    public:
        EngageThekalTigerAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage thekal tiger", { 15068 }, 50.0f) {}
    };

    class EngageArlokkProwlerAction : public EngageNearbyAddAction
    {
    public:
        EngageArlokkProwlerAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage arlokk prowler", { 15101 }, 50.0f) {}
    };

    class EngageVenoxisCobraAction : public EngageNearbyAddAction
    {
    public:
        EngageVenoxisCobraAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage venoxis cobra", { 11373 }, 50.0f) {}
    };

    class EngageGothikAddAction : public EngageNearbyAddAction
    {
    public:
        EngageGothikAddAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage gothik add",
                { 16124, 16125, 16126, 16127, 16148, 16149, 16150 }, 80.0f) {}
    };

    class EngageYaujBroodAction : public EngageNearbyAddAction
    {
    public:
        EngageYaujBroodAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage yauj brood", { 15621 }, 50.0f) {}
    };

    class EngageCthunTentacleAction : public EngageNearbyAddAction
    {
    public:
        EngageCthunTentacleAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage cthun tentacle",
                { 15726, 15725, 15728, 15334, 15802 }, 60.0f) {}
    };
}
