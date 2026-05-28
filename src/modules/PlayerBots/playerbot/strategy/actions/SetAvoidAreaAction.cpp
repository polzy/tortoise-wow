
#include "playerbot/playerbot.h"
#include "SetAvoidAreaAction.h"

#include "playerbot/strategy/values/PositionValue.h"
#include "Maps/PathFinder.h"
using namespace ai;


bool SetAvoidAreaAction::Execute(Event& event)
{
    // Penqle/Turtle WoW has no nav-mesh area cost support (setArea is a no-op).
    // Skip avoid-area computation entirely - the bot will use normal pathfinding instead.
    return false;
}

bool SetAvoidAreaAction::isUseful()
{
    if (bot->GetInstanceId())
        return false;

    PositionEntry p = AI_VALUE2(PositionEntry, "pos", "last avoid");

    if (!p.isSet())
        return true;

    return p.Get().distance(bot) > 20.0f;
}