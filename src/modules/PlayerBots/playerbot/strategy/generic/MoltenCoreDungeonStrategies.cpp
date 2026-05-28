
#include "playerbot/playerbot.h"
#include "MoltenCoreDungeonStrategies.h"
#include "DungeonMultipliers.h"

using namespace ai;

void MoltenCoreDungeonStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "start magmadar fight",
        NextAction::array(0, new NextAction("enable magmadar fight strategy", 100.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "start lucifron fight",
        NextAction::array(0, new NextAction("enable lucifron fight strategy", 100.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "start gehennas fight",
        NextAction::array(0, new NextAction("enable gehennas fight strategy", 100.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "start garr fight",
        NextAction::array(0, new NextAction("enable garr fight strategy", 100.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "start baron geddon fight",
        NextAction::array(0, new NextAction("enable baron geddon fight strategy", 100.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "start shazzrah fight",
        NextAction::array(0, new NextAction("enable shazzrah fight strategy", 100.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "start sulfuron fight",
        NextAction::array(0, new NextAction("enable sulfuron fight strategy", 100.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "start golemagg fight",
        NextAction::array(0, new NextAction("enable golemagg fight strategy", 100.0f), NULL)));
}

void MoltenCoreDungeonStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    /*
    triggers.push_back(new TriggerNode(
        "val::and::{"
        "action possible::use id::17333,"
        "has object::go usable filter::go trapped filter::entry filter::{gos in sight,mc runes},"
        "not::has object::entry filter::{gos close,mc runes}"
        "}",
        NextAction::array(0, new NextAction("move to::entry filter::{gos in sight,mc runes}", 1.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "val::has object::go usable filter::entry filter::{gos close,mc runes}",
        NextAction::array(0, new NextAction("use id::{17333,entry filter::{gos close,mc runes}}", 1.0f), NULL)));
        */

    triggers.push_back(new TriggerNode(
        "mc rune in sight",
        NextAction::array(0, new NextAction("move to mc rune", 1.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "mc rune close",
        NextAction::array(0,
            new NextAction("douse mc rune eternal", 2.0f),
            new NextAction("douse mc rune aqual", 1.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "fire protection potion ready",
        NextAction::array(0, new NextAction("fire protection potion", 100.0f), NULL)));
}

// ========== Magmadar ==========

void MagmadarFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    Player* bot = ai->GetBot();
    if (ai->IsRanged(bot) || ai->IsHeal(bot))
    {
        triggers.push_back(new TriggerNode(
            "magmadar too close",
            NextAction::array(0, new NextAction("move away from magmadar", 100.0f), NULL)));
    }

    triggers.push_back(new TriggerNode(
        "fire protection potion ready",
        NextAction::array(0, new NextAction("fire protection potion", 100.0f), NULL)));
}

void MagmadarFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end magmadar fight",
        NextAction::array(0, new NextAction("disable magmadar fight strategy", 100.0f), NULL)));
}

void MagmadarFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end magmadar fight",
        NextAction::array(0, new NextAction("disable magmadar fight strategy", 100.0f), NULL)));
}

void MagmadarFightStrategy::InitReactionTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "magmadar lava bomb",
        NextAction::array(0, new NextAction("move away from hazard", 100.0f), NULL)));
}

void MagmadarFightStrategy::InitCombatMultipliers(std::list<Multiplier*>& multipliers)
{
    Player* bot = ai->GetBot();
    if (ai->IsRanged(bot) || ai->IsHeal(bot))
    {
        multipliers.push_back(new PreventMoveAwayFromCreatureOnReachToCastMultiplier(ai));
    }
}

// ========== Lucifron (12118) ==========

void LucifronFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "fire protection potion ready",
        NextAction::array(0, new NextAction("fire protection potion", 100.0f), NULL)));

    // Lucifron's Curse (19703) doubles mana cost on the affected target — high-prio
    // dispel for casters/healers. Action is class-routed: druid/mage have a real
    // "remove curse" action, other classes the chain is a no-op so the trigger does
    // no harm. Same priority Magmadar uses for its fear cure pots.
    triggers.push_back(new TriggerNode(
        "lucifron curse",
        NextAction::array(0, new NextAction("remove curse", 80.0f), NULL)));

    // Impending Doom (19702) is a magic debuff that ticks heavy shadow damage; if a
    // priest is present they can dispel it off party members. The "dispel magic"
    // action node is defined in PriestActions; non-priests no-op. Priority slightly
    // higher than the curse since the DoT actively kills heals.
    triggers.push_back(new TriggerNode(
        "lucifron impending doom",
        NextAction::array(0, new NextAction("dispel magic", 90.0f), NULL)));
}

void LucifronFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end lucifron fight",
        NextAction::array(0, new NextAction("disable lucifron fight strategy", 100.0f), NULL)));
}

void LucifronFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end lucifron fight",
        NextAction::array(0, new NextAction("disable lucifron fight strategy", 100.0f), NULL)));
}

// ========== Gehennas (12259) ==========

void GehennasFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "fire protection potion ready",
        NextAction::array(0, new NextAction("fire protection potion", 100.0f), NULL)));

    // Gehennas' Curse (19716) reduces healing on the affected target by 75% — high prio
    // for healers to self-dispel. Same chain as Lucifron's curse, the "remove curse"
    // action node is class-routed.
    triggers.push_back(new TriggerNode(
        "gehennas curse",
        NextAction::array(0, new NextAction("remove curse", 80.0f), NULL)));
}

void GehennasFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end gehennas fight",
        NextAction::array(0, new NextAction("disable gehennas fight strategy", 100.0f), NULL)));
}

void GehennasFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end gehennas fight",
        NextAction::array(0, new NextAction("disable gehennas fight strategy", 100.0f), NULL)));
}

// ========== Garr (12057) ==========

void GarrFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "fire protection potion ready",
        NextAction::array(0, new NextAction("fire protection potion", 100.0f), NULL)));
}

void GarrFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end garr fight",
        NextAction::array(0, new NextAction("disable garr fight strategy", 100.0f), NULL)));
}

void GarrFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end garr fight",
        NextAction::array(0, new NextAction("disable garr fight strategy", 100.0f), NULL)));
}

// ========== Baron Geddon (12056) ==========

void BaronGeddonFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Living Bomb: move away from raid when debuffed
    triggers.push_back(new TriggerNode(
        "baron geddon living bomb",
        NextAction::array(0, new NextAction("baron geddon living bomb move away", 100.0f), NULL)));

    // Inferno: ranged/healers stay away from boss during AoE
    Player* bot = ai->GetBot();
    if (ai->IsRanged(bot) || ai->IsHeal(bot))
    {
        triggers.push_back(new TriggerNode(
            "baron geddon inferno",
            NextAction::array(0, new NextAction("move away from baron geddon", 100.0f), NULL)));
    }

    triggers.push_back(new TriggerNode(
        "fire protection potion ready",
        NextAction::array(0, new NextAction("fire protection potion", 100.0f), NULL)));
}

void BaronGeddonFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end baron geddon fight",
        NextAction::array(0, new NextAction("disable baron geddon fight strategy", 100.0f), NULL)));
}

void BaronGeddonFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end baron geddon fight",
        NextAction::array(0, new NextAction("disable baron geddon fight strategy", 100.0f), NULL)));
}

// ========== Shazzrah (12264) ==========

void ShazzrahFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "fire protection potion ready",
        NextAction::array(0, new NextAction("fire protection potion", 100.0f), NULL)));

    // Shazzrah's Curse (19713) deals heavy magic damage on cast — decurse asap.
    // Same class-routed chain as the Lucifron / Gehennas curses.
    triggers.push_back(new TriggerNode(
        "shazzrah curse",
        NextAction::array(0, new NextAction("remove curse", 80.0f), NULL)));
}

void ShazzrahFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end shazzrah fight",
        NextAction::array(0, new NextAction("disable shazzrah fight strategy", 100.0f), NULL)));
}

void ShazzrahFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end shazzrah fight",
        NextAction::array(0, new NextAction("disable shazzrah fight strategy", 100.0f), NULL)));
}

// ========== Sulfuron Harbinger (12098) ==========

void SulfuronFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "fire protection potion ready",
        NextAction::array(0, new NextAction("fire protection potion", 100.0f), NULL)));
}

void SulfuronFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end sulfuron fight",
        NextAction::array(0, new NextAction("disable sulfuron fight strategy", 100.0f), NULL)));
}

void SulfuronFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end sulfuron fight",
        NextAction::array(0, new NextAction("disable sulfuron fight strategy", 100.0f), NULL)));
}

// ========== Golemagg (11988) ==========

void GolemaggFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Ranged/healers stay away due to Magma Splash proximity damage
    Player* bot = ai->GetBot();
    if (ai->IsRanged(bot) || ai->IsHeal(bot))
    {
        triggers.push_back(new TriggerNode(
            "golemagg too close",
            NextAction::array(0, new NextAction("move away from golemagg", 100.0f), NULL)));
    }

    triggers.push_back(new TriggerNode(
        "fire protection potion ready",
        NextAction::array(0, new NextAction("fire protection potion", 100.0f), NULL)));
}

void GolemaggFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end golemagg fight",
        NextAction::array(0, new NextAction("disable golemagg fight strategy", 100.0f), NULL)));
}

void GolemaggFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end golemagg fight",
        NextAction::array(0, new NextAction("disable golemagg fight strategy", 100.0f), NULL)));
}

void GolemaggFightStrategy::InitCombatMultipliers(std::list<Multiplier*>& multipliers)
{
    Player* bot = ai->GetBot();
    if (ai->IsRanged(bot) || ai->IsHeal(bot))
    {
        multipliers.push_back(new PreventMoveAwayFromCreatureOnReachToCastMultiplier(ai));
    }
}
