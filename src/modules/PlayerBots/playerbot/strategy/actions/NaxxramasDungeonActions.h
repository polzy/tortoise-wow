#pragma once
#include "DungeonActions.h"
#include "ChangeStrategyAction.h"
#include "UseItemAction.h"
#include "MovementActions.h"
#include "Maps/GridNotifiers.h"
#include "Maps/GridNotifiersImpl.h"
#include "Maps/CellImpl.h"
#include "playerbot/strategy/values/NearestGameObjects.h"

namespace ai
{
    // Sapphiron Phase 2 (air): casts Frost Breath (28524, 7s) blocked by LOS
    // via GO_ICEBLOCK (181247) spawned where icebolted players stood. Action
    // finds nearest ice block within 50y and moves to its position. Bot ends
    // up next to the ice block, which puts Sapphiron's LOS line through it.
    // ScriptDev2 boss_sapphiron.cpp:56 GO_ICEBLOCK = 181247.
    // Anub'Rekhan Locust Swarm — move 30y from the boss (15956). The AOE
    // tick is ~20y radius; 30y gives a buffer for path completion.
    class MoveAwayFromAnubRekhanLocustSwarmAction : public MoveAwayFromCreature
    {
    public:
        MoveAwayFromAnubRekhanLocustSwarmAction(PlayerbotAI* ai)
            : MoveAwayFromCreature(ai, "move away from anubrekhan locust swarm", 15956, 30.0f) {}
    };

    class HideBehindSapphironIceBlockAction : public MovementAction
    {
    public:
        HideBehindSapphironIceBlockAction(PlayerbotAI* ai)
            : MovementAction(ai, "hide behind sapphiron ice block") {}

        bool Execute(Event& event) override
        {
            Player* bot = ai->GetBot();
            if (!bot) return false;

            std::list<GameObject*> blocks;
            GameObjectsInObjectRangeCheck check(bot, 50.0f, 181247);
            MaNGOS::GameObjectListSearcher<GameObjectsInObjectRangeCheck> searcher(blocks, check);
            Cell::VisitAllObjects(bot, searcher, 50.0f);

            GameObject* closest = nullptr;
            float bestDist = 1e9f;
            for (GameObject* go : blocks)
            {
                if (!go) continue;
                float d = bot->GetDistance(go);
                if (d < bestDist) { bestDist = d; closest = go; }
            }

            if (!closest) return false;
            return MoveTo(closest->GetMapId(), closest->GetPositionX(), closest->GetPositionY(), closest->GetPositionZ());
        }
    };

    class NaxxramasEnableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        NaxxramasEnableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable naxxramas strategy", "+naxxramas") {}
    };

    class NaxxramasDisableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        NaxxramasDisableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable naxxramas strategy", "-naxxramas") {}
    };

    class FourHorsemanEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        FourHorsemanEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable four horseman fight strategy", "+four horseman") {}
    };

    class FourHorsemanDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        FourHorsemanDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable four horseman fight strategy", "-four horseman") {}
    };

    // Naxx boss enable/disable actions
    class PatchwerkEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        PatchwerkEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable patchwerk fight strategy", "+patchwerk") {}
    };
    class PatchwerkDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        PatchwerkDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable patchwerk fight strategy", "-patchwerk") {}
    };
    class LoathebEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        LoathebEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable loatheb fight strategy", "+loatheb") {}
    };
    class LoathebDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        LoathebDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable loatheb fight strategy", "-loatheb") {}
    };
    class KelThuzadEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        KelThuzadEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable kelthuzad fight strategy", "+kel'thuzad") {}
    };
    class KelThuzadDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        KelThuzadDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable kelthuzad fight strategy", "-kel'thuzad") {}
    };

    class SapphironEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        SapphironEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable sapphiron fight strategy", "+sapphiron") {}
    };
    class SapphironDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        SapphironDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable sapphiron fight strategy", "-sapphiron") {}
    };
}