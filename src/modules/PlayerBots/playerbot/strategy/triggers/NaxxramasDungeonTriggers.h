#pragma once
#include "DungeonTriggers.h"
#include "GenericTriggers.h"

namespace ai
{
    class NaxxramasEnterDungeonTrigger : public EnterDungeonTrigger
    {
    public:
        NaxxramasEnterDungeonTrigger(PlayerbotAI* ai) : EnterDungeonTrigger(ai, "enter naxxramas", "naxxramas", 533) {}
    };

    class NaxxramasLeaveDungeonTrigger : public LeaveDungeonTrigger
    {
    public:
        NaxxramasLeaveDungeonTrigger(PlayerbotAI* ai) : LeaveDungeonTrigger(ai, "leave naxxramas", "naxxramas", 533) {}
    };

    class FourHorsemanStartFightTrigger : public StartBossFightTrigger
    {
    public:
        FourHorsemanStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start four horseman fight", "four horseman", 16062) {}
    };

    class FourHorsemanEndFightTrigger : public EndBossFightTrigger
    {
    public:
        FourHorsemanEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end four horseman fight", "four horseman", 16062) {}
    };

    // --- Patchwerk (16028) ---
    class PatchwerkStartFightTrigger : public StartBossFightTrigger
    {
    public:
        PatchwerkStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start patchwerk fight", "patchwerk", 16028) {}
    };
    class PatchwerkEndFightTrigger : public EndBossFightTrigger
    {
    public:
        PatchwerkEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end patchwerk fight", "patchwerk", 16028) {}
    };

    // --- Loatheb (16011) ---
    class LoathebStartFightTrigger : public StartBossFightTrigger
    {
    public:
        LoathebStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start loatheb fight", "loatheb", 16011) {}
    };
    class LoathebEndFightTrigger : public EndBossFightTrigger
    {
    public:
        LoathebEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end loatheb fight", "loatheb", 16011) {}
    };

    // --- Kel'Thuzad (15990) ---
    class KelThuzadStartFightTrigger : public StartBossFightTrigger
    {
    public:
        KelThuzadStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start kelthuzad fight", "kel'thuzad", 15990) {}
    };
    class KelThuzadEndFightTrigger : public EndBossFightTrigger
    {
    public:
        KelThuzadEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end kelthuzad fight", "kel'thuzad", 15990) {}
    };

    // Mana Detonation (27819): magic debuff that drains mana and explodes on the
    // target for AoE damage. Dispel asap to interrupt both.
    class KelThuzadManaDetonationTrigger : public Trigger
    {
    public:
        KelThuzadManaDetonationTrigger(PlayerbotAI* ai) : Trigger(ai, "kelthuzad mana detonation", 1) {}
        bool IsActive() override { return ai->HasAura(27819, bot); }
    };

    // --- Sapphiron (15989) ---
    class SapphironStartFightTrigger : public StartBossFightTrigger
    {
    public:
        SapphironStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start sapphiron fight", "sapphiron", 15989) {}
    };
    class SapphironEndFightTrigger : public EndBossFightTrigger
    {
    public:
        SapphironEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end sapphiron fight", "sapphiron", 15989) {}
    };

    // Life Drain (28542) — magic debuff, dispelable. Drains 1000 mana per tick
    // and heals Sapphiron for 4x the amount. Priority dispel for casters.
    class SapphironLifeDrainTrigger : public Trigger
    {
    public:
        SapphironLifeDrainTrigger(PlayerbotAI* ai) : Trigger(ai, "sapphiron life drain", 1) {}
        bool IsActive() override { return ai->HasAura(28542, bot); }
    };

    // --- Maexxna Necrotic Poison (28776) — poison, 90% healing reduction. ---
    class MaexxnaNecroticPoisonTrigger : public Trigger
    {
    public:
        MaexxnaNecroticPoisonTrigger(PlayerbotAI* ai) : Trigger(ai, "maexxna necrotic poison", 1) {}
        bool IsActive() override { return ai->HasAura(28776, bot); }
    };

    // --- Noth Curse of Plaguebringer (29213) — curse, lethal damage tick. ---
    class NothCursePlaguebringerTrigger : public Trigger
    {
    public:
        NothCursePlaguebringerTrigger(PlayerbotAI* ai) : Trigger(ai, "noth curse plaguebringer", 1) {}
        bool IsActive() override { return ai->HasAura(29213, bot); }
    };

    // --- Four Horsemen mark stacks ---
    // Marks (28832 Korth'azz fire / 28833 Blaumeux shadow / 28834 Mograine
    // unholy / 28835 Zeliek holy) stack on every Horseman cast (~12s). At 4
    // stacks the next stack lands as 5 and the dmg is lethal — players swap to
    // the opposite Horseman to drop stacks. Bot side: detect ≥3 stacks of any
    // mark on bot and route into the dispel chain.
    // Source: ScriptDev2 boss_four_horsemen.cpp SPELL_MARK_OF_*.
    class FourHorsemenMarkDangerTrigger : public Trigger
    {
    public:
        FourHorsemenMarkDangerTrigger(PlayerbotAI* ai) : Trigger(ai, "four horsemen mark danger", 2) {}
        bool IsActive() override
        {
            // Mark auras stack; we don't have a stack-count helper exposed via
            // the bot AI surface here. Detection collapses to "has any of the
            // four mark auras" — the trigger then fires the dispel chain
            // every tick the mark is present. Dispellers naturally throttle
            // (cooldown / global) so this stays cheap and reactive.
            return ai->HasAura(28832, bot) || ai->HasAura(28833, bot)
                || ai->HasAura(28834, bot) || ai->HasAura(28835, bot);
        }
    };
}