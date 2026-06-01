#pragma once
#include "MovementActions.h"
#include "playerbot/strategy/values/HazardsValue.h"
#include "playerbot/strategy/values/NearestGameObjects.h"
#include "Maps/GridNotifiers.h"
#include "Maps/GridNotifiersImpl.h"
#include "Maps/CellImpl.h"

namespace ai
{
    // Framework primitive: bot finds the nearest GameObject with a given
    // entry within `range` yards, moves to interact range if needed, and
    // sends CMSG_GAMEOBJ_USE to trigger its server-side use (clicks the
    // orb / picks up the egg / opens the door / etc.).
    //
    // Subclass to wire a specific encounter mechanic — Razuvious orbs,
    // Buru eggs, Anub'Rekhan bridge orb, etc. Pattern is identical to the
    // BattleGround flag/relic capture (uses HandleGameObjectUseOpcode).
    //
    // Interaction range: ~4y (vanilla GO use threshold). If bot is farther,
    // we MoveTo first; the next tick fires the use.
    class UseNearbyGameObjectAction : public MovementAction
    {
    public:
        UseNearbyGameObjectAction(PlayerbotAI* ai, std::string name, uint32 goEntry, float scanRange = 30.0f)
            : MovementAction(ai, name), m_goEntry(goEntry), m_scanRange(scanRange) {}

        bool Execute(Event& event) override
        {
            Player* bot = ai->GetBot();
            if (!bot) return false;

            std::list<GameObject*> targets;
            GameObjectsInObjectRangeCheck check(bot, m_scanRange, m_goEntry);
            MaNGOS::GameObjectListSearcher<GameObjectsInObjectRangeCheck> searcher(targets, check);
            Cell::VisitAllObjects(bot, searcher, m_scanRange);

            GameObject* closest = nullptr;
            float bestDist = 1e9f;
            for (GameObject* go : targets)
            {
                if (!go) continue;
                float d = bot->GetDistance(go);
                if (d < bestDist) { bestDist = d; closest = go; }
            }
            if (!closest) return false;

            // ~4y is the vanilla interact range. If we're outside, walk in.
            if (bestDist > 4.0f)
                return MoveTo(closest->GetMapId(), closest->GetPositionX(), closest->GetPositionY(), closest->GetPositionZ());

            // Bot is in range — send the use opcode. Same path as right-click.
            WorldPacket data(CMSG_GAMEOBJ_USE);
            data << closest->GetObjectGuid();
            bot->GetSession()->HandleGameObjectUseOpcode(data);
            return true;
        }

    protected:
        uint32 m_goEntry;
        float m_scanRange;
    };


    class MoveAwayFromHazard : public MovementAction
    {
    public:
        MoveAwayFromHazard(PlayerbotAI* ai, std::string name = "move away from hazard") : MovementAction(ai, name) {}
        bool Execute(Event& event) override;
        bool isPossible() override;

#ifdef GenerateBotHelp
        virtual std::string GetHelpName() { return "move away from hazard"; }
        virtual std::string GetHelpDescription()
        {
            return "This action makes the bot move away from hazardous areas in dungeons.\n"
                   "It identifies dangerous positions and navigates to a safer location.";
        }
        virtual std::vector<std::string> GetUsedActions() { return {}; }
        virtual std::vector<std::string> GetUsedValues() { return {"hazards"}; }
#endif 

    private:
        bool IsHazardNearby(const WorldPosition& point, const std::list<HazardPosition>& hazards) const;
    };

    class MoveAwayFromCreature : public MovementAction
    {
    public:
        MoveAwayFromCreature(PlayerbotAI* ai, std::string name, uint32 creatureID, float range) : MovementAction(ai, name), creatureID(creatureID), range(range) {}
        bool Execute(Event& event) override;
        bool isPossible() override;

#ifdef GenerateBotHelp
        virtual std::string GetHelpName() { return "move away from creature"; }
        virtual std::string GetHelpDescription()
        {
            return "This action makes the bot move away from a specific creature in dungeons.\n"
                   "It maintains a safe distance from the specified creature ID within a defined range.";
        }
        virtual std::vector<std::string> GetUsedActions() { return {}; }
        virtual std::vector<std::string> GetUsedValues() { return {"hazards"}; }
#endif 

    private:
        bool IsValidPoint(const WorldPosition& point, const std::list<Creature*>& creatures, const std::list<HazardPosition>& hazards);
        bool HasCreaturesNearby(const WorldPosition& point, const std::list<Creature*>& creatures) const;
        bool IsHazardNearby(const WorldPosition& point, const std::list<HazardPosition>& hazards) const;

    private:
        uint32 creatureID;
        float range;
    };
}
