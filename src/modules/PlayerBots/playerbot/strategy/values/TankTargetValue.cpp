
#include "playerbot/playerbot.h"
#include "TankTargetValue.h"
#include "PossibleAttackTargetsValue.h"

using namespace ai;

class FindTargetForTankStrategy : public FindNonCcTargetStrategy
{
public:
    FindTargetForTankStrategy(PlayerbotAI* ai) : FindNonCcTargetStrategy(ai)
    {
        minThreat = 0;
    }

public:
    virtual void CheckAttacker(Unit* creature, ThreatManager* threatManager) override
    {
        Player* bot = ai->GetBot();
        AiObjectContext* context = ai->GetAiObjectContext();

        if (IsCcTarget(creature)) return;

        if (!PossibleAttackTargetsValue::IsValid(creature, bot))
        {
            std::list<ObjectGuid> attackers = AI_VALUE(std::list<ObjectGuid>, "possible attack targets");
            if (std::find(attackers.begin(), attackers.end(), creature->GetObjectGuid()) == attackers.end())
                return;
        }

        float threat = threatManager->getThreat(bot);
        if (!result || (minThreat - threat) > 0.1f)
        {
            minThreat = threat;
            result = creature;
        }
    }

protected:
    float minThreat;
};


// Strategy used by off-tanks: prefer the LOWEST-HP attacker the bot can reach. The
// rationale: in raid fights with adds (Geddon, Lucifron, Garr Firesworn, Onyxia P2
// whelps, etc.) the main boss has 1M+ HP while adds top out at ~30-100k. Picking
// the lowest-HP attacker reliably biases the OT toward adds without requiring
// explicit add-detection logic. RTI override at the call site still wins if the
// master has marked a specific target.
class FindTargetForOffTankStrategy : public FindNonCcTargetStrategy
{
public:
    FindTargetForOffTankStrategy(PlayerbotAI* ai) : FindNonCcTargetStrategy(ai), minHp(0) {}

    virtual void CheckAttacker(Unit* creature, ThreatManager* /*threatManager*/) override
    {
        Player* bot = ai->GetBot();
        AiObjectContext* context = ai->GetAiObjectContext();  // needed for AI_VALUE expansion

        if (IsCcTarget(creature)) return;

        if (!PossibleAttackTargetsValue::IsValid(creature, bot))
        {
            std::list<ObjectGuid> attackers = AI_VALUE(std::list<ObjectGuid>, "possible attack targets");
            if (std::find(attackers.begin(), attackers.end(), creature->GetObjectGuid()) == attackers.end())
                return;
        }

        const uint32 hp = creature->GetMaxHealth();
        if (!result || hp < minHp)
        {
            minHp = hp;
            result = creature;
        }
    }

protected:
    uint32 minHp;
};


Unit* TankTargetValue::Calculate()
{
    Unit* rti = RtiTargetValue::Calculate();
    if (rti) return rti;

    // Off-tank branch: switch target priority to lowest-HP attacker so OTs naturally
    // pick up adds instead of fighting over the boss with the MT. The MT keeps the
    // standard "lowest current threat among attackers" logic, which corresponds to
    // "the thing most likely to slip aggro" — appropriate for boss-tank duty.
    if (PlayerbotAI::IsOffTank(ai->GetBot()))
    {
        FindTargetForOffTankStrategy otStrat(ai);
        return FindTarget(&otStrat);
    }

    FindTargetForTankStrategy strategy(ai);
    return FindTarget(&strategy);
}
