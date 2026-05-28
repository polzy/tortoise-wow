#include "playerbot/playerbot.h"
#include "BlackwingLairDungeonStrategies.h"
#include "DungeonMultipliers.h"

using namespace ai;

// ========== Vaelastrasz the Corrupt (13020) ==========

void VaelastraszFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Pure DPS race — Vaelastrasz becomes invulnerable after 3 minutes and Burning
    // Adrenaline starts wiping the raid. No mana conservation needed (Essence of
    // the Red 23513 gives unlimited mana/rage/energy during the encounter).
    triggers.push_back(new TriggerNode(
        "vaelastrasz burning adrenaline",
        NextAction::array(0, new NextAction("vael burning adrenaline move away", 100.0f), NULL)));
}

void VaelastraszFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end vaelastrasz fight",
        NextAction::array(0, new NextAction("disable vaelastrasz fight strategy", 100.0f), NULL)));
}

void VaelastraszFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end vaelastrasz fight",
        NextAction::array(0, new NextAction("disable vaelastrasz fight strategy", 100.0f), NULL)));
}

// ========== Broodlord Lashlayer (12017) ==========

void BroodlordFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    Player* bot = ai->GetBot();

    // Knock Away frequently strips the tank's threat lead. Tanks need to re-taunt
    // aggressively. The generic "lose aggro" -> "taunt" chain (warrior, paladin
    // prot, druid bear) already handles this, but we boost priority during the
    // Broodlord fight specifically.
    if (ai->IsTank(bot))
    {
        triggers.push_back(new TriggerNode(
            "lose aggro",
            NextAction::array(0, new NextAction("taunt", 100.0f), NULL)));
    }

    // Blast Wave is a frontal/PBAOE fire. Casters should already be at range; this
    // is here for the OT and melee dispel/heal classes who might be too close.
    // Currently no Blast Wave-specific trigger — handled by generic "enemy too
    // close" caster avoidance for ranged classes already in the fight.
}

void BroodlordFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end broodlord fight",
        NextAction::array(0, new NextAction("disable broodlord fight strategy", 100.0f), NULL)));
}

void BroodlordFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end broodlord fight",
        NextAction::array(0, new NextAction("disable broodlord fight strategy", 100.0f), NULL)));
}

// ========== Chromaggus (14020) ==========

void ChromaggusFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Stack of 4 Brood Afflictions on a single player is the critical state — at
    // 5 the player gets Chromatic Mutation 23174 and turns hostile. Any class
    // with a generic dispel ("remove curse" / "dispel magic" / "cleanse") should
    // strip a color off the affected target. The chain falls through quietly on
    // classes that can't dispel.
    triggers.push_back(new TriggerNode(
        "chromaggus affliction danger",
        NextAction::array(0,
            new NextAction("remove curse", 100.0f),
            new NextAction("dispel magic", 100.0f),
            new NextAction("cleanse", 100.0f),
            NULL)));
}

void ChromaggusFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end chromaggus fight",
        NextAction::array(0, new NextAction("disable chromaggus fight strategy", 100.0f), NULL)));
}

void ChromaggusFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end chromaggus fight",
        NextAction::array(0, new NextAction("disable chromaggus fight strategy", 100.0f), NULL)));
}

void BlackwingLairDungeonStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "suppression device close",
        NextAction::array(0, new NextAction("disarm suppression device", 80.0f), NULL)));

    // BWL boss auto-start triggers
    triggers.push_back(new TriggerNode(
        "start vaelastrasz fight",
        NextAction::array(0, new NextAction("enable vaelastrasz fight strategy", 100.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "start broodlord fight",
        NextAction::array(0, new NextAction("enable broodlord fight strategy", 100.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "start chromaggus fight",
        NextAction::array(0, new NextAction("enable chromaggus fight strategy", 100.0f), NULL)));
}

void BlackwingLairDungeonStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "suppression device need stealth",
        NextAction::array(0, new NextAction("stealth for suppression device", ACTION_HIGH + 3), NULL)));

    triggers.push_back(new TriggerNode(
        "suppression device in sight",
        NextAction::array(0, new NextAction("move to suppression device", ACTION_HIGH + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "suppression device close",
        NextAction::array(0, new NextAction("disarm suppression device", ACTION_HIGH + 4), NULL)));
}

class SuppressionRoomPassiveMultiplier : public Multiplier
{
public:
    SuppressionRoomPassiveMultiplier(PlayerbotAI* ai) : Multiplier(ai, "suppression room passive") {}

    float GetValue(ai::Action* action) override
    {
        if (!action)
            return 1.0f;

        if (ai->GetBot()->getClass() != CLASS_ROGUE)
            return 1.0f;

        const std::string& name = action->getName();

        // Enable only the following strats for suppression room to avoid regular combat breaking logic
        if (name == "stealth for suppression device" ||
            name == "move to suppression device" ||
            name == "disarm suppression device" ||
            name == "deactivate suppression device")
        {
            return 1.0f;
        }

        if (name == "stealth" ||
            name == "unstealth" ||
            name == "check stealth" ||
            name == "sprint" ||
            name == "vanish")
        {
            return 1.0f;
        }

        if (name == "co" ||
            name == "nc" ||
            name == "load ai" ||
            name == "save ai" ||
            name == "list ai" ||
            name == "reset ai" ||
            name == "reset strats" ||
            name == "reset values" ||
            name == "check mount state" ||
            name == "accept invitation" ||
            name == "set combat state" ||
            name == "set non combat state" ||
            name == "set dead state" ||
            name == "update pvp strats" ||
            name == "update pve strats" ||
            name == "update raid strats" ||
            name == "loot roll" ||
            name == "auto loot roll" ||
            name == "follow" ||
            name == "stay" ||
            name == "food" ||
            name == "drink")
        {
            return 1.0f;
        }

        return 0.0f;
    }
};

void SuppressionRoomStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "suppression device need stealth",
        NextAction::array(0, new NextAction("vanish", ACTION_EMERGENCY + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "suppression device in sight",
        NextAction::array(0, new NextAction("move to suppression device", ACTION_HIGH + 8), NULL)));

    triggers.push_back(new TriggerNode(
        "suppression device close",
        NextAction::array(0, new NextAction("disarm suppression device", 90.0f), NULL)));
}

void SuppressionRoomStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "suppression device need stealth",
        NextAction::array(0, new NextAction("stealth for suppression device", ACTION_MOVE), NULL)));

    triggers.push_back(new TriggerNode(
        "suppression device in sight",
        NextAction::array(0, new NextAction("move to suppression device", ACTION_HIGH + 8), NULL)));

    triggers.push_back(new TriggerNode(
        "suppression device close",
        NextAction::array(0, new NextAction("disarm suppression device", ACTION_MOVE + 2), NULL)));
}

void SuppressionRoomStrategy::InitCombatMultipliers(std::list<Multiplier*>& multipliers)
{
    multipliers.push_back(new SuppressionRoomPassiveMultiplier(ai));
}

void SuppressionRoomStrategy::InitNonCombatMultipliers(std::list<Multiplier*>& multipliers)
{
    multipliers.push_back(new SuppressionRoomPassiveMultiplier(ai));
}

void SuppressionRoomStrategy::OnStrategyAdded(BotState state)
{
    if (ai->GetBot()->getClass() == CLASS_ROGUE)
    {
        ai->ChangeStrategy("-avoid aoe", BotState::BOT_STATE_COMBAT);
        ai->ChangeStrategy("-avoid aoe", BotState::BOT_STATE_NON_COMBAT);
        ai->ChangeStrategy("-avoid aoe", BotState::BOT_STATE_REACTION);
        ai->ChangeStrategy("-avoid mobs", BotState::BOT_STATE_COMBAT);
        ai->ChangeStrategy("-avoid mobs", BotState::BOT_STATE_NON_COMBAT);
        ai->ChangeStrategy("-avoid mobs", BotState::BOT_STATE_REACTION);
    }
}