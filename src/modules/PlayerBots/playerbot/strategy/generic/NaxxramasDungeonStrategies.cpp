
#include "playerbot/playerbot.h"
#include "NaxxramasDungeonStrategies.h"
#include "DungeonMultipliers.h"

using namespace ai;

void NaxxramasDungeonStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
	triggers.push_back(new TriggerNode(
		"start four horseman fight",
		NextAction::array(0, new NextAction("enable four horseman fight strategy", 100.0f), NULL)));

    triggers.push_back(new TriggerNode(
        "start patchwerk fight",
        NextAction::array(0, new NextAction("enable patchwerk fight strategy", 100.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "start loatheb fight",
        NextAction::array(0, new NextAction("enable loatheb fight strategy", 100.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "start kelthuzad fight",
        NextAction::array(0, new NextAction("enable kelthuzad fight strategy", 100.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "start sapphiron fight",
        NextAction::array(0, new NextAction("enable sapphiron fight strategy", 100.0f), NULL)));

    // Pro-engage triggers wired at the dungeon-level (no per-boss strategy
    // needed): the entry scan only finds adds when we're in the matching
    // room, so the cost outside is one O(1) cell-visit miss.
    triggers.push_back(new TriggerNode(
        "anubrekhan crypt guard nearby",
        NextAction::array(0, new NextAction("engage anubrekhan crypt guard", 85.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "faerlina worshipper nearby",
        NextAction::array(0, new NextAction("engage faerlina worshipper", 80.0f), NULL)));
    // Gluth eats zombies within 10y to heal — pro-engage at 50y so OTs grab
    // them before they reach the boss. Priority 90 (above the standard
    // dungeon-level 85) because Gluth's heal is fight-defining.
    triggers.push_back(new TriggerNode(
        "gluth zombie chow nearby",
        NextAction::array(0, new NextAction("engage gluth zombie chow", 90.0f), NULL)));

    // Maexxna Spiderlings (17055) — 12 spawn at 75/50/25% HP. Raid AOE.
    triggers.push_back(new TriggerNode(
        "maexxna spiderling nearby",
        NextAction::array(0, new NextAction("engage maexxna spiderling", 85.0f), NULL)));

    // Noth Plagued adds (16981-16984) during teleport phase.
    triggers.push_back(new TriggerNode(
        "noth plagued adds nearby",
        NextAction::array(0, new NextAction("engage noth plagued add", 85.0f), NULL)));

    // Maexxna Necrotic Poison (28776) — poison on tank, 90% heal reduction.
    // *on party* so druid/shaman/paladin strips the tank.
    triggers.push_back(new TriggerNode(
        "maexxna necrotic poison",
        NextAction::array(0,
            new NextAction("cure poison on party", 90.0f),
            new NextAction("cleanse poison on party", 90.0f),
            NULL)));

    // Noth Curse of Plaguebringer (29213) — curse on 3 random victims,
    // deadly tick. *on party* so druid/mage strips victims.
    triggers.push_back(new TriggerNode(
        "noth curse plaguebringer",
        NextAction::array(0, new NextAction("remove curse on party", 90.0f), NULL)));

    // Gothik adds — both live (16124-16126) and dead (16127, 16148-16150)
    // sides. The tank role on each side grabs the heavies; AOE focus on
    // trainees. Priority 85.
    triggers.push_back(new TriggerNode(
        "gothik adds nearby",
        NextAction::array(0, new NextAction("engage gothik add", 85.0f), NULL)));

    // Faerlina Poison Bolt Volley (28796) — poison, raid-wide AOE. Use
    // *on party* variants — the dispeller will pick whoever has the debuff
    // (typically themselves on a raid-wide cast, but party-scan is safe).
    triggers.push_back(new TriggerNode(
        "faerlina poison bolt",
        NextAction::array(0,
            new NextAction("cure poison on party", 85.0f),
            new NextAction("cleanse poison on party", 85.0f),
            NULL)));

    // Anub'Rekhan Locust Swarm (28785) — 20s self-buff that AOE-ticks ~20y
    // around Anub. Cast every 80-120s. All bots move 30y+ out at priority 100
    // (fight-defining — bots in the AOE die in seconds).
    triggers.push_back(new TriggerNode(
        "anubrekhan locust swarm",
        NextAction::array(0, new NextAction("move away from anubrekhan locust swarm", 100.0f), NULL)));

    // Thaddius polarity (28059 Positive / 28084 Negative) — Polarity Shift
    // (28089) every ~30s re-rolls everyone. Same-polarity bots stack;
    // different-polarity bots eat amped damage from each other's tick.
    // Framework #5 multi-player coordination: action reads OTHER bots' aura
    // state, computes same-polarity centroid, moves there. Priority 95.
    triggers.push_back(new TriggerNode(
        "thaddius has polarity",
        NextAction::array(0, new NextAction("thaddius same polarity", 95.0f), NULL)));
}

void FourHorsemanFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
	triggers.push_back(new TriggerNode(
		"void zone too close",
		NextAction::array(0, new NextAction("move away from void zone", 100.0f), NULL)));

    // Mark stacks (28832 Korth'azz fire / 28833 Blaumeux shadow / 28834
    // Mograine unholy / 28835 Zeliek holy). At 5 stacks the mark damage is
    // lethal — players swap to the opposite Horseman zone to drop stacks.
    //
    // The trigger is kept registered (Combat tab + diagnostics) but NOT
    // wired to a dispel chain: the marks aren't classified as magic in the
    // vanilla DBC — `dispel magic` / `cleanse magic` no-op on them. The
    // only real solution is the mark-zone swap which needs a multi-tank
    // coord framework we don't have. See code-review note 2 (2026-05-29).
}

void FourHorsemanFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
	triggers.push_back(new TriggerNode(
		"end horseman fight",
		NextAction::array(0, new NextAction("disable four horseman fight strategy", 100.0f), NULL)));
}

void FourHorsemanFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
	triggers.push_back(new TriggerNode(
		"end horseman fight",
		NextAction::array(0, new NextAction("disable four horseman fight strategy", 100.0f), NULL)));
}

// ========== Patchwerk (16028) ==========

void PatchwerkFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Pure DPS race. Bots default behavior (melee stay in, ranged stay back) is
    // correct for Patchwerk; Hateful Strike targeting is server-side and the bots
    // can't do much about it directly. We just need to keep them alive — heals
    // priority and tank threat-hold are governed by class strategies.
    //
    // No special trigger yet. Future: detect Berserk cast (27680) at 7-min and
    // trigger a "burn cooldowns" macro for DPS classes.
    (void)triggers;
}

void PatchwerkFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end patchwerk fight",
        NextAction::array(0, new NextAction("disable patchwerk fight strategy", 100.0f), NULL)));
}

void PatchwerkFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end patchwerk fight",
        NextAction::array(0, new NextAction("disable patchwerk fight strategy", 100.0f), NULL)));
}

// ========== Loatheb (16011) ==========

void LoathebFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Anti-heal mechanic: Corrupted Mind (29201) puts a 60s lockout on healers
    // after each heal cast. Bots' healer logic already cooldowns appropriately so
    // we don't add a custom heal-throttle. Spore (16286) adds drop Fungal Bloom
    // (29232) which the raid should pick up — handled by generic loot/aura
    // behavior, no Loatheb-specific action needed.
    //
    // Inevitable Doom (29204) is unavoidable raid AOE damage. Healers ride out
    // the lockout via Greater Heal castings before Doom ticks.
    (void)triggers;
}

void LoathebFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end loatheb fight",
        NextAction::array(0, new NextAction("disable loatheb fight strategy", 100.0f), NULL)));
}

void LoathebFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end loatheb fight",
        NextAction::array(0, new NextAction("disable loatheb fight strategy", 100.0f), NULL)));
}

// ========== Sapphiron (15989) ==========

void SapphironFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Life Drain (28542) is a magic debuff that drains mana and heals Sapphiron.
    // Same dispel chain as Lucifron Impending Doom — priest/paladin handle it,
    // others silently no-op.
    triggers.push_back(new TriggerNode(
        "sapphiron life drain",
        NextAction::array(0,
            new NextAction("dispel magic on party", 80.0f),
            new NextAction("cleanse magic on party", 80.0f),
            NULL)));

    // Frost Breath (28524) — 7s cast AOE in air phase, blocked by LOS via
    // GO_ICEBLOCK (181247). When Sapphiron starts casting, bots find the
    // nearest ice block within 50y and reposition next to it. Priority 100
    // (raid-wide lethal if not LOS'd).
    triggers.push_back(new TriggerNode(
        "sapphiron frost breath",
        NextAction::array(0, new NextAction("hide behind sapphiron ice block", 100.0f), NULL)));
}

void SapphironFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end sapphiron fight",
        NextAction::array(0, new NextAction("disable sapphiron fight strategy", 100.0f), NULL)));
}

void SapphironFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end sapphiron fight",
        NextAction::array(0, new NextAction("disable sapphiron fight strategy", 100.0f), NULL)));
}

// ========== Kel'Thuzad (15990) ==========

void KelThuzadFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    Player* bot = ai->GetBot();

    // Frost Blast root + AOE — casters should be spread to minimize splash.
    if (ai->IsRanged(bot) || ai->IsHeal(bot))
    {
        triggers.push_back(new TriggerNode(
            "enemy too close for spell",
            NextAction::array(0, new NextAction("flee", 100.0f), NULL)));
    }

    // Mana Detonation (27819) and Chains of Kel'Thuzad (28408) are dispel-able.
    // Wire the same multi-class dispel chain we use for Lucifron / Chromaggus.
    triggers.push_back(new TriggerNode(
        "kelthuzad mana detonation",
        NextAction::array(0,
            new NextAction("dispel magic on party", 100.0f),
            new NextAction("cleanse magic on party", 100.0f),
            NULL)));
}

void KelThuzadFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end kelthuzad fight",
        NextAction::array(0, new NextAction("disable kelthuzad fight strategy", 100.0f), NULL)));
}

void KelThuzadFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end kelthuzad fight",
        NextAction::array(0, new NextAction("disable kelthuzad fight strategy", 100.0f), NULL)));
}
