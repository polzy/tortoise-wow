#pragma once

#include "playerbot/playerbot.h"
#include "playerbot/strategy/Value.h"
#include "playerbot/strategy/NamedObjectContext.h"

namespace ai
{
    // Framework #3 primitive: phase awareness.
    //
    // BossHpPctValue scans for a creature with the given entry within 100y of
    // the bot's position and returns its health percentage (0-100). Returns
    // 100.0f if not found (no boss in range → treat as "fully alive" so
    // phase-gated triggers don't false-fire).
    //
    // Qualifier: boss NPC entry as decimal string ("15263" = Skeram).
    //
    // Use to build phase-gated triggers without hardcoding the boss-find
    // logic in every trigger class. Example:
    //   AI_VALUE2(float, "boss hp pct", "15263") < 75.0f  // Skeram P75
    //
    // Cached at 1s checkInterval so spamming the value from multiple
    // triggers in the same tick is cheap.
    class BossHpPctValue : public CalculatedValue<float>, public Qualified
    {
    public:
        BossHpPctValue(PlayerbotAI* ai) : CalculatedValue<float>(ai, "boss hp pct", 1), Qualified() {}
        float Calculate() override;
    };

    // BossHasAuraValue returns true if a boss with the given entry within
    // 100y has the named aura (by spell id). Use for phase markers that are
    // aura-based rather than HP-based (Sapphiron air-phase hover aura,
    // C'Thun stomach-active marker, etc.).
    //
    // Qualifier format: "<bossEntry>:<spellId>" — e.g. "15989:28526".
    class BossHasAuraValue : public CalculatedValue<bool>, public Qualified
    {
    public:
        BossHasAuraValue(PlayerbotAI* ai) : CalculatedValue<bool>(ai, "boss has aura", 1), Qualified() {}
        bool Calculate() override;
    };

    // Framework #11 primitive: predictive boss-cast detection.
    //
    // BossIsCastingValue returns true if a boss with the given entry within
    // 100y is currently casting (GENERIC, CHANNELED, or AUTOREPEAT slot)
    // the specified spell id. Use for PREDICTIVE responses — Heigan's
    // Eruption cast tells us the dance is about to fire BEFORE the fissure
    // creature spawns; Loatheb's Corrupted Mind cast tells us the no-heal
    // window is about to start so healers can pre-stack HoTs / drop big
    // heals while the cast is in flight.
    //
    // Cast lookahead = spell GetCastedTime() − tick latency. For a 2.5s
    // Eruption cast detected at +0ms, bots get a ~2s heads up vs the 50ms
    // fissure-creature window of the reactive trigger.
    //
    // Qualifier format: "<bossEntry>:<spellId>" — identical to BossHasAura.
    // Cached at 1s checkInterval (sub-second cast resolution would over-
    // tick this hot path; per-tick re-fire of the action chain is fine).
    class BossIsCastingValue : public CalculatedValue<bool>, public Qualified
    {
    public:
        BossIsCastingValue(PlayerbotAI* ai) : CalculatedValue<bool>(ai, "boss is casting", 1), Qualified() {}
        bool Calculate() override;
    };
}
