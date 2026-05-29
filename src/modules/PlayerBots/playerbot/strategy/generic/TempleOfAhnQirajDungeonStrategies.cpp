
#include "playerbot/playerbot.h"
#include "TempleOfAhnQirajDungeonStrategies.h"
#include "DungeonMultipliers.h"

using namespace ai;

void TempleOfAhnQirajDungeonStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "start sartura fight",
        NextAction::array(0, new NextAction("enable sartura fight strategy", 100.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "start huhuran fight",
        NextAction::array(0, new NextAction("enable huhuran fight strategy", 100.0f), NULL)));

    // Pro-engage Fankriss adds (Spawn of Fankriss 15630 + Vekniss Hatchling
    // 15962). Dungeon-level so we don't need a dedicated Fankriss fight
    // strategy yet. Priority 85.
    triggers.push_back(new TriggerNode(
        "fankriss adds nearby",
        NextAction::array(0, new NextAction("engage fankriss add", 85.0f), NULL)));
}

// ========== Battleguard Sartura (15516) ==========

void SarturaFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    Player* bot = ai->GetBot();

    // Ranged/heal stay away during Whirlwind — she random-targets melees and
    // resets threat. Keeping casters out of 12y window means they're never in
    // the swap pool.
    if (ai->IsRanged(bot) || ai->IsHeal(bot))
    {
        triggers.push_back(new TriggerNode(
            "sartura too close",
            NextAction::array(0, new NextAction("move away from sartura", 100.0f), NULL)));
    }

    // Pro-engage: OTs grab Sartura's Royal Guards (15984) on spawn before they
    // also Whirlwind through the raid. Priority 80.
    triggers.push_back(new TriggerNode(
        "sartura royal guard nearby",
        NextAction::array(0, new NextAction("engage sartura royal guard", 80.0f), NULL)));
}

void SarturaFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end sartura fight",
        NextAction::array(0, new NextAction("disable sartura fight strategy", 100.0f), NULL)));
}

void SarturaFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end sartura fight",
        NextAction::array(0, new NextAction("disable sartura fight strategy", 100.0f), NULL)));
}

void SarturaFightStrategy::InitCombatMultipliers(std::list<Multiplier*>& multipliers)
{
    Player* bot = ai->GetBot();
    if (ai->IsRanged(bot) || ai->IsHeal(bot))
    {
        multipliers.push_back(new PreventMoveAwayFromCreatureOnReachToCastMultiplier(ai));
    }
}

// ========== Princess Huhuran (15509) ==========

void HuhuranFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Hunter Tranquilizing Shot (19801) on Frenzy 26051 — same chain as
    // Flamegor / standard "tranquilizing shot" action. Non-hunters silently
    // no-op.
    triggers.push_back(new TriggerNode(
        "huhuran frenzy",
        NextAction::array(0, new NextAction("tranquilizing shot", 90.0f), NULL)));

    // Noxious Poison (26053) — nature debuff curable by druid 'cure poison'.
    // Class-routed: druid handles it, others no-op. Priority 80 matches the
    // Lucifron Curse cadence.
    triggers.push_back(new TriggerNode(
        "huhuran noxious poison",
        NextAction::array(0, new NextAction("cure poison", 80.0f), NULL)));
}

void HuhuranFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end huhuran fight",
        NextAction::array(0, new NextAction("disable huhuran fight strategy", 100.0f), NULL)));
}

void HuhuranFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end huhuran fight",
        NextAction::array(0, new NextAction("disable huhuran fight strategy", 100.0f), NULL)));
}
