
#include "playerbot/playerbot.h"
#include "RuinsOfAhnQirajDungeonStrategies.h"

using namespace ai;

void RuinsOfAhnQirajDungeonStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Pro-engage triggers wired at the dungeon-level so they fire in any AQ20
    // room (entry scan picks the matching mobs only when their room is the
    // current location).
    triggers.push_back(new TriggerNode(
        "moam mana fiend nearby",
        NextAction::array(0, new NextAction("engage moam mana fiend", 90.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "buru hatchling nearby",
        NextAction::array(0, new NextAction("engage buru hatchling", 85.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "ayamiss adds nearby",
        NextAction::array(0, new NextAction("engage ayamiss add", 85.0f), NULL)));
    // Rajaxx wave commanders — covers Colonel Zerran + 2 Majors + 4 Captains
    // (15385/15386/15388/15389/15390/15391/15392). The whole 7-wave cycle.
    triggers.push_back(new TriggerNode(
        "rajaxx wave commander nearby",
        NextAction::array(0, new NextAction("engage rajaxx wave commander", 85.0f), NULL)));

    // Ossirian Curse of Tongues (25195) — -50% casting speed on casters.
    // Druid/mage 'remove curse'.
    triggers.push_back(new TriggerNode(
        "ossirian curse of tongues",
        NextAction::array(0, new NextAction("remove curse", 80.0f), NULL)));
}
