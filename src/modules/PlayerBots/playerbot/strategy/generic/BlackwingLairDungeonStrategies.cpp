#include "playerbot/playerbot.h"
#include "BlackwingLairDungeonStrategies.h"
#include "DungeonMultipliers.h"

using namespace ai;

// ========== Razorgore the Untamed (12435) ==========

void RazorgoreFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // P1: boss is mind-controlled. Switch to attacking adds (lowest-HP attacker).
    // This priority overrides the default tank-target / dps-assist selection so
    // ranged stay on adds even if Razorgore aggros them via tail/AOE.
    triggers.push_back(new TriggerNode(
        "razorgore phase 1",
        NextAction::array(0, new NextAction("attack least hp target", 90.0f), NULL)));

    // Pro-engage: the moment a Dragonkin or Grethok mage walks through one of
    // the 4 doors (60y scan), tanks/dps grab it before it reaches the orb-
    // bound player. Priority 95 ranks above "attack least hp target" because
    // pro-engage works on a creature NOT YET in our threat list (which the
    // least-hp action can't pick).
    triggers.push_back(new TriggerNode(
        "razorgore adds nearby",
        NextAction::array(0, new NextAction("engage razorgore add", 95.0f), NULL)));

    // P2 has no special bot-side mechanic — tank-and-spank handles it via the
    // class strategies. Future: Conflagration (23023) disorient dispel,
    // Fireball Volley (22425) ranged spread.
}

void RazorgoreFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end razorgore fight",
        NextAction::array(0, new NextAction("disable razorgore fight strategy", 100.0f), NULL)));
}

void RazorgoreFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end razorgore fight",
        NextAction::array(0, new NextAction("disable razorgore fight strategy", 100.0f), NULL)));
}

// ========== Vaelastrasz the Corrupt (13020) ==========

void VaelastraszFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Pure DPS race — Vaelastrasz becomes invulnerable after 3 minutes and Burning
    // Adrenaline starts wiping the raid. No mana conservation needed (Essence of
    // the Red 23513 gives unlimited mana/rage/energy during the encounter).
    triggers.push_back(new TriggerNode(
        "vaelastrasz burning adrenaline",
        NextAction::array(0, new NextAction("vael burning adrenaline move away", 100.0f), NULL)));
}

void VaelastraszFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end vaelastrasz fight",
        NextAction::array(0, new NextAction("disable vaelastrasz fight strategy", 100.0f), NULL)));
}

void VaelastraszFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end vaelastrasz fight",
        NextAction::array(0, new NextAction("disable vaelastrasz fight strategy", 100.0f), NULL)));
}

// ========== Broodlord Lashlayer (12017) ==========

void BroodlordFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    Player* bot = ai->GetBot();

    // Knock Away frequently strips the tank's threat lead. Tanks need to re-taunt
    // aggressively. The generic "lose aggro" -> "taunt" chain (warrior, paladin
    // prot, druid bear) already handles this, but we boost priority during the
    // Broodlord fight specifically.
    if (ai->IsTank(bot))
    {
        triggers.push_back(new TriggerNode(
            "lose aggro",
            NextAction::array(0, new NextAction("taunt", 100.0f), NULL)));
    }

    // Blast Wave is a frontal/PBAOE fire. Casters should already be at range; this
    // is here for the OT and melee dispel/heal classes who might be too close.
    // Currently no Blast Wave-specific trigger — handled by generic "enemy too
    // close" caster avoidance for ranged classes already in the fight.
}

void BroodlordFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end broodlord fight",
        NextAction::array(0, new NextAction("disable broodlord fight strategy", 100.0f), NULL)));
}

void BroodlordFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end broodlord fight",
        NextAction::array(0, new NextAction("disable broodlord fight strategy", 100.0f), NULL)));
}

// ========== Nefarian (11583) ==========

void NefarianFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "fire protection potion ready",
        NextAction::array(0, new NextAction("fire protection potion", 50.0f), NULL)));

    // Bellowing Roar (22686) — 8s AoE fear. Race/class self-breaks: WotF for
    // Forsaken, Berserker Rage for warrior. Mirrors Onyxia Bellowing Roar
    // priority. Cast = ~2s so the chain can fire before the fear actually
    // lands if we detect it in time (currently we read the aura on bot, so
    // the break fires while feared, ending it early).
    triggers.push_back(new TriggerNode(
        "nefarian bellowing roar",
        NextAction::array(0,
            new NextAction("will of the forsaken", 100.0f),
            new NextAction("berserker rage fear", 100.0f),
            NULL)));

    // Veil of Shadow (22687) — Shadow-school 90% healing reduction on the
    // main tank. DB: dispel=Magic (2), school=Shadow (5), recurring debuff
    // cast frequently in P3. Group-scan so warrior MT (no self-cleanse)
    // gets stripped by priest/paladin before the next heal lands at 10%.
    // Priority 95 (fight-defining — tank dies without removal).
    triggers.push_back(new TriggerNode(
        "nefarian veil of shadow",
        NextAction::array(0,
            new NextAction("dispel magic on party", 95.0f),
            new NextAction("cleanse magic on party", 95.0f),
            NULL)));

    // P2/P3 hostile adds — Bone Construct (14605, raised drakonid skeletons)
    // and Corrupted Infernal (14668, warlock-call spawn). Both need raid
    // AOE focus to prevent the boss being free-cast on. Pro-engage gated
    // on Nefarian (11583) alive, prio 95.
    triggers.push_back(new TriggerNode(
        "nefarian adds nearby",
        NextAction::array(0, new NextAction("engage nefarian adds", 95.0f), NULL)));

    // Priest Class Call (23401 Corrupted Healing) — priest's heals become
    // damage on the heal target. SELF-only trigger on priests. dispel=0,
    // ride out the 12s window. Action: self-defensives (healing potion +
    // bandage) — gives the priest something to do that ISN'T harmful.
    // Priority 100 (fight-defining: a priest who keeps healing during
    // Corrupted Healing kills the tank).
    triggers.push_back(new TriggerNode(
        "nefarian priest call",
        NextAction::array(0,
            new NextAction("healing potion", 100.0f),
            new NextAction("use bandage", 95.0f),
            NULL)));

    // Shaman Class Call (23425 Corrupted Totems) — shaman's totems damage
    // the raid instead of buffing them. SELF-only trigger on shamans.
    // Action: `totemic call` recalls all active totems immediately so the
    // bleed stops. Priority 100 (raid AOE dmg = wipe risk in P3).
    triggers.push_back(new TriggerNode(
        "nefarian shaman call",
        NextAction::array(0, new NextAction("totemic call", 100.0f), NULL)));
}

void NefarianFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end nefarian fight",
        NextAction::array(0, new NextAction("disable nefarian fight strategy", 100.0f), NULL)));
}

void NefarianFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end nefarian fight",
        NextAction::array(0, new NextAction("disable nefarian fight strategy", 100.0f), NULL)));
}

// ========== Firemaw (11983) ==========

void FiremawFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "fire protection potion ready",
        NextAction::array(0, new NextAction("fire protection potion", 50.0f), NULL)));

    // Tank-swap on Flame Buffet 23341 (Framework #4). Fires only on
    // off-tank bots when the other tank carries 3+ stacks. Action: taunt
    // → become the new MT, let the old MT's stacks decay. Priority 95.
    triggers.push_back(new TriggerNode(
        "firemaw flame buffet swap",
        NextAction::array(0, new NextAction("taunt", 95.0f), NULL)));
}

void FiremawFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end firemaw fight",
        NextAction::array(0, new NextAction("disable firemaw fight strategy", 100.0f), NULL)));
}

void FiremawFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end firemaw fight",
        NextAction::array(0, new NextAction("disable firemaw fight strategy", 100.0f), NULL)));
}

// ========== Ebonroc (14601) ==========

void EbonrocFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "fire protection potion ready",
        NextAction::array(0, new NextAction("fire protection potion", 50.0f), NULL)));
}

void EbonrocFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end ebonroc fight",
        NextAction::array(0, new NextAction("disable ebonroc fight strategy", 100.0f), NULL)));
}

void EbonrocFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end ebonroc fight",
        NextAction::array(0, new NextAction("disable ebonroc fight strategy", 100.0f), NULL)));
}

// ========== Flamegor (11981) ==========

void FlamegorFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "fire protection potion ready",
        NextAction::array(0, new NextAction("fire protection potion", 50.0f), NULL)));

    // Hunter Tranquilizing Shot (19801) — only the action node exists; non-hunters
    // silently no-op on the chain. Priority 90 matches existing Lucifron-class
    // class-routed dispel cadence.
    triggers.push_back(new TriggerNode(
        "flamegor frenzy",
        NextAction::array(0, new NextAction("tranquilizing shot", 90.0f), NULL)));
}

void FlamegorFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end flamegor fight",
        NextAction::array(0, new NextAction("disable flamegor fight strategy", 100.0f), NULL)));
}

void FlamegorFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end flamegor fight",
        NextAction::array(0, new NextAction("disable flamegor fight strategy", 100.0f), NULL)));
}

// ========== Chromaggus (14020) ==========

void ChromaggusFightStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    // Stack of 4 Brood Afflictions on a single player is the critical state — at
    // 5 the player gets Chromatic Mutation 23174 and turns hostile. Any class
    // with a generic dispel ("remove curse" / "dispel magic" / "cleanse") should
    // strip a color off the affected target. The chain falls through quietly on
    // classes that can't dispel.
    triggers.push_back(new TriggerNode(
        "chromaggus affliction danger",
        NextAction::array(0,
            new NextAction("remove curse on party", 100.0f),
            new NextAction("dispel magic on party", 100.0f),
            new NextAction("cleanse magic on party", 100.0f),
            NULL)));
}

void ChromaggusFightStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end chromaggus fight",
        NextAction::array(0, new NextAction("disable chromaggus fight strategy", 100.0f), NULL)));
}

void ChromaggusFightStrategy::InitDeadTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "end chromaggus fight",
        NextAction::array(0, new NextAction("disable chromaggus fight strategy", 100.0f), NULL)));
}

void BlackwingLairDungeonStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "suppression device close",
        NextAction::array(0, new NextAction("disarm suppression device", 80.0f), NULL)));

    // BWL boss auto-start triggers
    triggers.push_back(new TriggerNode(
        "start razorgore fight",
        NextAction::array(0, new NextAction("enable razorgore fight strategy", 100.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "start vaelastrasz fight",
        NextAction::array(0, new NextAction("enable vaelastrasz fight strategy", 100.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "start broodlord fight",
        NextAction::array(0, new NextAction("enable broodlord fight strategy", 100.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "start chromaggus fight",
        NextAction::array(0, new NextAction("enable chromaggus fight strategy", 100.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "start firemaw fight",
        NextAction::array(0, new NextAction("enable firemaw fight strategy", 100.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "start ebonroc fight",
        NextAction::array(0, new NextAction("enable ebonroc fight strategy", 100.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "start flamegor fight",
        NextAction::array(0, new NextAction("enable flamegor fight strategy", 100.0f), NULL)));
    triggers.push_back(new TriggerNode(
        "start nefarian fight",
        NextAction::array(0, new NextAction("enable nefarian fight strategy", 100.0f), NULL)));
}

void BlackwingLairDungeonStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "suppression device need stealth",
        NextAction::array(0, new NextAction("stealth for suppression device", ACTION_HIGH + 3), NULL)));

    triggers.push_back(new TriggerNode(
        "suppression device in sight",
        NextAction::array(0, new NextAction("move to suppression device", ACTION_HIGH + 2), NULL)));

    triggers.push_back(new TriggerNode(
        "suppression device close",
        NextAction::array(0, new NextAction("disarm suppression device", ACTION_HIGH + 4), NULL)));
}

class SuppressionRoomPassiveMultiplier : public Multiplier
{
public:
    SuppressionRoomPassiveMultiplier(PlayerbotAI* ai) : Multiplier(ai, "suppression room passive") {}

    float GetValue(ai::Action* action) override
    {
        if (!action)
            return 1.0f;

        if (ai->GetBot()->getClass() != CLASS_ROGUE)
            return 1.0f;

        const std::string& name = action->getName();

        // Enable only the following strats for suppression room to avoid regular combat breaking logic
        if (name == "stealth for suppression device" ||
            name == "move to suppression device" ||
            name == "disarm suppression device" ||
            name == "deactivate suppression device")
        {
            return 1.0f;
        }

        if (name == "stealth" ||
            name == "unstealth" ||
            name == "check stealth" ||
            name == "sprint" ||
            name == "vanish")
        {
            return 1.0f;
        }

        if (name == "co" ||
            name == "nc" ||
            name == "load ai" ||
            name == "save ai" ||
            name == "list ai" ||
            name == "reset ai" ||
            name == "reset strats" ||
            name == "reset values" ||
            name == "check mount state" ||
            name == "accept invitation" ||
            name == "set combat state" ||
            name == "set non combat state" ||
            name == "set dead state" ||
            name == "update pvp strats" ||
            name == "update pve strats" ||
            name == "update raid strats" ||
            name == "loot roll" ||
            name == "auto loot roll" ||
            name == "follow" ||
            name == "stay" ||
            name == "food" ||
            name == "drink")
        {
            return 1.0f;
        }

        return 0.0f;
    }
};

void SuppressionRoomStrategy::InitCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "suppression device need stealth",
        NextAction::array(0, new NextAction("vanish", ACTION_EMERGENCY + 1), NULL)));

    triggers.push_back(new TriggerNode(
        "suppression device in sight",
        NextAction::array(0, new NextAction("move to suppression device", ACTION_HIGH + 8), NULL)));

    triggers.push_back(new TriggerNode(
        "suppression device close",
        NextAction::array(0, new NextAction("disarm suppression device", 90.0f), NULL)));
}

void SuppressionRoomStrategy::InitNonCombatTriggers(std::list<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode(
        "suppression device need stealth",
        NextAction::array(0, new NextAction("stealth for suppression device", ACTION_MOVE), NULL)));

    triggers.push_back(new TriggerNode(
        "suppression device in sight",
        NextAction::array(0, new NextAction("move to suppression device", ACTION_HIGH + 8), NULL)));

    triggers.push_back(new TriggerNode(
        "suppression device close",
        NextAction::array(0, new NextAction("disarm suppression device", ACTION_MOVE + 2), NULL)));
}

void SuppressionRoomStrategy::InitCombatMultipliers(std::list<Multiplier*>& multipliers)
{
    multipliers.push_back(new SuppressionRoomPassiveMultiplier(ai));
}

void SuppressionRoomStrategy::InitNonCombatMultipliers(std::list<Multiplier*>& multipliers)
{
    multipliers.push_back(new SuppressionRoomPassiveMultiplier(ai));
}

void SuppressionRoomStrategy::OnStrategyAdded(BotState state)
{
    if (ai->GetBot()->getClass() == CLASS_ROGUE)
    {
        ai->ChangeStrategy("-avoid aoe", BotState::BOT_STATE_COMBAT);
        ai->ChangeStrategy("-avoid aoe", BotState::BOT_STATE_NON_COMBAT);
        ai->ChangeStrategy("-avoid aoe", BotState::BOT_STATE_REACTION);
        ai->ChangeStrategy("-avoid mobs", BotState::BOT_STATE_COMBAT);
        ai->ChangeStrategy("-avoid mobs", BotState::BOT_STATE_NON_COMBAT);
        ai->ChangeStrategy("-avoid mobs", BotState::BOT_STATE_REACTION);
    }
}