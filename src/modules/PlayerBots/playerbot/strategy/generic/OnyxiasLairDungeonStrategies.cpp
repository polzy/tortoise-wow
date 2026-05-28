
#include "playerbot/playerbot.h"
#include "OnyxiasLairDungeonStrategies.h"

using namespace ai;

void OnyxiasLairDungeonStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "start onyxia fight",
        NextAction::array(0, new NextAction("enable onyxia fight strategy", 100.0f), NULL)));
}

void OnyxiaFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // First-pass implementation. Upstream (cmangos/playerbots, ike3/mangosbot,
    // celguar/mangosbot-bots) all ship this method as an empty stub — there is no
    // canonical reference to lift from. The triggers below mirror the Magmadar /
    // Geddon patterns from MoltenCoreDungeonStrategies.cpp:
    //
    //   1. Range classes flee when an enemy gets into melee distance (was the
    //      primary source of caster deaths to Onyxia tail/cleave/breath).
    //   2. Fire Protection Potion when off cooldown (P2 Deep Breath, Lava breath).
    //   3. Generic hazard avoidance (works for any AoE proc the dummy aura system
    //      tags — Deep Breath fire patch, etc.) — see ReactionTriggers below.
    //
    // TODO: dedicated Onyxia phase tracking + Deep Breath cast detection (spell
    // 17086 / 23461) for a perpendicular kite. Filed as #39 follow-up.
    Player* bot = ai->GetBot();

    if (ai->IsRanged(bot) || ai->IsHeal(bot))
    {
        triggers.push_back(new TriggerNode(
            "enemy too close for spell",
            NextAction::array(0, new NextAction("flee", 100.0f), NULL)));
    }

    triggers.push_back(new TriggerNode(
        "fire protection potion ready",
        NextAction::array(0, new NextAction("fire protection potion", 100.0f), NULL)));
}

void OnyxiaFightStrategy::InitCombatMultipliers(std::list<Multiplier*>& multipliers)
{
    Player* bot = ai->GetBot();
    if (ai->IsRanged(bot) || ai->IsHeal(bot))
    {
        // Same multiplier Magmadar uses — prevents the "move away" action from
        // re-firing while the bot is already mid-cast on the boss. Avoids the
        // jitter where a caster steps back, starts a cast, steps back again, etc.
        multipliers.push_back(new PreventMoveAwayFromCreatureOnReachToCastMultiplier(ai));
    }
}

void OnyxiaFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end onyxia fight",
        NextAction::array(0, new NextAction("disable onyxia fight strategy", 100.0f), NULL)));
}

void OnyxiaFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end onyxia fight",
        NextAction::array(0, new NextAction("disable onyxia fight strategy", 100.0f), NULL)));
}

void OnyxiaFightStrategy::InitReactionTriggers(std::list<TriggerNode*>& triggers)
{
    // ...
}
