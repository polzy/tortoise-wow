#pragma once
#include "DungeonTriggers.h"
#include "GenericTriggers.h"

// AQ20 (Ruins of Ahn'Qiraj, map 509). Boss + add entries lifted from
// `src/scripts/dungeons/ruins_of_ahnqiraj/`. Strategy coverage starts with
// the pro-engage adds for Moam / Buru / Ayamiss; per-boss fight strategies
// can be wired later as we cover more mechanics (Ossirian tornado kite,
// Rajaxx wave commanders, Kurinnaxx mortal wound).

namespace ai
{
    class RuinsOfAhnQirajEnterDungeonTrigger : public EnterDungeonTrigger
    {
    public:
        RuinsOfAhnQirajEnterDungeonTrigger(PlayerbotAI* ai)
            : EnterDungeonTrigger(ai, "enter ruins of ahnqiraj", "ruins of ahnqiraj", 509) {}
    };

    class RuinsOfAhnQirajLeaveDungeonTrigger : public LeaveDungeonTrigger
    {
    public:
        RuinsOfAhnQirajLeaveDungeonTrigger(PlayerbotAI* ai)
            : LeaveDungeonTrigger(ai, "leave ruins of ahnqiraj", "ruins of ahnqiraj", 509) {}
    };
}
