
#include "playerbot/playerbot.h"
#include "playerbot/strategy/Action.h"
#include "playerbot/strategy/generic/PullStrategy.h"
#include "PullTriggers.h"
#include "playerbot/strategy/values/PositionValue.h"
#include "playerbot/strategy/actions/PullActions.h"

using namespace ai;

bool PullStartTrigger::IsActive()
{
    const PullStrategy* strategy = PullStrategy::Get(ai);
    return strategy && strategy->IsPullPendingToStart();
}

bool ShouldPullTrigger::IsActive()
{
    // Dungeons only, deliberately. Outdoors a bot already has grinding and travel
    // behaviour that this would compete with, and a pull that goes wrong out there
    // only adds to the death numbers. Inside, somebody has to start the fight or
    // the group stands around until a real player does it.
    Map* map = bot->GetMap();
    if (!map || !map->IsDungeon())
        return false;

    // IsDungeon() is true for raids too. Keep auto-pull out of raids: our raid
    // stack gates engages through the pro-engage layer and the master's call —
    // a tank deciding to open on Ragnaros because the healer has mana is a wipe.
    if (map->IsRaid())
        return false;

    if (!PlayerbotAI::IsTank(bot))
        return false;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->getSource();
        if (!member || !member->IsInWorld() || member->GetMapId() != bot->GetMapId())
            continue;

        // Never pull on top of a fight that is still running, and never onto a
        // corpse - somebody has to be raised first.
        if (member->IsInCombat() || !member->IsAlive())
            return false;

        // The healer decides the pace. Pulling with an empty healer is how a
        // group wipes on trash it could otherwise walk through.
        if (PlayerbotAI::IsHeal(member) && member->GetPowerType() == POWER_MANA)
        {
            const uint32 maxMana = member->GetMaxPower(POWER_MANA);
            if (maxMana && (100 * member->GetPower(POWER_MANA)) / maxMana < sPlayerbotAIConfig.mediumMana)
                return false;
        }
    }

    return PullNearestTargetAction::FindPullTarget(ai) != nullptr;
}

bool PullEndTrigger::IsActive()
{
    const PullStrategy* strategy = PullStrategy::Get(ai);
    if (strategy && strategy->HasPullStarted())
    {
        Unit* target = strategy->GetTarget();
        if (target)
        {
            // Check if the pull is taking too long
            const time_t secondsSincePullStarted = time(0) - strategy->GetPullStartTime();
            if (secondsSincePullStarted >= strategy->GetMaxPullTime())
            {
                return true;
            }
            else
            {
                float distanceToPullTarget = target->GetDistance(ai->GetBot());
                // sometimes creatures can reach slightly more than normal attack distance
                float creatureMeleeRange = ATTACK_DISTANCE + BASE_MELEERANGE_OFFSET + 1;

                if (distanceToPullTarget <= creatureMeleeRange || (target->IsNonMeleeSpellCasted(true) && target->IsInCombat()) || (secondsSincePullStarted >= 10 && target->GetTarget() != bot) || (ai->IsRanged(bot) && distanceToPullTarget <= ai->GetRange("spell")))
                {
                    if (ai->HasStrategy("pull back", BotState::BOT_STATE_COMBAT))
                    {
                        PositionMap& posMap = AI_VALUE(PositionMap&, "position");
                        PositionEntry pullPosition = posMap["pull"];
                        if (pullPosition.isSet())
                        {
                            distanceToPullTarget = bot->GetDistance(pullPosition.x, pullPosition.y, pullPosition.z);
                            return distanceToPullTarget <= ai->GetRange("follow");
                        }
                    }

                    // Check if the pulled target has approached the bot
                    return true;
                }
            }
        }
    }

    return false;
}
