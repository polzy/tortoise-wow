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

    // Kurinnaxx Mortal Wound (25646) — physical debuff stacks on MT, each
    // stack reduces healing by 10%. Threshold 4: at that point heals are
    // -40% effective, OT taunts so MT's stacks decay. Framework #4 pattern.
    class KurinnaxxMortalWoundSwapTrigger : public PartyOtherTankHasAuraStacksTrigger
    {
    public:
        KurinnaxxMortalWoundSwapTrigger(PlayerbotAI* ai)
            : PartyOtherTankHasAuraStacksTrigger(ai, "kurinnaxx mortal wound swap", 25646, 4, 1) {}
    };

    // Framework #10 wire: Kurinnaxx Sand Trap (GO 180647). Standing in
    // the trap roots + ticks damage. Range 8y triggers move-away.
    class KurinnaxxSandTrapNearbyTrigger : public NearbyHazardGameObjectTrigger
    {
    public:
        KurinnaxxSandTrapNearbyTrigger(PlayerbotAI* ai)
            : NearbyHazardGameObjectTrigger(ai, "kurinnaxx sand trap nearby",
                180647 /* GO_SAND_TRAP */, 8.0f) {}
    };

    // Ossirian Sand Vortex (creature 15428, not a GO). Chases raid, gives
    // Ossirian back his weakness immunity if it touches him. Bots within
    // 12y need to move out so the vortex doesn't path-find onto the boss.
    // ScriptDev2 boss_ossirian.cpp NPC_SAND_VORTEX = 15428.
    class OssirianSandVortexNearbyTrigger : public Trigger
    {
    public:
        OssirianSandVortexNearbyTrigger(PlayerbotAI* ai) : Trigger(ai, "ossirian sand vortex nearby", 1) {}
        bool IsActive() override
        {
            std::list<Unit*> vortexes;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 15428, 12.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(vortexes, check);
            Cell::VisitAllObjects(bot, searcher, 12.0f);
            for (Unit* v : vortexes)
                if (v && v->IsAlive()) return true;
            return false;
        }
    };
}
