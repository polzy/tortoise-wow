#pragma once
#include "DungeonActions.h"
#include "ChangeStrategyAction.h"
#include "UseItemAction.h"

namespace ai
{
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
}