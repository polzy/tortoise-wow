
#include "playerbot/playerbot.h"
#include "OnyxiasLairDungeonStrategies.h"
#include "DungeonMultipliers.h"

using namespace ai;

void OnyxiasLairDungeonStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "start onyxia fight",
        NextAction::array(0, new NextAction("enable onyxia fight strategy", 100.0f), NULL)));
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
    // Deep Breath dodge. Detects any of the 8 directional Breath variants
    // (17086 / 18351 / 18576 / 18609 / 18564 / 18584 / 18596 / 18617 — taken from
    // boss_onyxia.cpp). Triggers a 40y move-away sweep. Not a true perpendicular
    // kite (would require flight-path projection math) but clears the bot from
    // the fire-patch corridor in most cases.
    triggers.push_back(new TriggerNode(
        "onyxia deep breath",
        NextAction::array(0, new NextAction("move away from onyxia breath", 100.0f), NULL)));
}

void OnyxiaFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    Player* bot = ai->GetBot();

    // Phase-1/3 ground combat: range classes flee when Onyxia (huge hitbox, cleave +
    // tail sweep cones) gets close. Already covered by the generic "enemy too close
    // for spell" trigger but boss fights warrant a higher-priority retest.
    if (ai->IsRanged(bot) || ai->IsHeal(bot))
    {
        triggers.push_back(new TriggerNode(
            "enemy too close for spell",
            NextAction::array(0, new NextAction("flee", 100.0f), NULL)));
    }

    // Universal fire prot pot — covers Flame Breath cleave (P1/P3) and the P2
    // Deep Breath corridor.
    triggers.push_back(new TriggerNode(
        "fire protection potion ready",
        NextAction::array(0, new NextAction("fire protection potion", 100.0f), NULL)));

    // Phase 2 specific. When Onyxia is hovering (aura 17131 from boss_onyxia.cpp
    // SPELL_HOVER), melee can't reach her — but ranged/casters MUST keep firing on
    // her. Without this trigger ranged bots stop attacking because the AI thinks
    // the target became unreachable. The "attack target" action retargets the bot
    // onto Onyxia explicitly (high priority so DpsAssist doesn't override).
    //
    // Melee classes are left alone here: in P2 they should be helping the OT clean
    // up whelps, which the FindTargetForOffTankStrategy already handles by picking
    // the lowest-HP attacker. Ranged keeps DPSing the boss because their range
    // (~30y) reaches her at any normal altitude.
    if (ai->IsRanged(bot) || ai->IsHeal(bot))
    {
        triggers.push_back(new TriggerNode(
            "onyxia phase 2",
            NextAction::array(0, new NextAction("attack", 90.0f), NULL)));
    }

    // Phase 3 Bellowing Roar fear (18431). The actual anti-fear cooldowns
    // (Berserker Rage / Will of the Forsaken / Tremor Totem) are already wired
    // in class strategies. This trigger boosts their priority during the
    // 2-second cast window so they fire before the fear lands.
    triggers.push_back(new TriggerNode(
        "onyxia bellowing roar",
        NextAction::array(0, new NextAction("will of the forsaken", 100.0f),
                          new NextAction("berserker rage fear", 100.0f),
                          NULL)));
}
