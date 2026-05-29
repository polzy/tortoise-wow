#pragma once
#include "DungeonActions.h"
#include "ChangeStrategyAction.h"
#include "AttackAction.h"

namespace ai
{
    class OnyxiasLairEnableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        OnyxiasLairEnableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable onyxia's lair strategy", "+onyxia's lair") {}
    };

    class OnyxiasLairDisableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        OnyxiasLairDisableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable onyxia's lair strategy", "-onyxia's lair") {}
    };

    class OnyxiaEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        OnyxiaEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable onyxia fight strategy", "+onyxia") {}
    };

    class OnyxiaDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        OnyxiaDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable onyxia fight strategy", "-onyxia") {}
    };

    // Move away from Onyxia (creature entry 10184) in any direction up to 40y. The base
    // MoveAwayFromCreature sweeps for a valid angle behind the bot relative to the creature,
    // so even though we don't compute a true perpendicular-to-flight-path vector, the bot
    // ends up out of the Deep Breath corridor in the majority of cases. 40y matches the
    // observed reach of the fire patch P2 cast leaves on the ground.
    class OnyxiaMoveAwayFromBreathAction : public MoveAwayFromCreature
    {
    public:
        OnyxiaMoveAwayFromBreathAction(PlayerbotAI* ai)
            : MoveAwayFromCreature(ai, "move away from onyxia breath", 10184, 40.0f) {}
    };

    // MT face-away positioning. The tank computes the raid centroid (mean of all
    // group member positions in the same map) and moves to the OPPOSITE side of
    // Onyxia, so when she turns to face the tank her cleave + tail face the wall
    // instead of the raid.
    //
    // Math: T_pos = boss_pos + normalize(boss_pos - centroid_pos) * tank_distance
    // (where tank_distance ~5y = melee reach). The bot then sets its destination
    // and lets the normal move-to pathing handle terrain.
    //
    // Only fires when the bot is the MT (lowest-GUID tank in the group) and Onyxia
    // is in P1 or P3 (ground). In P2 she's flying and untargetable to melee.
    class TankOnyxiaFaceAwayAction : public MovementAction
    {
    public:
        TankOnyxiaFaceAwayAction(PlayerbotAI* ai) : MovementAction(ai, "tank onyxia face away") {}

        bool Execute(Event& event) override
        {
            // MT only — OTs handle whelps via the OffTank target priority.
            if (!ai->IsTank(bot) || PlayerbotAI::IsOffTank(bot))
                return false;

            // Find Onyxia.
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 10184, 80.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 80.0f);
            Unit* boss = nullptr;
            for (Unit* u : units)
                if (u && u->IsAlive()) { boss = u; break; }
            if (!boss)
                return false;

            // Skip in P2 (Hover aura 17131) — boss is airborne.
            if (boss->HasAura(17131))
                return false;

            // Compute raid centroid in the same map as the bot.
            Group* group = bot->GetGroup();
            if (!group)
                return false;
            float cx = 0.0f, cy = 0.0f;
            uint32 nLive = 0;
            const uint32 myMap = bot->GetMapId();
            for (GroupReference* gref = group->GetFirstMember(); gref; gref = gref->next())
            {
                Player* m = gref->getSource();
                if (!m || !m->IsAlive() || m->GetMapId() != myMap || m == bot)
                    continue;
                cx += m->GetPositionX();
                cy += m->GetPositionY();
                ++nLive;
            }
            if (nLive < 2)
                return false;  // not enough raid members for a meaningful centroid
            cx /= nLive;
            cy /= nLive;

            // Vector boss -> centroid, then flip it: tank target = boss + flip * dist.
            const float bx = boss->GetPositionX();
            const float by = boss->GetPositionY();
            const float dx = bx - cx;
            const float dy = by - cy;
            const float len = sqrt(dx * dx + dy * dy);
            if (len < 1.0f)
                return false;  // raid is already on top of the boss, nothing to fix
            // Onyxia is a huge boss (combat_reach ~10-12y). A 5y TANK_DISTANCE
            // placed the tank's destination INSIDE her hitbox, which the path
            // generator refuses and which would put the tank in her cleave/tail
            // swipe arc anyway. 14y is melee-reach on her outer perimeter with a
            // small buffer for push-back. Tolerance bumped to 5y so the tank
            // doesn't jitter trying to hit an exact spot at the longer range.
            const float TANK_DISTANCE = 14.0f;
            const float tx = bx + (dx / len) * TANK_DISTANCE;
            const float ty = by + (dy / len) * TANK_DISTANCE;
            const float tz = boss->GetPositionZ();

            if (sServerFacade.GetDistance2d(bot, tx, ty) < 5.0f)
                return false;

            return MoveTo(myMap, tx, ty, tz);
        }

        bool isUseful() override
        {
            if (!ai->IsTank(bot) || PlayerbotAI::IsOffTank(bot))
                return false;
            // Cheap check — only useful if Onyxia is nearby and on the ground.
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 10184, 80.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 80.0f);
            for (Unit* u : units)
                if (u && u->IsAlive() && !u->HasAura(17131))
                    return true;
            return false;
        }
    };

    // Force-attack Onyxia herself (entry 10184), bypassing the regular
    // current-target / tank-target / dps-assist target selection. Used in P2 so
    // ranged/heal bots keep DPSing the boss while whelps engage them — without
    // this, they reactively switch to whatever whelp is hitting them and never
    // touch Onyxia again.
    //
    // Inherits AttackAction so we re-use its Attack(requester, target) which
    // handles: SetSelectionGuid, old/current target values, pet attack,
    // facing, bot->Attack(), OnCombatStarted. Previously the class extended
    // raw Action and called ChangeEngine() inline — the cast pipeline never
    // fired, and the engine swap was re-entrant (same crash class as the AI
    // re-entrancy gate).
    class AttackOnyxiaAction : public AttackAction
    {
    public:
        AttackOnyxiaAction(PlayerbotAI* ai) : AttackAction(ai, "attack onyxia") {}

        bool Execute(Event& event) override
        {
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 10184, 120.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 120.0f);

            Unit* onyxia = nullptr;
            for (Unit* u : units)
            {
                if (u && u->IsAlive())
                {
                    onyxia = u;
                    break;
                }
            }
            if (!onyxia)
                return false;

            // Pin "attack target" so downstream DPS / heal-on-target / spell
            // actions see Onyxia. Attack() also sets "current target" and the
            // bot's selection.
            SET_AI_VALUE(ObjectGuid, "attack target", onyxia->GetObjectGuid());

            Player* requester = event.getOwner() ? event.getOwner() : GetMaster();
            return Attack(requester, onyxia);
        }

        bool isUseful() override
        {
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 10184, 120.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 120.0f);
            for (Unit* u : units)
                if (u && u->IsAlive())
                    return true;
            return false;
        }
    };
}
