
#include "playerbot/playerbot.h"
#include "TempleOfAhnQirajDungeonStrategies.h"
#include "DungeonMultipliers.h"

using namespace ai;

void TempleOfAhnQirajDungeonStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "start sartura fight",
        NextAction::array(0, new NextAction("enable sartura fight strategy", 100.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "start huhuran fight",
        NextAction::array(0, new NextAction("enable huhuran fight strategy", 100.0f), NULL)));

    // Pro-engage Fankriss adds (Spawn of Fankriss 15630 + Vekniss Hatchling
    // 15962). Dungeon-level so we don't need a dedicated Fankriss fight
    // strategy yet. Priority 85.
    triggers.push_back(new TriggerNode(
        "fankriss adds nearby",
        NextAction::array(0, new NextAction("engage fankriss add", 85.0f), NULL)));

    // Bug Trio Princess Yauj Broods (15621) — adds during the Yauj phase.
    triggers.push_back(new TriggerNode(
        "yauj brood nearby",
        NextAction::array(0, new NextAction("engage yauj brood", 80.0f), NULL)));

    // C'Thun tentacles (15726/15728/15334/15802) — P2 body fight. All sizes
    // covered, AOE focus. Priority 90 — leaving tentacles up wipes the raid.
    triggers.push_back(new TriggerNode(
        "cthun tentacle nearby",
        NextAction::array(0, new NextAction("engage cthun tentacle", 90.0f), NULL)));

    // Skeram True Fulfillment (785) — MIND-CONTROL the affected raid member.
    // Magic-school, dispelable. Uses *on party* actions (CurePartyMemberAction)
    // because the MC'd bot can't self-dispel — the priest/paladin scans the
    // party for the magic debuff and strips it on the victim.
    // Priority 95.
    triggers.push_back(new TriggerNode(
        "skeram true fulfillment",
        NextAction::array(0,
            new NextAction("dispel magic on party", 95.0f),
            new NextAction("cleanse magic on party", 95.0f),
            NULL)));

    // Bug Trio — Kri Toxic Volley (25812) poison AOE every cycle. Cure-poison
    // chain. Raid-wide AOE so the dispel target is anyone in party — use
    // the *on party* variants for the cure to dispel a party member.
    triggers.push_back(new TriggerNode(
        "kri toxic volley",
        NextAction::array(0,
            new NextAction("cure poison on party", 85.0f),
            new NextAction("cleanse poison on party", 85.0f),
            NULL)));

    // Bug Trio — Yauj Fear (19408 placeholder for 25807) magic dispel. Group-
    // scan trigger + party-targeted dispel so the non-feared healer strips
    // the fear on the feared bot. Priority 90.
    triggers.push_back(new TriggerNode(
        "yauj fear",
        NextAction::array(0,
            new NextAction("dispel magic on party", 90.0f),
            new NextAction("cleanse magic on party", 90.0f),
            NULL)));

    // Twin Emperors — Mutate Bug (802) transforms the affected player into
    // a Qiraji bug that detonates after 8s. Magic dispel via priest/paladin
    // *on party* — polymorphed bot can't self-dispel. Priority 95.
    triggers.push_back(new TriggerNode(
        "twin emperors mutate bug",
        NextAction::array(0,
            new NextAction("dispel magic on party", 95.0f),
            new NextAction("cleanse magic on party", 95.0f),
            NULL)));

    // Viscidus (15299) frost phase — needs 200 frost-school hits to freeze.
    // Only mage (frostbolt) and shaman (frost shock) actually deal frost
    // damage in vanilla. Druid moonfire is Arcane school and would NOT
    // count toward the freeze counter — removed per code-review 2026-06-01
    // round 3 finding #4. Non-mage/shaman bots fall through to normal
    // attack (correct — they'll prep DPS for the post-freeze shatter phase).
    // README note: composition lacking 5+ casters between mage + shaman will
    // struggle to freeze Viscidus inside the soft enrage window.
    triggers.push_back(new TriggerNode(
        "viscidus frost phase",
        NextAction::array(0,
            new NextAction("frostbolt", 95.0f),
            new NextAction("frost shock", 95.0f),
            NULL)));

    // Bug Trio — Kri Toxic Vapors cloud (NPC 15933 "Poison Cloud") spawns on
    // Kri's death via EventAI in Turtle WoW (verified in
    // tw_world_creature_template — entry 15933 is EventAI-driven, not the
    // broken SD2 script path). Bots within 12y trigger 15y move-out at
    // priority 95. (Wire restored after re-verification — earlier removal
    // in commit 3c4332e assumed creature never spawns, which is true for
    // stock cmangos SD2 but NOT for Turtle's EventAI port.)
    triggers.push_back(new TriggerNode(
        "kri toxic cloud nearby",
        NextAction::array(0, new NextAction("move away from kri toxic cloud", 95.0f), NULL)));
}

// ========== Battleguard Sartura (15516) ==========

void SarturaFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    Player* bot = ai->GetBot();

    // Ranged/heal stay away during Whirlwind — she random-targets melees and
    // resets threat. Keeping casters out of 12y window means they're never in
    // the swap pool.
    if (ai->IsRanged(bot) || ai->IsHeal(bot))
    {
        triggers.push_back(new TriggerNode(
            "sartura too close",
            NextAction::array(0, new NextAction("move away from sartura", 100.0f), NULL)));
    }

    // Pro-engage: OTs grab Sartura's Royal Guards (15984) on spawn before they
    // also Whirlwind through the raid. Priority 80.
    triggers.push_back(new TriggerNode(
        "sartura royal guard nearby",
        NextAction::array(0, new NextAction("engage sartura royal guard", 80.0f), NULL)));
}

void SarturaFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end sartura fight",
        NextAction::array(0, new NextAction("disable sartura fight strategy", 100.0f), NULL)));
}

void SarturaFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end sartura fight",
        NextAction::array(0, new NextAction("disable sartura fight strategy", 100.0f), NULL)));
}

void SarturaFightStrategy::InitCombatMultipliers(std::list<Multiplier*>& multipliers)
{
    Player* bot = ai->GetBot();
    if (ai->IsRanged(bot) || ai->IsHeal(bot))
    {
        multipliers.push_back(new PreventMoveAwayFromCreatureOnReachToCastMultiplier(ai));
    }
}

// ========== Princess Huhuran (15509) ==========

void HuhuranFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Hunter Tranquilizing Shot (19801) on Frenzy 26051 — same chain as
    // Flamegor / standard "tranquilizing shot" action. Non-hunters silently
    // no-op.
    triggers.push_back(new TriggerNode(
        "huhuran frenzy",
        NextAction::array(0, new NextAction("tranquilizing shot", 90.0f), NULL)));

    // Noxious Poison (26053) — poison debuff on the current victim (tank).
    // *on party* so druid/shaman/paladin strips the tank — tank typically has
    // no self-cleanse. Priority 80 matches the Lucifron Curse cadence.
    triggers.push_back(new TriggerNode(
        "huhuran noxious poison",
        NextAction::array(0,
            new NextAction("cure poison on party", 80.0f),
            new NextAction("cleanse poison on party", 80.0f),
            NULL)));

    // Wyvern Sting (26180) — sleep on current victim (tank) during berserk
    // phase (<30% HP). Magic-school. Tank sleep = wipe. Group-scan + on-party
    // dispel. Priority 95 — fight-critical.
    triggers.push_back(new TriggerNode(
        "huhuran wyvern sting",
        NextAction::array(0,
            new NextAction("dispel magic on party", 95.0f),
            new NextAction("cleanse magic on party", 95.0f),
            NULL)));
}

void HuhuranFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end huhuran fight",
        NextAction::array(0, new NextAction("disable huhuran fight strategy", 100.0f), NULL)));
}

void HuhuranFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end huhuran fight",
        NextAction::array(0, new NextAction("disable huhuran fight strategy", 100.0f), NULL)));
}
