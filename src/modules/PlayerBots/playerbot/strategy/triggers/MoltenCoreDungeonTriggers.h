#pragma once
#include "DungeonTriggers.h"
#include "GenericTriggers.h"

namespace ai
{
    class MoltenCoreEnterDungeonTrigger : public EnterDungeonTrigger
    {
    public:
        MoltenCoreEnterDungeonTrigger(PlayerbotAI* ai) : EnterDungeonTrigger(ai, "enter molten core", "molten core", 409) {}
    };

    class MoltenCoreLeaveDungeonTrigger : public LeaveDungeonTrigger
    {
    public:
        MoltenCoreLeaveDungeonTrigger(PlayerbotAI* ai) : LeaveDungeonTrigger(ai, "leave molten core", "molten core", 409) {}
    };

    class MagmadarStartFightTrigger : public StartBossFightTrigger
    {
    public:
        MagmadarStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start magmadar fight", "magmadar", 11982) {}
    };

    class MagmadarEndFightTrigger : public EndBossFightTrigger
    {
    public:
        MagmadarEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end magmadar fight", "magmadar", 11982) {}
    };

    class MagmadarLavaBombTrigger : public CloseToGameObjectHazardTrigger
    {
    public:
        MagmadarLavaBombTrigger(PlayerbotAI* ai) : CloseToGameObjectHazardTrigger(ai, "magmadar lava bomb", 177704, 5.0f, 60) {}
    };

    class MagmadarTooCloseTrigger : public CloseToCreatureTrigger
    {
    public:
        MagmadarTooCloseTrigger(PlayerbotAI* ai) : CloseToCreatureTrigger(ai, "magmadar too close", 11982, 30.0f) {}
    };

    class FireProtectionPotionReadyTrigger : public ItemBuffReadyTrigger
    {
    public:
        FireProtectionPotionReadyTrigger(PlayerbotAI* ai) : ItemBuffReadyTrigger(ai, "fire protection potion ready", 13457, 17543) {}
    };

    class MCRuneInSightTrigger : public ValueTrigger
    {
    public:
        MCRuneInSightTrigger(PlayerbotAI* ai) : ValueTrigger(ai, "mc rune in sight", 1)
        {
            qualifier = "and::{"
                "action possible::use id::17333,"
                "has object::go usable filter::go trapped filter::entry filter::{gos in sight,mc runes},"
                "not::has object::entry filter::{gos close,mc runes}"
                "}";
        }
    };

    class MCRuneCloseTrigger : public ValueTrigger
    {
    public:
        MCRuneCloseTrigger(PlayerbotAI* ai) : ValueTrigger(ai, "mc rune close", 1) { qualifier = "has object::go usable filter::entry filter::{gos close,mc runes}"; }
    };

    // --- Lucifron (12118) ---
    class LucifronStartFightTrigger : public StartBossFightTrigger
    {
    public:
        LucifronStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start lucifron fight", "lucifron", 12118) {}
    };

    class LucifronEndFightTrigger : public EndBossFightTrigger
    {
    public:
        LucifronEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end lucifron fight", "lucifron", 12118) {}
    };

    // Lucifron's Curse (19703) - curse, needs decurse
    class LucifronCurseTrigger : public Trigger
    {
    public:
        LucifronCurseTrigger(PlayerbotAI* ai) : Trigger(ai, "lucifron curse", 2) {}
        bool IsActive() override { return ai->HasAura(19703, bot); }
    };

    // Impending Doom (19702) - magic, needs dispel
    class LucifronImpendingDoomTrigger : public Trigger
    {
    public:
        LucifronImpendingDoomTrigger(PlayerbotAI* ai) : Trigger(ai, "lucifron impending doom", 2) {}
        bool IsActive() override { return ai->HasAura(19702, bot); }
    };

    // --- Gehennas (12259) ---
    class GehennasStartFightTrigger : public StartBossFightTrigger
    {
    public:
        GehennasStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start gehennas fight", "gehennas", 12259) {}
    };

    class GehennasEndFightTrigger : public EndBossFightTrigger
    {
    public:
        GehennasEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end gehennas fight", "gehennas", 12259) {}
    };

    // Gehennas' Curse (19716) - curse, needs decurse
    class GehennasCurseTrigger : public Trigger
    {
    public:
        GehennasCurseTrigger(PlayerbotAI* ai) : Trigger(ai, "gehennas curse", 2) {}
        bool IsActive() override { return ai->HasAura(19716, bot); }
    };

    // --- Garr (12057) ---
    class GarrStartFightTrigger : public StartBossFightTrigger
    {
    public:
        GarrStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start garr fight", "garr", 12057) {}
    };

    class GarrEndFightTrigger : public EndBossFightTrigger
    {
    public:
        GarrEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end garr fight", "garr", 12057) {}
    };

    // Eruption (19497) is cast by Garr on a random Firesworn add (entry 12099)
    // when it dies, making the corpse explosion-detonate for ~5k fire AOE.
    //
    // Earlier rev tried to detect Garr's cast via `GetCurrentSpell == 19497`
    // but Eruption is fired from `JustDied` (`boss_garr.cpp:170-178`) on the
    // dying Firesworn — by the time the cast is observable the creature is
    // already gone (the observation window is microseconds, see code-review
    // 2026-05-29 note 4).
    //
    // More robust proxy: any Firesworn at < 25% HP within 20y. The explosion
    // splash is ~15y so by the time a Firesworn is near-dead we want ranged to
    // already be 20y away. Healers and ranged bots react every tick; mêlée
    // tanks/dps eat the splash regardless.
    class GarrFireswornEruptionTrigger : public Trigger
    {
    public:
        GarrFireswornEruptionTrigger(PlayerbotAI* ai) : Trigger(ai, "garr firesworn eruption") {}
        bool IsActive() override
        {
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 12099, 20.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 20.0f);
            for (Unit* u : units)
            {
                if (!u || !u->IsAlive()) continue;
                uint32 maxHp = u->GetMaxHealth();
                if (maxHp == 0) continue;
                if (u->GetHealth() * 4 < maxHp)  // < 25%
                    return true;
            }
            return false;
        }
    };

    // --- Baron Geddon (12056) ---
    class BaronGeddonStartFightTrigger : public StartBossFightTrigger
    {
    public:
        BaronGeddonStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start baron geddon fight", "baron geddon", 12056) {}
    };

    class BaronGeddonEndFightTrigger : public EndBossFightTrigger
    {
    public:
        BaronGeddonEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end baron geddon fight", "baron geddon", 12056) {}
    };

    // Living Bomb (20475) - must move away from raid
    class BaronGeddonLivingBombTrigger : public Trigger
    {
    public:
        BaronGeddonLivingBombTrigger(PlayerbotAI* ai) : Trigger(ai, "baron geddon living bomb", 1) {}
        bool IsActive() override { return ai->HasAura(20475, bot); }
    };

    // Inferno - stay away from boss during cast (boss is rooted and pulses AoE)
    class BaronGeddonInfernoTrigger : public CloseToCreatureTrigger
    {
    public:
        BaronGeddonInfernoTrigger(PlayerbotAI* ai) : CloseToCreatureTrigger(ai, "baron geddon inferno", 12056, 20.0f) {}
    };

    // --- Shazzrah (12264) ---
    class ShazzrahStartFightTrigger : public StartBossFightTrigger
    {
    public:
        ShazzrahStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start shazzrah fight", "shazzrah", 12264) {}
    };

    class ShazzrahEndFightTrigger : public EndBossFightTrigger
    {
    public:
        ShazzrahEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end shazzrah fight", "shazzrah", 12264) {}
    };

    // Shazzrah's Curse (19713) - curse, needs decurse
    class ShazzrahCurseTrigger : public Trigger
    {
    public:
        ShazzrahCurseTrigger(PlayerbotAI* ai) : Trigger(ai, "shazzrah curse", 2) {}
        bool IsActive() override { return ai->HasAura(19713, bot); }
    };

    // --- Sulfuron Harbinger (12098) ---
    class SulfuronStartFightTrigger : public StartBossFightTrigger
    {
    public:
        SulfuronStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start sulfuron fight", "sulfuron", 12098) {}
    };

    class SulfuronEndFightTrigger : public EndBossFightTrigger
    {
    public:
        SulfuronEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end sulfuron fight", "sulfuron", 12098) {}
    };

    // Sulfuron Demoralizing Shout (19778) — debuff is magic, dispelable. Reduces
    // melee AP on the raid; high-priority strip for healers/priest dispel.
    class SulfuronDemoralizingShoutTrigger : public Trigger
    {
    public:
        SulfuronDemoralizingShoutTrigger(PlayerbotAI* ai) : Trigger(ai, "sulfuron demoralizing shout", 2) {}
        bool IsActive() override { return ai->HasAura(19778, bot); }
    };

    // --- Golemagg (11988) ---
    class GolemaggStartFightTrigger : public StartBossFightTrigger
    {
    public:
        GolemaggStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start golemagg fight", "golemagg", 11988) {}
    };

    class GolemaggEndFightTrigger : public EndBossFightTrigger
    {
    public:
        GolemaggEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end golemagg fight", "golemagg", 11988) {}
    };

    // Ranged/healers should stay away from Golemagg (Magma Splash hits nearby targets)
    class GolemaggTooCloseTrigger : public CloseToCreatureTrigger
    {
    public:
        GolemaggTooCloseTrigger(PlayerbotAI* ai) : CloseToCreatureTrigger(ai, "golemagg too close", 11988, 30.0f) {}
    };

    // --- Majordomo Executus (12018) ---
    class MajordomoStartFightTrigger : public StartBossFightTrigger
    {
    public:
        MajordomoStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start majordomo fight", "majordomo", 12018) {}
    };
    class MajordomoEndFightTrigger : public EndBossFightTrigger
    {
    public:
        MajordomoEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end majordomo fight", "majordomo", 12018) {}
    };

    // --- Ragnaros (11502) ---
    class RagnarosStartFightTrigger : public StartBossFightTrigger
    {
    public:
        RagnarosStartFightTrigger(PlayerbotAI* ai) : StartBossFightTrigger(ai, "start ragnaros fight", "ragnaros", 11502) {}
    };
    class RagnarosEndFightTrigger : public EndBossFightTrigger
    {
    public:
        RagnarosEndFightTrigger(PlayerbotAI* ai) : EndBossFightTrigger(ai, "end ragnaros fight", "ragnaros", 11502) {}
    };

    // Wrath of Ragnaros (20566): PBAOE knockback. Bot has the debuff after being hit
    // — too late to dodge. Use the cast detection variant instead so we can sidestep.
    class RagnarosWrathTrigger : public Trigger
    {
    public:
        RagnarosWrathTrigger(PlayerbotAI* ai) : Trigger(ai, "ragnaros wrath") {}
        bool IsActive() override
        {
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 11502, 60.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 60.0f);
            for (Unit* u : units)
            {
                if (!u) continue;
                Spell* cast = u->GetCurrentSpell(CURRENT_GENERIC_SPELL);
                if (cast && cast->m_spellInfo && cast->m_spellInfo->Id == 20566)
                    return true;
            }
            return false;
        }
    };

    // Submerge (21859): Ragnaros vanishes for ~90s and Sons of Flame spawn. OT picks
    // up the Sons (handled by OffTank target priority); detection here lets ranged
    // know to switch to Sons (they're the lowest-HP attackers when up).
    class RagnarosSubmergeTrigger : public Trigger
    {
    public:
        RagnarosSubmergeTrigger(PlayerbotAI* ai) : Trigger(ai, "ragnaros submerge") {}
        bool IsActive() override
        {
            std::list<Unit*> units;
            MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, 11502, 80.0f);
            MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
            Cell::VisitAllObjects(bot, searcher, 80.0f);
            for (Unit* u : units)
            {
                if (!u) continue;
                // SPELL_SUBMERGE_FADE (21107) is the stealth aura applied during submerge.
                if (u->HasAura(21107))
                    return true;
            }
            return false;
        }
    };
}