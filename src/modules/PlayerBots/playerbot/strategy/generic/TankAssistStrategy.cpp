
#include "playerbot/playerbot.h"
#include "TankAssistStrategy.h"

using namespace ai;

void TankAssistStrategy::InitCombatTriggers(std::list<TriggerNode*> &triggers)
{
    triggers.push_back(new TriggerNode(
        "tank assist",
        NextAction::array(0, new NextAction("tank assist", 60.0f), NULL)));

    // Reposition the boss away from the raid. Above the rotation (ACTION_HIGH)
    // but far below tank assist (60) and any boss-strategy anchor, so scripted
    // encounter positioning always wins over the generic face-away.
    triggers.push_back(new TriggerNode(
        "tank face",
        NextAction::array(0, new NextAction("tank face", ACTION_HIGH), NULL)));
}
