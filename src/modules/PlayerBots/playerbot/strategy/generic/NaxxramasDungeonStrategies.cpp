
#include "playerbot/playerbot.h"
#include "NaxxramasDungeonStrategies.h"
#include "DungeonMultipliers.h"

using namespace ai;

void NaxxramasDungeonStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
	triggers.push_back(new TriggerNode(
		"start four horseman fight",
		NextAction::array(0, new NextAction("enable four horseman fight strategy", 100.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "start patchwerk fight",
        NextAction::array(0, new NextAction("enable patchwerk fight strategy", 100.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "start loatheb fight",
        NextAction::array(0, new NextAction("enable loatheb fight strategy", 100.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "start kelthuzad fight",
        NextAction::array(0, new NextAction("enable kelthuzad fight strategy", 100.0f), NULL)));
}

void FourHorsemanFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
	triggers.push_back(new TriggerNode(
		"void zone too close",
		NextAction::array(0, new NextAction("move away from void zone", 100.0f), NULL)));
}

void FourHorsemanFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
	triggers.push_back(new TriggerNode(
		"end horseman fight",
		NextAction::array(0, new NextAction("disable four horseman fight strategy", 100.0f), NULL)));
}

void FourHorsemanFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
	triggers.push_back(new TriggerNode(
		"end horseman fight",
		NextAction::array(0, new NextAction("disable four horseman fight strategy", 100.0f), NULL)));
}

// ========== Patchwerk (16028) ==========

void PatchwerkFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Pure DPS race. Bots default behavior (melee stay in, ranged stay back) is
    // correct for Patchwerk; Hateful Strike targeting is server-side and the bots
    // can't do much about it directly. We just need to keep them alive — heals
    // priority and tank threat-hold are governed by class strategies.
    //
    // No special trigger yet. Future: detect Berserk cast (27680) at 7-min and
    // trigger a "burn cooldowns" macro for DPS classes.
    (void)triggers;
}

void PatchwerkFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end patchwerk fight",
        NextAction::array(0, new NextAction("disable patchwerk fight strategy", 100.0f), NULL)));
}

void PatchwerkFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end patchwerk fight",
        NextAction::array(0, new NextAction("disable patchwerk fight strategy", 100.0f), NULL)));
}

// ========== Loatheb (16011) ==========

void LoathebFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Anti-heal mechanic: Corrupted Mind (29201) puts a 60s lockout on healers
    // after each heal cast. Bots' healer logic already cooldowns appropriately so
    // we don't add a custom heal-throttle. Spore (16286) adds drop Fungal Bloom
    // (29232) which the raid should pick up — handled by generic loot/aura
    // behavior, no Loatheb-specific action needed.
    //
    // Inevitable Doom (29204) is unavoidable raid AOE damage. Healers ride out
    // the lockout via Greater Heal castings before Doom ticks.
    (void)triggers;
}

void LoathebFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end loatheb fight",
        NextAction::array(0, new NextAction("disable loatheb fight strategy", 100.0f), NULL)));
}

void LoathebFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end loatheb fight",
        NextAction::array(0, new NextAction("disable loatheb fight strategy", 100.0f), NULL)));
}

// ========== Kel'Thuzad (15990) ==========

void KelThuzadFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    Player* bot = ai->GetBot();

    // Frost Blast root + AOE — casters should be spread to minimize splash.
    if (ai->IsRanged(bot) || ai->IsHeal(bot))
    {
        triggers.push_back(new TriggerNode(
            "enemy too close for spell",
            NextAction::array(0, new NextAction("flee", 100.0f), NULL)));
    }

    // Mana Detonation (27819) and Chains of Kel'Thuzad (28408) are dispel-able.
    // Wire the same multi-class dispel chain we use for Lucifron / Chromaggus.
    triggers.push_back(new TriggerNode(
        "kelthuzad mana detonation",
        NextAction::array(0,
            new NextAction("dispel magic", 100.0f),
            new NextAction("cleanse", 100.0f),
            NULL)));
}

void KelThuzadFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end kelthuzad fight",
        NextAction::array(0, new NextAction("disable kelthuzad fight strategy", 100.0f), NULL)));
}

void KelThuzadFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end kelthuzad fight",
        NextAction::array(0, new NextAction("disable kelthuzad fight strategy", 100.0f), NULL)));
}
