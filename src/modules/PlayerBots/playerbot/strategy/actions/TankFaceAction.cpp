
#include "playerbot/playerbot.h"
#include "TankFaceAction.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "playerbot/ServerFacade.h"

using namespace ai;

// Everything below ~120 degrees between (boss->tank) and (boss->raid center)
// counts as "raid in the danger cone". Cos threshold: cos(120 deg) = -0.5.
static const float TANK_FACE_COS_SAFE = -0.5f;

bool TankFaceAction::NeedsReposition(PlayerbotAI* ai, Player* bot, float* outRaidX, float* outRaidY)
{
    if (!ai->IsTank(bot))
        return false;

    AiObjectContext* context = ai->GetAiObjectContext();
    Unit* target = context->GetValue<Unit*>("current target")->Get();
    if (!target || !target->IsAlive() || target->IsPlayer())
        return false;

    Creature* creature = (Creature*)target;
    if (!creature->IsElite())
        return false;

    // Only the tank the boss is actually hitting should reposition it.
    if (target->GetVictim() != bot)
        return false;

    // Boss must already be in melee contact, otherwise we are still pulling.
    if (sServerFacade.GetDistance2d(bot, target) > ATTACK_DISTANCE)
        return false;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    // Barycenter of the non-tank group members near the fight.
    float raidX = 0.f, raidY = 0.f;
    uint32 count = 0;
    Group::MemberSlotList const& groupSlot = group->GetMemberSlots();
    for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); ++itr)
    {
        Player* member = sObjectMgr.GetPlayer(itr->guid);
        if (!member || member == bot || !sServerFacade.IsAlive(member))
            continue;

        if (member->GetMapId() != bot->GetMapId())
            continue;

        if (sServerFacade.GetDistance2d(member, target) > sPlayerbotAIConfig.sightDistance)
            continue;

        PlayerbotAI* memberAi = member->GetPlayerbotAI();
        if ((memberAi && memberAi->IsTank(member)) || (!memberAi && ai->IsTank(member)))
            continue;

        raidX += member->GetPositionX();
        raidY += member->GetPositionY();
        ++count;
    }

    if (count < 2)
        return false;

    raidX /= count;
    raidY /= count;

    if (outRaidX) *outRaidX = raidX;
    if (outRaidY) *outRaidY = raidY;

    // Angle at the boss between the tank and the raid center. If the raid is
    // roughly behind the boss (angle close to 180 deg) everything is fine.
    float toTankX = bot->GetPositionX() - target->GetPositionX();
    float toTankY = bot->GetPositionY() - target->GetPositionY();
    float toRaidX = raidX - target->GetPositionX();
    float toRaidY = raidY - target->GetPositionY();

    float lenTank = sqrtf(toTankX * toTankX + toTankY * toTankY);
    float lenRaid = sqrtf(toRaidX * toRaidX + toRaidY * toRaidY);
    if (lenTank < 0.1f || lenRaid < 0.1f)
        return false;

    float cosAngle = (toTankX * toRaidX + toTankY * toRaidY) / (lenTank * lenRaid);
    return cosAngle > TANK_FACE_COS_SAFE;
}

bool TankFaceAction::isUseful()
{
    return NeedsReposition(ai, bot);
}

bool TankFaceAction::Execute(Event& event)
{
    float raidX = 0.f, raidY = 0.f;
    if (!NeedsReposition(ai, bot, &raidX, &raidY))
        return false;

    Unit* target = AI_VALUE(Unit*, "current target");
    if (!target)
        return false;

    // Anchor on the far side of the boss from the raid, inside melee reach so
    // we never drop aggro contact while stepping through.
    float awayX = target->GetPositionX() - raidX;
    float awayY = target->GetPositionY() - raidY;
    float len = sqrtf(awayX * awayX + awayY * awayY);
    if (len < 0.1f)
        return false;

    float reach = std::max(sPlayerbotAIConfig.contactDistance, bot->GetCombinedCombatReach(target, true) * 0.7f);
    float destX = target->GetPositionX() + (awayX / len) * reach;
    float destY = target->GetPositionY() + (awayY / len) * reach;
    float destZ = target->GetPositionZ();
    bot->UpdateAllowedPositionZ(destX, destY, destZ);

    return MoveTo(bot->GetMapId(), destX, destY, destZ, false, true);
}
