#pragma once
#include "DungeonActions.h"
#include "ChangeStrategyAction.h"

namespace ai
{
    class OnyxiasLairEnableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        OnyxiasLairEnableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable onyxia's lair strategy", "+onyxia's lair") {}
    };

    class OnyxiasLairDisableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        OnyxiasLairDisableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable onyxia's lair strategy", "-onyxia's lair") {}
    };

    class OnyxiaEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        OnyxiaEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable onyxia fight strategy", "+onyxia") {}
    };

    class OnyxiaDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        OnyxiaDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable onyxia fight strategy", "-onyxia") {}
    };

    // Move away from Onyxia (creature entry 10184) in any direction up to 40y. The base
    // MoveAwayFromCreature sweeps for a valid angle behind the bot relative to the creature,
    // so even though we don't compute a true perpendicular-to-flight-path vector, the bot
    // ends up out of the Deep Breath corridor in the majority of cases. 40y matches the
    // observed reach of the fire patch P2 cast leaves on the ground.
    class OnyxiaMoveAwayFromBreathAction : public MoveAwayFromCreature
    {
    public:
        OnyxiaMoveAwayFromBreathAction(PlayerbotAI* ai)
            : MoveAwayFromCreature(ai, "move away from onyxia breath", 10184, 40.0f) {}
    };

    // Force-attack Onyxia herself (entry 10184), bypassing the regular
    // current-target / tank-target / dps-assist target selection. Used in P2 so
    // ranged/heal bots keep DPSing the boss while whelps engage them — without
    // this, they reactively switch to whatever whelp is hitting them and never
    // touch Onyxia again. The action sets the bot's current target to her and
    // routes through the standard AttackAction execute path (move-to-spell-range +
    // auto-cast).
    class AttackOnyxiaAction : public Action
    {
    public:
        AttackOnyxiaAction(PlayerbotAI* ai) : Action(ai, "attack onyxia") {}

        bool Execute(Event& event) override
        {
            // Find Onyxia within 120y (covers the whole instance).
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 10184, 120.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 120.0f);

            Unit* onyxia = nullptr;
            for (Unit* u : units)
            {
                if (u && u->IsAlive())
                {
                    onyxia = u;
                    break;
                }
            }
            if (!onyxia)
                return false;

            // Pin the AI context target to Onyxia and let the bot's existing
            // ranged-cast pipeline (reach spell, cast, etc.) handle the rest.
            context->GetValue<ObjectGuid>("attack target")->Set(onyxia->GetObjectGuid());
            context->GetValue<Unit*>("current target")->Set(onyxia);
            // Update selection so spell targeting uses Onyxia.
            bot->SetSelectionGuid(onyxia->GetObjectGuid());
            ai->ChangeEngine(BotState::BOT_STATE_COMBAT);
            return true;
        }

        bool isUseful() override
        {
            // Only useful if a live Onyxia is actually in sight; otherwise let the
            // regular target-selection path stay in charge.
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 10184, 120.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 120.0f);
            for (Unit* u : units)
                if (u && u->IsAlive())
                    return true;
            return false;
        }
    };
}
