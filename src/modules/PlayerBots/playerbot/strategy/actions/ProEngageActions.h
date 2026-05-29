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
            : EngageNearbyAddAction(ai, "engage razorgore add", { 12422, 12420, 14036 }, 60.0f) {}
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
            : EngageNearbyAddAction(ai, "engage sulfuron priestess", { 12099 }, 40.0f) {}
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

    class EngageNothPlaguedAddAction : public EngageNearbyAddAction
    {
    public:
        EngageNothPlaguedAddAction(PlayerbotAI* ai)
            : EngageNearbyAddAction(ai, "engage noth plagued add",
                { 16981, 16982, 16983, 16984 }, 80.0f) {}
    };
}
