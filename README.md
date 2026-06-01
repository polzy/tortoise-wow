
# Tortoise-WoW

This is an unofficial, community driven, restoration of the 1.18.1 patch of Turtle-WoW, with some additions for solo play.  
This project is not to be used for profit or to misrepresent itself, or anyone using it, as the original creators  
This project targets version 1.18.1 build 7272

## Client Version

The client version targetted is patch 1.18.1, build 7272  
Any client that does not match this version or build will likely have a myriad of issues

## Additions
Additions will be added as the core code reaches feature completion

#### Current Additions

- **Autoscale** - Rudimentary toggleable dungeon/raid auto scaling system, found in mangosd.conf
- **Leech** - Basic toggleable leech system designed for solo play, found in mangosd.conf
- **Additional Talent Points** - Mostly used for testing, found in tw_char.characters

#### Planned Additions

- **[Playerbots][20]** - Currently implemented in a very basic fashion, not ready for use
- **[Eluna][19]** - The WoW lua engine

## Database Setup

1. Manually import sql/create_databases.sql
2. Manually import all sql scripts in the sql/base folder
3. Run mangosd to automatically import and track updates  

This will be streamlined once the core is more up to date

## Contributing

Contributions are welcome, but I may be slow to review and merge PRs

See `CONTRIBUTING.md` for ways to get started.

---

# Fork additions

This fork extends Penqle's tortoise-wow with a heavily patched cmangos/playerbots
integration (PR#79 base from alexisrichard) plus an addon for raid management.
Goal: stable 40-bot raids on Turtle WoW 1.18.1.

**Intended usage profile**: 1 human master + up to 40 personally-owned bots, running solo
content (questing, dungeons, raids).

**Multi-master correctness**: the strategy layer itself is instance-scoped correctly —
pro-engage triggers use `Cell::VisitAllObjects(bot, ...)` which only sees mobs in the
bot's own map+instance, `EngageNearbyAddAction::FindClosest()` is per-bot, `BotStatusBroadcaster`
whispers each bot's own master, `OffTank` target selection reads the bot's own threat
list, etc. Two simultaneous BWL raids in two instances would each run pro-engage
independently with zero cross-pollution.

The real ceiling for multi-master is NOT the strategy logic but the
**MaNGOS world thread (single-threaded)**: every player/bot tick + map update +
packet serialization runs on one core, so adding raids past ~3-4 simultaneously
degrades tick rate visibly. A box with very strong single-thread performance can
host more bot-raids; a box with many slow cores cannot.

**Dispel trigger scope**: dispels for single-target debuffs (e.g. Skeram MC,
Ossirian Curse of Tongues, Hakkar Aspects on tank) use the
`PartyHasAuraBySpellIdTrigger` base in `GenericTriggers.h`. The base scans the
bot's whole group (same map, alive) for the named aura so a dispel-capable
bot fires the chain even when the affected bot can't act (MC'd, polymorphed,
slept, or a warrior tank with no `remove curse`). Self-aura check stays as
first short-circuit for raid-wide AOEs (Faerlina Poison Bolt Volley, Lucifron
Impending Doom).

## What works

### Bot lifecycle
| Feature                                | Status | Notes |
|----------------------------------------|--------|-------|
| 40-bot raid cascade login              | ✅     | Cascade login fix (`CharacterHandler.cpp` guard) |
| Auto-rejoin master's group/raid        | ✅     | Throttled queue (3s between adds, 10s grace) |
| `.bot add / remove / login / logout`   | ✅     | |
| `.bot revive <name>` (+ remove sickness)| ✅    | Spells 15007 / 27819 / 20584 |
| Group auto-create on first bot         | ✅     | |
| Cross-instance summon                  | ✅     | Restriction removed |
| Stable concurrent 40-bot AI ticks      | ✅     | Quarantine + re-entrancy gate + thread mutex |

### Gear pipeline
| Feature                                | Status | Notes |
|----------------------------------------|--------|-------|
| `.bot init <name>` — full randomize    | ✅     | 17-18/19 slots filled, all qualities considered |
| `.bot bis <name>` — T2 8-piece set overlay| ✅  | Battlegear of Wrath / Dragonstalker / etc with set bonuses |
| `.bot fr <name>` — Fire Resist 243-339 | ✅     | MC / Onyxia |
| `.bot frostres <name>` — Frost 340-373 | ✅     | Naxx Sapphiron/KT |
| `.bot natres <name>` — Nature 279-337  | ✅     | AQ Huhuran / Hydraxian |
| `.bot shadowres <name>` — Shadow 240-280| ✅    | Naxx Four Horsemen |
| `.bot inspect <name>` — chat dump      | ✅     | Bypass vanilla UnitInParty inspect limitation |
| Hunter ammo re-init after BiS swap     | ✅     | |
| Equip swap with post-verify            | ✅     | Destroy-then-equip + GetItemByPos check |

### Class spell init
| Feature                                | Status | Notes |
|----------------------------------------|--------|-------|
| Class spells via SkillLineAbility      | ✅     | `LearnPenqleClassSpells()` — workaround for Penqle's empty `learnClassLevelSpells` stub. Bots now have Taunt / Sunder / Mortal Strike / Charge / etc |
| Profession recipes                     | ✅     | |
| Talent build per spec                  | ✅     | `InitTalentsTree()` |
| Weapon skills capped at maxValue       | ✅     | |

### Combat behavior
| Feature                                | Status | Notes |
|----------------------------------------|--------|-------|
| DPS assist tank's target               | ✅     | `dps assist` strategy |
| Tank face-away from raid               | ✅     | Onyxia TankFaceAway (TANK_DISTANCE=14y, centroid-based opposite-side placement) |
| Tank/OT taunt on aggro loss            | ⚠️    | Trigger fires; needs validation post spell-init fix |
| OT add pickup (reactive)               | ✅     | OffTank target priority biases to lowest-MaxHP attacker (Geddon/Lucifron summons, Garr Firesworn, Onyxia whelps) |
| Pro-engage adds (proactive)            | ✅     | `NearbyHostileCreaturesTrigger` + `EngageNearbyAddAction` — pulls adds the moment they spawn (Razorgore Dragonkin/Grethok, Garr Firesworn, Sulfuron Priestesses, Onyxia P2 whelps, Ragnaros Sons of Flame, Sartura Royal Guards, Anub'Rekhan Crypt Guards, Faerlina Worshippers, Gluth Zombie Chow, Fankriss Spawn/Hatchling) |
| Caster stays at range                  | ✅     | `enemy too close for spell` fires for casters even with `follow` active |
| Hunter pet + ranged shot               | ⚠️    | Ammo fix applied, needs in-game validation |
| Healer triage                          | ✅     | |
| `AttackAnythingAction` proactive aggro | 🚫     | Disabled (dangling Unit* CPU drain). Bots only attack assigned targets |

### Raid encounter strategies
| Boss / Raid                            | Status | Notes |
|----------------------------------------|--------|-------|
| Magmadar (MC)                          | ✅     | Caster move away, fire prot potion, lava bomb dodge |
| Lucifron (MC)                          | ✅     | Fire prot + Curse decurse + Impending Doom dispel |
| Gehennas (MC)                          | ✅     | Fire prot + Curse decurse |
| Garr (MC)                              | ✅     | Firesworn Eruption (19497) cast-detect → ranged dodge 20y |
| Baron Geddon (MC)                      | ✅     | Living Bomb move-away + Inferno avoidance for ranged |
| Shazzrah (MC)                          | ✅     | Fire prot + Curse decurse |
| Sulfuron / Golemagg (MC)               | ⚠️    | Fire prot + Magma Splash dodge + Demo Shout dispel — Priestess heal interrupts still missing |
| Majordomo (MC)                         | ✅     | Fire prot — CC discipline left to per-class strategies |
| Ragnaros (MC)                          | ✅     | Wrath PBAoE dodge, Submerge phase Sons of Flame switch |
| Onyxia                                 | ✅     | Full 3-phase strat: 8 Deep Breath directions, P2 attack-onyxia, P3 Bellowing Roar |
| Vaelastrasz (BWL)                      | ✅     | Burning Adrenaline move-away from raid |
| Broodlord Lashlayer (BWL)              | ✅     | Tank re-taunt priority boost on Knock Away |
| Chromaggus (BWL)                       | ✅     | Brood Affliction 4-of-5 danger dispel |
| Firemaw (BWL)                          | ⚠️    | Fire prot pot only. Flame Buffet tank-swap needs multi-tank coord framework |
| Ebonroc (BWL)                          | ⚠️    | Fire prot pot only. Shadow of Ebonroc self-heal = burst through |
| Flamegor (BWL)                         | ✅     | Fire prot pot + Frenzy (23342) Tranquilizing Shot |
| Razorgore (BWL)                        | ✅     | P1 add-priority via 'razorgore phase 1' (Possess 19832) + pro-engage Dragonkin/Grethok at 60y; P2 tank-and-spank. **Skip patch**: kill at any phase = encounter DONE |
| Nefarian (BWL)                         | ⚠️    | P2: Bellowing Roar fear-break (Will of Forsaken / Berserker Rage). Class Calls 23397-23436 need per-class fear/MC plumbing |
| Battleguard Sartura (AQ40)             | ✅     | Whirlwind (26083) — ranged stay >12y |
| Princess Huhuran (AQ40)                | ✅     | Frenzy (26051) tranq + Noxious Poison (26053) cure (group-scan: tank can't self-cleanse) |
| Fankriss (AQ40)                        | ⚠️    | Pro-engage Spawn of Fankriss (15630) + Vekniss Hatchling (15962). Mortal Wound dispel TODO |
| Bug Trio (AQ40)                        | ✅     | Pro-engage Yauj Broods (15621) + Kri Toxic Volley (25812) cure poison + Yauj Fear (19408) magic dispel group-scan |
| C'Thun P2 (AQ40)                       | ⚠️    | Pro-engage 5 tentacle types (Eye 15726, Small Claw 15725, Giant Claw 15728, Giant Eye 15334, Flesh 15802) at priority 90 |
| Skeram (AQ40)                          | ⚠️    | True Fulfillment (785) MC dispel — priority 95, group-scan trigger (MC'd bot can't self-cleanse). Split-clone targeting still requires phase awareness |
| Twin Emperors (AQ40)                   | ⚠️    | Mutate Bug (802) magic dispel — priority 95, group-scan (polymorphed bot can't act). Multi-tank teleport swap still TODO |
| Other AQ40 bosses                      | ❌     | Ouro burrow (GO interaction), Viscidus freeze/shatter (frost CD coord) |
| Moam (AQ20)                            | ⚠️    | Pro-engage Mana Fiends (15527) on summon |
| Buru (AQ20)                            | ⚠️    | Pro-engage Hivezara Hatchlings (15521). Egg-explode mechanic still raid-side |
| Ayamiss (AQ20)                         | ⚠️    | Pro-engage Larva/Hornet/Swarmer (15555/15934/15546) |
| Rajaxx (AQ20)                          | ⚠️    | Pro-engage all 7 wave commanders (Zerran/Yeggeth/Pakkon/Drenn/Xurrem/Qeez/Tuubid) |
| Ossirian (AQ20)                        | ⚠️    | Curse of Tongues (25195) remove curse — group-scan (tank can't self-cleanse). Tornado-kite shield-break TODO |
| Kurinnaxx (AQ20)                       | ❌     | Mortal Wound is physical — no dispel. Sand trap dodge needs GO awareness |
| Hakkar (ZG)                            | ✅     | Marli/Jeklik dispel + Venoxis cure poison + Thekal Tranquilizing Shot — all group-scan (cast on tank, who can't self-cleanse) |
| Mandokir (ZG)                          | ⚠️    | Pro-engage Ohgan (14988) at 40y → +25% dmg on Mandokir per ScriptDev2 |
| Marli (ZG)                             | ⚠️    | Pro-engage Spawn of Marli (15041) — adds with poison aura |
| Jin'do (ZG)                            | ⚠️    | Pro-engage Brainwash Totem (15112, MCs raid members) + Healing Ward (14987). Priority 95 — fight-defining |
| Thekal (ZG)                            | ⚠️    | Pro-engage P1 Zealots (Lor'Khan 11347 + Zath 11348) + P2 Tigers (15068) |
| Arlokk (ZG)                            | ⚠️    | Pro-engage Zulian Prowlers (15101) during vanish phases |
| Hazzarah (ZG)                          | ⚠️    | Sleep (24664) magic dispel — group-scan (slept bot is incapacitated, can't self-cleanse) |
| Venoxis (ZG)                           | ⚠️    | Pro-engage Razzashi Cobras (11373) |
| Other ZG bosses                        | ❌     | Jeklik (Bat form / Charge / Screech fear-break covered by class strats) |
| Patchwerk (Naxx)                       | ✅     | Tank-and-spank — no bot-side mitigation needed beyond class strats |
| Loatheb (Naxx)                         | ✅     | Anti-heal mechanic handled by per-class healer cooldowns |
| Kel'Thuzad (Naxx)                      | ✅     | Mana Detonation (27819) dispel — group-scan + caster spread |
| Four Horsemen (Naxx)                   | ⚠️    | Void zone dodge only — mark-swap mechanic TODO (marks not magic-dispelable per ScriptDev2) |
| Anub'Rekhan (Naxx)                     | ⚠️    | Pro-engage Crypt Guards (16573). Locust Swarm dodge TODO |
| Grand Widow Faerlina (Naxx)            | ⚠️    | Pro-engage Worshippers/Followers (16505/16506) + Poison Bolt Volley (28796) cure poison on party. Enrage mechanic relies on add detonation, raid-side |
| Gluth (Naxx)                           | ⚠️    | Pro-engage Zombie Chow (16360) — OTs kite zombies away from boss. Decimate raid heal handled by class healers |
| Sapphiron (Naxx)                       | ✅     | Life Drain (28542) on-party dispel + frost resist gear + Frost Breath (28524) detection → move to nearest GO_ICEBLOCK (181247) within 50y |
| Noth (Naxx)                            | ⚠️    | Pro-engage Plagued Warriors/Guardians/Constructs/Champions (16981-16984) + Curse of Plaguebringer (29213) remove curse — group-scan (3 random victims) |
| Maexxna (Naxx)                         | ⚠️    | Pro-engage Spiderlings (17055) at 60y + Necrotic Poison (28776) cure poison — group-scan (tank only) |
| Gothik (Naxx)                          | ⚠️    | Pro-engage all 7 add types (Unrelenting Trainee/DK/Rider 16124-16126, Spectral Trainee/DK/Rider/Horse 16127/16148/16149/16150) |
| Other Naxx bosses                      | ❌     | Heigan dance (positional puzzle), Razuvious mind-control orbs (GO use), Thaddius polarity swap (multi-player coord) |

### Server / infra
| Feature                                | Status | Notes |
|----------------------------------------|--------|-------|
| Crash handler with stack walk          | ✅     | Symbolized stack in `crash_*.dmp.txt`, handles NULL PC |
| Heap-corruption crashes resolved       | ✅     | AddAura double-free, AuraProcHandler NULL, m_antiCheat NULL |
| `LevelFor` / `CheckLevelFor` dangling Player* | ✅ | `__try` inlined (LTCG-resistant) + DENY_ALL short-circuit |
| Spam errors downgrade                  | ✅     | Dummy aura `EffectTriggered[0]` proc warnings outDetail |
| MariaDB deadlock on `character_aura`   | ⚠️    | Rare, benign |

### Eluna Lua scripting engine
| Feature                                | Status | Notes |
|----------------------------------------|--------|-------|
| `BUILD_ELUNA` CMake option             | ✅     | Default OFF; build flag wired in `src/game/CMakeLists.txt` |
| `src/game/LuaEngine/` scaffold         | ✅     | `CMakeLists.txt` collects sources, `hooks/` + `methods/CMangos/` folders ready |
| Eluna source vendored                  | ❌     | Clone https://github.com/ElunaLuaEngine/Eluna into `src/game/LuaEngine/` before building with `-DBUILD_ELUNA=ON` |
| `#ifdef BUILD_ELUNA` host hooks        | ⚠️    | Partial — to validate once Eluna is cloned and a sample script is wired |

## Todo

### Raid priority
1. Tank face-away boss positioning — boss reste face au raid
2. OT add-pickup logic — whelps Onyxia P2, Geddon, Lucifron summons
3. Onyxia Deep Breath detection (spell 17086 / 23461) + perpendicular kite
4. Onyxia Scale Cloak (item 15138) into BiS BACK slot for full Deep Breath immunity

### Boss strategies to complete
- Lucifron AoE Silence handling
- Garr / Geddon add explosion dodge
- Shazzrah Magic Grounding + Arcane Explosion
- Sulfuron Healers add pickup
- Golemagg explosion + add removal
- Majordomo CC management
- Ragnaros submerge → Sons of Flame add waves
- BWL: Razorgore mind control, Vael fire breath, Broodlord knockback, Chromaggus dispel
- AQ40: Twin Emp positioning, Princess Yauj resurrection prevent, Huhuran nature resist
- Naxx: 4 Horsemen mark swap, KT mind control, Sapphiron ice block

### Polish
- Smart Roles context-aware per raid map (Onyxia 1 MT 1 OT vs MC 2 MT vs Naxx 3 MT)
- Auto-resist on map enter (`.bot fr *` when master enters MC/Onyxia)
- Pre-raid buffs auto-dispense (Mark of the Wild, PW: Fortitude, blessings, totems)
- Bot inventory cleanup command (`.bot cleanup` to clear non-BiS bag clutter)
- Inspect UI (addon click-handler on portrait → `.bot inspect`)

### Infra / dev
- Auto-test harness `.testbots` (init/bis/inspect per class, JSON-ish report)
- Crash dump Discord webhook (parsed top frame → asynchronous monitoring)
- Performance benchmark for `LearnPenqleClassSpells` (~5-10 ms per bot expected)

## Companion repos / external references

- **Addon `MCWoWBots`** — separate repository (not bundled with server). Provides the
  Apply Resist picker, Prepare Raid, Smart Roles, Revive All buttons, instance teleport.
  - **V2 tabbed UI** (`/mcwb`) with Roster / Combat / Gear / Strategy / Logs tabs.
    Combat & Strategy tabs consume the live bot status fed by
    `BotStatusBroadcaster` (server-side). Gear tab wraps the legacy V1 buttons.
  - **Bot Status broadcaster**: server whispers the master with the `MCWBS\t`
    prefix every 2s (only on snapshot change). Filtered + parsed client-side
    so the chat stays clean; data is exposed as
    `MCWoWBotsStatus.botData[name] = { state, target, strategies, lastUpdate }`.
  - Slash commands: `/mcwb` (V2), `/mcwbpanel`/`/mcwbp` (standalone status
    panel), `/mcwbs all|<name>|panel|clear|help` (debug).
- **Strategy cross-reference** — `ike3/mangosbot`, `cmangos/playerbots`,
  `celguar/mangosbot-bots`, `azerothcore/mod-playerbots`. Onyxia is an empty stub upstream
  in all of these. MC has partial implementations worth comparing against ours.
- **Boss AI source** — `cmangos/scriptdev2` provides spell IDs + timings for every raid
  boss in the AI scripts, useful for choosing trigger spell IDs when implementing fight
  strategies.

## License

This fork preserves the upstream **AGPL-3.0** license inherited from Penqle/tortoise-wow,
which itself inherits from MaNGOS Zero. All custom additions are released under the same
license. Files added by this fork are clearly identifiable: source code uses
`mcwow_bis::`, `mcwow_resist::` namespaces, and inline comments tagged with
`// CRITICAL:`, `// BUG FIX:`, or `// MCWoW` when surgical changes are made to
upstream files.

## Upstream tracking

Recommended remotes for downstream forks:
```sh
git remote add upstream https://github.com/Penqle/tortoise-wow
git fetch upstream
git checkout main && git rebase upstream/main   # periodically
```

Custom changes live in a separate long-running branch (e.g. `mcwow-polzy`) and are
periodically rebased on top of upstream so the divergence stays manageable.


[19]: https://github.com/ElunaLuaEngine/Eluna
[20]: https://github.com/ike3/mangosbot-bots
