
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
| Tank face-away from raid               | ❌     | Tank stationary, boss faces raid |
| Tank/OT taunt on aggro loss            | ⚠️    | Trigger fires; needs validation post spell-init fix |
| OT add pickup                          | ❌     | OT applies MT logic |
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
| Garr (MC)                              | ⚠️    | Fire prot potion only — Firesworn explosion handling missing |
| Baron Geddon (MC)                      | ✅     | Living Bomb move-away + Inferno avoidance for ranged |
| Shazzrah (MC)                          | ✅     | Fire prot + Curse decurse |
| Sulfuron / Golemagg (MC)               | ⚠️    | Fire prot + Magma Splash dodge — healer interrupt missing |
| Majordomo (MC)                         | ✅     | Fire prot — CC discipline left to per-class strategies |
| Ragnaros (MC)                          | ✅     | Wrath PBAoE dodge, Submerge phase Sons of Flame switch |
| Onyxia                                 | ✅     | Full 3-phase strat: 8 Deep Breath directions, P2 attack-onyxia, P3 Bellowing Roar |
| Vaelastrasz (BWL)                      | ✅     | Burning Adrenaline move-away from raid |
| Broodlord Lashlayer (BWL)              | ✅     | Tank re-taunt priority boost on Knock Away |
| Chromaggus (BWL)                       | ✅     | Brood Affliction 4-of-5 danger dispel |
| Firemaw/Ebonroc/Flamegor (BWL)         | ❌     | Drake trio — Shadow Flame + tail swipe TODO |
| Razorgore / Nefarian (BWL)             | ❌     | Mind Control / Class Calls TODO |
| AQ20 / AQ40 bosses                     | ❌     | |
| Naxxramas bosses                       | ⚠️    | Four Horsemen void zone dodge only — rest of wing unscripted |

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
