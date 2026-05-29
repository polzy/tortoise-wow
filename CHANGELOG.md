# Changelog

All notable changes to this fork. Format inspired by [Keep a Changelog](https://keepachangelog.com/).

Upstream tortoise-wow doesn't keep a changelog file — entries below cover only what
this fork adds on top of `Penqle/tortoise-wow` and `alexisrichard/cmangos-playerbots`.

## [Unreleased] — 2026-05-29

### Added — ZG Arlokk + Hazzarah
- Arlokk Zulian Prowlers (15101) — pro-engage panther adds spawned during
  her vanish phases.
- Hazzarah Sleep (24664) — magic dispel chain. The sleep takes the
  affected raid member out of the fight; dispel asap.

### Added — ZG Thekal pro-engage (P1 zealots + P2 tigers)
- Thekal Lor'Khan (11347) + Zath (11348) pro-engage — focus all 3 zealots
  together so they die in the 6s rez window.
- Thekal Tigers (15068) — P2 summons.

### Added — ZG Marli + Jin'do pro-engage (3 more encounters)
- Marli Spawn of Marli (15041) — pro-engage, poison-aura adds.
- Jin'do Brainwash Totem (15112) — **MCs raid members**, fight-defining
  kill priority. Priority 95 (highest pro-engage we have).
- Jin'do Powerful Healing Ward (14987) — heals Jin'do significantly.
  Same trigger, priority 95.

### Added — Naxx wing-bosses pro-engage + 2 dispels (Maexxna, Noth)
- Pro-engage Maexxna Spiderlings (17055) — 12 spawn at 75/50/25% HP per
  ScriptDev2. Priority 85.
- Pro-engage Noth Plagued adds (Guardian 16981 / Construct 16982 /
  Champion 16983 / Warrior 16984) — spawn during teleport-to-balcony.
  Priority 85.
- Maexxna Necrotic Poison (28776) — poison, 90% heal reduction. Chain:
  `cure poison` (druid/shaman) + `cleanse poison` (paladin). Priority 90.
- Noth Curse of Plaguebringer (29213) — curse, deadly tick. Chain:
  `remove curse` (druid/mage). Priority 90.

### Added — pro-engage extensions + minimap drag
- AQ20 Rajaxx wave commanders pro-engage. All 7 named officers
  (Colonel Zerran 15385 / Major Yeggeth 15386 / Major Pakkon 15388 /
  Captain Drenn 15389 / Captain Xurrem 15390 / Captain Qeez 15391 /
  Captain Tuubid 15392). Wired in `RuinsOfAhnQirajDungeonStrategy`
  priority 85 — covers the entire 7-wave cycle.
- ZG Mandokir Ohgan (14988) pro-engage at 40y. Burning Ohgan first
  makes Mandokir take +25% dmg per ScriptDev2 — priority 90.
  Wired in `ZulGurubDungeonStrategy`.
- Addon: `MCWoWBotsMinimap.lua` (new file). Right-click drag on the
  MCWoWBots minimap button repositions it; position saved in
  `MCWoWBotsV2DB.minimapPos`. Tooltip extended with the drag hint.

### Added — AQ20 pro-engage + dungeon strategy
- New `RuinsOfAhnQirajDungeon{Triggers,Actions,Strategies}.h/.cpp` files
  (map 509, auto-enable wired in DungeonStrategy.cpp).
- Pro-engage triggers + actions:
  - Moam Mana Fiend (15527) — priority 90 (fight-defining mana drain).
  - Buru Hivezara Hatchling (15521) — priority 85.
  - Ayamiss adds (Larva 15555 / Hornet 15934 / Swarmer 15546) — priority 85.

### Added — broadcaster ‹current action› payload
- `BotStatusBroadcaster::BroadcastAction` — emits `MCWBS\tA|<name>|<action>`
  whenever the bot's current engine's last-executed action name changes.
- `PlayerbotAI::m_lastActionName` member + UpdateAI hook (same 2s
  throttling as the strategies snapshot).
- Addon `MCWoWBotsStatus.lua` parses the new `A|` payload into
  `botData[name].action`. V2 Combat tab cell now reads
  `[<action>] > <target>`; V2 Strategy tab header shows
  `State: ... Target: ... Action: ...`.

### Fixed — code review pass (post-deploy review by background agent)
- **Razorgore P1 trigger never fired** (CRITICAL). `RazorgorePhase1Trigger`
  was checking `HasAura(23014)` but Razorgore actually carries
  SPELL_POSSESS_ORB **19832** (boss_razorgore.cpp:44,
  instance_blackwing_lair.cpp:622). 23014 is the channel-spell value used
  cosmetically on the trigger creature, not a real aura on Razorgore.
  Corrected to 19832 — bots now properly switch to add-only in P1.
- **Four Horsemen mark dispel chain** disabled. Marks 28832-28835 aren't
  classified as magic in vanilla DBC; `dispel magic` / `cleanse magic`
  no-op on them. Chain removed (trigger kept registered for diagnostics).
- **Garr Firesworn Eruption detection** rewritten. Cast-based observation
  (`GetCurrentSpell == 19497`) had a microsecond window because Eruption
  is fired from `JustDied`. New proxy: any Firesworn within 20y at <25%
  HP triggers the move-away. More reliable.
- **MCWoWBotsStatus.lua hook chain** now gated by `MCWoWBotsStatus._hooked`
  flag. Each `/reload` was previously adding 1 extra indirection to
  `ChatFrame_OnEvent`; long sessions had O(N) chain depth per whisper.

### Added — Étape 1: pro-engage triggers (proactive add detection)
- New `playerbot/strategy/triggers/ProEngageTriggers.h` — generic
  `NearbyHostileCreaturesTrigger` (entries[], rangeY) plus per-encounter
  subclasses for Razorgore (Dragonkin 12422 / Grethok 12420 /
  Captain 14036), Garr (Firesworn 12099), Sulfuron (Priestess 12099),
  Onyxia (Whelps 11262), Ragnaros (Sons of Flame 12143).
- New `playerbot/strategy/actions/ProEngageActions.h` — `EngageNearbyAddAction`
  inherits `AttackAction`, finds closest add of an entry list, pins
  `attack target` and calls `Attack(requester, target)`. Subclasses for
  each of the 5 encounters above.
- Wired into:
  - `RazorgoreFightStrategy` priority 95 (above the existing P1 'attack
    least hp target' at 90 which only works on already-aggroed adds).
  - `GarrFightStrategy` priority 70 — OTs grab Firesworn on spawn before
    the 50%-HP eruption routine fires.
  - `SulfuronFightStrategy` priority 80 — OTs grab Priestesses before
    they cast Heal (Inspire) on Sulfuron.
  - `OnyxiaFightStrategy` priority 75 — P2 whelps focus on landing.
  - `RagnarosFightStrategy` priority 85 — Sons of Flame on submerge wave.
  - `SarturaFightStrategy` priority 80 — Royal Guards (AQ40, 15984).
  - `NaxxramasDungeonStrategy` (dungeon-level) priorities 85/80 — Anub'Rekhan
    Crypt Guards (16573) and Faerlina Worshippers/Followers (16505/16506).
  This complements the existing reactive triggers (HasAura debuff, lost
  aggro) — the pro-engage layer pre-empts the wave by entry-scanning the
  cell window so tanks/DPS pull adds BEFORE they aggro a random target.

### Added — server→addon Status channel (Phase A)
- `playerbot/BotStatusBroadcaster.h/.cpp` — server-side. Whispers the
  master with `MCWBS\t` prefix every 2s when the bot's current-engine
  strategy snapshot changes. Encoding: `S|<botname>|<STATE>:<strats>`.
- `PlayerbotAI.h` — added `m_lastStrategySnapshot` (string) +
  `m_statusBroadcastAcc` (uint32 ms accumulator).
- `PlayerbotAI::UpdateAI` — hooks the throttled call to BroadcastStrategies
  after the main tick.

### Added — MCWoWBots addon V2 + Status panel (Phase B + C)
- Client addon `MCWoWBots` (in `D:\MCWow\client\twmoa_1181_cn\twmoa_1181\
  Interface\AddOns\MCWoWBots\`) gained 3 new Lua files:
  - `MCWoWBotsStatus.lua` — hooks `ChatFrame_OnEvent` to intercept the
    `MCWBS\t` whispers, parse them into `MCWoWBotsStatus.botData[name]`,
    and SUPPRESS the chat display. Slash `/mcwbs all|<name>|clear|help`.
  - `MCWoWBotsStatusPanel.lua` — standalone draggable panel POC (Phase C).
    `/mcwbpanel` or `/mcwbp`.
  - `MCWoWBotsV2.lua` — full tabbed window (Phase B). 5 tabs:
    Roster / Combat / Gear / Strategy / Logs. Saved variables for frame
    position and last-tab. Slash `/mcwb`.
- `MCWoWBots.toc` updated with `SavedVariables: MCWoWBotsV2DB` and the
  3 new files.
- `MCWoWBots.xml` — minimap button moved from TOPLEFT (conflictful with
  other addons) to TOPRIGHT (-4, -78); OnClick now routes to V2 with
  V1 fallback.

### Fixed — `.recall` AV (CheckLevelFor)
- `PlayerbotSecurity.cpp` — `CheckLevelFor` now goes through
  `SehSafeLevelFor` + `SehSafeProbe(from)` wrappers. The previous
  DENY_ALL short-circuit guard only caught some of the dangling-`from`
  derefs; crash_20260529_125238 hit READ at Player+0x350 inside
  `CheckLevelFor+0x77`, BEFORE the guard. Now any AV on `from` during
  LevelFor or the early deref converts to a clean "deny safely" without
  the world thread crashing.

### Fixed — Razorgore reconnect softlock + skip patch
- `src/scripts/dungeons/blackwing_lair/boss_razorgore.cpp` — `JustDied`
  unconditionally `SetData(TYPE_RAZORGORE, DONE)`. Vanilla penalty (kill
  Razorgore in P1 → raid-wide SPELL_EXPLOSION + boss respawn) disabled.
  Reasons: reconnects mid-MC softlock the instance (bots aggro and kill
  Razorgore while no MCer is alive → TYPE_RAZORGORE=FAIL → EXIT door
  stays closed permanently); small groups can't roster 5+ MCers and bots
  can't take the orb yet.



### Added — BWL drake trio + Nefarian
- Firemaw (11983), Ebonroc (14601), Flamegor (11981) — separate
  `*FightStrategy` per drake. Common fire-prot pot. Flamegor's
  Frenzy (23342) wires hunter Tranquilizing Shot via
  `FlamegorFrenzyTrigger`. Other drake mechanics (Wing Buffet threat
  reset, Flame Buffet tank-swap, Shadow of Ebonroc heal-aura) require a
  multi-tank coord framework we don't have — documented in
  `BlackwingLairDungeonStrategies.h`.
- Nefarian P2 (11583) — `NefarianFightStrategy` with fire-prot pot +
  Bellowing Roar (22686) fear-break chain (`will of the forsaken` →
  `berserker rage fear`), mirroring the Onyxia P3 trigger. Class Calls
  (23397/23398/23401/23410/23414/23418/23425/23427/23436) require
  per-class fear/MC handling beyond this strategy.

### Added — Naxx Sapphiron + Four Horsemen
- Sapphiron (15989) — `SapphironFightStrategy` with Life Drain (28542)
  magic dispel chain. Frost Aura (28529) handled by `.bot frostres`
  gear; Frost Breath / Ice Block hide mechanics need a GO finder
  primitive — TODO.
- Four Horsemen mark danger trigger — collapsed all four marks
  (28832/28833/28834/28835) into one trigger that routes through the
  raid dispel chain. Stack-count detection isn't currently exposed; the
  trigger fires on any mark presence. ScriptDev2 uses Mograine as the
  Unholy mark (NPC 16062) — not Rivendare which is TBC-only.

### Added — AQ40 Sartura + Huhuran
- New `TempleOfAhnQirajDungeon{Triggers,Actions,Strategies}.h/.cpp`
  files with parent dungeon strategy (map 531).
- Battleguard Sartura (15516) — Whirlwind (26083) ranged stay-out
  trigger `sartura too close` (12y radius) → `move away from sartura`.
  Threat resets during Whirlwind mean any in-range non-tank is a target
  candidate.
- Princess Huhuran (15509) — Frenzy (26051) → hunter Tranquilizing
  Shot. Noxious Poison (26053) → druid `cure poison` chain. Nature
  resist gear via existing `.bot natres`.
- Skeram / Twin Emperors / Ouro / C'Thun / Viscidus / Bug Trio skipped
  — phase mechanics (MC retake, mutate swap, burrow/dirt, eye phase,
  frozen/shatter, healer-MC) require frameworks we don't yet expose.

### Added — Zul'Gurub Hakkar
- New `ZulGurubDungeon{Triggers,Actions,Strategies}.h/.cpp` files with
  parent dungeon strategy (map 309).
- Hakkar (14834) Aspects fall-through chain:
  - Marli stun (24686) + Jeklik silence (24687) → `dispel magic` /
    `cleanse`.
  - Venoxis poison (24688) → `cure poison` / `cleanse`.
  - Thekal enrage on Hakkar himself (24689) → Tranquilizing Shot.
  Cause Insanity (24327) charm + Bloodsiphon (24322-24324) untreated
  (charm requires party-target plumbing; Bloodsiphon non-dispellable).

### Fixed — auto-enable for Naxx + AQ40 + ZG dungeon strategies
- `DungeonStrategy::InitCombatTriggers` and `InitNonCombatTriggers`
  were missing the enter/leave wiring for Naxxramas — so the
  `NaxxramasDungeonStrategy` only ever fired if the admin manually
  ran `+naxxramas`. Same for the newly-added AQ40 and ZG dungeons.
  Wired all three so the parent dungeon strategy activates on map
  entry like Onyxia/MC/BWL/Karazhan already did.

### Added — MC / BWL boss mechanics fill-in
- Garr (12057) Firesworn Eruption (19497) dodge. Detection is cast-based
  (`GarrFireswornEruptionTrigger` scans 12099 adds within 35y for the
  spell's currentGenericSpell). Ranged/heal bots `move away from garr
  firesworn` (20y around the Firesworn) — mêlée eats the splash.
- Sulfuron Harbinger (12098) Demoralizing Shout (19778) dispel. Magic
  debuff → existing `dispel magic` / `cleanse` class-routed chain at
  prio 80. Spell ID was wrong on the README earlier (23511 was TBC); the
  vanilla ScriptDev2 const SPELL_DEMORALIZINGSHOUT confirms 19778.
- Razorgore the Untamed (12435) Phase 1 awareness. New
  `RazorgoreFightStrategy` + `RazorgorePhase1Trigger` (boss HasAura
  23014 Possess). During the egg-orb phase ranged/melee bots prioritize
  `attack least hp target` (the dragonkin/Grethok adds) over the
  default tank-target / dps-assist selection. P2 falls through to
  normal tank-and-spank.

### Fixed — code review pass (Onyxia strategy)
- `TankOnyxiaFaceAwayAction` TANK_DISTANCE 5.0f → 14.0f. The previous value
  placed the MT's move-target inside Onyxia's combat hitbox
  (combat_reach ~10y), so MoveTo refused to path or dropped the tank in her
  cleave/tail-swipe arc. Skip-already-placed tolerance bumped 3y → 5y to
  prevent jitter at the longer range. (`OnyxiasLairDungeonActions.h:109`)
- `AttackOnyxiaAction` re-rooted on `AttackAction` (was raw `Action`). The
  upstream comment claimed it 'routed through the standard AttackAction
  execute path' but the class didn't inherit from it — meaning the actual
  bot->Attack() / facing / pet-attack / OnCombatStarted pipeline never
  fired. Likely explains the user-reported 'un seul mage tape Onyxia en P2'
  on the previous build. Now calls `Attack(requester, onyxia)` after
  pinning 'attack target', re-using the proven AttackAction logic.
  (`OnyxiasLairDungeonActions.h:146`)
- Removed `ai->ChangeEngine(BOT_STATE_COMBAT)` from
  `AttackOnyxiaAction::Execute`. Re-entrant engine swap from inside an
  Execute() loop matches the AI re-entrancy crash class already covered by
  the `g_readyAIs` gate. Pinning the target via `SET_AI_VALUE` is enough —
  the combat tick picks it up on the next iteration.
  (`OnyxiasLairDungeonActions.h:177`)

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
- BWL boss strategies: Vaelastrasz (Burning Adrenaline move-away), Broodlord
  (Knock Away tank re-taunt priority), Chromaggus (Brood Affliction 4-of-5
  dispel before Chromatic Mutation). Spell IDs lifted from
  `src/scripts/dungeons/blackwing_lair/boss_*.cpp`.
- Naxx boss strategies: Patchwerk (tank-and-spank, no special mitigation
  beyond class strategies — placeholder for the Berserk 7-min DPS race),
  Loatheb (anti-heal mechanic — healers' class cooldowns already match
  the Corrupted Mind 60s lockout cadence), Kel'Thuzad (Mana Detonation
  27819 dispel + ranged spread on Frost Blast).
- Tank face-away positioning. `TankOnyxiaFaceAwayAction` computes the
  raid centroid and moves the MT to the opposite side of Onyxia so her
  cleave + tail face the wall instead of the raid. Gated on IsTank &&
  !IsOffTank, skips P2 (Hover aura), skips when raid <2 members.
- Eluna integration progress: `src/game/_eluna_compat/` shim tree (48
  headers) bridges cmangos-style `Globals/SharedDefines.h` /
  `Entities/Player.h` includes to Penqle's flat layout. Cross-tree shims
  reach into `src/shared/` and `src/framework/`. Empty stubs for TBC+
  features (ArenaTeam, Vehicle, etc.). LuaEngine.h patched to use
  vanilla `SpellEntry` instead of TBC+ `SpellInfo`, forward-declare
  WotLK+ types (AuraEffect, DispelInfo, SpellDestination) as empty
  classes — the hooks fire only on expansions we don't have. `Log/Log.h`
  shim maps `outErrorEluna`/`outBasicEluna` to `outError`/`outBasic`.
  Remaining blockers (filed as #44 follow-up): `Map::GetEluna()`
  accessor and `InstanceData::Save/GetData64` overrides need host-side
  wiring before `BUILD_ELUNA=ON` produces a working binary.
- AttackOnyxiaAction: P2 fix so ranged/heal bots stay locked on Onyxia
  (entry 10184) instead of switching to whelps that aggro them. Pins the
  AI context's "current target" and selection guid to Onyxia, then routes
  through the standard reach-spell + cast pipeline. Was needed because the
  generic "attack" action inherits the bot's existing target — which is a
  whelp once they start attacking.
- BiS swap pre-check: `HandleBotBis` and `OverlayResistSet` now call
  CanEquipNewItem(swap=true) BEFORE destroying the old item. If the new
  item can't be equipped (class proficiency, allowable_class, etc.) the
  bot keeps the old item instead of ending up with an empty slot.
  Observed: Trollbane / Grunthar lost their ranged weapon entirely under
  the previous destroy-then-check flow.
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
