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

    // Ossirian Curse of Tongues (25195) — curse, -50% casting speed on raid
    // casters. Dispelable with 'remove curse' (druid/mage). ScriptDev2
    // boss_ossirian.cpp:36 SPELL_CURSE_OF_TONGUES.
    //
    // Note: scans the whole raid for the aura. Ossirian's curse lands on the
    // current victim (typically the tank, warrior/paladin — no 'remove
    // curse'). The trigger must fire for the druid/mage bots in the group so
    // they cleanse the tank.
    class OssirianCurseOfTonguesTrigger : public PartyHasAuraBySpellIdTrigger
    {
    public:
        OssirianCurseOfTonguesTrigger(PlayerbotAI* ai)
            : PartyHasAuraBySpellIdTrigger(ai, "ossirian curse of tongues", 25195, 1) {}
    };
}
