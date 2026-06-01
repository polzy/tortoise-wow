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
    // Anub'Rekhan Locust Swarm — move 30y from the boss (15956). Uses the
    // *AndStay* variant so the bot doesn't oscillate back into the 20s AOE
    // via re-engaged Chase movement.
    class MoveAwayFromAnubRekhanLocustSwarmAction : public MoveAwayAndStayFromCreature
    {
    public:
        MoveAwayFromAnubRekhanLocustSwarmAction(PlayerbotAI* ai)
            : MoveAwayAndStayFromCreature(ai, "move away from anubrekhan locust swarm", 15956, 30.0f) {}
    };

    // Heigan dance: move 20y from the nearest Plague Fissure creature
    // (533001). Reactive, not predictive. *AndStay* variant so the bot
    // doesn't immediately re-engage chase after dodging.
    class MoveAwayFromHeiganFissureAction : public MoveAwayAndStayFromCreature
    {
    public:
        MoveAwayFromHeiganFissureAction(PlayerbotAI* ai)
            : MoveAwayAndStayFromCreature(ai, "move away from heigan fissure", 533001, 20.0f) {}
    };

    // Framework #5 demo: Thaddius polarity. Each raid member carries either
    // Positive Charge (28059) or Negative Charge (28084). Same-polarity bots
    // must stack within ~10y; different-polarity bots must be >10y apart.
    // Action: find the same-polarity peer with the lowest GUID (deterministic
    // anchor across bots — they all pick the same one) and move within 5y of
    // it. Anchor approach avoids the centroid-on-Thaddius failure mode: if
    // peers are spread on both sides of the boss, the average X/Y lands on
    // the boss = melee = mixed-polarity stack = wipe. Anchoring to a single
    // peer guarantees the stack converges to one side. Code-review 2026-06-01
    // round 3 finding #2.
    class ThaddiusMoveToSamePolarityAction : public MovementAction
    {
    public:
        ThaddiusMoveToSamePolarityAction(PlayerbotAI* ai)
            : MovementAction(ai, "thaddius same polarity") {}

        bool Execute(Event& event) override
        {
            Player* bot = ai->GetBot();
            if (!bot) return false;

            uint32 myAura = 0;
            if (ai->HasAura(28059, bot)) myAura = 28059;
            else if (ai->HasAura(28084, bot)) myAura = 28084;
            else return false;  // No polarity assigned yet

            Group* group = bot->GetGroup();
            if (!group) return false;

            // Deterministic anchor: lowest ObjectGuid among same-polarity peers
            // (excluding self). All same-polarity bots compute the same anchor.
            Player* anchor = nullptr;
            for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
            {
                Player* member = ref->getSource();
                if (!member || member == bot) continue;
                if (member->GetMapId() != bot->GetMapId()) continue;
                if (!member->IsAlive()) continue;
                if (!ai->HasAura(myAura, member)) continue;
                if (!anchor || member->GetObjectGuid() < anchor->GetObjectGuid())
                    anchor = member;
            }
            if (!anchor) return false;  // No same-polarity peers (rare)

            // Already within stack range — keep position.
            if (bot->GetDistance(anchor) < 5.0f) return false;

            return MoveTo(anchor->GetMapId(), anchor->GetPositionX(),
                          anchor->GetPositionY(), anchor->GetPositionZ());
        }
    };

    // Hide on the FAR SIDE of the nearest ice block from Sapphiron — standing
    // AT the block leaves LOS clear to the airborne boss. ScriptDev2
    // boss_sapphiron.cpp:56 GO_ICEBLOCK = 181247, boss entry 15989.
    class HideBehindSapphironIceBlockAction : public MovementAction
    {
    public:
        HideBehindSapphironIceBlockAction(PlayerbotAI* ai)
            : MovementAction(ai, "hide behind sapphiron ice block") {}

        bool Execute(Event& event) override
        {
            Player* bot = ai->GetBot();
            if (!bot) return false;

            // Locate Sapphiron — needed to compute the "far side" of each block.
            std::list<Unit*> bosses;
            MaNGOS::AllCreaturesOfEntryInRangeCheck bossCheck(bot, 15989, 100.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> bossSearcher(bosses, bossCheck);
            Cell::VisitAllObjects(bot, bossSearcher, 100.0f);
            Unit* sapphiron = nullptr;
            for (Unit* u : bosses) { if (u && u->IsAlive()) { sapphiron = u; break; } }
            if (!sapphiron) return false;

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

            // Hide point = block_position + (block - sapphiron).normalized * 4y
            // → bot 4y past the block from Sapphiron's perspective → block sits
            // between bot and boss → horizontal LOS broken for the 7s breath.
            float dx = closest->GetPositionX() - sapphiron->GetPositionX();
            float dy = closest->GetPositionY() - sapphiron->GetPositionY();
            float len = sqrtf(dx * dx + dy * dy);
            float targetX, targetY;
            if (len < 0.001f) {
                targetX = closest->GetPositionX();
                targetY = closest->GetPositionY();
            } else {
                targetX = closest->GetPositionX() + (dx / len) * 4.0f;
                targetY = closest->GetPositionY() + (dy / len) * 4.0f;
            }
            return MoveTo(closest->GetMapId(), targetX, targetY, closest->GetPositionZ());
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

    // Framework #9 wire — Razuvious: command charmed DK Understudy (16803)
    // to attack Instructor Razuvious (16061). Only fires when the bot is
    // already charming something (CharmPetAttackTargetAction::isUseful).
    // The charm itself still needs to be established manually (priest uses
    // orb → MC cast); this primitive handles step 4 of the pipeline
    // documented in DungeonActions.h "Framework #9 scaffold".
    // ScriptDev2 boss_razuvious.cpp NPC_RAZUVIOUS = 16061 (implicit, the
    // boss creature_template entry); NPC_DK_UNDERSTUDY = 16803.
    class CommandUnderstudyAttackRazuviousAction : public CharmPetAttackTargetAction
    {
    public:
        CommandUnderstudyAttackRazuviousAction(PlayerbotAI* ai)
            : CharmPetAttackTargetAction(ai, "command understudy attack razuvious", 16061) {}
    };
}