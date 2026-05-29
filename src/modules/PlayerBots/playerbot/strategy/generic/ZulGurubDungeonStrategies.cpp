
#include "playerbot/playerbot.h"
#include "ZulGurubDungeonStrategies.h"

using namespace ai;

void ZulGurubDungeonStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "start hakkar fight",
        NextAction::array(0, new NextAction("enable hakkar fight strategy", 100.0f), NULL)));

    // Mandokir's pet Ohgan (14988) — pro-engage at 40y. Burning Ohgan first
    // makes Mandokir take +25% dmg per ScriptDev2; priority 90 reflects this.
    triggers.push_back(new TriggerNode(
        "mandokir ohgan nearby",
        NextAction::array(0, new NextAction("engage mandokir ohgan", 90.0f), NULL)));
}

// ========== Hakkar the Soulflayer (14834) ==========

void HakkarFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Marli (24686) stun + Jeklik (24687) silence — magic, prio 80 matches
    // existing Sulfuron Demo Shout / KT Mana Detonation cadence.
    triggers.push_back(new TriggerNode(
        "hakkar magic aspect",
        NextAction::array(0,
            new NextAction("dispel magic", 80.0f),
            new NextAction("cleanse magic", 80.0f),
            NULL)));

    // Venoxis aspect (24688) poison.
    triggers.push_back(new TriggerNode(
        "hakkar venoxis aspect",
        NextAction::array(0,
            new NextAction("cure poison", 80.0f),
            new NextAction("cleanse poison", 80.0f),
            NULL)));

    // Thekal aspect on Hakkar (24689) — hunter Tranquilizing Shot strips it.
    triggers.push_back(new TriggerNode(
        "hakkar thekal enrage",
        NextAction::array(0, new NextAction("tranquilizing shot", 90.0f), NULL)));
}

void HakkarFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end hakkar fight",
        NextAction::array(0, new NextAction("disable hakkar fight strategy", 100.0f), NULL)));
}

void HakkarFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end hakkar fight",
        NextAction::array(0, new NextAction("disable hakkar fight strategy", 100.0f), NULL)));
}
