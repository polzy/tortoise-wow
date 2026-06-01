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

            // INTERACTION_DISTANCE = 5.0f (Object.h). Walk closer if outside.
            // Use 5.0f to match the engine constant — using 4.0f could loop
            // MoveTo at exactly 4-5y where the bot oscillates between "too
            // far" and "moved to position" without ever sending the use.
            if (bestDist > 5.0f)
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

    // Variant: same as MoveAwayFromCreature but, once the bot reaches the safe
    // point, clears Chase/Follow movement state so the bot doesn't oscillate
    // back into the AOE. Use for sustained-AOE mechanics (Anub'Rekhan Locust
    // Swarm 20s, Sartura Whirlwind 8s, future similar). The trigger should
    // fire every tick for the duration of the AOE so the clear-state happens
    // continuously; once the AOE ends and the trigger stops firing, the bot's
    // normal threat-driven Chase resumes naturally.
    class MoveAwayAndStayFromCreature : public MovementAction
    {
    public:
        MoveAwayAndStayFromCreature(PlayerbotAI* ai, std::string name, uint32 creatureID, float range)
            : MovementAction(ai, name), creatureID(creatureID), range(range) {}
        bool Execute(Event& event) override;
        bool isPossible() override;

    private:
        uint32 creatureID;
        float range;
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

    // ====================================================================
    // Framework #9 scaffold: MC charm orchestration (Razuvious).
    // ====================================================================
    //
    // STATUS: scaffold only — wiring deferred until charm-pet command
    // primitives land. Documented here so a future session has the
    // architecture map without re-discovering it.
    //
    // The Razuvious fight (Naxx, NPC 16061) requires 2-4 charmed Death
    // Knight Understudies (NPC 16803) to tank the boss. Players cannot
    // tank Razuvious directly — his Disrupting Shout one-shots casters
    // and Unbalancing Strike crits melee for ~9k. The boss MUST be
    // tanked by an Understudy that the raid Mind Controls via Tribute
    // Orbs (4 GOs in the room, charges-based).
    //
    // BOT-SIDE PIPELINE (target architecture):
    //
    //   1. Detect orb GO nearby → trigger `razuvious orb available`
    //      (consumes Framework #10 NearbyHazardGameObjectTrigger but
    //      inverted: "orb to use" not "hazard to avoid").
    //      Orb GO entry: TBD — Turtle DB lookup did not surface a
    //      Razuvious-specific orb; standard SD2 references unclear.
    //      Probable entry: 181605 ("Tribute Chest") or a Naxx-specific
    //      entry that needs DB inspection on a populated server.
    //
    //   2. Priest uses orb via Framework #1 UseNearbyGameObjectAction.
    //      Orb grants a channeled MC spell (mind-control charge).
    //
    //   3. Priest casts MC on nearest Understudy (16803).
    //      → NEEDS: a charm-cast action that targets a creature by
    //        entry. Existing actions either target current target or
    //        a party member; charm-targeting NPCs by entry is new.
    //
    //   4. Charmed Understudy auto-tanks Razuvious.
    //      → NEEDS: pet-command primitive that sends CMSG_PET_ACTION
    //        to attack a specific GUID. Hunter pet AI already does
    //        this for its summoned pet; the Razuvious case reuses the
    //        same opcode but the "pet" is a charmed NPC.
    //
    // ALL OTHER MC MECHANICS BENEFIT: same pipeline serves Hex Lord
    // (TBC raid), any future "charm-and-command" boss. Building it once
    // here unlocks the pattern.
    //
    // For 2026-06-01 the bot-controlled Razuvious remains ❌ in README.
    // The standard workaround is: a HUMAN priest operates the orbs;
    // bots provide DPS / healing. The trigger names below are reserved
    // for the future implementation:
    //
    //   - trigger "razuvious orb available"   (orb GO nearby, alive priest)
    //   - action  "use razuvious orb"         (UseNearbyGameObject subclass)
    //   - action  "charm razuvious understudy" (MC cast on 16803)
    //   - action  "command pet attack razuvious" (CMSG_PET_ACTION)
    //
    // ====================================================================

    // Framework #10 primitive: GO-region kiting.
    //
    // MoveAwayFromGameObject: GameObject equivalent of MoveAwayFromCreature.
    // When any GO of `goEntry` is within `range` yards of the bot, flee
    // perpendicular to the nearest GO until safe. Use for hazard
    // GameObjects: Ossirian tornadoes, Kurinnaxx sand traps, Ouro burrow
    // mounds, dummy ground-target spells that drop a GO (e.g. Geddon
    // Living Bomb projectile).
    //
    // The Cell visitor scans GOs by entry within range. Picks the closest,
    // then sends the bot to a point `range + 5y` away on the opposite side
    // (simple back-away — fancier kiting can subclass and override the
    // direction logic).
    class MoveAwayFromGameObject : public MovementAction
    {
    public:
        MoveAwayFromGameObject(PlayerbotAI* ai, std::string name, uint32 goEntry, float range)
            : MovementAction(ai, name), goEntry(goEntry), range(range) {}
        bool Execute(Event& event) override;
        bool isPossible() override;

    private:
        uint32 goEntry;
        float range;
    };
}
