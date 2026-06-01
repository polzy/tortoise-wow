#pragma once
#include "DungeonActions.h"
#include "ChangeStrategyAction.h"

namespace ai
{
    class RuinsOfAhnQirajEnableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        RuinsOfAhnQirajEnableDungeonStrategyAction(PlayerbotAI* ai)
            : ChangeAllStrategyAction(ai, "enable ruins of ahnqiraj strategy", "+ruins of ahnqiraj") {}
    };

    class RuinsOfAhnQirajDisableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        RuinsOfAhnQirajDisableDungeonStrategyAction(PlayerbotAI* ai)
            : ChangeAllStrategyAction(ai, "disable ruins of ahnqiraj strategy", "-ruins of ahnqiraj") {}
    };

    // Framework #10 wire — Kurinnaxx Sand Trap (GO 180647). Standing in
    // the trap roots the bot + ticks damage. 8y move-away. Subclasses
    // MoveAwayFromGameObject (which scans GameObjects by entry; range
    // tuned so we don't oscillate on the trap boundary).
    class MoveAwayFromKurinnaxxSandTrapAction : public MoveAwayFromGameObject
    {
    public:
        MoveAwayFromKurinnaxxSandTrapAction(PlayerbotAI* ai)
            : MoveAwayFromGameObject(ai, "move away from kurinnaxx sand trap",
                180647 /* GO_SAND_TRAP */, 8.0f) {}
    };

    // Framework #2 reuse — Ossirian Sand Vortex (creature 15428). Chases
    // raid; touching Ossirian removes his weakness. Use the move-and-
    // STAY variant so the bot doesn't oscillate back into the vortex
    // path after the first nudge.
    class MoveAwayFromOssirianSandVortexAction : public MoveAwayAndStayFromCreature
    {
    public:
        MoveAwayFromOssirianSandVortexAction(PlayerbotAI* ai)
            : MoveAwayAndStayFromCreature(ai, "move away from ossirian sand vortex",
                15428 /* NPC_SAND_VORTEX */, 12.0f) {}
    };
}
