#pragma once
#include "DungeonActions.h"
#include "ChangeStrategyAction.h"
#include "UseItemAction.h"

namespace ai
{
    class MoltenCoreEnableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        MoltenCoreEnableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable molten core strategy", "+molten core") {}
    };

    class MoltenCoreDisableDungeonStrategyAction : public ChangeAllStrategyAction
    {
    public:
        MoltenCoreDisableDungeonStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable molten core strategy", "-molten core") {}
    };

    class MagmadarEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        MagmadarEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable magmadar fight strategy", "+magmadar") {}
    };

    class MagmadarDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        MagmadarDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable magmadar fight strategy", "-magmadar") {}
    };

    class MagmadarMoveAwayFromLavaBombAction : public MoveAwayFromHazard
    {
    public:
        MagmadarMoveAwayFromLavaBombAction(PlayerbotAI* ai) : MoveAwayFromHazard(ai, "move away from magmadar lava bomb") {}
    };

    class MagmadarMoveAwayAction : public MoveAwayFromCreature
    {
    public:
        MagmadarMoveAwayAction(PlayerbotAI* ai) : MoveAwayFromCreature(ai, "move away from magmadar", 11982, 31.0f) {}
    };

    class MoveToMCRuneAction : public MoveToAction
    {
    public:
        MoveToMCRuneAction(PlayerbotAI* ai) : MoveToAction(ai, "move to mc rune") { qualifier = "entry filter::{gos in sight,mc runes}"; }
    };

    class DouseMCRuneActionAqual : public UseItemIdAction
    {
    public:
        DouseMCRuneActionAqual(PlayerbotAI* ai) : UseItemIdAction(ai, "douse mc rune aqual") { qualifier = "{17333,entry filter::{gos close,mc runes}}"; }
    };

    class DouseMCRuneActionEternal : public UseItemIdAction
    {
    public:
        DouseMCRuneActionEternal(PlayerbotAI* ai) : UseItemIdAction(ai, "douse mc rune eternal") { qualifier = "{22754,entry filter::{gos close,mc runes}}"; }
    };

    // --- Lucifron (12118) ---
    class LucifronEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        LucifronEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable lucifron fight strategy", "+lucifron") {}
    };

    class LucifronDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        LucifronDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable lucifron fight strategy", "-lucifron") {}
    };

    // --- Gehennas (12259) ---
    class GehennasEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        GehennasEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable gehennas fight strategy", "+gehennas") {}
    };

    class GehennasDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        GehennasDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable gehennas fight strategy", "-gehennas") {}
    };

    // --- Garr (12057) ---
    class GarrEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        GarrEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable garr fight strategy", "+garr") {}
    };

    class GarrDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        GarrDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable garr fight strategy", "-garr") {}
    };

    // --- Baron Geddon (12056) ---
    class BaronGeddonEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        BaronGeddonEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable baron geddon fight strategy", "+baron geddon") {}
    };

    class BaronGeddonDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        BaronGeddonDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable baron geddon fight strategy", "-baron geddon") {}
    };

    // Move away from raid when afflicted with Living Bomb - flee from boss to avoid splash on raid
    class BaronGeddonLivingBombMoveAwayAction : public MovementAction
    {
    public:
        BaronGeddonLivingBombMoveAwayAction(PlayerbotAI* ai) : MovementAction(ai, "baron geddon living bomb move away") {}
        bool Execute(Event& event) override
        {
            return Flee(AI_VALUE(Unit*, "current target"));
        }
    };

    // Move away from Baron Geddon during Inferno
    class BaronGeddonMoveAwayAction : public MoveAwayFromCreature
    {
    public:
        BaronGeddonMoveAwayAction(PlayerbotAI* ai) : MoveAwayFromCreature(ai, "move away from baron geddon", 12056, 21.0f) {}
    };

    // --- Shazzrah (12264) ---
    class ShazzrahEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        ShazzrahEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable shazzrah fight strategy", "+shazzrah") {}
    };

    class ShazzrahDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        ShazzrahDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable shazzrah fight strategy", "-shazzrah") {}
    };

    // --- Sulfuron Harbinger (12098) ---
    class SulfuronEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        SulfuronEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable sulfuron fight strategy", "+sulfuron") {}
    };

    class SulfuronDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        SulfuronDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable sulfuron fight strategy", "-sulfuron") {}
    };

    // --- Golemagg (11988) ---
    class GolemaggEnableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        GolemaggEnableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "enable golemagg fight strategy", "+golemagg") {}
    };

    class GolemaggDisableFightStrategyAction : public ChangeAllStrategyAction
    {
    public:
        GolemaggDisableFightStrategyAction(PlayerbotAI* ai) : ChangeAllStrategyAction(ai, "disable golemagg fight strategy", "-golemagg") {}
    };

    // Ranged/healers stay away from Golemagg (Magma Splash)
    class GolemaggMoveAwayAction : public MoveAwayFromCreature
    {
    public:
        GolemaggMoveAwayAction(PlayerbotAI* ai) : MoveAwayFromCreature(ai, "move away from golemagg", 11988, 31.0f) {}
    };
}