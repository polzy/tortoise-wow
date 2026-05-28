# Changelog

All notable changes to this fork. Format inspired by [Keep a Changelog](https://keepachangelog.com/).

Upstream tortoise-wow doesn't keep a changelog file — entries below cover only what
this fork adds on top of `Penqle/tortoise-wow` and `alexisrichard/cmangos-playerbots`.

## [Unreleased] — 2026-05-28

### Added — bot lifecycle / gear
- 40-bot raid login pipeline (cascade login fix, throttled queue, AI re-entrancy
  gate, quarantine for broken AIs).
- `.bot init` overhaul: default ctor passes `itemQuality=0` so InitEquipment loops
  all qualities and sorts by stat weight. Lvl-60 bots now have 17-18/19 slots
  filled instead of 1-3.
- `.bot bis` overlay with class-coherent T2 8-piece sets (Battlegear of Wrath,
  Dragonstalker, Bloodfang, Netherwind, Nemesis, Stormrage, Vestments of
  Transcendence, Ten Storms, Judgement) → set bonuses active.
- `.bot fr` / `.bot frostres` / `.bot natres` / `.bot shadowres` — per-school
  resistance gear overlays (Fire/Frost/Nature/Shadow).
- `.bot inspect <name>` — chat dump of currently equipped gear, slot by slot,
  with quality letter + ilvl and a `X/19 slots / avg ilvl Y` footer. Bypasses
  the vanilla 1.12 client's UnitInParty restriction.
- `.bot revive` — also removes Resurrection Sickness auras 15007 / 27819 / 20584.
- Onyxia Scale Cloak (15138) now in Fire resist set BACK slot — Deep Breath
  immunity overrides the +4 fire res tradeoff vs the previous Wildfire Cape.
- Hunter ammo re-init after `.bot bis` so the swapped ranged weapon keeps
  matching ammo.

### Added — class spell init
- `PlayerbotFactory::LearnPenqleClassSpells()`. Penqle declares
  `learnDefaultSpells` / `learnClassLevelSpells` as empty stubs (`Player.h:2458-9`)
  with a misleading comment about an "equivalent". Before this fork, lvl-60 bots
  had ZERO class combat abilities — no Taunt, Sunder Armor, Mortal Strike,
  Charge, Heroic Strike. The 200+ spells they had were profession recipes plus a
  handful of manual `bot->learnSpell()` pushes. The workaround iterates
  `skill_line_ability` for the bot's class + race + level and learns every spell
  that passes `IsSpellFitByClassAndRace`. Logged as
  `[SPELLS] BotName (cls=N lvl=M): learned X new class spells`.

### Added — combat behavior
- Caster bots flee melee even when "follow" strategy is active
  (`RangeTriggers.h::EnemyTooCloseForSpellTrigger`). Raid bots always have
  follow active (`RaidFollowDistance=5`) so the original guard was always-true
  for casters, meaning they never backed off when a mob ran into their face.
- `AttackAnythingAction::isUseful` returns false. The cached `target` Unit*
  goes dangling when mobs despawn; subsequent derefs AV under SEH, and 80 bots
  catching the exception per tick drained ~1.5 cores. Bots still attack
  tank-assist / master target via other Actions.
- Onyxia: first-pass fight strategy (was empty stub upstream — same in
  `cmangos/playerbots`, `ike3/mangosbot`, `celguar/mangosbot-bots`). Caster/heal
  flee on melee proximity, fire prot potion, multiplier to avoid cast-step
  jitter.
- Onyxia P2 Deep Breath dodge — `OnyxiaDeepBreathTrigger` detects spell 17086
  cast within 80y; `OnyxiaMoveAwayFromBreathAction` sweeps the bot 40y away
  from her position. Reaction-tier priority.
- Off-tank target priority. `PlayerbotAI::IsOffTank()` flags any tank in a
  group that is NOT the lowest-GUID tank (stable role assignment across
  reconnects). For OTs, `TankTargetValue` switches to
  `FindTargetForOffTankStrategy` which picks the lowest-MaxHP attacker —
  reliably biases OTs toward adds (Geddon spawns, Lucifron summons, Garr
  Firesworn, Onyxia P2 whelps) rather than fighting the MT over the boss.
  RTI marker still overrides for both MT and OT.
- Molten Core dispels wired into existing fight strategies:
  - Lucifron's Curse (19703) → `remove curse`
  - Lucifron Impending Doom (19702) → `dispel magic`
  - Gehennas' Curse (19716) → `remove curse`
  - Shazzrah's Curse (19713) → `remove curse`
  Triggers already existed in upstream; only the wiring into the strategies
  was missing.

### Added — Eluna scaffold
- `BUILD_ELUNA` CMake option (OFF by default).
- `src/game/LuaEngine/` directory with `CMakeLists.txt`, `hooks/`,
  `methods/CMangos/` empty folders ready for the Eluna source tree.
- `#ifdef BUILD_ELUNA` host hooks wired in selected places (more to validate
  once Eluna is cloned).

### Added — addon
- `MCWoWBots` addon at https://github.com/polzy/MCWoWBots — separate repo.
  Provides Prepare Raid, Smart Roles, Apply Resist picker, Revive All,
  Instance Teleport dropdown, slash commands.

### Fixed — crashes
- AddAura double-free (`PlayerbotAI.cpp:6983`) — `Unit::AddSpellAuraHolder`
  deletes the holder itself on every false-return path; the old
  `if (!unit->AddSpellAuraHolder(holder)) delete holder;` was a guaranteed
  double-free, triggered by `DrinkAction`.
- AuraProcHandler NULL function pointer (`Unit.cpp:4868`) — `AuraProcHandler`
  array entries are null for unimplemented aura types; without a guard the proc
  jumped to address 0.
- `m_antiCheat` NULL in `MovementHandler.cpp` 7 sites — boss knockbacks crashed
  bots since bot sessions skip anticheat setup.
- `PlayerbotSecurity::LevelFor` 0x330 — the `SafeProbeFrom` helper was inlined
  by LTCG despite `__declspec(noinline)`. Moved the `__try`/`__except` block
  directly into `LevelFor`.
- `PlayerbotSecurity::CheckLevelFor` 0x328 follow-up — the first fix only
  protected one deref of `from`; `CheckLevelFor` made two more after `LevelFor`
  returned. Added a short-circuit on `realLevel == DENY_ALL`.
- Cascade login (`CharacterHandler.cpp`) — bot sessions had `m_Address == "<BOT>"`
  which the original disconnected/bot string check missed, so every bot login
  triggered its own group reconnect.
- AI re-entrancy gate (Engine.cpp + HostHooks.cpp) — `PlayerbotAI` constructor
  called `DoSpecificAction("auto talents")` before `OnBotLogin` finished, AVing
  on half-built context. Global `g_readyAIs` set gates the main tick path,
  `DoSpecificAction`, and per-Player tick.
- Global double-OnBotLogin guard — Polz's PlayerbotMgr and RandomPlayerbotMgr
  both tried to claim the same bot.
- Thread mutex on `m_pendingBotLogins` — MySQL worker thread vs World thread
  race observed in `std::_Tree::_Extract` at 0xFFFFFFFFFFFFFFFF.
- BiS swap silently dropping items into bags — `EquipNewItem(dest, ..., true)`
  returns non-null even when the swap fails. Replaced with destroy-then-equip
  pattern + `GetItemByPos(slot)->GetEntry() == itemId` verification.
- Server freeze during Prepare Raid — `bot->SaveToDB()` × 40 sequential SQL
  saves on the world thread (~4-20 s stall). Removed; the periodic save tick
  catches the changes.
- Stack walker null-PC special case (`Master.cpp`) — when the crash is
  EXECUTE at 0x0 (CALL through null member-function pointer), reads the return
  address from `*Rsp` and re-seeds the walker so the .dmp.txt has a real frame
  list instead of just `PC=0`.

### Changed — log noise
- `Unit::HandleProcTriggerSpell` "Spell N have 0 in EffectTriggered[K]" →
  `outDetail` (was `outError`). Vanilla dummy auras (Furor 19556 etc.) take
  this code path normally; the previous level produced thousands of
  errors.log lines per boss fight.
- `Aura::TriggerSpell` "Spell N have 0 in EffectTriggered[K]" → `outDetail`.
- `[OBL-PHASE]` macro neutered to `do { } while(0)`. Call sites preserved for
  fast re-enable.
- `[INIT-DBG]` per-slot diagnostic removed (served its purpose finding the
  itemQuality bug); `[INIT]` and `[BIS]` per-bot summaries kept.

### Removed
- `src/game/PlayerBots/` stub from Penqle. Replaced by the vendored
  `cmangos/playerbots` module at `src/modules/PlayerBots/`.

### Internal
- `.gitignore` extended to exclude `/bin/`, `*.pdb`, `*.ilk`, `*.obj`, `*.lib`.
- README extended with `Fork additions` section (done/todo tables per category).
