#pragma once
#include "DungeonTriggers.h"

namespace ai
{
    class OnyxiasLairEnterDungeonTrigger : public EnterDungeonTrigger
    {
    public:
        OnyxiasLairEnterDungeonTrigger(PlayerbotAI* ai) : EnterDungeonTrigger(ai, "enter onyxia's lair", "onyxia's lair", 249) {}
    };

    class OnyxiasLairLeaveDungeonTrigger : public LeaveDungeonTrigger
    {
    public:
        OnyxiasLairLeaveDungeonTrigger(PlayerbotAI* ai) : LeaveDungeonTrigger(ai, "leave onyxia's lair", "onyxia's lair", 249) {}
    };

    class OnyxiaStartFightTrigger : public StartBossFightTrigger
    {
    public:
        OnyxiaStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start onyxia fight", "onyxia", 10184) {}
    };

    class OnyxiaEndFightTrigger : public EndBossFightTrigger
    {
    public:
        OnyxiaEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end onyxia fight", "onyxia", 10184) {}
    };

    // Fires when an Onyxia (entry 10184) in sight is currently casting spell 17086
    // ("Breath", the P2 Deep Breath). The action "move away from onyxia breath" is
    // wired in OnyxiaFightStrategy::InitReactionTriggers.
    class OnyxiaDeepBreathTrigger : public Trigger
    {
    public:
        OnyxiaDeepBreathTrigger(PlayerbotAI* ai) : Trigger(ai, "onyxia deep breath") {}

        bool IsActive() override
        {
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 10184, 80.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 80.0f);
            for (Unit* u : units)
            {
                if (!u) continue;
                Spell* cast = u->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                if (!cast) cast = u->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
                if (cast && cast->m_spellInfo && cast->m_spellInfo->Id == 17086)
                    return true;
            }
            return false;
        }
    };
}