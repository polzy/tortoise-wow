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

    // Fires when an Onyxia (entry 10184) in sight is casting any of the 8 Deep Breath
    // directional variants. Spell IDs taken from boss_onyxia.cpp ScriptDev2 source:
    //   17086 N->S, 18351 S->N, 18576 E->W, 18609 W->E,
    //   18564 SE->NW, 18584 NW->SE, 18596 SW->NE, 18617 NE->SW.
    // The action "move away from onyxia breath" is wired in
    // OnyxiaFightStrategy::InitReactionTriggers.
    class OnyxiaDeepBreathTrigger : public Trigger
    {
    public:
        OnyxiaDeepBreathTrigger(PlayerbotAI* ai) : Trigger(ai, "onyxia deep breath") {}

        bool IsActive() override
        {
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 10184, 100.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 100.0f);

            static const uint32 BREATH_SPELL_IDS[8] = {
                17086, 18351, 18576, 18609, 18564, 18584, 18596, 18617
            };

            for (Unit* u : units)
            {
                if (!u) continue;
                Spell* cast = u->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                if (!cast) cast = u->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
                if (!cast || !cast->m_spellInfo) continue;
                const uint32 id = cast->m_spellInfo->Id;
                for (uint32 b : BREATH_SPELL_IDS)
                    if (id == b)
                        return true;
            }
            return false;
        }
    };

    // Fires when Onyxia is in Phase 2 (flying / hovering, untargetable by melee).
    // Phase detection uses spell 17131 "Hover" which the AI casts on phase transition,
    // OR the boss's `IsFlying()` state. Bots that match this trigger should keep
    // targeting Onyxia (she is still attackable at range in P2) while the OT (if any)
    // handles the whelp adds spawned during this phase.
    class OnyxiaPhase2Trigger : public Trigger
    {
    public:
        OnyxiaPhase2Trigger(PlayerbotAI* ai) : Trigger(ai, "onyxia phase 2") {}

        bool IsActive() override
        {
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 10184, 120.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 120.0f);
            for (Unit* u : units)
            {
                if (!u) continue;
                // Hover aura 17131 is applied for the whole P2 duration.
                if (u->HasAura(17131))
                    return true;
                // Fallback: she's airborne (movement flag or fly state).
                if (u->m_movementInfo.HasMovementFlag(MOVEFLAG_LEVITATING) ||
                    u->m_movementInfo.HasMovementFlag(MOVEFLAG_FLYING))
                    return true;
            }
            return false;
        }
    };

    // Fires when Onyxia is casting Bellowing Roar (18431 in P3, the AOE fear).
    // Tanks/melee should use anti-fear (Berserker Rage, Will of the Forsaken, etc.)
    // — those actions are already wired in their class strategies; this trigger just
    // amplifies the priority during the cast window.
    class OnyxiaBellowingRoarTrigger : public Trigger
    {
    public:
        OnyxiaBellowingRoarTrigger(PlayerbotAI* ai) : Trigger(ai, "onyxia bellowing roar") {}

        bool IsActive() override
        {
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 10184, 60.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 60.0f);
            for (Unit* u : units)
            {
                if (!u) continue;
                Spell* cast = u->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                if (cast && cast->m_spellInfo && cast->m_spellInfo->Id == 18431)
                    return true;
            }
            return false;
        }
    };
}