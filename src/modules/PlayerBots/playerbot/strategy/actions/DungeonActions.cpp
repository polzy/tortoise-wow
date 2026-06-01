#include "DungeonActions.h"
#include "playerbot/strategy/values/PositionValue.h"
#include "playerbot/strategy/AiObjectContext.h"
#include "playerbot/PlayerbotAI.h"
#include "Maps/GridNotifiers.h"
#include "Maps/GridNotifiersImpl.h"
#include "Maps/CellImpl.h"

using namespace ai;

bool MoveAwayFromHazard::Execute(Event& event)
{
    const std::list<HazardPosition>& hazards = AI_VALUE(std::list<HazardPosition>, "hazards");

    // Get the closest hazard to move away from
    const HazardPosition* closestHazard = nullptr;
    float closestHazardDistance = 9999.0f;
    for (const HazardPosition& hazard : hazards)
    {
        const WorldPosition& hazardPosition = hazard.first;
        const float distance = bot->GetDistance(hazardPosition.getX(), hazardPosition.getY(), hazardPosition.getZ());
        if (distance < closestHazardDistance)
        {
            closestHazardDistance = distance;
            closestHazard = &hazard;
        }
    }

    if (closestHazard)
    {
        // Check if the bot is inside the closest hazard
        const float hazardRadius = closestHazard->second;
        if (closestHazardDistance <= hazardRadius)
        {
            float angle = 0.0f;
            const WorldPosition initialPosition(closestHazard->first);
            const float distance = frand(hazardRadius, hazardRadius * 1.5f);

            Unit* currentTarget = AI_VALUE(Unit*, "current target");
            if (currentTarget)
            {
                const int8 startDir = urand(0, 1) * 2 - 1;
                const WorldPosition targetPosition(currentTarget);
                angle = targetPosition.getAngleTo(initialPosition) + (0.5 * M_PI_F * startDir);
            }
            else
            {
                angle = frand(0, M_PI_F * 2.0f);
            }

            const uint8 attempts = 10;
            float angleIncrement = (float)((2 * M_PI) / attempts);

            for (uint8 i = 0; i < attempts; i++)
            {
                WorldPosition point = initialPosition + WorldPosition(0, distance * cos(angle), distance * sin(angle), 1.0f);
                point.setZ(point.getHeight());

                // Check if the point is not near other hazards
                if (!IsHazardNearby(point, hazards))
                {
                    if (bot->IsWithinLOS(point.getX(), point.getY(), point.getZ() + bot->GetCollisionHeight()) && initialPosition.canPathTo(point, bot))
                    {
                        if (ai->HasStrategy("debug move", BotState::BOT_STATE_COMBAT))
                        {
                            bot->SummonCreature(15631, point.getX(), point.getY(), point.getZ(), 0.0f, TEMPSPAWN_TIMED_DESPAWN, 5000.0f);
                        }

                        if (MoveTo(bot->GetMapId(), point.getX(), point.getY(), point.getZ(), false, IsReaction(), false, true))
                        {
                            if (IsReaction())
                            {
                                WaitForReach(point.distance(initialPosition));
                            }

                            return true;
                        }
                    }
                }

                if (ai->HasStrategy("debug move", BotState::BOT_STATE_COMBAT))
                {
                    bot->SummonCreature(1, point.getX(), point.getY(), point.getZ(), 0.0f, TEMPSPAWN_TIMED_DESPAWN, 5000.0f);
                }

                angle += angleIncrement;
            }
        }
    }

    return false;
}

bool MoveAwayFromHazard::isPossible()
{
    if (MovementAction::isPossible())
    {
        return ai->CanMove();
    }

    return false;
}

bool MoveAwayFromHazard::IsHazardNearby(const WorldPosition& point, const std::list<HazardPosition>& hazards) const
{
    for (const HazardPosition& hazard : hazards)
    {
        const float hazardRange = hazard.second;
        const WorldPosition& hazardPosition = hazard.first;
        const float distance = point.distance(hazardPosition);
        if (distance < hazardRange)
        {
            return true;
        }
    }

    return false;
}

bool MoveAwayFromCreature::Execute(Event& event)
{
    // Get the active attacking creatures
    std::list<Creature*> creatures;
    size_t closestCreatureIdx = 0;
    float closestCreatureDistance = 9999.0f;

    // Iterate through the near creatures
    std::list<Unit*> units;
    MaNGOS::AllCreaturesOfEntryInRangeCheck u_check(bot, creatureID, range);
    MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, u_check);
    Cell::VisitAllObjects(bot, searcher, range);
    for (Unit* unit : units)
    {
        Creature* creature = (Creature*)unit;
        if (creature)
        {
            creatures.push_back(creature);

            // Get the closest creature to the bot
            const float distance = bot->GetDistance(creature);
            if (distance < closestCreatureDistance)
            {
                closestCreatureDistance = distance;
                closestCreatureIdx = creatures.size() - 1;
            }
        }
    }

    if (creatures.empty())
    {
        return false;
    }

    const std::list<HazardPosition>& hazards = AI_VALUE(std::list<HazardPosition>, "hazards");

    // Get the closest creature reference
    auto it = creatures.begin();
    advance(it, closestCreatureIdx);
    Creature* closestCreature = *it;
    // Remove the closest creature from the list to prevent checking it twice
    creatures.erase(it);

    // Generate the initial angle directly behind the bot looking at the closest creature
    const WorldPosition botPosition(bot);
    const WorldPosition creaturePosition(closestCreature);
    float angleLeft = creaturePosition.getAngleTo(botPosition);
    float angleRight = angleLeft;

    const uint8 attempts = 20;
    const uint8 halfAtempts = (uint8)(attempts * 0.5f);
    float angleIncrement = (float)((M_PI) / halfAtempts);

    const float sizeFactor = bot->GetCombatReach() + closestCreature->GetCombatReach();
    const float distance = (range + sizeFactor);

    for (uint8 i = 0; i < halfAtempts; i++)
    {
        WorldPosition* validPoint = nullptr;

        // Calculate a point to the left and right
        WorldPosition pointLeft = creaturePosition + WorldPosition(0, distance * cos(angleLeft), distance * sin(angleLeft), 1.0f);
        pointLeft.setZ(pointLeft.getHeight());
        WorldPosition pointRight = creaturePosition + WorldPosition(0, distance * cos(angleRight), distance * sin(angleRight), 1.0f);
        pointRight.setZ(pointRight.getHeight());

        if (IsValidPoint(pointLeft, creatures, hazards))
        {
            validPoint = &pointLeft;
        }
        else if (IsValidPoint(pointRight, creatures, hazards))
        {
            validPoint = &pointRight;
        }

        if (validPoint)
        {
            if (ai->HasStrategy("debug move", BotState::BOT_STATE_COMBAT))
            {
                bot->SummonCreature(15631, validPoint->getX(), validPoint->getY(), validPoint->getZ(), 0.0f, TEMPSPAWN_TIMED_DESPAWN, 5000.0f);
            }

            if (MoveTo(bot->GetMapId(), validPoint->getX(), validPoint->getY(), validPoint->getZ(), false, IsReaction(), false, true))
            {
                if (IsReaction())
                {
                    WaitForReach(validPoint->distance(botPosition));
                }

                return true;
            }
        }

        if (ai->HasStrategy("debug move", BotState::BOT_STATE_COMBAT))
        {
            bot->SummonCreature(1, pointLeft.getX(), pointLeft.getY(), pointLeft.getZ(), 0.0f, TEMPSPAWN_TIMED_DESPAWN, 5000.0f);
            bot->SummonCreature(1, pointRight.getX(), pointRight.getY(), pointRight.getZ(), 0.0f, TEMPSPAWN_TIMED_DESPAWN, 5000.0f);
        }

        angleLeft += angleIncrement;
        angleRight -= angleIncrement;
    }

    return false;
}

// MoveAwayAndStayFromCreature: identical scan + safe-point logic as
// MoveAwayFromCreature, but if the bot is already outside `range` of every
// matching creature, it actively clears Chase/Follow state instead of
// returning false — this is what stops the oscillation on sustained AOEs.
bool MoveAwayAndStayFromCreature::Execute(Event& event)
{
    std::list<Unit*> units;
    MaNGOS::AllCreaturesOfEntryInRangeCheck u_check(bot, creatureID, range);
    MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, u_check);
    Cell::VisitAllObjects(bot, searcher, range);

    // If we're already outside `range` of every match, the trigger is still
    // active (e.g. AOE aura still on boss) but we're physically safe.
    // Suppress chase so the bot's normal attack loop doesn't pull us back in.
    // Return FALSE so that lower-priority actions in the chain (heal,
    // dispel, ranged dps) still run — otherwise healers stop healing for
    // the entire AOE duration. Code-review 2026-06-01 round 3 finding #3.
    if (units.empty())
    {
        ai->StopMoving();
        bot->clearUnitState(UNIT_STAT_CHASE);
        bot->clearUnitState(UNIT_STAT_FOLLOW);
        AI_VALUE(LastMovement&, "last movement").Set(NULL);
        return false;
    }

    // Otherwise delegate to the standard move-away logic by constructing a
    // throwaway peer. (We don't inherit because their range/creatureID are
    // private; cleanest is to inline the scan and use the same outward-pick.)
    MoveAwayFromCreature delegate(ai, "move away from creature", creatureID, range);
    return delegate.Execute(event);
}

bool MoveAwayAndStayFromCreature::isPossible()
{
    if (MovementAction::isPossible())
    {
        return ai->CanMove();
    }

    return false;
}

bool MoveAwayFromCreature::isPossible()
{
    if (MovementAction::isPossible())
    {
        return ai->CanMove();
    }

    return false;
}

// Framework #10: GO-region kiting. Scan for the configured GO entry
// within `range`, pick the closest, send the bot to a safe point on
// the opposite side. Mirrors MoveAwayFromCreature but for GameObjects
// (which IsHazardNearby's HazardPosition stream doesn't cover —
// HazardPosition is for spell-effect AOEs, not generic GOs).
bool MoveAwayFromGameObject::Execute(Event& event)
{
    std::list<GameObject*> hazards;
    GameObjectsInObjectRangeCheck check(bot, range, goEntry);
    MaNGOS::GameObjectListSearcher<GameObjectsInObjectRangeCheck> searcher(hazards, check);
    Cell::VisitAllObjects(bot, searcher, range);

    GameObject* closest = nullptr;
    float closestDistSq = range * range + 1.0f;
    for (GameObject* go : hazards)
    {
        if (!go) continue;
        float d = bot->GetDistance(go);
        float dsq = d * d;
        if (dsq < closestDistSq)
        {
            closestDistSq = dsq;
            closest = go;
        }
    }
    if (!closest) return false;

    // Move to a point on the opposite side of the closest GO. Offset
    // = range + 3y safety margin. Vector from GO to bot, extended.
    float botX = bot->GetPositionX();
    float botY = bot->GetPositionY();
    float dx = botX - closest->GetPositionX();
    float dy = botY - closest->GetPositionY();
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist < 0.01f)
    {
        // Bot is on top of the GO — pick an arbitrary direction so we
        // don't divide by zero. Random offset 90° from boss-facing.
        dx = cosf(bot->GetOrientation()); dy = sinf(bot->GetOrientation());
        dist = 1.0f;
    }
    float scale = (range + 3.0f) / dist;
    float targetX = closest->GetPositionX() + dx * scale;
    float targetY = closest->GetPositionY() + dy * scale;
    float targetZ = closest->GetPositionZ();
    bot->UpdateGroundPositionZ(targetX, targetY, targetZ);

    return MoveTo(closest->GetMapId(), targetX, targetY, targetZ);
}

bool MoveAwayFromGameObject::isPossible()
{
    return MovementAction::isPossible() && ai->CanMove();
}

// Framework #9: command a charmed unit to attack a specific NPC entry.
// Same CMSG_PET_ACTION payload as the hunter/warlock pet-attack chain in
// GenericActions.cpp, but operates on Player::GetCharm() instead of
// Player::GetPet(). HandlePetAction on the server side resolves the GUID
// regardless of whether the source is a summoned pet or a charmed NPC.
bool CharmPetAttackTargetAction::Execute(Event& event)
{
    Unit* charm = bot->GetCharm();
    if (!charm || !charm->IsAlive())
        return false;

    // Find the target NPC by entry within 100y. Closest live match wins.
    std::list<Unit*> matches;
    MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, m_targetEntry, 100.0f);
    MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(matches, check);
    Cell::VisitAllObjects(bot, searcher, 100.0f);

    Unit* target = nullptr;
    float bestDistSq = 1e9f;
    for (Unit* u : matches)
    {
        if (!u || !u->IsAlive()) continue;
        float d = charm->GetDistance(u);
        float dsq = d * d;
        if (dsq < bestDistSq) { bestDistSq = dsq; target = u; }
    }
    if (!target) return false;

    // Already attacking the right target — no-op so we don't spam packets.
    if (charm->GetVictim() == target)
        return true;

    const uint8 flag = ACT_COMMAND;
    const uint32 spellId = COMMAND_ATTACK;
    const uint32 command = (flag << 24) | spellId;

    WorldPacket data(CMSG_PET_ACTION);
    data << charm->GetObjectGuid();
    data << command;
    data << target->GetObjectGuid();
    bot->GetSession()->HandlePetAction(data);
    return true;
}

bool CharmPetAttackTargetAction::isUseful()
{
    // Cheap gate: only useful when the bot is currently charming something.
    return bot && bot->GetCharm() != nullptr;
}

// Auto-destroy gray quality loot when bags are full. Conservative pass —
// only ITEM_QUALITY_POOR (gray = vendor trash); whites/greens/blues left
// alone because they may be class upgrades, quest items, or recipes the
// bot could equip. Iterates backpack + equipped bags; destroys up to 5
// items per call so we don't stall the AI tick. BotBagFullTrigger fires
// every 10s so 5/call clears typical overflow within a minute.
bool AutoDestroyGrayLootAction::Execute(Event& event)
{
    if (!bot) return false;
    int destroyed = 0;
    const int kMaxPerCall = 5;

    auto tryDestroy = [&](Item* item) -> bool {
        if (!item) return false;
        ItemPrototype const* proto = item->GetProto();
        if (!proto) return false;
        if (proto->Quality != ITEM_QUALITY_POOR) return false;
        // Bind-on-pickup grays are still safe to destroy — they're not
        // BiS gear by definition (quality=poor).
        bot->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
        ++destroyed;
        return destroyed >= kMaxPerCall;
    };

    // Backpack.
    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        if (destroyed >= kMaxPerCall) break;
        if (Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            tryDestroy(item);
    }
    // Equipped bags.
    for (uint8 b = INVENTORY_SLOT_BAG_START; b < INVENTORY_SLOT_BAG_END; ++b)
    {
        if (destroyed >= kMaxPerCall) break;
        Bag* bag = (Bag*)bot->GetItemByPos(INVENTORY_SLOT_BAG_0, b);
        if (!bag) continue;
        for (uint32 s = 0; s < bag->GetBagSize(); ++s)
        {
            if (destroyed >= kMaxPerCall) break;
            if (Item* item = bag->GetItemByPos(s))
                tryDestroy(item);
        }
    }
    return destroyed > 0;
}

bool MoveAwayFromCreature::IsValidPoint(const WorldPosition& point, const std::list<Creature*>& creatures, const std::list<HazardPosition>& hazards)
{
    // Check if the point is not near other game objects
    if (!HasCreaturesNearby(point, creatures) && !IsHazardNearby(point, hazards))
    {
        if (bot->IsWithinLOS(point.getX(), point.getY(), point.getZ() + bot->GetCollisionHeight()))
        {
            const WorldPosition botPosition(bot);
            return botPosition.canPathTo(point, bot);
        }
    }

    return false;
}

bool MoveAwayFromCreature::HasCreaturesNearby(const WorldPosition& point, const std::list<Creature*>& creatures) const
{
    for (const Creature* creature : creatures)
    {
        const float distance = creature->GetDistance(point.getX(), point.getY(), point.getZ());
        if (distance <= range)
        {
            return true;
        }
    }

    return false;
}

bool MoveAwayFromCreature::IsHazardNearby(const WorldPosition& point, const std::list<HazardPosition>& hazards) const
{
    for (const HazardPosition& hazard : hazards)
    {
        const float hazardRange = hazard.second;
        const WorldPosition& hazardPosition = hazard.first;
        const float distance = point.distance(hazardPosition);
        if (distance < hazardRange)
        {
            return true;
        }
    }

    return false;
}