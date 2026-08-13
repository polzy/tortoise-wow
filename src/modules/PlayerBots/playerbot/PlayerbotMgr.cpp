#include "playerbot/playerbot.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "PlayerbotDbStore.h"
#include "playerbot/PlayerbotFactory.h"
#include "playerbot/PlayerbotBiS.h"
#include "playerbot/PlayerbotResistSet.h"
#include "playerbot/RandomPlayerbotFactory.h"
#include "playerbot/RandomPlayerbotMgr.h"
#include "playerbot/ServerFacade.h"
#include "playerbot/TravelMgr.h"
#include "playerbot/PlayerbotLoginMgr.h"
#include "BotDiagnostics.h"
#include "BotActionLog.h"
#include "Chat/ChannelMgr.h"
#include "SocialMgr.h"
#include "AccountMgr.h"
#include "strategy/actions/ChangeTalentsAction.h"
#include "strategy/actions/InviteToGroupAction.h"
#include "AiFactory.h"
#include "Guild/GuildMgr.h"
#include "World.h"
#include "Database/DatabaseImpl.h"
#include "ObjectMgr.h"
#include "Handlers/LoginQueryHolder.h"
#include <set>
#include <algorithm>
#include <mutex>

#ifdef GenerateBotTests
#include "strategy/tests/TestAction.h"
#include "strategy/tests/TestRegistry.h"
#endif

class CharacterHandler;

// real bot login flow.
//
// AddPlayerBot creates a synthetic WorldSession for the bot if needed, queues the bot's
// character data load via the standard Penqle CharacterDatabase pipeline, and routes the
// callback to HandlePlayerBotLoginCallback below. After the holder fires, we hand it off
// to WorldSession::HandlePlayerLogin (the same path real players use), then attach the
// PlayerbotAI via OnBotLogin.
//
// PlayerbotLoginQueryHolder is defined in PlayerbotLoginMgr.cpp (file-local class). Since
// it's not exported, we use Penqle's plain LoginQueryHolder here — the bot doesn't need
// the cmangos extra fields (masterAccountId stored on the holder is convenient but
// non-essential; we already have it via the m_pendingBotLogins map below).

// Pending logins: maps holder pointer to (botGuid, masterAccountId, owner) so the callback can
// resolve which bot is being added. Cleared on callback completion.
//
// THREAD SAFETY: AddPlayerBot inserts from the WORLD thread; HandlePlayerBotLoginCallback
// erases from a MYSQL WORKER thread (see ThreadPool::worker_mysql in the stack trace of
// crash_20260528_115701.dmp.txt). Without a mutex, concurrent std::map mutation produced
// access violations at 0xFFFFFFFFFFFFFFFF in _Tree::_Extract. The mutex is also used by
// PlayerbotHolder_OnDestroy and PlayerbotHolder_PendingLoginCount which both touch the map.
//
// `owner` is the PlayerbotHolder* that scheduled the load. Set to nullptr in ~PlayerbotHolder()
// so a callback firing after the master logged out detects the dead manager and bails before
// the virtual OnBotLoginInternal dispatch (which would otherwise jump through a deleted vtable
// and crash the world thread).
//
// g_aliveHolders is a parallel guard used for an early exit at the very top of the callback,
// before *any* member access on `this`. It is the only thing that protects us from the case
// where the DB layer dispatches `this->HandlePlayerBotLoginCallback(...)` on a deleted holder.
// Both data structures live at file scope so they outlive any holder.
namespace {
    struct PendingBotLogin {
        ObjectGuid botGuid;
        uint32 masterAccountId;
        PlayerbotHolder* owner;
    };
    std::map<SqlQueryHolder*, PendingBotLogin> m_pendingBotLogins;
    std::set<PlayerbotHolder*> g_aliveHolders;
    // Single mutex guarding both containers. Acquired on every write (insert/erase) and
    // every traversal. Low contention — only fires during bot-add/login-callback.
    std::mutex g_pendingMutex;
}

// Called from ~PlayerbotHolder() to invalidate any in-flight callbacks for this manager.
void PlayerbotHolder_OnDestroy(PlayerbotHolder* self)
{
    std::lock_guard<std::mutex> lock(g_pendingMutex);
    g_aliveHolders.erase(self);
    for (auto& kv : m_pendingBotLogins)
    {
        if (kv.second.owner == self)
            kv.second.owner = nullptr;
    }
}

void PlayerbotHolder_OnConstruct(PlayerbotHolder* self)
{
    std::lock_guard<std::mutex> lock(g_pendingMutex);
    g_aliveHolders.insert(self);
}

bool PlayerbotHolder_IsAlive(PlayerbotHolder* self)
{
    std::lock_guard<std::mutex> lock(g_pendingMutex);
    return g_aliveHolders.find(self) != g_aliveHolders.end();
}

// True if any bot DB query is still in flight for this holder. Used by the queue
// processor to enforce strict serialization: don't AddPlayerBot for guid N+1 while
// guid N's DB callback hasn't fired yet. Without this gate, DB queries finish out
// of order and bursts of 5+ HandlePlayerBotLoginCallback fire on the same tick →
// concurrent OnBotLogin allocations → heap corruption ~10s after master login.
uint32 PlayerbotHolder_PendingLoginCount(PlayerbotHolder* self)
{
    std::lock_guard<std::mutex> lock(g_pendingMutex);
    uint32 count = 0;
    for (auto& kv : m_pendingBotLogins)
        if (kv.second.owner == self)
            count++;
    return count;
}

void PlayerbotHolder::AddPlayerBot(uint32 guidLow, uint32 masterAccountId)
{
    if (!sPlayerbotAIConfig.enabled)
        return;

    ObjectGuid botGuid(HIGHGUID_PLAYER, guidLow);

    // 1. Resolve the bot's account from its character GUID (just to validate; not used here).
    uint32 botAccountId = sObjectMgr.GetPlayerAccountIdByGUID(botGuid);
    if (!botAccountId)
    {
        sLog.outError("[PlayerBots] AddPlayerBot: no account for guid %u", guidLow);
        return;
    }

    // 2. If the bot character is already in world, just attach AI (idempotent).
    Player* existing = sObjectMgr.GetPlayer(botGuid, false);
    if (existing && existing->IsInWorld())
    {
        SC_LOG("AddPlayerBot guid=%u — already in-world, attaching via OnBotLogin", guidLow);
        OnBotLogin(existing);
        return;
    }

    // 2b. ghost-online guard.
    // The Player object can sit in the global HashMapHolder<Player> registry
    // while not being in any Map (i.e. IsInWorld() == false). This happens
    // when a bot far-teleport starts (Player is removed from world map) but
    // the worldport ACK never fires (e.g. UpdateSessions wasn't ticking, or
    // the bot's master logged off mid-port). FindPlayer() filters on IsInWorld
    // so the check above misses this. If we then proceed to queue a fresh
    // LoginQueryHolder, HandlePlayerLogin sees the existing entry in
    // HashMapHolder and rejects with "[CRASH] Trying to login already ingame
    // character guid X" + KickPlayer(). The user reports `.bot add` doing
    // nothing in this state, with no apparent logout possible because the
    // bot isn't in playerBots either (master never re-attached after the
    // botched teleport).
    Player* ghost = HashMapHolder<Player>::Find(botGuid);
    if (ghost && !ghost->IsInWorld())
    {
        SC_LOG("AddPlayerBot guid=%u — GHOST detected (in HashMapHolder, not in world). isBeingTeleported=%d",
               guidLow, (int)ghost->IsBeingTeleported());

        // Try to force-finish a stuck teleport. If the bot was mid-far-teleport
        // and an ACK never arrived, we drive it through HandleMoveWorldportAckOpcode
        // here so the bot lands and re-enters the world. Then OnBotLogin attaches.
        if (ghost->IsBeingTeleportedFar())
        {
            SC_LOG("AddPlayerBot guid=%u — driving stuck worldport ACK", guidLow);
            ghost->GetSession()->HandleMoveWorldportAckOpcode();
        }
        else if (ghost->IsBeingTeleportedNear())
        {
            // Near-teleport ACK uses MSG_MOVE_TELEPORT_ACK; less common but cover it.
            WorldPacket p(MSG_MOVE_TELEPORT_ACK, 8 + 4 + 4);
            p << ghost->GetObjectGuid();
            p << uint32(0);
            p << uint32(time(0));
            ghost->GetSession()->HandleMoveTeleportAckOpcode(p);
        }

        // After ACK, the player should be in-world again. If so, treat it
        // exactly like the in-world path. If not, we leave it alone and warn —
        // forcibly destroying the Player from here is unsafe (could be mid-tick).
        if (ghost->IsInWorld())
        {
            SC_LOG("AddPlayerBot guid=%u — ghost recovered after ACK, attaching", guidLow);
            OnBotLogin(ghost);
            return;
        }
        else
        {
            sLog.outError("[PlayerBots] AddPlayerBot: bot %u is in HashMapHolder but not in world and not "
                          "mid-teleport — refusing to retry login (would cause [CRASH] kick). Use `.bot remove` "
                          "or restart the server to recover.", guidLow);
            return;
        }
    }

    // 3. Queue the bot's character data load via the standard pipeline.
    //    Critically, we do NOT pre-create a WorldSession here — the callback below allocates
    //    a fresh, free-floating session per bot. This avoids stomping the master's existing
    //    session when the bot is on the master's own account (alt-bot pattern, the common case).
    LoginQueryHolder* holder = new LoginQueryHolder(botAccountId, botGuid);
    if (!holder->Initialize())
    {
        sLog.outError("[PlayerBots] AddPlayerBot: holder Initialize() failed for guid %u", guidLow);
        delete holder;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(g_pendingMutex);
        m_pendingBotLogins[holder] = { botGuid, masterAccountId, this };
    }

    CharacterDatabase.DelayQueryHolder(this, &PlayerbotHolder::HandlePlayerBotLoginCallback, holder);
}

// Called when CharacterDatabase finishes the holder's queries. Allocates a fresh WorldSession
// for the bot (NOT registered in sWorld.m_sessions — free-floating, owned by the bot's AI),
// hands the holder off to WorldSession::HandlePlayerLogin (which materializes the Player into
// THIS new session, not the master's), then attaches PlayerbotAI.
//
// Why a fresh session rather than reusing master's: WoW sessions are keyed by accountId; only
// one active session per account in sWorld.m_sessions. For alt-bots on the master's account,
// reusing the master's session would stomp the master's Player. cmangos's reference solves this
// by creating a parallel free-floating WorldSession (bypassing sWorld.AddSession) — the master's
// session keeps owning the master's Player, the bot's session owns the bot's Player, both are
// active simultaneously even though they share an accountId.
// Helpers wrapping the SEH-sensitive parts of the callback. C2712 forbids __try in functions
// with C++ unwinding (PendingBotLogin info has an ObjectGuid destructor), so we split them out.

// Only the SEH-required dangerous call lives here; the WorldSession allocation creates
// std::string temporaries that trigger C2712 if wrapped in __try.
static bool SafeHandlePlayerLogin(WorldSession* botSession, LoginQueryHolder* lqh, uint32 guidLow)
{
    __try { botSession->HandlePlayerLogin(lqh); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        sLog.outError("[PlayerBots] CRASH in HandlePlayerLogin for guid %u — bot skipped, server stays up", guidLow);
        return false;
    }
}

static Player* SafeBotLoginAndEnterWorld(LoginQueryHolder* lqh, uint32 guidLow)
{
    WorldSession* botSession = new WorldSession(lqh->GetAccountId(), /*sock*/ nullptr, SEC_PLAYER,
                                                /*mute_time*/ 0, LOCALE_enUS, /*remote_ip*/ "disconnected/bot",
                                                /*binaryIp*/ 0);
    botSession->SetNoAnticheat();
    botSession->SetPlayerLoading(true);
    if (!SafeHandlePlayerLogin(botSession, lqh, guidLow))
        return nullptr;
    return botSession->GetPlayer();
}

static void SafeOnBotLogin(PlayerbotHolder* holder, Player* bot)
{
    __try
    {
        holder->OnBotLogin(bot);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        sLog.outError("[PlayerBots] CRASH in OnBotLogin for %s — bot is in-world but AI not attached",
                      bot ? bot->GetName() : "?");
    }
}

static void SafeAddPlayerBot(PlayerbotHolder* holder, uint32 guidLow, uint32 masterAccountId)
{
    __try
    {
        holder->AddPlayerBot(guidLow, masterAccountId);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        sLog.outError("[PLAYERBOTS] CRASH in AddPlayerBot(guid=%u) — skipped, server stays up", guidLow);
    }
}

void PlayerbotHolder::HandlePlayerBotLoginCallback(QueryResult* /*dummy*/, SqlQueryHolder* holder)
{
    if (!holder)
        return;

    // CRITICAL: this callback runs on a MySQL worker thread (see stack trace of
    // crash_20260528_115701.dmp.txt). The whole find/erase pair must be under the mutex,
    // otherwise concurrent AddPlayerBot from the world thread corrupts the std::map tree
    // and we crash in _Tree::_Extract at 0xFFFFFFFFFFFFFFFF.
    PendingBotLogin info;
    {
        std::lock_guard<std::mutex> lock(g_pendingMutex);
        auto it = m_pendingBotLogins.find(holder);
        if (it == m_pendingBotLogins.end())
        {
            // Not one of ours (likely a derived class's holder). Defensive — nothing to do.
            return;
        }
        info = it->second;
        m_pendingBotLogins.erase(it);
    }

    // Owner-side guard: the manager that scheduled this load got deleted (master logged off).
    // The bot's holder data was loaded for nothing; discard quietly. We MUST NOT proceed to
    // HandlePlayerLogin/OnBotLogin because they would touch `this` (virtual dispatch).
    if (info.owner == nullptr || !PlayerbotHolder_IsAlive(info.owner))
    {
        sLog.outDetail("[PlayerBots] Orphaned bot login for guid %u — master gone, skipping",
                       info.botGuid.GetCounter());
        delete holder;
        return;
    }

    LoginQueryHolder* lqh = static_cast<LoginQueryHolder*>(holder);

    // Already loaded? (race protection)
    if (sObjectMgr.GetPlayer(lqh->GetGuid(), false))
    {
        delete holder;
        return;
    }

    // Allocate the bot's dedicated WorldSession + run HandlePlayerLogin in a helper so the SEH
    // wrapper doesn't conflict with the C++ object unwinding of locals above (C2712).
    Player* bot = SafeBotLoginAndEnterWorld(lqh, info.botGuid.GetCounter());

    sLog.outString("[PlayerBots] LoginCallback: bot=%p inWorld=%d guid=%u",
                   bot, bot ? (int)bot->IsInWorld() : -1, info.botGuid.GetCounter());
    if (!bot || !bot->IsInWorld())
    {
        sLog.outError("[PlayerBots] HandlePlayerBotLoginCallback: bot %u failed to enter world",
                      info.botGuid.GetCounter());
        return;
    }

    sLog.outString("[PlayerBots] Bot %s (guid %u) entered world at map %u, calling OnBotLogin",
                   bot->GetName(), info.botGuid.GetCounter(), bot->GetMapId());
    SafeOnBotLogin(this, bot);
}

PlayerbotHolder::PlayerbotHolder() : PlayerbotAIBase()
{
    PlayerbotHolder_OnConstruct(this);
    m_holderHandlers["fill"] = &PlayerbotHolder::HandleFill;
    m_holderHandlers["revive"] = &PlayerbotHolder::HandleRevive;
    m_holderHandlers["rez"] = &PlayerbotHolder::HandleRevive;
    m_holderHandlers["list"] = &PlayerbotHolder::HandleList;
    m_holderHandlers["help"] = &PlayerbotHolder::HandleHelp;
    m_holderHandlers["reload"] = &PlayerbotHolder::HandleReload;
    m_holderHandlers["tweak"] = &PlayerbotHolder::HandleTweak;
    m_holderHandlers["self"] = &PlayerbotHolder::HandleSelf;
    m_holderHandlers["spoof"] = &PlayerbotHolder::HandleSpoof;
    m_holderHandlers["p"] = &PlayerbotHolder::HandleParty;
    m_holderHandlers["g"] = &PlayerbotHolder::HandleGuild;
    m_holderHandlers["r"] = &PlayerbotHolder::HandleRaid;
    m_holderHandlers["rl"] = &PlayerbotHolder::HandleRaidLeader;
    m_holderHandlers["create"] = &PlayerbotHolder::HandleCreate;
    m_holderHandlers["group"] = &PlayerbotHolder::HandleGroup;
#ifdef GenerateBotTests
    m_holderHandlers["runtest"] = &PlayerbotHolder::HandleRunTest;
#endif

    m_botCommandHandlers["add"] = &PlayerbotHolder::HandleBotAddLogin;
    m_botCommandHandlers["login"] = &PlayerbotHolder::HandleBotAddLogin;
    m_botCommandHandlers["remove"] = &PlayerbotHolder::HandleBotRemoveLogout;
    m_botCommandHandlers["logout"] = &PlayerbotHolder::HandleBotRemoveLogout;
    m_botCommandHandlers["rm"] = &PlayerbotHolder::HandleBotRemoveLogout;
    m_botCommandHandlers["delete"] = &PlayerbotHolder::HandleBotDelete;
    m_botCommandHandlers["gear"] = &PlayerbotHolder::HandleBotGear;
    m_botCommandHandlers["equip"] = &PlayerbotHolder::HandleBotGear;
    m_botCommandHandlers["train"] = &PlayerbotHolder::HandleBotTrainLearn;
    m_botCommandHandlers["learn"] = &PlayerbotHolder::HandleBotTrainLearn;
    m_botCommandHandlers["food"] = &PlayerbotHolder::HandleBotFoodDrink;
    m_botCommandHandlers["drink"] = &PlayerbotHolder::HandleBotFoodDrink;
    m_botCommandHandlers["potions"] = &PlayerbotHolder::HandleBotPotions;
    m_botCommandHandlers["pots"] = &PlayerbotHolder::HandleBotPotions;
    m_botCommandHandlers["consumes"] = &PlayerbotHolder::HandleBotConsumes;
    m_botCommandHandlers["consumables"] = &PlayerbotHolder::HandleBotConsumes;
    m_botCommandHandlers["consums"] = &PlayerbotHolder::HandleBotConsumes;
    m_botCommandHandlers["regs"] = &PlayerbotHolder::HandleBotReagents;
    m_botCommandHandlers["reg"] = &PlayerbotHolder::HandleBotReagents;
    m_botCommandHandlers["reagents"] = &PlayerbotHolder::HandleBotReagents;
    m_botCommandHandlers["prepare"] = &PlayerbotHolder::HandleBotPrepare;
    m_botCommandHandlers["prep"] = &PlayerbotHolder::HandleBotPrepare;
    m_botCommandHandlers["init"] = &PlayerbotHolder::HandleBotInit;
    m_botCommandHandlers["enchants"] = &PlayerbotHolder::HandleBotEnchants;
    m_botCommandHandlers["ammo"] = &PlayerbotHolder::HandleBotAmmo;
    m_botCommandHandlers["pet"] = &PlayerbotHolder::HandleBotPet;
    m_botCommandHandlers["levelup"] = &PlayerbotHolder::HandleBotLevelUp;
    m_botCommandHandlers["level"] = &PlayerbotHolder::HandleBotLevelUp;
    m_botCommandHandlers["random"] = &PlayerbotHolder::HandleBotRandom;
    m_botCommandHandlers["summon"] = &PlayerbotHolder::HandleBotSummon;
    m_botCommandHandlers["recall"] = &PlayerbotHolder::HandleBotSummon;
    m_botCommandHandlers["come"]   = &PlayerbotHolder::HandleBotSummon;
    m_botCommandHandlers["revive"] = &PlayerbotHolder::HandleBotRevive;
    m_botCommandHandlers["rez"]    = &PlayerbotHolder::HandleBotRevive;
    m_botCommandHandlers["rez all"] = &PlayerbotHolder::HandleBotRevive;
    m_botCommandHandlers["bis"]    = &PlayerbotHolder::HandleBotBis;
    // .bot fr     == .bot resist fire (kept for the addon's FireResist button + chat shortcut)
    // .bot resist <fire|frost|nature|shadow> dispatches via HandleBotResist
    m_botCommandHandlers["fr"]      = &PlayerbotHolder::HandleBotFR;
    m_botCommandHandlers["fireres"] = &PlayerbotHolder::HandleBotFR;
    m_botCommandHandlers["resist"]  = &PlayerbotHolder::HandleBotResist;
    m_botCommandHandlers["res"]     = &PlayerbotHolder::HandleBotResist;
    // Per-school aliases consumed by the addon's Apply Resist picker.
    m_botCommandHandlers["frostres"]  = &PlayerbotHolder::HandleBotFrostRes;
    m_botCommandHandlers["natres"]    = &PlayerbotHolder::HandleBotNatRes;
    m_botCommandHandlers["shadowres"] = &PlayerbotHolder::HandleBotShadowRes;
    // Pure-chat inspect — works around the vanilla client's UnitInParty restriction
    // which prevents inspecting raid members outside the current 5-man party.
    m_botCommandHandlers["inspect"] = &PlayerbotHolder::HandleBotInspectGear;
    m_botCommandHandlers["look"]    = &PlayerbotHolder::HandleBotInspectGear;

    m_botCommandHandlers["always"] = &PlayerbotHolder::HandleBotAlways;
    m_botCommandHandlers["debug"] = &PlayerbotHolder::HandleBotDebug;
    m_botCommandHandlers["c"] = &PlayerbotHolder::HandleBotC;
    m_botCommandHandlers["w"] = &PlayerbotHolder::HandleConsoleWhisper;
    m_botCommandHandlers["cmd"] = &PlayerbotHolder::HandleConsoleCmd;
    m_botCommandHandlers["test"] = &PlayerbotHolder::HandleBotTest;
    m_botCommandHandlers["do"] = &PlayerbotHolder::HandleBotDo;
    m_botCommandHandlers["record"] = &PlayerbotHolder::HandleBotRecord;
    m_botCommandHandlers["read"] = &PlayerbotHolder::HandleBotRead;
    m_botCommandHandlers["clear"] = &PlayerbotHolder::HandleBotClear;

    for (uint32 spellId = 0; spellId < sServerFacade.GetSpellInfoRows(); spellId++)
    {
        sServerFacade.LookupSpellInfo(spellId);
    }
}

PlayerbotHolder::~PlayerbotHolder()
{
    PlayerbotHolder_OnDestroy(this);
}

// SEH-safe invocation of the per-bot callback. Must be a separate function:
// MSVC rejects __try inside ForEachPlayerbot itself because the std::function
// callback and Player* parameter have destructors (C2712). With this helper
// we isolate the SEH frame, so an AV inside one bot's iteration (dangling
// Player* from a failed login / rolled-back transaction / orphaned holder
// entry) doesn't kill the entire UpdateSessions loop.
//
// Live trigger 2026-06-01 17:26:40+: SQL Deadlock on character_pet INSERT
// left the session/player in an inconsistent state. The orphaned playerBots[]
// entry pointed at freed memory, producing 12,000+ "UpdateAI crash #N at
// phase 1" log lines in a tight loop — the whole RandomPlayerbotMgr::UpdateAI
// was wedged. With this per-iter guard the bad entry's exception is
// contained; the caller continues with the next bot, and at least the
// crash-count log spam stops being a permanent state.
static bool SafeCall_BotCallback(const std::function<void(Player*)>& cb, Player* bot)
{
    __try { cb(bot); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

void PlayerbotHolder::ForEachPlayerbot(std::function<void(Player*)> callback) const
{
    // Snapshot pointers first so the callback can mutate playerBots (e.g.
    // LogoutPlayerBot) without invalidating our iterator. Belt + suspenders
    // alongside the SEH guard below.
    std::vector<std::pair<uint32, Player*>> snapshot;
    snapshot.reserve(playerBots.size());
    for (auto& itr : playerBots)
    {
        if (itr.second)
            snapshot.emplace_back(itr.first, itr.second);
    }

    for (auto& kv : snapshot)
    {
        Player* bot = kv.second;
        if (!bot) continue;
        if (!SafeCall_BotCallback(callback, bot))
        {
            // Defensive: clear the orphan entry so we don't AV again next
            // tick on the same dangling pointer. Cast away const because
            // this is recovery code; the map mutation is intentional and
            // confined to a known-bad entry.
            const_cast<PlayerBotMap&>(playerBots)[kv.first] = nullptr;
            sLog.outError("[PLAYERBOTS] ForEachPlayerbot AV on bot guid=%u — cleared orphan playerBots entry", kv.first);
        }
    }
}

void PlayerbotHolder::MovePlayerBot(uint32 guid, PlayerbotHolder* newHolder)
{
    if (newHolder)
    {
        auto it = playerBots.find(guid); 
        if (it != playerBots.end() && it->second != nullptr)
        {
            newHolder->OnBotLogin(it->second);
            playerBots[guid] = nullptr;
        }
    }
}

void PlayerbotHolder::UpdateAIInternal(uint32 elapsed, bool minimal)
{
#ifdef GenerateBotTests
    UpdatePendingTests(elapsed);
#endif
}

void PlayerbotHolder::UpdateSessions(uint32 elapsed)
{
    ForEachPlayerbot([&](Player* bot)
    {
        // Per-iteration diagnostic snapshot. We only emit it for "interesting"
        // states (mid-teleport, ghost, logout-pending) to keep log volume sane —
        // a healthy in-world bot looks identical every tick. If a bot is stuck
        // mid-teleport, this line will fire every master tick and we'll be able
        // to see exactly how long it's been stuck.
        const bool isMidTeleport = bot->IsBeingTeleported();
        const bool isInWorld     = bot->IsInWorld();
        const bool isLoading     = bot->GetSession() && bot->GetSession()->PlayerLoading();
        // True ghost: not in world, not mid-teleport, not mid-login. The login
        // path can briefly land in (!IsInWorld && !IsBeingTeleported) before
        // SetMap finishes; we must not yank such a bot to homebind.
        const bool isGhost       = !isInWorld && !isMidTeleport && !isLoading;
        const bool wantsLogout   = bot->GetPlayerbotAI() && bot->GetPlayerbotAI()->GetShouldLogOut();
        if (isMidTeleport || isGhost || wantsLogout)
        {
            SC_LOG("UpdateSessions iter bot=%s guid=%u midTeleport=%d inWorld=%d "
                   "ghost=%d wantsLogout=%d isBeingTeleportedFar=%d isBeingTeleportedNear=%d",
                   bot->GetName(), bot->GetGUIDLow(),
                   (int)isMidTeleport, (int)isInWorld, (int)isGhost, (int)wantsLogout,
                   (int)bot->IsBeingTeleportedFar(),
                   (int)bot->IsBeingTeleportedNear());
        }

        if (bot->GetPlayerbotAI() && bot->IsBeingTeleported())
        {
            bot->GetPlayerbotAI()->HandleTeleportAck();
        }
        else if (bot->IsInWorld())
        {
            bot->GetSession()->HandleBotPackets();
        }
        else
        {
            // Underlying root-cause fix candidate (ghost branch). Bot has no
            // teleport flag but is also not in world. This is the limbo state
            // that produces "Trying to login already ingame" on the next
            // .bot add. Most likely cause: HandleMoveWorldportAckOpcode's
            // map->Add returned false and HandleReturnOnTeleportFail's
            // chained TeleportTo also failed — leaving the Player removed
            // from old map but never added to any new map. SemaphoreTeleportFar
            // was reset by HandleReturnOnTeleportFail so we can no longer
            // re-drive the ACK.
            //
            // Recovery: kick the bot to homebind synchronously so it lands
            // *somewhere* in-world. Better than ghost-state forever.
            if (bot->GetPlayerbotAI())
            {
                SC_LOG("UpdateSessions GHOST RECOVERY bot=%s guid=%u — driving "
                       "TeleportToHomebind to break out of limbo",
                       bot->GetName(), bot->GetGUIDLow());
                bot->TeleportToHomebind();
            }
        }

        if (bot->GetPlayerbotAI() && bot->GetPlayerbotAI()->GetShouldLogOut() && !bot->IsStunnedByLogout() && !bot->GetSession()->isLogingOut())
        {
            LogoutPlayerBot(bot->GetObjectGuid().GetRawValue());
        }
    });

    Cleanup();
}

void PlayerbotHolder::Cleanup()
{
    auto it = playerBots.begin();
    while (it != playerBots.end())
    {
        if (it->second == nullptr)
        {
            it = playerBots.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void PlayerbotHolder::LogoutAllBots()
{
    int total = 0, kept = 0, skippedRealPlayer = 0, skippedNoAI = 0, loggedOut = 0;
    ForEachPlayerbot([&](Player* bot)
    {
        ++total;
        const bool hasAI    = bot->GetPlayerbotAI() != nullptr;
        const bool realPlay = hasAI && bot->GetPlayerbotAI()->IsRealPlayer();
        if (!hasAI)        { ++skippedNoAI; return; }
        if (realPlay)      { ++skippedRealPlayer; return; }
        SC_LOG("LogoutAllBots: logging out bot=%s guid=%u (remote=%s)",
               bot->GetName(), bot->GetGUIDLow(),
               bot->GetSession() ? bot->GetSession()->GetRemoteAddress().c_str() : "(no-session)");
        LogoutPlayerBot(bot->GetGUIDLow());
        ++loggedOut;
    });
    SC_LOG("LogoutAllBots summary: total=%d loggedOut=%d skippedRealPlayer=%d skippedNoAI=%d",
           total, loggedOut, skippedRealPlayer, skippedNoAI);

    Cleanup();
}

void PlayerbotMgr::CancelLogout()
{
    Player* master = GetMaster();
    if (!master)
        return;

    ForEachPlayerbot([&](Player* bot)
    {
        PlayerbotAI* ai = bot->GetPlayerbotAI();
        if (ai && !ai->IsRealPlayer())
        {
            if (bot->IsStunnedByLogout() || bot->GetSession()->isLogingOut())
            {
                WorldPacket p;
                bot->GetSession()->HandleLogoutCancelOpcode(p);
                ai->TellPlayer(GetMaster(), BOT_TEXT("logout_cancel"));
            }
        }
    });

    sRandomPlayerbotMgr.ForEachPlayerbot([&](Player* bot)
    {
        PlayerbotAI* ai = bot->GetPlayerbotAI();
        if (ai && !ai->IsRealPlayer() && ai->GetMaster() == master)
        {
            if (bot->IsStunnedByLogout() || bot->GetSession()->isLogingOut())
            {
                WorldPacket p;
                bot->GetSession()->HandleLogoutCancelOpcode(p);
            }
        }
    });
}

void PlayerbotHolder::LogoutPlayerBot(uint32 guid, bool allowInstant, bool forDelete)
{
    SC_LOG("LogoutPlayerBot entry guid=%u allowInstant=%d forDelete=%d",
           guid, (int)allowInstant, (int)forDelete);
    Player* bot = GetPlayerBot(guid);
    if (bot)
    {
        SC_LOG("LogoutPlayerBot bot=%s found in playerBots map", bot->GetName());
        PlayerbotAI* ai = bot->GetPlayerbotAI();
        if (!ai)
        {
            SC_LOG("LogoutPlayerBot bot=%s has no AI — early return", bot->GetName());
            return;
        }

        // BotActionLog: write LIFECYCLE LOGOUT and close the per-bot log
        // file. Done early in the logout sequence so the file flushes
        // before any potentially-crashing teardown work runs.
        ai::botdiag::BotActionLog::Write(ai, "LIFECYCLE",
            "event=LOGOUT bot=%s guid=%u allowInstant=%d forDelete=%d",
            bot->GetName(), guid, (int)allowInstant, (int)forDelete);
        ai::botdiag::BotActionLog::LogState(ai, "pre-logout");
        ai::botdiag::BotActionLog::Close(ai);

        if (!sPlayerbotAIConfig.bExplicitDbStoreSave)
        {
           Group* group = bot->GetGroup();
           if (group && !bot->InBattleGround() && !bot->InBattleGroundQueue() && ai->HasActivePlayerMaster())
           {
              sPlayerbotDbStore.Save(ai);
           }
        }
        SC_LOG("LogoutPlayerBot bot=%s — about to SaveToDB", bot->GetName());
        sLog.outDebug("Bot %s logging out", bot->GetName());
        if (!forDelete)
            bot->SaveToDB();
        SC_LOG("LogoutPlayerBot bot=%s — SaveToDB done", bot->GetName());

        WorldSession* botWorldSessionPtr = bot->GetSession();
        WorldSession* masterWorldSessionPtr = nullptr;

        Player* master = ai->GetMaster();
        if (master)
            masterWorldSessionPtr = master->GetSession();

        // check for instant logout
        bool logout = botWorldSessionPtr->ShouldLogOut(time(nullptr));

        // if no instant logout, request normal logout
        if (!allowInstant)
        {
            if (bot && (bot->IsStunnedByLogout() || bot->GetSession()->isLogingOut()))
            {
                return;
            }
            else if (bot)
            {
                SC_LOG("LogoutPlayerBot bot=%s — queueing CMSG_LOGOUT_REQUEST", bot->GetName());
                ai->TellPlayer(ai->GetMaster(), BOT_TEXT("logout_start"));

                WorldPacket p(CMSG_LOGOUT_REQUEST);
                std::unique_ptr<WorldPacket> packet(new WorldPacket(p));
                botWorldSessionPtr->QueuePacket(std::move(packet));
                SC_LOG("LogoutPlayerBot bot=%s — CMSG_LOGOUT_REQUEST queued, returning", bot->GetName());

                //WorldPacket p;
                //botWorldSessionPtr->HandleLogoutRequestOpcode(p);
                if (!bot)
                {
                    playerBots[guid] = nullptr;
                    delete botWorldSessionPtr;    
                }
                
                return;
            }
            else
            {
                playerBots[guid] = nullptr;  // deletes bot player ptr inside this WorldSession PlayerBotMap
                delete botWorldSessionPtr;  // finally delete the bot's WorldSession
            }
            
            return;
        } 
        // if instant logout possible, do it
        else if (bot && (logout || !botWorldSessionPtr->isLogingOut()))
        {
            ai->TellPlayer(ai->GetMaster(), BOT_TEXT("goodbye"));
            playerBots[guid] = nullptr;    // deletes bot player ptr inside this WorldSession PlayerBotMap
            botWorldSessionPtr->LogoutPlayer(); // this will delete the bot Player object and PlayerbotAI object
            //botWorldSessionPtr->LogoutPlayer(true); // this will delete the bot Player object and PlayerbotAI object
            if(!sWorld.FindSession(botWorldSessionPtr->GetAccountId())) //Real player sessions will get removed later.
                delete botWorldSessionPtr;  // finally delete the bot's WorldSession
        }
    }
}

void PlayerbotHolder::DisablePlayerBot(uint32 guid, bool logOutPlayer)
{
    Player* bot = GetPlayerBot(guid);
    if (bot)
    {
        if (logOutPlayer && bot->GetPlayerbotAI()->IsRealPlayer() && bot->GetGroup() && sPlayerbotAIConfig.IsFreeAltBot(guid))
            bot->GetSession()->SetOffline(); //Prevent groupkick
        bot->GetPlayerbotAI()->TellPlayer(bot->GetPlayerbotAI()->GetMaster(), BOT_TEXT("goodbye"));
        bot->GetPlayerbotAI()->StopMoving();
        MotionMaster& mm = *bot->GetMotionMaster();
        mm.Clear();

        if (!sPlayerbotAIConfig.bExplicitDbStoreSave)
        {
           Group* group = bot->GetGroup();
           if (group && !bot->InBattleGround() && !bot->InBattleGroundQueue() && bot->GetPlayerbotAI()->HasActivePlayerMaster())
           {
              sPlayerbotDbStore.Save(bot->GetPlayerbotAI());
           }
        }

        sLog.outDebug("Bot %s logged out", bot->GetName());
        bot->SaveToDB();

        WorldSession* botWorldSessionPtr = bot->GetSession();
        playerBots[guid] = nullptr;    // deletes bot player ptr inside this WorldSession PlayerBotMap

        if (bot->GetPlayerbotAI()) 
        {
            bot->RemovePlayerbotAI();
        }
    }
}

Player* PlayerbotHolder::GetPlayerBot(uint32 playerGuid) const
{
    PlayerBotMap::const_iterator it = playerBots.find(playerGuid);
    return (it == playerBots.end()) ? nullptr : it->second ? it->second : nullptr;
}

void PlayerbotHolder::JoinChatChannels(Player* bot)
{
    // bots join World chat if not solo oriented
    if (bot->GetLevel() >= 10 && sRandomPlayerbotMgr.IsFreeBot(bot) && bot->GetPlayerbotAI() && bot->GetPlayerbotAI()->GetGrouperType() != GrouperType::SOLO)
    {
        // TODO make action/config
        // Make the bot join the world channel for chat
        WorldPacket pkt(CMSG_JOIN_CHANNEL);
#ifndef MANGOSBOT_ZERO
        pkt << uint32(0) << uint8(0) << uint8(0);
#endif
        pkt << std::string("World");
        pkt << ""; // Pass
        bot->GetSession()->HandleJoinChannelOpcode(pkt);
    }
    // join standard channels
    uint8 locale = BroadcastHelper::GetLocale();

    AreaTableEntry const* current_zone = bot->GetPlayerbotAI()->GetCurrentZone();
    ChannelMgr* cMgr = channelMgr(bot->GetTeam());
    std::string current_zone_name = current_zone ? bot->GetPlayerbotAI()->GetLocalizedAreaName(current_zone) : "";

    if (current_zone && cMgr)
    {
        for (uint32 i = 0; i < sChatChannelsStore.GetNumRows(); ++i)
        {
            ChatChannelsEntry const* channel = sChatChannelsStore.LookupEntry(i);
            if (!channel) continue;

            Channel* new_channel = nullptr;
            switch (channel->ChannelID)
            {
                case ChatChannelId::GENERAL:
                case ChatChannelId::LOCAL_DEFENSE:
                {
                    char new_channel_name_buf[100];
                    snprintf(new_channel_name_buf, 100, channel->pattern[locale], current_zone_name.c_str());
#ifdef MANGOSBOT_ZERO
                    new_channel = cMgr->GetJoinChannel(new_channel_name_buf);
#else
                    new_channel = cMgr->GetJoinChannel(new_channel_name_buf, channel->ChannelID);
#endif
                    break;
                }
                case ChatChannelId::TRADE:
                case ChatChannelId::GUILD_RECRUITMENT:
                {
                    char new_channel_name_buf[100];
                    //3459 is ID for a zone named "City" (only exists for the sake of using its name)
                    //Currently in magons TBC, if you switch zones, then you join "Trade - <zone>" and "GuildRecruitment - <zone>"
                    //which is a core bug, should be "Trade - City" and "GuildRecruitment - City" in both 1.12 and TBC
                    //but if you (actual player) logout in a city and log back in - you join "City" versions
                    snprintf(
                        new_channel_name_buf,
                        100,
                        channel->pattern[locale],
                        bot->GetPlayerbotAI()->GetLocalizedAreaName(GetAreaEntryByAreaID(ImportantAreaId::CITY)).c_str()
                    );

#ifdef MANGOSBOT_ZERO
                    new_channel = cMgr->GetJoinChannel(new_channel_name_buf);
#else
                    new_channel = cMgr->GetJoinChannel(new_channel_name_buf, channel->ChannelID);
#endif
                    break;
                }
                case ChatChannelId::LOOKING_FOR_GROUP:
                case ChatChannelId::WORLD_DEFENSE:
                {
#ifdef MANGOSBOT_ZERO
                    new_channel = cMgr->GetJoinChannel(channel->pattern[locale]);
#else
                    new_channel = cMgr->GetJoinChannel(channel->pattern[locale], channel->ChannelID);
#endif
                    break;
                }
                default:
                    break;
            }
            if (new_channel)
                new_channel->Join(bot, "");
        }
    }
}

void PlayerbotHolder::OnBotLogin(Player * const bot)
{
    if (!sPlayerbotAIConfig.enabled)
        return;

    // PHASE TRACKING — call sites preserved below in case we need them again for a future
    // OnBotLogin investigation. Disabled by default now that the original cascade login,
    // re-entrancy, and double-OnBotLogin bugs are fixed (see playerbots-fixes-applied
    // memory entries #1-4). To re-enable: change `do { } while(0)` to `sLog.outString(...)`.
    [[maybe_unused]] const char* botName = bot ? bot->GetName() : "?";
    [[maybe_unused]] uint32 botGuidLow = bot ? bot->GetGUIDLow() : 0;
    #define OBL_PHASE(msg) do { } while(0)

    OBL_PHASE("entry");

    // GUARD: bot already fully initialized (any holder). Calling OnBotLogin twice for the
    // same bot was observed at 11:34:01 then 11:34:02 for Merlion → corruption of internal
    // state (double AddMember, double ResetStrategies, double LearnSkill etc.) → 0xc0000374.
    //
    // The double-call can happen ACROSS HOLDERS — when both Polz's PlayerbotMgr and
    // RandomPlayerbotMgr try to claim the same bot simultaneously, each calls OnBotLogin on
    // their own playerBots map (so a per-holder map.find guard misses the duplicate).
    // We use the GLOBAL g_readyAIs set (set by Engine_MarkAIReady at the end of OnBotLogin)
    // as the universal "this AI has already been fully initialized once" signal.
    extern bool Engine_IsAIReady(PlayerbotAI* ai);
    if (bot && bot->GetPlayerbotAI() && Engine_IsAIReady(bot->GetPlayerbotAI()))
    {
        OBL_PHASE("ALREADY ATTACHED (global) — early exit");
        // Still register the bot in THIS holder's playerBots map so the holder's commands
        // and lifecycle can address it; just skip the heavy re-init.
        playerBots[bot->GetGUIDLow()] = bot;
        return;
    }

    PlayerbotAI* ai = bot->GetPlayerbotAI();
    if (!ai)
    {
        OBL_PHASE("creating PlayerbotAI");
        bot->CreatePlayerbotAI();
        ai = bot->GetPlayerbotAI();
    }

    OBL_PHASE("HasRealPlayerMaster check");
    if(!ai->HasRealPlayerMaster())
    {
        OBL_PHASE("OnBotLoginInternal");
	    OnBotLoginInternal(bot);
    }

    OBL_PHASE("insert into playerBots map");
    playerBots[bot->GetGUIDLow()] = bot;

    OBL_PHASE("GetMaster");
    Player* master = ai->GetMaster();
    if (!master && sPlayerbotAIConfig.IsFreeAltBot(bot))
    {
        OBL_PHASE("SetMaster to self (FreeAltBot)");
        ai->SetMaster(bot);
        master = bot;
    }

    if (master && master != bot)
    {
        OBL_PHASE("master path: GetObjectGuid");
        ObjectGuid masterGuid = master->GetObjectGuid();

        // A random bot loaded from disk restores its saved bot-only group (most
        // bots are stuck in one from the grouper behaviour). The add-to-master
        // checks below all require !bot->GetGroup(), so without this the bot
        // stays in its bot group and never joins the master — ".bot fill" then
        // only ever adds the single bot that happened to be groupless. Kick it
        // out of any group that is not the master's first.
        if (bot->GetGroup() && bot->GetGroup() != master->GetGroup())
        {
            OBL_PHASE("RemoveFromGroup (leave saved bot group)");
            bot->RemoveFromGroup();
        }

        if (master->GetGroup() && !master->GetGroup()->IsLeader(masterGuid) && !sPlayerbotAIConfig.IsFreeAltBot(bot))
        {
            OBL_PHASE("ChangeLeader");
            master->GetGroup()->ChangeLeader(masterGuid);
        }

        if (!bot->GetGroup() && master->GetGroup())
        {
            OBL_PHASE("AddMember to existing master group");
            Group* masterGroup = master->GetGroup();
            if (masterGroup->GetMembersCount() < (masterGroup->IsRaidGroup() ? 40 : 5))
            {
                OBL_PHASE("AddMember (existing raid/party)");
                masterGroup->AddMember(bot->GetObjectGuid(), bot->GetName());
                OBL_PHASE("AddMember done");
                sLog.outString("[PLAYERBOTS] Auto-invited %s to master's group", bot->GetName());
            }
            else if (!masterGroup->IsRaidGroup() && masterGroup->GetMembersCount() >= 5)
            {
                OBL_PHASE("ConvertToRaid");
                masterGroup->ConvertToRaid();
                OBL_PHASE("AddMember after convert");
                masterGroup->AddMember(bot->GetObjectGuid(), bot->GetName());
                OBL_PHASE("AddMember after convert done");
                sLog.outString("[PLAYERBOTS] Converted to raid, auto-invited %s", bot->GetName());
            }
        }
        else if (!bot->GetGroup() && !master->GetGroup())
        {
            OBL_PHASE("new Group()");
            Group* newGroup = new Group();
            if (newGroup->Create(master->GetObjectGuid(), master->GetName()))
            {
                OBL_PHASE("AddGroup");
                sObjectMgr.AddGroup(newGroup);
                OBL_PHASE("first AddMember (new group)");
                newGroup->AddMember(bot->GetObjectGuid(), bot->GetName());
                OBL_PHASE("first AddMember done");
                sLog.outString("[PLAYERBOTS] Created group and invited %s", bot->GetName());
            }
        }
    }

    Group* group = bot->GetGroup();
    if (group)
    {
        bool groupValid = false;
        Group::MemberSlotList const& slots = group->GetMemberSlots();
        for (Group::MemberSlotList::const_iterator i = slots.begin(); i != slots.end(); ++i)
        {
            ObjectGuid member = i->guid;
            if (master)
            {
                if (master->GetObjectGuid() == member)
                {
                    groupValid = true;
                    break;
                }
            }

            // Don't disband alt groups when master goes away
            // (will need to manually disband with leave command)
            uint32 account = sObjectMgr.GetPlayerAccountIdByGUID(member);
            if (!sPlayerbotAIConfig.IsInRandomAccountList(account))
            {
                groupValid = true;
                break;
            }
        }

        if (!groupValid)
        {
            WorldPacket p;
            std::string member = bot->GetName();
            p << uint32(PARTY_OP_LEAVE) << member << uint32(0);
            bot->GetSession()->HandleGroupDisbandOpcode(p);
        }
    }

    ai->ResetStrategies();

    if (master && !master->IsTaxiFlying())
    {
        bot->GetMotionMaster()->MovementExpired();
    }

    // check activity
    ai->AllowActivity(ALL_ACTIVITY, true);
    // set delay on login
    ai->SetActionDuration(urand(2000, 4000));

    ai->TellPlayer(ai->GetMaster(), BOT_TEXT("hello"));

    JoinChatChannels(bot);

    if (sRandomPlayerbotMgr.IsRandomBot(bot))
    {
        uint32 lowguid = bot->GetObjectGuid().GetCounter();
        auto result = CharacterDatabase.PQuery("SELECT 1 FROM character_social WHERE flags='%u' and friend='%d'", SOCIAL_FLAG_FRIEND, lowguid);
        if (result)
            bot->GetPlayerbotAI()->SetPlayerFriend(true);
        else
            bot->GetPlayerbotAI()->SetPlayerFriend(false);

        if (sPlayerbotAIConfig.instantRandomize && !sPlayerbotAIConfig.disableRandomLevels && !bot->GetTotalPlayedTime())
        {
            sRandomPlayerbotMgr.InstaRandomize(bot);
        }
    }

    if (!bot->HasItemCount(6948, 1)
#ifdef MANGOSBOT_TWO
        && !bot->HasItemCount(40582, 1)
#endif
        )
    {
#ifdef MANGOSBOT_TWO
        if (bot->getClass() == CLASS_DEATH_KNIGHT && bot->GetMapId() == 609)
            bot->StoreNewItemInBestSlots(40582, 1);
        else
#endif
            bot->StoreNewItemInBestSlots(6948, 1);
    }

    // OnBotLogin completed successfully. Mark the AI as ready so Engine::DoNextAction will
    // start ticking it. Until now any AI tick (triggered by re-entrancy during the
    // initialization above) was silently skipped to prevent the half-initialized-state AVs
    // that were producing 0xc0000374 heap corruption.
    extern void Engine_MarkAIReady(PlayerbotAI* ai);
    Engine_MarkAIReady(ai);
    OBL_PHASE("ready (Engine_MarkAIReady)");
    #undef OBL_PHASE
}

std::string PlayerbotHolder::ProcessBotCommand(std::string cmd, ObjectGuid guid, ObjectGuid masterguid, bool admin, uint32 masterAccountId, uint32 masterGuildId, const std::string param)
{
    Player* bot = sObjectMgr.GetPlayer(guid);
    Player* master = masterguid ? sObjectMgr.GetPlayer(masterguid) : nullptr;

    if (!sPlayerbotAIConfig.enabled || guid.IsEmpty())
        return "Bot system is disabled";

    uint32 botAccount = sObjectMgr.GetPlayerAccountIdByGUID(guid);
    bool isRandomAccount = sPlayerbotAIConfig.IsInRandomAccountList(botAccount);
    bool isMasterAccount = (masterAccountId == botAccount);

    std::string subType;
    size_t eqPos = cmd.find('=');
    if (eqPos != std::string::npos)
    {
        subType = cmd.substr(eqPos + 1);
        cmd = cmd.substr(0, eqPos);
    }

    auto it = m_botCommandHandlers.find(cmd);
    if (it != m_botCommandHandlers.end())
    {
        std::string realParam;

        if (!subType.empty())
            realParam = subType;
        else if (it->second == &PlayerbotHolder::HandleBotAddLogin || it->second == &PlayerbotHolder::HandleBotAlways || it->second == &PlayerbotHolder::HandleBotDelete)
            realParam = std::to_string(guid.GetRawValue());
        else
            realParam = param;

        // Bot offline guard: nearly every HandleBot* function dereferences `bot` (gear, init,
        // refresh, summon, etc.). The dispatcher resolves a character GUID then calls
        // GetPlayer(guid) — returns null when the character exists in DB but isn't in-world
        // (typical for `.bot gear *` over the saved-group list where some bots failed to
        // reconnect). The only handlers safe to call with bot=null are the login/add/delete
        // paths because they operate on guids, not Player*.
        bool botRequired = !(it->second == &PlayerbotHolder::HandleBotAddLogin ||
                             it->second == &PlayerbotHolder::HandleBotDelete);
        if (botRequired && !bot)
            return "Bot is offline.";

        return (this->*it->second)(bot, master, realParam);
    }

    return "unknown command";
}

bool PlayerbotMgr::HandlePlayerbotMgrCommand(ChatHandler* handler, char const* args)
{
	if (!sPlayerbotAIConfig.enabled)
	{
		handler->PSendSysMessage("|cffff0000Playerbot system is currently disabled!");
        return false;
	}

    WorldSession *m_session = handler->GetSession();

    if (!m_session)
    {
        handler->PSendSysMessage("You may only add bots from an active session");
        return false;
    }

    Player* player = m_session->GetPlayer();
    if (!player)
    {
        handler->PSendSysMessage("No player found");
        return false;
    }

    PlayerbotMgr* mgr = player->GetPlayerbotMgr();
    if (!mgr)
    {
        handler->PSendSysMessage("you cannot control bots yet");
        return false;
    }

    sLog.outString("[BOT_CMD] .bot command from %s, args='%s', enabled=%u", player->GetName(), args ? args : "(null)", (uint32)sPlayerbotAIConfig.enabled);

    std::list<std::string> messages = mgr->HandlePlayerbotCommand(args, player);
    if (messages.empty())
        return true;

    for (std::list<std::string>::iterator i = messages.begin(); i != messages.end(); ++i)
    {
        handler->PSendSysMessage("%s",i->c_str());
    }

    return true;
}

std::list<std::string> PlayerbotHolder::HandlePlayerbotCommand(const std::string args, Player* master, AccountTypes security)
{
    AccountTypes useSecurity = master ? master->GetSession()->GetSecurity() : security;

    if (!master && m_spoofGuid)
        master = sObjectMgr.GetPlayer(m_spoofGuid);

    std::vector<std::string> params = Qualified::getMultiQualifiers(args, " ");

    std::list<std::string> messages;

    if (params.empty())
    {
        std::string helpText = GetCommandTexts("");
        messages.push_back(helpText);
        return messages;
    }

    std::string command = params[0];
    std::string param, charname;

    if (params.size() > 1)
    {
        param = args.substr(params[0].size() + 1);
        charname = params[1];
    }
    
    for (auto& [prefix, handler] : m_holderHandlers)
    {
        if (command != prefix)
            continue;

        messages = (this->*handler)(master, param, useSecurity);
        return messages;
    }

    std::set<std::string> bots;

    if (charname.empty())
    {
        if (master && master->GetTarget() && master->GetTarget()->IsPlayer() && !((Player*)master->GetTarget())->isRealPlayer())
        {
            bots.insert(master->GetTarget()->GetName());
        }
        else
        {
            std::string helpText = GetCommandTexts("");
            messages.push_back(helpText);
            return messages;
        }
    }    

    if (charname == "*" && master)
    {
        Group* group = master->GetGroup();
        if (!group)
        {
            messages.push_back("you must be in group");
            return messages;
        }

        Group::MemberSlotList slots = group->GetMemberSlots();
        for (Group::member_citerator i = slots.begin(); i != slots.end(); i++)
        {
            ObjectGuid member = i->guid;

            if (member.GetRawValue() == master->GetObjectGuid().GetRawValue())
                continue;

            std::string bot;
            if (sObjectMgr.GetPlayerNameByGUID(member, bot))
                bots.insert(bot);
        }
    }

    if (charname == "guild" && master)
    {
        if (!master->GetGuildId())
        {
            messages.push_back("you must be in a guild");
            return messages;
        }

        auto result = CharacterDatabase.PQuery("SELECT m.guid, (select name from characters c where c.guid = m.guid) FROM guild_member m WHERE guildid = '%u'", master->GetGuildId());

        if (!result)
        {
            messages.push_back("No guild members");
            return messages;
        }

        do
        {
            Field* fields = result->Fetch();
            uint32 guid = fields[0].GetUInt32();
            std::string bot = fields[1].GetString();

            if (guid == master->GetGUIDLow())
                continue;

            bots.insert(bot);
        } while (result->NextRow());
    }

    if (charname == "!" && useSecurity > SEC_GAMEMASTER)
    {
        for (auto& itr : playerBots)
        {
            Player* bot = itr.second;
            if (bot && (bot->IsInWorld() || param.find("add") == 0 || param.find("login") == 0 || param.find("delete") == 0))
                bots.insert(bot->GetName());
        }
    }

    if (bots.empty())
    {
        std::vector<std::string> chars = split(charname, ',');
        for (auto name : chars)
        {
            uint32 accountId = GetAccountId(name);
            if (!accountId)
            {
                bots.insert(name);
                continue;
            }

            auto results = CharacterDatabase.PQuery(
                "SELECT name FROM characters WHERE account = '%u'",
                accountId);
            if (results)
            {
                do
                {
                    Field* fields = results->Fetch();
                    std::string charName = fields[0].GetString();
                    bots.insert(charName);
                } while (results->NextRow());
            }
        }
    }

    if (bots.size())
    {
        if (params.size() > 2)
            param = args.substr(params[0].size() + params[1].size() + 2);
        else
            param = "";
    }

    for (auto bot :  bots)
    {
        std::ostringstream out;
        out << command << ": " << bot << " - ";

        ObjectGuid member = sObjectMgr.GetPlayerGuidByName(bot);
        if (!member)
        {
            out << "character not found";
        }
        else if (master)
        {
            out << ProcessBotCommand(command, member, master->GetObjectGuid(), useSecurity >= SEC_GAMEMASTER, master->GetSession()->GetAccountId(), master->GetGuildId(), param);
        }
        else
        {
            out << ProcessBotCommand(command, member, ObjectGuid(), useSecurity >= SEC_GAMEMASTER, -1, -1, param);
        }

        messages.push_back(out.str());
    }

    if (messages.empty())
        messages.push_back("Unknown command. Use 'help' for more information.");

    return messages;
}

uint32 PlayerbotHolder::GetAccountId(std::string name)
{
    uint32 accountId = 0;

    auto results = LoginDatabase.PQuery("SELECT id FROM account WHERE username = '%s'", name.c_str());
    if(results)
    {
        Field* fields = results->Fetch();
        accountId = fields[0].GetUInt32();
    }

    return accountId;
}

std::string PlayerbotHolder::ListBots(Player* master, const std::string param)
{
    std::set<std::string> bots;
    std::map<uint8, std::string> classNames;
    classNames[CLASS_DRUID] = "Druid";
    classNames[CLASS_HUNTER] = "Hunter";
    classNames[CLASS_MAGE] = "Mage";
    classNames[CLASS_PALADIN] = "Paladin";
    classNames[CLASS_PRIEST] = "Priest";
    classNames[CLASS_ROGUE] = "Rogue";
    classNames[CLASS_SHAMAN] = "Shaman";
    classNames[CLASS_WARLOCK] = "Warlock";
    classNames[CLASS_WARRIOR] = "Warrior";
#ifdef MANGOSBOT_TWO
    classNames[CLASS_DEATH_KNIGHT] = "DeathKnight";
#endif

    std::map<std::string, std::string> online;
    std::list<std::string> names;
    std::map<std::string, std::string> classes;

    for (auto& itr : playerBots)
    {
        Player* bot = itr.second;

        if (!bot)
            continue;

        std::string name = bot->GetName();

        if (!param.empty() && name.find(param) != 0)
            continue;

        bots.insert(name);
        names.push_back(name);
        online[name] = "+";
        classes[name] = classNames[bot->getClass()];
    }

    if (master)
    {
        auto results = CharacterDatabase.PQuery("SELECT class,name FROM characters where account = '%u'",
            master->GetSession()->GetAccountId());
        if (results != NULL)
        {
            do
            {
                Field* fields = results->Fetch();
                uint8 cls = fields[0].GetUInt8();
                std::string name = fields[1].GetString();

                if (!param.empty() && name.find(param) != 0)
                    continue;

                if (bots.find(name) == bots.end() && name != master->GetSession()->GetPlayerName())
                {
                    names.push_back(name);
                    online[name] = "-";
                    classes[name] = classNames[cls];
                }
            } while (results->NextRow());
        }
    }

    names.sort();

    if (master)
    {
        Group* group = master->GetGroup();
        if (group)
        {
            Group::MemberSlotList const& groupSlot = group->GetMemberSlots();
            for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
            {
                Player* member = sObjectMgr.GetPlayer(itr->guid);

                if (member && sRandomPlayerbotMgr.IsFreeBot(member))
                {
                    std::string name = member->GetName();

                    if (!param.empty() && name.find(param) != 0)
                        continue;

                    names.push_back(name);
                    online[name] = "+";
                    classes[name] = classNames[member->getClass()];
                }
            }
        }
    }

    std::ostringstream out;
    bool first = true;
    out << "Bot roster: ";
    for (std::list<std::string>::iterator i = names.begin(); i != names.end(); ++i)
    {
        if (first)
            first = false;
        else
            out << ", ";
        std::string name = *i;
        out << online[name] << name << " " << classes[name];
    }

    return out.str();
}


uint32 PlayerbotHolder::GetPlayerbotsAmount() const
{
    uint32 amount = 0;
    for (const auto& pair : playerBots)
    {
        if (pair.second)
        {
            amount++;
        }
    }

    return amount;
}

PlayerbotMgr::PlayerbotMgr(Player* const master) : PlayerbotHolder(),  master(master), lastErrorTell(0)
{
}

PlayerbotMgr::~PlayerbotMgr()
{
}

void PlayerbotMgr::UpdateAIInternal(uint32 elapsed, bool minimal)
{
    SetAIInternalUpdateDelay(sPlayerbotAIConfig.reactDelay);
    CheckTellErrors(elapsed);

    // Process queued bot re-adds with STRICT SERIALIZATION to keep the world thread responsive
    // AND prevent heap corruption from concurrent OnBotLogin calls.
    //
    // Safety gates (all must hold before we add the next bot):
    //   * master is alive, in-world, has a valid session, and not mid-teleport
    //   * grace period elapsed since master login (10s) so the client finishes its initial
    //     CMSG_PLAYER_LOGIN/world enter handshake before we spawn helpers on top of it
    //   * NO pending DB callbacks for our holder. AddPlayerBot is async (queues a DB query),
    //     the callback fires when DB returns. Without this gate, fast queries pile up and
    //     5+ HandlePlayerBotLoginCallback fire on the same tick → concurrent OnBotLogin →
    //     heap corruption ~10s after master login (observed crash_20260528_103535).
    //   * inter-bot delay (3s) so the bot's HandlePlayerLogin + OnBotLogin fully complete
    //     before we even consider starting the next one
    //   * AddPlayerBot wrapped in SEH so a corrupt bot character can't take down the server
    //   * hard cap on queue length so an unbounded group_member result can't OOM us
    if (!m_pendingGroupBotGuids.empty() && master && master->IsInWorld() &&
        master->GetSession() && !master->IsBeingTeleported())
    {
        time_t now = time(nullptr);

        if (m_nextBotAddTime == 0)
            m_nextBotAddTime = now + 10; // 10s grace after master login

        // NOTE: original design had a serialization gate (PendingLoginCount == 0) that waited
        // for the previous bot's DB callback to fire before adding the next. Removed because if
        // any callback got lost (DB layer dropped it, manager destroyed mid-flight), the queue
        // would block forever and the next tick would never make progress → server freeze
        // (observed 11:12:44 → 11:16:00 with no log activity). The 3s delay alone gives the DB
        // enough time for the previous query to complete in practice.
        if (now >= m_nextBotAddTime)
        {
            uint32 guid = m_pendingGroupBotGuids.back();
            m_pendingGroupBotGuids.pop_back();

            // SafeAddPlayerBot internally handles both paths: if the bot is already in world
            // (e.g. RandomPlayerbotMgr already loaded it during boot), it calls OnBotLogin to
            // attach the bot to THIS holder; otherwise it queues a DB load and the callback
            // does the attach. We don't short-circuit "already in world" here anymore — that
            // skip path silently lost the master/bot link, so bots stayed as random bots
            // instead of joining the master's group.
            SafeAddPlayerBot(this, guid, master->GetSession()->GetAccountId());
            m_nextBotAddTime = now + 3; // 3s inter-bot delay, applies to both fresh-load and attach paths

            if (!m_pendingGroupBotGuids.empty() && m_pendingGroupBotGuids.size() % 5 == 0)
                sLog.outString("[PLAYERBOTS] %u bots still queued, serialized + 3s delay", (uint32)m_pendingGroupBotGuids.size());
        }
    }

    // Tick our bots' sessions so any queued bot-only handling fires per
    // master frame. Most importantly:
    // HandleTeleportAck for cross-map teleports — without this call, a
    // bot scheduled for a far-teleport via TeleportTo() stays in
    // IsBeingTeleported() forever (no client to ACK), its UpdateAI
    // early-exits, and the bot is effectively frozen on the source map.
    // This call was defined in PlayerbotHolder but never wired in the
    // cmangos→Penqle port. Symptom: `.bot add a bot` (alt on different
    // continent) auto-teleport to master never completes, bot doesn't
    // appear in /who, can't be invited.
    UpdateSessions(elapsed);
}

void PlayerbotMgr::HandleCommand(uint32 type, const std::string& text, uint32 lang)
{
    Player *master = GetMaster();
    if (!master)
        return;

    if (!sPlayerbotAIConfig.enabled)
        return;

    if (text.find(sPlayerbotAIConfig.commandSeparator) != std::string::npos)
    {
        std::vector<std::string> commands;
        split(commands, text, sPlayerbotAIConfig.commandSeparator.c_str());
        for (std::vector<std::string>::iterator i = commands.begin(); i != commands.end(); ++i)
        {
            HandleCommand(type, *i,lang);
        }
        return;
    }

    ForEachPlayerbot([&](Player *bot)
    {
        if (type == CHAT_MSG_SAY)
            if (bot->GetMapId() != master->GetMapId() || sServerFacade.GetDistance2d(bot, master) > 25)
                return;

        if (type == CHAT_MSG_YELL)
            if (bot->GetMapId() != master->GetMapId() || sServerFacade.GetDistance2d(bot, master) > 300)
               return;

        bot->GetPlayerbotAI()->HandleCommand(type, text, *master, lang);
    });

    sRandomPlayerbotMgr.ForEachPlayerbot([&](Player* bot)
    {
        if (type == CHAT_MSG_SAY)
            if (bot->GetMapId() != master->GetMapId() || sServerFacade.GetDistance2d(bot, master) > 25)
               return;

        if (type == CHAT_MSG_YELL)
            if (bot->GetMapId() != master->GetMapId() || sServerFacade.GetDistance2d(bot, master) > 300)
               return;

        if (bot->GetPlayerbotAI()->GetMaster() == master)
            bot->GetPlayerbotAI()->HandleCommand(type, text, *master, lang);
    });
}

void PlayerbotMgr::HandleMasterIncomingPacket(const WorldPacket& packet)
{
    ForEachPlayerbot([&](Player* bot)
    {
        bot->GetPlayerbotAI()->HandleMasterIncomingPacket(packet);
    });

    sRandomPlayerbotMgr.ForEachPlayerbot([&](Player* bot)
    {
        if (bot->GetPlayerbotAI()->GetMaster() == GetMaster())
            bot->GetPlayerbotAI()->HandleMasterIncomingPacket(packet);
    });

    switch (packet.GetOpcode())
    {
        // if master is logging out, log out all bots
        case CMSG_LOGOUT_REQUEST:
        {
            LogoutAllBots();
            return;
        }
        // if master cancelled logout, cancel too
        case CMSG_LOGOUT_CANCEL:
        {
            CancelLogout();
            return;
        }
    }
}
void PlayerbotMgr::HandleMasterOutgoingPacket(const WorldPacket& packet)
{
   ForEachPlayerbot([&](Player* bot)
   {
        if (!bot->GetPlayerbotAI())
            return;

        bot->GetPlayerbotAI()->HandleMasterOutgoingPacket(packet);
    });

    sRandomPlayerbotMgr.ForEachPlayerbot([&](Player* bot)
    {
        if (bot->GetPlayerbotAI()->GetMaster() == GetMaster())
            bot->GetPlayerbotAI()->HandleMasterOutgoingPacket(packet);
    });
}

void PlayerbotMgr::SaveToDB()
{
    ForEachPlayerbot([&](Player* bot)
    {
        bot->SaveToDB();
    });

    sRandomPlayerbotMgr.ForEachPlayerbot([&](Player* bot)
    {
        if (bot->GetPlayerbotAI()->GetMaster() == GetMaster())
            bot->SaveToDB();
    });
}

void PlayerbotMgr::OnBotLoginInternal(Player * const bot)
{
    bot->GetPlayerbotAI()->SetMaster(master);
    bot->GetPlayerbotAI()->ResetStrategies();
    sLog.outDebug("Bot %s logged in", bot->GetName());
}

void PlayerbotMgr::OnPlayerLogin(Player* player)
{
    if (player->GetSession() != player->GetPlayerMenu()->GetGossipMenu().GetMenuSession())
    {
        player->GetPlayerMenu()->GetGossipMenu() = GossipMenu(player->GetSession());
    }

    if (!sPlayerbotAIConfig.enabled)
        return;

    // set locale priority for bot texts
    sPlayerbotTextMgr.AddLocalePriority(player->GetSession()->GetSessionDbLocaleIndex());
    sLog.outDetail("Player %s logged in, localeDbc %i, localeDb %i", player->GetName(), (uint32)(player->GetSession()->GetSessionDbcLocale()), player->GetSession()->GetSessionDbLocaleIndex());

    if (sPlayerbotAIConfig.IsFreeAltBot(player))
    {
        sLog.outDetail("Enabling selfbot on login for %s", player->GetName());
        HandlePlayerbotCommand("self", player);
    }

    // --- Auto-reconnect bots from saved group ---
    // CRITICAL: bots themselves go through this same OnPlayerLogin path when they're loaded
    // (HandlePlayerLogin doesn't distinguish real players from bots). Without this guard, each
    // bot login would re-trigger the group_member query for itself, find its 39 squadmates,
    // queue them all, and each of those would do the same → exponential cascade → heap exhaustion
    // → 0xc0000374 crash within ~60s after a real player logs in with a full saved group.
    //
    // Detection: bot sessions have remote IP "disconnected/bot" (set in HandlePlayerBotLoginCallback).
    // Real player sessions have a real IP. Don't use GetPlayerbotAI() here — the AI is attached
    // LATER in OnBotLogin, so it's still null at this point.
    if (player->GetSession() && player->GetSession()->GetRemoteAddress() == "disconnected/bot")
    {
        // Bot login — no group reconnect, no autologin. Just return.
        return;
    }

    // The queue is processed by UpdateAIInternal at 1 bot / 3s after a 10s grace period.
    // We cap at 40 here because:
    //   * raid is 40 max anyway
    //   * an uncapped result from a malformed group_member row would let one bad row OOM us
    //   * we throttle to 3s, so 40 bots = 2min to settle — acceptable
    constexpr uint32 MAX_GROUP_REJOIN = 40;
    {
        uint32 playerGuidLow = player->GetObjectGuid().GetCounter();
        auto groupResult = CharacterDatabase.PQuery(
            "SELECT memberGuid FROM group_member WHERE groupId = "
            "(SELECT groupId FROM group_member WHERE memberGuid = '%u') LIMIT %u",
            playerGuidLow, MAX_GROUP_REJOIN + 1);
        if (groupResult)
        {
            uint32 botCount = 0;
            do
            {
                Field* fields = groupResult->Fetch();
                uint32 memberGuid = fields[0].GetUInt32();

                // Skip the player themselves
                if (memberGuid == playerGuidLow)
                    continue;

                if (m_pendingGroupBotGuids.size() >= MAX_GROUP_REJOIN)
                    break;

                m_pendingGroupBotGuids.push_back(memberGuid);
                botCount++;
            } while (groupResult->NextRow());

            if (botCount > 0)
            {
                m_nextBotAddTime = 0; // sentinel: queue processor sets the 10s grace on first tick
                sLog.outString("[PLAYERBOTS] Re-adding %u bots from saved group for player %s (1 every 3s after 10s grace)",
                               botCount, player->GetName());
            }
        }
    }

    if (sPlayerbotAIConfig.botAutologin == BotAutoLogin::DISABLED)
        return;

    // Autologin bots from master's account. The original cmangos code did `HandlePlayerbotCommand("add Bot1,Bot2,...")`
    // which calls AddPlayerBot for each one IMMEDIATELY in a tight loop, bypassing the throttled
    // queue. With 40-char accounts this burst-loads 40 bots in 1s → DB callbacks pile up →
    // concurrent OnBotLogin → heap corruption ~10s after master login (observed crash_20260528_110728).
    //
    // Fix: push the guids into m_pendingGroupBotGuids so they go through the same strictly serialized
    // queue used by group reconnect (1 bot at a time, wait for DB callback to complete before next).
    uint32 accountId = player->GetSession()->GetAccountId();
    auto results = CharacterDatabase.PQuery(
        "SELECT guid, name FROM characters WHERE account = '%u'",
        accountId);
    if (results)
    {
        uint32 added = 0;
        do
        {
            Field* fields = results->Fetch();
            uint32 botGuid = fields[0].GetUInt32();
            if (botGuid == player->GetObjectGuid().GetCounter())
                continue; // skip the master himself
            if (sPlayerbotAIConfig.botAutologin == BotAutoLogin::LOGIN_ONLY_ALWAYS_ACTIVE
                && !sPlayerbotAIConfig.IsFreeAltBot(botGuid))
                continue;
            // Avoid duplicate queueing (e.g. bot already in m_pendingGroupBotGuids from saved group)
            if (std::find(m_pendingGroupBotGuids.begin(), m_pendingGroupBotGuids.end(), botGuid)
                != m_pendingGroupBotGuids.end())
                continue;
            m_pendingGroupBotGuids.push_back(botGuid);
            added++;
        } while (results->NextRow());

        if (added > 0)
            sLog.outString("[PLAYERBOTS] Autologin queued %u alt-bots for %s (serialized throughput)",
                           added, player->GetName());
    }
}

void PlayerbotMgr::TellError(std::string botName, std::string text)
{
    std::set<std::string> names = errors[text];
    if (names.find(botName) == names.end())
    {
        names.insert(botName);
    }
    errors[text] = names;
}

std::vector<std::string> PlayerbotMgr::GetBotErrors(std::string botName)
{
    std::vector<std::string> botErrors;
    for (auto& [error, names] : errors)
    {
        if (names.find(botName) != names.end())
            botErrors.push_back(error);
    }

    return botErrors;
}

void PlayerbotMgr::CheckTellErrors(uint32 elapsed)
{
    time_t now = time(0);
    if ((now - lastErrorTell) < sPlayerbotAIConfig.errorDelay / 1000)
        return;

    lastErrorTell = now;

    for (PlayerBotErrorMap::iterator i = errors.begin(); i != errors.end(); ++i)
    {
        std::string text = i->first;
        std::set<std::string> names = i->second;

        std::ostringstream out;
        bool first = true;
        for (std::set<std::string>::iterator j = names.begin(); j != names.end(); ++j)
        {
            if (!first) out << ", "; else first = false;
            out << *j;
        }
        out << "|cfff00000: " << text;
        
        ChatHandler(master->GetSession()).PSendSysMessage("%s", out.str().c_str());
    }
    errors.clear();
}

std::list<std::string> PlayerbotHolder::HandleList(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;
    messages.push_back(ListBots(master, param));
    return messages;
}

std::list<std::string> PlayerbotHolder::HandleHelp(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;
    
    if (param.empty())
    {
        messages.push_back("Available commands: list, reload, tweak, always, self, debug, c, do, record, read, clear");
        messages.push_back("Type 'help <command>' for more information on a specific command.");
        return messages;
    }

    if (param == "commands")
    {
        std::string commands = "Commands: ";
        for (auto& [command, help] : GetCommandTexts())
        {
            commands += command + ", ";
        }

        commands = commands.substr(0, commands.size() - 2);
        messages.push_back(commands);
        return messages;
    }
    
    std::string helpText = GetCommandTexts(param);
    if (helpText.empty())
    {
        messages.push_back("No help available for '" + param + "'");
    }
    else
    {
        messages.push_back(helpText);
    }
    
    return messages;
}

std::list<std::string> PlayerbotHolder::HandleReload(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;
    if (security < SEC_GAMEMASTER)
    {
        messages.push_back("You do not have permission to use this command.");
        return messages;
    }
    messages.push_back("Reloading config");
    sPlayerbotAIConfig.Initialize();
    return messages;
}

std::list<std::string> PlayerbotHolder::HandleTweak(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;
    if (security < SEC_GAMEMASTER)
    {
        messages.push_back("You do not have permission to use this command.");
        return messages;
    }
    sPlayerbotAIConfig.tweakValue = sPlayerbotAIConfig.tweakValue++;
    if (sPlayerbotAIConfig.tweakValue > 2)
        sPlayerbotAIConfig.tweakValue = 0;
    messages.push_back("Set tweakvalue to " + std::to_string(sPlayerbotAIConfig.tweakValue));
    return messages;
}

std::string PlayerbotHolder::HandleBotAlways(Player* bot, Player* master, const std::string param)
{
    if (sPlayerbotAIConfig.selfBotLevel == BotSelfBotLevel::DISABLED)
    {
        return "Self-bot is disabled";
    }

    ObjectGuid guid = ObjectGuid(uint64(std::stoull(param)));
    uint32 accountId = sObjectMgr.GetPlayerAccountIdByGUID(guid);
    std::string alwaysName;    

    if (!sObjectMgr.GetPlayerNameByGUID(guid, alwaysName))
        return "Unable to find player.";


    BotAlwaysOnline always = BotAlwaysOnline(sRandomPlayerbotMgr.GetValue(guid.GetCounter(), "always"));

    if (always == BotAlwaysOnline::DISABLED || always == BotAlwaysOnline::DISABLED_BY_COMMAND)
    {
        sRandomPlayerbotMgr.SetValue(guid.GetCounter(), "always", (uint32)BotAlwaysOnline::ACTIVE);
        sPlayerbotAIConfig.freeAltBots.push_back(std::make_pair(accountId, guid.GetCounter()));

        Player* existingBot = sRandomPlayerbotMgr.GetPlayerBot(guid);
        if (existingBot)
        {
            if (master)
            {
                ProcessBotCommand("add", guid, master->GetObjectGuid(), false, master->GetSession()->GetAccountId(), master->GetGuildId());
            }
        }
        else
        {
            Player* player = sObjectMgr.GetPlayer(guid, false);
            if (player)
                OnBotLogin(player);
        }

        return "Enabled offline player ai for " + alwaysName;
    }
    else
    {
        sRandomPlayerbotMgr.SetValue(guid.GetCounter(), "always", (uint32)BotAlwaysOnline::DISABLED_BY_COMMAND);

        Player* onlineBot = sObjectMgr.GetPlayer(guid, false);
        if (onlineBot && onlineBot->GetPlayerbotAI())
        {
            if (!master || guid != master->GetObjectGuid())
            {
                if (sPlayerbotAIConfig.IsFreeAltBot(onlineBot))
                    sRandomPlayerbotMgr.LogoutPlayerBot(guid);
                else
                    DisablePlayerBot(guid, false);
            }
            else if (master)
            {
                DisablePlayerBot(guid, false);
            }
        }

        auto it = std::remove_if(sPlayerbotAIConfig.freeAltBots.begin(), sPlayerbotAIConfig.freeAltBots.end(), [guid](std::pair<uint32, uint32> i) { return i.second == guid.GetCounter(); });
        sPlayerbotAIConfig.freeAltBots.erase(it, sPlayerbotAIConfig.freeAltBots.end());

        return "Disabled offline player ai for " + alwaysName;
    }
}

std::list<std::string> PlayerbotHolder::HandleSelf(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;
    if (!master)
    {
        messages.push_back("self requires a master (in-game)");
        return messages;
    }

    if (master->GetPlayerbotAI())
    {
        DisablePlayerBot(master->GetGUIDLow(), false);
       
        if (sRandomPlayerbotMgr.GetValue(master->GetObjectGuid().GetCounter(), "selfbot"))
        {
            messages.push_back("Disable player ai (on login)");
            sRandomPlayerbotMgr.SetValue(master->GetObjectGuid().GetCounter(), "selfbot", (uint32)BotAlwaysOnline::DISABLED);
        }
        else
            messages.push_back("Disable player ai");
    }
    else if (sPlayerbotAIConfig.selfBotLevel == BotSelfBotLevel::DISABLED)
        messages.push_back("Self-bot is disabled");
    else if (sPlayerbotAIConfig.selfBotLevel == BotSelfBotLevel::GM_ONLY && security < SEC_GAMEMASTER)
        messages.push_back("You do not have permission to enable player ai");
    else
    {
        OnBotLogin(master);

        if (!param.empty() && param == "login")
        {
            messages.push_back("Enable player ai (on login)");
            sRandomPlayerbotMgr.SetValue(master->GetObjectGuid().GetCounter(), "selfbot", 1);
        }
        else
            messages.push_back("Enable player ai");
    }
   return messages;
}

std::string PlayerbotHolder::HandleBotDebug(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "debug requires a bot";

    PlayerbotAI* ai = bot->GetPlayerbotAI();
    if (!ai)
        return "Bot has no AI";

    ai->RecordMessages(true, true);

    std::string command = param;

    if(!ai->DoSpecificAction("cdebug", Event(".bot", command, master ? master : bot), true))
    {
        return "debug failed";
    }

    std::vector<std::string> output = ai->GetRecordedMessages();
    if (output.empty())
        return "(no output)";

    std::string result;
    for (const auto& line : output)
    {
        result += line + "\n";
    }
    return result;
}

std::string PlayerbotHolder::HandleBotC(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "c requires a bot";

    PlayerbotAI* ai = bot->GetPlayerbotAI();
    if (!ai)
        return "Bot has no AI";

    ai->DoSpecificAction("cdebug", Event(".bot", "monstertalk " + param, master ? master : bot), true);
    return "ok";
}

std::string PlayerbotHolder::HandleConsoleWhisper(Player* bot, Player* master, const std::string param)
{
    Player* sender = master;
    Player* reciever = bot;


    if (!reciever)
        return "d requires a bot";

    PlayerbotAI* ai = bot->GetPlayerbotAI();
    if (!ai)
        return "Bot has no AI";

    std::string message = param;

    if (!sender)
    {
        //Try format .(rnd)bot w <sender> <reciever> <message>

        std::string botName = param.substr(0, param.find(" "));

        master = sObjectAccessor.FindPlayerByName(botName.c_str());

        if (master)
        {
            if (message.size() > param.find(" ") + 1)
                message = param.substr(param.find(" ") + 1);
            else
                message = "";

            sender = bot; //Switch sender reciever
            reciever = master; 
        }
    }

    if (!sender)
        sender = bot;

    if (message.empty())
    {
        std::ostringstream out;
        if (!sender->GetPlayerbotAI())
            out << "Player ";
        if (!sender->GetPlayerbotAI()->IsRealPlayer())
            out << "Player bot ";
        else if (sRandomPlayerbotMgr.IsRandomBot(sender))
            out << "Random bot ";
        else if (sPlayerbotAIConfig.IsFreeAltBot(sender))
            out << "Free alt bot ";
        else
            out << "Bot ";

        out << reciever->GetName();
        out << " level " << std::to_string(reciever->GetLevel());
        out << " " << ChatHelper::formatRace(reciever->getRace());
        out << " " << ChatHelper::formatClass(reciever->getClass());

        if (sender->GetPlayerbotAI() && sender->GetPlayerbotAI()->GetMaster())
            out << " (master " << sender->GetPlayerbotAI()->GetMaster()->GetName() << ")";

        return out.str(); 
    }

    WorldPacket packet_template(CMSG_MESSAGECHAT);

    packet_template << CHAT_MSG_WHISPER;
    packet_template << LANG_UNIVERSAL;
    packet_template << reciever->GetName();
    packet_template << message;

    std::unique_ptr<WorldPacket> packetPtr(new WorldPacket(packet_template));

    sender->GetSession()->QueuePacket(std::move(packetPtr));

    std::string msg = "Sending whisper " + message + " to player " + reciever->GetName() + " from " + sender->GetName();

    return msg;
}

std::string PlayerbotHolder::HandleConsoleCmd(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "do requires a bot";

    PlayerbotAI* ai = bot->GetPlayerbotAI();
    if (!ai)
        return "Bot has no AI";

    ExternalEventHelper helper(ai->GetAiObjectContext());

    std::string msg = "Sending command " + param + " to player " + bot->GetName();

    if (!helper.ParseChatCommand(param, master ? master : bot))
    {
        return "command failed";
    }    

    return msg;
}

std::string PlayerbotHolder::HandleBotTest(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "test requires a bot";

    PlayerbotAI* ai = bot->GetPlayerbotAI();
    if (!ai)
        return "Bot has no AI";

    if (param.empty())
    {
        return "Usage: test <testName>. Available tests: walk_to_ironforge, flight_ratchet_to_booty_bay";
    }

    // Activate test strategy which will run the test over multiple ticks
    std::string strategyName = "test::" + param;
    ai->ChangeStrategy("+" + strategyName, BotState::BOT_STATE_NON_COMBAT);
    
    return "Test '" + param + "' started for bot " + bot->GetName();
}

std::string PlayerbotHolder::HandleBotDo(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "do requires a bot";

    PlayerbotAI* ai = bot->GetPlayerbotAI();
    if (!ai)
        return "Bot has no AI";

    std::string actionName = param;
    std::string subparam = "";

    Action* action = nullptr;

    size_t i = std::string::npos;
    while (true)
    {
        action = ai->GetAiObjectContext()->GetAction(param);

        if (action)
            break;

        size_t found = param.rfind(" ", i);
        if (found == std::string::npos || !found)
            break;

        actionName = param.substr(0, found);
        subparam = param.substr(found + 1);

        i = found - 1;
    }

    if (!action)
        return "action not found";

    ai->RecordMessages(true, true);

    std::vector<std::string> output;

    if (!ai->DoSpecificAction(actionName, Event(".bot", subparam, master ? master : bot), true))
    {
        output = GetBotErrors(bot->GetName());

        if (output.empty())
            return "action failed";

        std::string result;
        for (const auto& line : output)
        {
            result += line + "\n";
        }
        return result;
    }

    output = ai->GetRecordedMessages();
    if (output.empty())
        return "(no output)";

    std::string result;
    for (const auto& line : output)
    {
        result += line + "\n";
    }
    return result;
}

std::string PlayerbotHolder::HandleBotRecord(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "record requires a bot";

    PlayerbotAI* ai = bot->GetPlayerbotAI();
    if (!ai)
        return "Bot has no AI";

    ai->RecordMessages(true, !param.empty());
    return "Recording enabled on " + std::string(bot->GetName());
}

std::string PlayerbotHolder::HandleBotRead(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "read requires a bot";

    PlayerbotAI* ai = bot->GetPlayerbotAI();
    if (!ai)
        return "Bot has no AI";

    std::vector<std::string> output = ai->GetRecordedMessages();

    if (output.empty())
        return "(no messages)";

    std::string result;
    for (const auto& line : output)
    {
        result += line + "\n";
    }
    return result;
}

std::string PlayerbotHolder::HandleBotClear(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "clear requires a bot";

    PlayerbotAI* ai = bot->GetPlayerbotAI();
    if (!ai)
        return "Bot has no AI";

    ai->ClearRecordedMessages();
    return "Messages cleared";
}

std::list<std::string> PlayerbotHolder::HandleParty(Player* master, const std::string param, AccountTypes security)
{
    std::string message;
    std::string botName;

    if (!master)
    {
        botName = param.substr(0, param.find(" "));
        master = sObjectAccessor.FindPlayerByName(botName.c_str());
    }

    if (!master)
        return {"No sender found"};

    if (param.find(" ") == std::string::npos)
        message = param;
    else if (param.size() > param.find(" ") + 1)
        message = param.substr(param.find(" ") + 1);

    if (!master->GetGroup())
        return {"Sender is not in a group"};

    if (message.empty())
    {
        Group* group = master->GetGroup();
        Group::MemberSlotList const& members = group->GetMemberSlots();
        Player* leader = sObjectMgr.GetPlayer(group->GetLeaderGuid());

        std::string leaderName = leader ? leader->GetName() : "Unknown";
        std::string otherMembers;

        for (auto const& slot : members)
        {
            if (slot.guid == master->GetObjectGuid())
                continue;

            Player* member = sObjectMgr.GetPlayer(slot.guid);
            if (member)
            {
                if (!otherMembers.empty())
                    otherMembers += ", ";
                otherMembers += member->GetName();
            }
        }

        return {"Party with " + leaderName + " as leader" + (otherMembers.empty() ? "" : " and " + otherMembers)};
    }

    WorldPacket packet_template(CMSG_MESSAGECHAT);
    packet_template << CHAT_MSG_PARTY;
    packet_template << LANG_UNIVERSAL;
    packet_template << message;

    std::unique_ptr<WorldPacket> packetPtr(new WorldPacket(packet_template));
    master->GetSession()->QueuePacket(std::move(packetPtr));
    return {"Sent party message \"" + message + "\" as " + master->GetName()};
}

std::list<std::string> PlayerbotHolder::HandleGuild(Player* master, const std::string param, AccountTypes security)
{
    std::string message;
    std::string botName;

    if (!master)
    {
        botName = param.substr(0, param.find(" "));
        master = sObjectAccessor.FindPlayerByName(botName.c_str());
    }

    if (!master)
        return {"No sender found"};

    if (param.find(" ") == std::string::npos)
        message = param;
    else if (param.size() > param.find(" ") + 1)
        message = param.substr(param.find(" ") + 1);

    if (!master->GetGuildId())
        return {"Sender is not in a guild"};

    if (message.empty())
    {
        Guild* guild = sGuildMgr.GetGuildById(master->GetGuildId());
        if (!guild)
            return {"Guild info not found"};

        std::string guildName = guild->GetName();
        std::string guildLeader;
        sObjectMgr.GetPlayerNameByGUID(guild->GetLeaderGuid(), guildLeader);
        uint32 memberCount = guild->GetMemberSize();

        return {"Guild: " + guildName + ", Leader: " + guildLeader + ", Members: " + std::to_string(memberCount)};
    }

    WorldPacket packet_template(CMSG_MESSAGECHAT);
    packet_template << CHAT_MSG_GUILD;
    packet_template << LANG_UNIVERSAL;
    packet_template << message;

    std::unique_ptr<WorldPacket> packetPtr(new WorldPacket(packet_template));
    master->GetSession()->QueuePacket(std::move(packetPtr));
    return {"Sent guild message \"" + message + "\" as " + master->GetName()};
}

std::list<std::string> PlayerbotHolder::HandleRaid(Player* master, const std::string param, AccountTypes security)
{
    std::string message = param;

    if (!master)
    {
        std::string botName = param.substr(0, param.find(" "));

        master = sObjectAccessor.FindPlayerByName(botName.c_str());
        if (message.size() > param.find(" ") + 1)
            message = param.substr(param.find(" ") + 1);
    }

    if (!master)
        return {"No sender found"};

    if (!master->GetGroup() || !master->GetGroup()->IsRaidGroup())
        return {"Sender is not in a raid group"};

    if (message.empty())
    {
        Group* group = master->GetGroup();
        Group::MemberSlotList const& members = group->GetMemberSlots();
        Player* leader = sObjectMgr.GetPlayer(group->GetLeaderGuid());

        std::string leaderName = leader ? leader->GetName() : "Unknown";
        std::string otherMembers;

        for (auto const& slot : members)
        {
            if (slot.guid == master->GetObjectGuid())
                continue;

            Player* member = sObjectMgr.GetPlayer(slot.guid);
            if (member)
            {
                if (!otherMembers.empty())
                    otherMembers += ", ";
                otherMembers += member->GetName();
            }
        }

        return {"Raid with " + leaderName + " as leader" + (otherMembers.empty() ? "" : " and " + otherMembers)};
    }

    WorldPacket packet_template(CMSG_MESSAGECHAT);
    packet_template << CHAT_MSG_RAID;
    packet_template << LANG_UNIVERSAL;
    packet_template << message;

    std::unique_ptr<WorldPacket> packetPtr(new WorldPacket(packet_template));
    master->GetSession()->QueuePacket(std::move(packetPtr));
    return {"Sent raid message \"" + message + "\" as " + master->GetName()};
}

std::list<std::string> PlayerbotHolder::HandleRaidLeader(Player* master, const std::string param, AccountTypes security)
{
    std::string message = param;

    if (!master)
    {
        std::string botName = param.substr(0, param.find(" "));

        master = sObjectAccessor.FindPlayerByName(botName.c_str());
        if (message.size() > param.find(" ") + 1)
            message = param.substr(param.find(" ") + 1);
    }

    if (!master)
        return {"No sender found"};

    if (!master->GetGroup() || !master->GetGroup()->IsRaidGroup())
        return {"Sender is not in a raid group"};

    WorldPacket packet_template(CMSG_MESSAGECHAT);
    packet_template << CHAT_MSG_RAID;
    packet_template << LANG_UNIVERSAL;
    packet_template << message;

    std::unique_ptr<WorldPacket> packetPtr(new WorldPacket(packet_template));
    master->GetSession()->QueuePacket(std::move(packetPtr));
    std::string result = "Sent raid leader transfer request as " + std::string(master->GetName());
    return {result};
}

std::string PlayerbotHolder::HandleBotAddLogin(Player* bot, Player* master, const std::string param)
{
    SC_LOG("HandleBotAddLogin entry bot=%s master=%s param=%s",
           bot ? bot->GetName() : "(null)",
           master ? master->GetName() : "(null)",
           param.c_str());

    if (bot)
    {
        SC_LOG("HandleBotAddLogin bot already online — returning early");
        return "Player already logged in";
    }

    if (!Qualified::isValidNumberString(param))
        return "Add: Error parsing " + param;

    ObjectGuid guid = ObjectGuid(uint64(std::stoull(param)));

    uint32 guildId = Player::GetGuildIdFromDB(guid);
    uint32 masterAccountId = master ? master->GetSession()->GetAccountId() : 0;
    uint32 masterGuildId = master ? master->GetGuildId() : 0;
    uint32 botAccount = sObjectMgr.GetPlayerAccountIdByGUID(guid);
    bool isMasterAccount = (masterAccountId == botAccount);
    bool isRandomAccount = sPlayerbotAIConfig.IsInRandomAccountList(botAccount);

    SC_LOG("HandleBotAddLogin guid=%u botAccount=%u masterAccount=%u isMaster=%d isRandom=%d",
           guid.GetCounter(), botAccount, masterAccountId,
           (int)isMasterAccount, (int)isRandomAccount);

    if (isRandomAccount)
    {
        SC_LOG("HandleBotAddLogin -> RandomPlayerbotMgr.AddRandomBot");
        sRandomPlayerbotMgr.AddRandomBot(guid);
    }
    else if (isMasterAccount || sPlayerbotAIConfig.allowMultiAccountAltBots)
    {
        SC_LOG("HandleBotAddLogin -> AddPlayerBot (master-account path)");
        AddPlayerBot(guid, masterAccountId);
    }
    else
    {
        SC_LOG("HandleBotAddLogin -> rejected: not in account");
        return "Not in your account";
    }

    SC_LOG("HandleBotAddLogin returning ok bot=%u", guid.GetCounter());
    return "ok";
}

// `.bot summon <name>` (also aliased as
// `recall` / `come`). Teleports an already-online bot to the master's
// current position. Distinct from `.bot add` which brings the bot online
// at its last logout location. Useful when:
//   - The auto-teleport-on-summon flag failed (race / master null at tick)
//   - Bot wandered off via grind/free strategy and you want it back
//   - Bot disconnected from master and is stuck on a different map
std::string PlayerbotHolder::HandleBotSummon(Player* bot, Player* master, const std::string param)
{
    SC_LOG("HandleBotSummon entry bot=%s master=%s",
           bot ? bot->GetName() : "(null)",
           master ? master->GetName() : "(null)");

    if (!bot)
        return "Bot is offline (use `.bot add <name>` first)";
    if (!master)
        return "No master session — can only be called by an in-world player";

    uint32 masterAccountId = master->GetSession()->GetAccountId();
    uint32 botAccount = sObjectMgr.GetPlayerAccountIdByGUID(bot->GetObjectGuid());
    const bool isMasterAccount = (masterAccountId == botAccount);
    const bool isRandomAccount = sPlayerbotAIConfig.IsInRandomAccountList(botAccount);

    // Security: only allow summoning bots on the master's account, random
    // bots, or bots that have explicitly accepted this master via group.
    bool allowed = isMasterAccount || isRandomAccount;
    if (!allowed)
    {
        PlayerbotAI* ai = bot->GetPlayerbotAI();
        if (ai && ai->GetMaster() == master)
            allowed = true;
    }
    if (!allowed)
        return "This bot isn't yours to summon.";

    // Block when in combat, BG, or instance — these usually mean the bot
    // is mid-fight or in restricted content. The master can manually
    // address those (e.g. wait for combat to end, leave the BG queue).
    if (bot->IsInCombat())
        return "Bot is in combat — wait for it to settle.";
    if (bot->InBattleGround() || bot->InBattleGroundQueue())
        return "Bot is in BG / BG queue.";
    // Cross-instance pull is now ALLOWED. The user couldn't get their MC raid back together
    // after leaving the instance because half the bots were still bound to MC's map. The
    // teleport below handles cross-map relocation; the bot leaves its instance cleanly.
    // (Was: return "Bot is in an instance — can't pull cross-instance.")

    SC_LOG("HandleBotSummon teleport bot=%s -> master=%s map=%u pos=%.1f,%.1f,%.1f",
           bot->GetName(), master->GetName(),
           master->GetMapId(),
           master->GetPositionX(), master->GetPositionY(), master->GetPositionZ());

    bot->TeleportTo(master->GetMapId(),
                    master->GetPositionX(),
                    master->GetPositionY(),
                    master->GetPositionZ(),
                    master->GetOrientation());

    return "ok — teleporting to you";
}

std::string PlayerbotHolder::HandleBotRemoveLogout(Player* bot, Player* master, const std::string param)
{
    SC_LOG("HandleBotRemoveLogout entry bot=%s master=%s",
           bot ? bot->GetName() : "(null)",
           master ? master->GetName() : "(null)");

    if (!bot)
        return "Player is offline";

    uint32 guildId = Player::GetGuildIdFromDB(bot->GetObjectGuid());
    uint32 masterAccountId = master ? master->GetSession()->GetAccountId() : 0;
    uint32 masterGuildId = master ? master->GetGuildId() : 0;
    uint32 botAccount = sObjectMgr.GetPlayerAccountIdByGUID(bot->GetObjectGuid());
    bool isMasterAccount = (masterAccountId == botAccount);
    bool isRandomAccount = sPlayerbotAIConfig.IsInRandomAccountList(botAccount);

    SC_LOG("HandleBotRemoveLogout botAccount=%u masterAccount=%u isMaster=%d isRandom=%d",
           botAccount, masterAccountId, (int)isMasterAccount, (int)isRandomAccount);

    //:
    // Refuse `.rndbot remove` for random-account bots that aren't currently
    // master-linked to the player issuing the command. Symptom we're guarding
    // against: WorldSession::LogoutPlayer crashed during the group-cleanup
    // chain (between CleanupChannels and SMSG_LOGOUT_COMPLETE) for a bot
    // whose group/master pointers may have been left dangling by the
    // previous mangosd session. By requiring an explicit master-link, we
    // ensure the bot's state is "fresh" enough to logout safely. The user
    // can still acquire-then-remove via /invite ... /uninvite ... or via
    // the standard `.bot rm` path for master-account bots.
    if (isRandomAccount && master)
    {
        PlayerbotAI* botAi = bot->GetPlayerbotAI();
        if (!botAi || botAi->GetMaster() != master)
            return "This bot isn't bound to you. /invite first to acquire, then /uninvite + .rndbot remove.";
    }

    if (isRandomAccount)
        sRandomPlayerbotMgr.Remove(bot);
    else if (GetPlayerBot(bot->GetGUIDLow()))
        LogoutPlayerBot(bot->GetGUIDLow());
    else
        return "Not your bot";

    return "ok";
}

void PlayerbotHolder::CreateBot(Player* master, const std::string param, std::list<std::string>& messages, ObjectGuid& guid)
{    
    // Allow null master for RA/console usage
    // Player* master can be null when called via .rndbot commands

    std::string name;
    std::string testName;
    uint8 race = 0;
    uint8 cls = 0;
    uint32 level = 0;
    bool autoAdd = master;
    bool temporary = false;
    uint8 gender = GENDER_NONE;
    Team team = Team::TEAM_BOTH_ALLOWED;
    BotRoles role = BotRoles::BOT_ROLE_NONE;
    std::string groupWith = master ? master->GetName() : "";
    std::string gear = "default";

    std::vector<std::string> args = Qualified::getMultiQualifiers(param, " ");
    for (const auto& arg : args)
    {
        size_t eqPos = arg.find('=');
        if (eqPos == std::string::npos)
            continue;

        std::string key = arg.substr(0, eqPos);
        std::string value = arg.substr(eqPos + 1);

        if (key == "name")
            name = value;
        else if (key == "faction")
            team = ChatHelper::parseTeam(value);
        else if (key == "race")
            race = ChatHelper::parseRace(value);
        else if (key == "class")
            cls = ChatHelper::parseClass(value);
        else if (key == "gender")
            gender = ChatHelper::parseGender(value);
        else if (key == "level")
            level = std::stoul(value);
        else if (key == "role")
            role = ChatHelper::parseRole(value);
        else if (key == "login")
            autoAdd = (value == "1" || value == "true" || value == "yes");
        else if (key == "group")
            groupWith = value;
        else if (key == "gear")
            gear = value;
        else if (key == "test")
        {
            testName = value;
            autoAdd = true;
        }
        else if (key == "temporary")
            temporary = (value == "1" || value == "true" || value == "yes");
    }

    std::string error;
    uint32 accountId = GetOrCreateAccount(master, error);
    if (accountId == 0)
    {
        messages.push_back(error);
        return;
    }

    uint32 maxCharsPerAccount = 9;
#ifdef MANGOSBOT_TWO
    maxCharsPerAccount = 10;
#endif

    if (sAccountMgr.GetCharactersCount(accountId) >= maxCharsPerAccount)
    {
        messages.push_back("Account has max characters");
        return;
    }

    uint8 skin = 0, face = 0, hairStyle = 0, hairColor = 0, facialHair = 0;

    if (!name.empty())
    {
        auto result = CharacterDatabase.PQuery("SELECT guid FROM characters WHERE name = '%s'", name.c_str());
        if (result)
        {
            messages.push_back("Name already exists");
            return;
        }
    }

    if (team == TEAM_BOTH_ALLOWED && master)
        team = master->GetTeam();

    if (gender == GENDER_NONE)
        gender = urand(GENDER_MALE, GENDER_FEMALE);

    RandomPlayerbotFactory factory(0);

    if (cls == 0)
        cls = factory.GetRandomClass(race);

    if (race == 0)
    {
        race = factory.GetRandomRace(cls, team);
    }

    if (name.empty())
    {
        RandomPlayerbotFactory::NameRaceAndGender raceAndGender = RandomPlayerbotFactory::CombineRaceAndGender(gender, race);
        name = RandomPlayerbotFactory::CreateRandomBotName(raceAndGender);
    }

    // remote_ip MUST be "disconnected/bot" — see comment in HandlePlayerBotLoginCallback above.
    // Empty string makes PlayerbotAI::IsRealPlayer() return TRUE, breaking HandleTeleportAck.
    WorldSession* botSession = new WorldSession(accountId, NULL, SEC_PLAYER,
#ifdef MANGOSBOT_TWO
        2,
        0,
        LOCALE_enUS,
        "disconnected/bot",
        0,
        0,
        false);
#endif
#ifdef MANGOSBOT_ONE
        2, 0, LOCALE_enUS, "disconnected/bot", 0, 0, false);
#endif
#ifdef MANGOSBOT_ZERO
        0, LOCALE_enUS, "disconnected/bot", 0);
#endif

        botSession->SetNoAnticheat();

        Player* newBot = new Player(botSession);
        if (!newBot->Create(sObjectMgr.GeneratePlayerLowGuid(), name, race, cls, gender, skin, face, hairStyle, hairColor, facialHair, 0))
        {
            delete botSession;
            delete newBot;
            messages.push_back("Failed to create character");
            return;
        }

        newBot->setCinematic(2);
        newBot->SetAtLoginFlag(AT_LOGIN_NONE);
        sObjectAccessor.AddObject(newBot);

        uint32 botGuid = newBot->GetGUIDLow();
        guid = newBot->GetObjectGuid();

        if (level > 1)
        {
            newBot->SetLevel(level);
            newBot->SetUInt32Value(PLAYER_XP, 0);
            newBot->InitStatsForLevel(true);
#ifdef MANGOSBOT_ZERO
            newBot->InitTaxiNodes();
#else
        newBot->InitTaxiNodesForLevel();
#endif
            newBot->InitTalentForLevel();
            newBot->InitPrimaryProfessions();
            newBot->learnDefaultSpells();

            std::ostringstream out;
            ChangeTalentsAction::AutoSelectTalents(newBot, &out, role);

            sRandomPlayerbotMgr.SetValue(botGuid, "create levelup", 1);
            sRandomPlayerbotMgr.SetValue(botGuid, "create gear", 1, gear);
        }
        else
            newBot->SetLevel(1);

        // Grouping means something at every level, gear and talents do not, so
        // this does not belong in the branch above. HandleGroup passes the
        // master's own level into create, so a level 1 master never reached it
        // and never got the deferred auto-invite at all.
        if (!groupWith.empty())
            sRandomPlayerbotMgr.SetValue(botGuid, "create group", 1, groupWith);

        if (!testName.empty())
        {
            testName = std::regex_replace(testName, std::regex("'"), "\\'");

            sRandomPlayerbotMgr.SetValue(botGuid, "test", 1, testName);
        }
        if (temporary)
        {
            sRandomPlayerbotMgr.SetValue(botGuid, "temporary", 1, name);
        }

        if (master)
        {
            newBot->SetMap(master->GetMap());
            newBot->SetPosition(master->GetPositionX(), master->GetPositionY(), master->GetPositionZ(), master->GetOrientation());
        }

        newBot->SaveToDB();

        // Register the new character in the player cache by hand.
        //
        // botSession is a throwaway and never has SetPlayer() called on it, so
        // the `if (_player)` body of LogoutPlayer() below - which is what
        // normally registers a character - does nothing here. The character
        // then misses from m_playerNameToGuid, which breaks
        // `.rndbot add/summon <name>`, and from m_playerCacheData, so
        // GetPlayerAccountIdByGUID answers 0, AddPlayerBot refuses to log the
        // bot in, and it retries every tick for as long as the server runs.
        //
        // Re-reading the row is not enough on its own for a character created
        // mid-session: SaveToDB has not necessarily become visible to the next
        // SELECT yet. So check afterwards, and fall back to the Player object
        // that is still in memory.
        {
            ObjectGuid const cacheGuid(HIGHGUID_PLAYER, botGuid);

            sObjectMgr.LoadPlayerCacheData(botGuid);

            if (!sObjectMgr.GetPlayerAccountIdByGUID(cacheGuid))
            {
                if (!sObjectMgr.InsertPlayerInCache(newBot))
                    sLog.outError("PlayerbotHolder::CreateBot: could not put %s (guid %u) into the player cache - "
                                  "the bot will not be loginable by name", name.c_str(), botGuid);
            }
        }

        messages.push_back("Bot created: " + name);

        botSession->LogoutPlayer();
        sObjectAccessor.RemoveObject(newBot);
        delete newBot;
        delete botSession;

        if (autoAdd)
        {
            sPlayerbotAIConfig.freeAltBots.push_back(std::make_pair(accountId, botGuid));
            messages.push_back("Bot is now online");
        }
        else
        {
            messages.push_back("Use '.rndbot add " + name + "' to bring this bot online");
        }

        return;
}

std::list<std::string> PlayerbotHolder::HandleCreate(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;
    ObjectGuid guid;

    CreateBot(master, param, messages, guid);

    return messages;
}

// `.bot revive` / `.bot rez` — holder-level command (single dispatch, not per-bot).
// Iterates the master's group/raid and revives every dead member that is a bot.
// Refuses if the master is in combat (UX: also blocked on the addon button).
std::list<std::string> PlayerbotHolder::HandleRevive(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;
    if (!master)
    {
        messages.push_back("Need a master player");
        return messages;
    }
    if (master->IsInCombat())
    {
        messages.push_back("Cannot revive while in combat.");
        return messages;
    }
    Group* group = master->GetGroup();
    if (!group)
    {
        messages.push_back("You are not in a group/raid.");
        return messages;
    }

    auto reviveOne = [](Player* p) -> bool
    {
        if (!p || p->IsAlive()) return false;
        p->ResurrectPlayer(1.0f);
        p->SpawnCorpseBones();
        p->SetHealth(p->GetMaxHealth());
        if (p->GetPowerType() == POWER_MANA)
            p->SetPower(POWER_MANA, p->GetMaxPower(POWER_MANA));
        // Strip resurrection sickness (15007) which would otherwise nuke the bot's stats by 75%
        // for 10 minutes — useless after a tank wipe in MC. Strip the rebirth/ankh sickness
        // variants too (the lvl < 10 short version and the priest soul revival aura).
        p->RemoveAurasDueToSpell(15007);
        p->RemoveAurasDueToSpell(27819);
        p->RemoveAurasDueToSpell(20584);
        return true;
    };

    uint32 revived = 0;
    for (auto const& slot : group->GetMemberSlots())
    {
        Player* member = sObjectMgr.GetPlayer(slot.guid);
        if (!member || member == master) continue;
        // Only revive bots (skip real player teammates — they have their own corpse-run).
        if (!member->GetPlayerbotAI()) continue;
        if (reviveOne(member))
            revived++;
    }
    std::ostringstream out;
    out << "Revived " << revived << " bot(s) in your " << (group->IsRaidGroup() ? "raid" : "group") << ".";
    messages.push_back(out.str());
    return messages;
}

std::list<std::string> PlayerbotHolder::HandleFill(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;
    if (!master)
    {
        messages.push_back("Need a master player");
        return messages;
    }

    uint32 maxSize = 40;
    if (!param.empty())
        maxSize = std::min((uint32)atoi(param.c_str()), (uint32)40);
    if (maxSize == 0) maxSize = 40;

    bool isAlliance = (master->GetTeam() == ALLIANCE);
    std::string factionRaces = isAlliance ? "1,3,4,7" : "2,5,6,8";

    uint32 alreadyInGroup = 1;
    Group* group = master->GetGroup();
    if (group)
        alreadyInGroup = group->GetMembersCount();
    uint32 needed = (maxSize > alreadyInGroup) ? maxSize - alreadyInGroup : 0;

    sLog.outString("[BOT_FILL] param='%s' maxSize=%u alreadyInGroup=%u needed=%u isRaid=%d",
        param.c_str(), maxSize, alreadyInGroup, needed, group ? (int)group->IsRaidGroup() : -1);

    if (!needed)
    {
        messages.push_back("Group/raid is already full (" + std::to_string(alreadyInGroup) + "/" + std::to_string(maxSize) + ")");
        return messages;
    }

    // Only OFFLINE bots: an online random bot is usually already in a bot-only
    // group, and you cannot invite a player who is already grouped — it gets
    // controlled but never joins the master's party (looked like "nothing
    // happens"). Freshly-loaded offline bots go through the full login path that
    // invites them to the group. There are plenty of offline level-60 bots; the
    // pipeline's ".bot init *" gears whichever ones join. Highest level first.
    auto result = CharacterDatabase.PQuery(
        "SELECT c.guid, c.name FROM characters c "
        "WHERE c.level >= 55 AND c.race IN (%s) AND c.online = 0 "
        "ORDER BY c.level DESC, RAND() LIMIT %u",
        factionRaces.c_str(), needed);

    if (!result)
    {
        messages.push_back("No available bots found");
        return messages;
    }

    PlayerbotMgr* mgr = master->GetPlayerbotMgr();
    if (!mgr)
    {
        messages.push_back("PlayerbotMgr not initialized");
        return messages;
    }

    uint32 queued = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 guid = fields[0].GetUInt32();
        mgr->m_pendingGroupBotGuids.push_back(guid);
        queued++;
    } while (result->NextRow() && queued < needed);

    messages.push_back("Queued " + std::to_string(queued) + " bots. Adding 1 per second (auto-joining group).");
    return messages;
}

std::list<std::string> PlayerbotHolder::HandleGroup(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;

    if (!master)
    {
        messages.push_back("group command requires a master (in-game)");
        return messages;
    }

    uint32 masterLevel = master->GetLevel();
    uint8 masterClass = master->getClass();
    Team team = master->GetTeam();
    BotRoles masterRole = AiFactory::GetPlayerRoles(master);
    uint8 groupSize = 5;
    uint8 currentGroupSize = 1;
    Group* group = master->GetGroup();
    if (group)
        currentGroupSize = group->GetMembersCount();

    std::string passThroughParam = "";

    std::vector<std::string> args = Qualified::getMultiQualifiers(param, " ");
    for (const auto& arg : args)
    {
        size_t eqPos = arg.find('=');
        if (eqPos == std::string::npos)
            continue;

        std::string key = arg.substr(0, eqPos);
        std::string value = arg.substr(eqPos + 1);

        if (key == "size" && Qualified::isValidNumberString(value))
            groupSize = stoi(value);
        else
            passThroughParam += key + "=" + value + " ";
    }
    
    std::unordered_map<uint8, std::unordered_map<BotRoles, uint32>> allowedClassNr = LfgAction::AllowedClassRoleNr(master, groupSize);

    RandomPlayerbotFactory factory(0);

    uint32 maxTries = 10*groupSize;

    uint32 botsCreated = 0;
    uint32 continue_role = 0, continue_race = 0, continue_class = 0;
    std::map<uint8, uint32> classesCreated;

    while (currentGroupSize < groupSize)
    {
        maxTries--;
        if (!maxTries)
            break;

        BotRoles role = BotRoles(urand(BotRoles::BOT_ROLE_TANK, BotRoles::BOT_ROLE_DPS));

        if (allowedClassNr[0][role] == 0)
        {
            continue_role++;
            continue;
        }

        uint8 cls = factory.GetRandomClass(0, role);

#ifdef MANGOSBOT_ZERO
        if (cls == CLASS_PALADIN && team == HORDE)
        {
            continue_race++;
            continue;
        }
        if (cls == CLASS_SHAMAN && team == ALLIANCE)
        {
            continue_race++;
            continue;
        }
#endif

        if (allowedClassNr[cls].find(role) != allowedClassNr[cls].end() && allowedClassNr[cls][role] == 0)
        {
            continue_class++;
            continue;
        }

        std::ostringstream paramStr;
        paramStr << "level=" << masterLevel << " class=" << ChatHelper::formatClass(cls) << " group=" << master->GetName() << " " << passThroughParam; //Passthrough will override.

        auto result = HandleCreate(master, paramStr.str(), security);
        messages.splice(messages.end(), result);

        if (!messages.empty())
        {
            auto lastMsg = messages.front();
            if (lastMsg.find("Bot created:") != std::string::npos)
            {
                classesCreated[cls]++;
                botsCreated++;
                currentGroupSize++;
            }
        }
    
        allowedClassNr[0][role]--; 
        
        if (allowedClassNr[cls].find(role) != allowedClassNr[cls].end())
            allowedClassNr[cls][role]--;
    }

    std::ostringstream debugInfo;
    debugInfo << "DEBUG group: target=" << (int)groupSize << ", created=" << botsCreated;
    if (maxTries == 0)
        debugInfo << " (maxTries exhausted)";
    debugInfo << ", continues: role=" << continue_role << ", race=" << continue_race << ", class=" << continue_class;
    debugInfo << ", classes: ";
    for (auto& kv : classesCreated)
        debugInfo << ChatHelper::formatClass(kv.first) << "=" << kv.second << ",";
    sLog.outString("%s", debugInfo.str().c_str());

    return messages;
}

#ifdef GenerateBotTests
std::list<std::string> PlayerbotHolder::HandleRunTest(Player* master, const std::string param, AccountTypes security)
{    
    std::list<std::string> messages;

    if (param.empty())
    {
        messages.push_back("Usage: .rndbot runtest <testnamepart> [count]");
        messages.push_back("Available tests:");
        std::vector<std::string> availableTests = TestRegistry::GetAvailableTests();
        for (const auto& test : availableTests)
            messages.push_back("  " + test);
        return messages;
    }

    std::string testNamePart = param;
    size_t maxTests = 0;

    size_t lastSpace = param.find_last_of(" \t");
    if (lastSpace != std::string::npos)
    {
        std::string maybeLimit = param.substr(lastSpace + 1);
        if (!maybeLimit.empty())
        {
            bool numericLimit = std::all_of(maybeLimit.begin(), maybeLimit.end(), ::isdigit);
            if (numericLimit)
            {
                try
                {
                    maxTests = std::stoul(maybeLimit);
                }
                catch (...)
                {
                    messages.push_back("Invalid count '" + maybeLimit + "'");
                    return messages;
                }

                if (!maxTests)
                {
                    messages.push_back("Count must be greater than 0");
                    return messages;
                }

                testNamePart = param.substr(0, lastSpace);
                size_t nameEnd = testNamePart.find_last_not_of(" \t");
                testNamePart = (nameEnd == std::string::npos) ? "" : testNamePart.substr(0, nameEnd + 1);
            }
        }
    }

    if (testNamePart.empty())
    {
        messages.push_back("Usage: .rndbot runtest <testnamepart> [count]");
        return messages;
    }

    bool listTests = false;

    if (testNamePart[0] == '?')
    {
        listTests = true;
        testNamePart = testNamePart.substr(2);
    }

    std::transform(testNamePart.begin(), testNamePart.end(), testNamePart.begin(), ::tolower);

    std::vector<std::string> matchingTests;
    std::vector<std::string> allTests = TestRegistry::GetAvailableTests();
    for (const auto& test : allTests)
    {
        std::string lowerTest = test;
        std::transform(lowerTest.begin(), lowerTest.end(), lowerTest.begin(), ::tolower);
        if (lowerTest.find(testNamePart) != std::string::npos || testNamePart == "*")
        {
            matchingTests.push_back(test);
            if (maxTests && matchingTests.size() >= maxTests)
                break;
        }
    }

    if (matchingTests.empty())
    {
        messages.push_back("No tests matching '" + param + "' found");
        return messages;
    }

    if (listTests)
    {
        messages.push_back("Tests matching '" + param + "':");
        for (const auto& test : matchingTests)
            messages.push_back("  " + test);
        return messages;
    }

    {
        std::lock_guard<std::mutex> lock(testResultsMutex);
        for (const auto& test : matchingTests)
        {
            PendingTest pt;
            pt.testName = test;
            pt.result = "";
            pt.pending = false;
            pt.completed = false;
            pt.expectedBotSpawnCount = TestRegistry::ExpectedBotSpawnCount(test);
            pt.retry = 0;
            pendingTests.push_back(pt);
        }
    }

    std::ostringstream out;
    out << "Queued " << matchingTests.size() << " test(s): ";
    for (size_t i = 0; i < matchingTests.size() && i < 3; i++)
        out << matchingTests[i] << (i < matchingTests.size() - 1 && i < 2 ? ", " : "");
    if (matchingTests.size() > 3)
        out << "...";
    messages.push_back(out.str());

    return messages;
}

void PlayerbotHolder::UpdatePendingTests(uint32 elapsed)
{
    std::lock_guard<std::mutex> lock(testResultsMutex);

    for (auto& pt : pendingTests)
    {
        if (pt.pending)
            continue;

        if (pt.completed)
            continue;

        if (!TestRegistry::HasTest(pt.testName))
        {
            pt.result = "Test not found";
            pt.completed = true;
            continue;
        }

        if (dynamic_cast<PlayerbotMgr*>(this))
        {
            uint32 maxCharsPerAccount = 9;
#ifdef MANGOSBOT_TWO
            maxCharsPerAccount = 10;
#endif
            uint32 accountId = sObjectMgr.GetPlayerAccountIdByGUID((dynamic_cast<PlayerbotMgr*>(this))->GetMaster()->GetObjectGuid());
                if (accountId == 0) continue;

            uint32 currentChars = sAccountMgr.GetCharactersCount(accountId);
            if (currentChars >= maxCharsPerAccount)
                continue;
        }

        static constexpr uint32 maxActiveTestBots = 50;
        uint32 activeTestBots = 0;
        for (const auto& test : pendingTests)
        {
            if (test.pending && !test.completed)
                activeTestBots += std::max<uint32>(1, test.expectedBotSpawnCount);
        }

        uint32 newTestBotCount = std::max<uint32>(1, pt.expectedBotSpawnCount);
        if (activeTestBots + newTestBotCount >= maxActiveTestBots)
            continue;

        std::string createParams = TestRegistry::GetBotCreationRequirement(pt.testName);

        createParams += " login=0 temporary=1 test=" + pt.testName;       

        std::list<std::string> createMsgs = HandleCreate(nullptr, createParams, SEC_PLAYER);

        pt.pending = true;
    }
}

void PlayerbotHolder::DepositTestResult(const std::string& testName, const std::string& result)
{
    std::lock_guard<std::mutex> lock(testResultsMutex);

    for (auto& pt : pendingTests)
    {
        if (!pt.pending)
            continue;

        if (pt.testName == testName && !pt.completed)
        {
            pt.pending = false;
            if (result == "ABORT") //Failed this time but might work next time.
            {
                pt.result = result;
                pt.retry++;
                break;
            }

            pt.result = result;
            pt.completed = true;
            testResults.push_back(pt);
            break;
        }
    }
}
#endif

uint32 PlayerbotHolder::GetOrCreateAccount(Player* master, std::string& error)
{
    if (!master)
    {
        // For console/RA usage without master - delegate to derived class
        error = "GetOrCreateAccount requires master or override in derived class";
        return 0;
    }
    
    uint32 masterAccountId = master->GetSession()->GetAccountId();
    return masterAccountId;
}

void PlayerbotHolder::OnBotDeleted(uint32 botGuid, uint32 accountId)
{
}

bool PlayerbotHolder::DeleteBot(ObjectGuid guid, bool allowInstant)
{
    uint32 botAccount = sObjectMgr.GetPlayerAccountIdByGUID(guid);

    if (Player* player = sObjectMgr.GetPlayer(guid, true))
    {
        //Attempt instant logout.
        player->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_RESTING); 
        LogoutPlayerBot(guid, allowInstant, true);
    }

    Player::DeleteFromDB(guid, botAccount, true, true);

    OnBotDeleted(guid, botAccount);

    return true;
}

std::string PlayerbotHolder::HandleBotDelete(Player* bot, Player* master, const std::string param)
{
    ObjectGuid guid;
    if (!bot)
    {
        if (!Qualified::isValidNumberString(param))
            return "Add: Error parsing " + param;

        guid = ObjectGuid(uint64(std::stoull(param)));
    }
    else
    {
        guid = bot->GetObjectGuid();
    }

    uint32 masterAccountId = master ? master->GetSession()->GetAccountId() : 0;
    PlayerbotMgr* mgr = master ? master->GetPlayerbotMgr() : nullptr;
    
    uint32 botAccount = sObjectMgr.GetPlayerAccountIdByGUID(guid);
    bool isRandomAccount = sPlayerbotAIConfig.IsInRandomAccountList(botAccount);

    if (!isRandomAccount && masterAccountId != botAccount)
        return "Not your bot";

    if (isRandomAccount && mgr == this)
        return "Not your bot";

    DeleteBot(guid);

    return "ok";
}

std::string PlayerbotHolder::HandleBotGear(Player* bot, Player* master, const std::string param)
{
    // Bot may be offline when the command dispatcher couldn't resolve the character to a live
    // Player object (it's in the DB but not in the world). The old code crashed in
    // bot->GetLevel() at HandleBotGear+0x5f (crash_20260528_150911.dmp.txt).
    if (!bot)
        return "Bot is offline. Use `.bot add <name>` first.";

    if (param.empty())
    {
        PlayerbotFactory factory(bot, bot->GetLevel());
        factory.EquipGear();
        return "random gear equipped";
    }
    if (param == "green" || param == "uncommon")
    {
        PlayerbotFactory factory(bot, bot->GetLevel(), ITEM_QUALITY_UNCOMMON);
        factory.EquipGear();
        return "random green gear equipped";
    }
    if (param == "blue" || param == "rare")
    {
        PlayerbotFactory factory(bot, bot->GetLevel(), ITEM_QUALITY_RARE);
        factory.EquipGear();
        return "random blue gear equipped";
    }
    if (param == "purple" || param == "epic")
    {
        PlayerbotFactory factory(bot, bot->GetLevel(), ITEM_QUALITY_EPIC);
        factory.EquipGear();
        return "random epic gear equipped";
    }
    if (param == "upgrade")
    {
        PlayerbotFactory factory(bot, master ? master->GetLevel() : bot->GetLevel(), ITEM_QUALITY_NORMAL);
        factory.UpgradeGear(false);
        return "gear upgraded";
    }
    if (param == "sync")
    {
        PlayerbotFactory factory(bot, master ? master->GetLevel() : bot->GetLevel(), ITEM_QUALITY_NORMAL);
        factory.UpgradeGear(true);
        return "gear upgraded";
    }
    if (param == "best")
    {
        PlayerbotFactory factory(bot, bot->GetLevel());
        factory.EquipGearBest();
        return "random best gear equipped";
    }
    if (param == "partial")
    {
        PlayerbotFactory factory(bot, bot->GetLevel());
        factory.EquipGearPartialUpgrade();
        return "random gear upgraded to some slots";
    }

    return "unknown gear command";
}

std::string PlayerbotHolder::HandleBotTrainLearn(Player* bot, Player* master, const std::string param)
{
#ifndef MANGOSBOT_ONE
    bot->learnClassLevelSpells();
#endif
    return "class level spells learned";
}

std::string PlayerbotHolder::HandleBotFoodDrink(Player* bot, Player* master, const std::string param)
{
    uint32 level = master ? master->GetLevel() : bot->GetLevel();
    PlayerbotFactory factory(bot, level, ITEM_QUALITY_NORMAL);
    factory.AddFood();
    return "food added";
}

std::string PlayerbotHolder::HandleBotPotions(Player* bot, Player* master, const std::string param)
{
    uint32 level = master ? master->GetLevel() : bot->GetLevel();
    PlayerbotFactory factory(bot, level, ITEM_QUALITY_NORMAL);
    factory.AddPotions();
    return "potions added";
}

std::string PlayerbotHolder::HandleBotConsumes(Player* bot, Player* master, const std::string param)
{
    uint32 level = master ? master->GetLevel() : bot->GetLevel();
    PlayerbotFactory factory(bot, level, ITEM_QUALITY_NORMAL);
    factory.AddConsumes();
    return "consumables added";
}

std::string PlayerbotHolder::HandleBotReagents(Player* bot, Player* master, const std::string param)
{
    uint32 level = master ? master->GetLevel() : bot->GetLevel();
    PlayerbotFactory factory(bot, level, ITEM_QUALITY_NORMAL);
    factory.AddReagents();
    return "reagents added";
}

std::string PlayerbotHolder::HandleBotPrepare(Player* bot, Player* master, const std::string param)
{
    uint32 level = master ? master->GetLevel() : bot->GetLevel();
    PlayerbotFactory factory(bot, level, ITEM_QUALITY_NORMAL);
    factory.Refresh();
    return "consumes/regs added";
}

std::string PlayerbotHolder::HandleBotInit(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "Bot is offline. Use `.bot add <name>` first.";
    uint32 level = master ? master->GetLevel() : bot->GetLevel();

    if (param.empty())
    {
        // BUG FIX: passing ITEM_QUALITY_NORMAL (=1) makes InitEquipment query ONLY white
        // items, which for endgame slots means ~1 candidate per slot. That single candidate
        // usually fails one of the many filters (stat weight=0, CanEquipUnseenItem rejects
        // missing proficiency, etc.) → slot stays empty → bot ends up naked (1-3/19 slots).
        //
        // Default ctor (itemQuality=0) sets setQuality=false in InitEquipment, which makes
        // the per-quality loop accumulate items across ALL qualities and sort by stat
        // weight. That gives 100+ candidates per slot and proper gear at the bot's level.
        PlayerbotFactory factory(bot, level);
        factory.Randomize(false, false);
    }
    else if (param == "white" || param == "common")
    {
        PlayerbotFactory factory(bot, level, ITEM_QUALITY_NORMAL);
        factory.Randomize(false, false);
    }
    else if (param == "green" || param == "uncommon")
    {
        PlayerbotFactory factory(bot, level, ITEM_QUALITY_UNCOMMON);
        factory.Randomize(false, false);
    }
    else if (param == "blue" || param == "rare")
    {
        PlayerbotFactory factory(bot, level, ITEM_QUALITY_RARE);
        factory.Randomize(false, false);
    }
    else if (param == "epic" || param == "purple")
    {
        PlayerbotFactory factory(bot, level, ITEM_QUALITY_EPIC);
        factory.Randomize(false, false);
    }
    else if (param == "legendary" || param == "yellow")
    {
        PlayerbotFactory factory(bot, level, ITEM_QUALITY_LEGENDARY);
        factory.Randomize(false, false);
    }
    else if (param == "sync")
    {
        PlayerbotFactory factory(bot, level, ITEM_QUALITY_LEGENDARY);
        factory.Randomize(false, true);
    }

    // Diagnostic: how many slots ended up populated after the randomize path. If this
    // returns 0/1 for a bot we expected to be geared, InitEquipment is bailing early
    // (spec lookup, missing items in DB, etc.) and Prepare Raid will leave it naked.
    uint32 filled = 0;
    for (uint8 s = EQUIPMENT_SLOT_HEAD; s < EQUIPMENT_SLOT_END; ++s)
    {
        if (bot->GetItemByPos(INVENTORY_SLOT_BAG_0, s))
            filled++;
    }
    sLog.outString("[INIT] %s (cls=%u lvl=%u): %u/19 equipment slots filled after Randomize",
                   bot->GetName(), bot->getClass(), bot->GetLevel(), filled);

    std::ostringstream r;
    r << "init: " << filled << "/19 slots equipped";
    return r.str();
}

std::string PlayerbotHolder::HandleBotEnchants(Player* bot, Player* master, const std::string param)
{
    PlayerbotFactory factory(bot, bot->GetLevel(), ITEM_QUALITY_LEGENDARY);
    factory.EnchantEquipment();
    return "ok";
}

std::string PlayerbotHolder::HandleBotAmmo(Player* bot, Player* master, const std::string param)
{
    PlayerbotFactory factory(bot, bot->GetLevel(), ITEM_QUALITY_LEGENDARY);
    factory.InitAmmo();
    return "ok";
}

std::string PlayerbotHolder::HandleBotPet(Player* bot, Player* master, const std::string param)
{
    PlayerbotFactory factory(bot, bot->GetLevel(), ITEM_QUALITY_LEGENDARY);
    factory.InitPet();
    factory.InitPetSpells();
    return "ok";
}

std::string PlayerbotHolder::HandleBotLevelUp(Player* bot, Player* master, const std::string param)
{
    PlayerbotFactory factory(bot, bot->GetLevel());
    factory.Randomize(true, false);
    return "ok";
}

std::string PlayerbotHolder::HandleBotRefresh(Player* bot, Player* master, const std::string param)
{
    PlayerbotFactory factory(bot, bot->GetLevel());
    factory.Refresh();
    return "ok";
}

std::string PlayerbotHolder::HandleBotRandom(Player* bot, Player* master, const std::string param)
{
    sRandomPlayerbotMgr.Randomize(bot);
    return "ok";
}

// `.bot revive [name]` — resurrect dead bots.
// - Without `name`: revive ALL dead bots that are currently in the master's group/raid.
// - With `name`: revive that specific bot (still must be in master's group).
// The MaNGOS-correct revive sequence is: ResurrectPlayer(percent) → SpawnCorpseBones() →
// SetHealth(GetMaxHealth()) → optionally restore mana. SetHealth + SpawnCorpseBones must
// happen after ResurrectPlayer or the player object stays in ghost form.
std::string PlayerbotHolder::HandleBotRevive(Player* bot, Player* master, const std::string param)
{
    if (!master)
        return "No master session";

    Group* group = master->GetGroup();
    auto reviveOne = [](Player* p) -> bool
    {
        if (!p) return false;
        if (p->IsAlive()) return false;
        p->ResurrectPlayer(1.0f);
        p->SpawnCorpseBones();
        p->SetHealth(p->GetMaxHealth());
        if (p->GetPowerType() == POWER_MANA)
            p->SetPower(POWER_MANA, p->GetMaxPower(POWER_MANA));
        return true;
    };

    // Single-bot path: `.bot revive Drukorn`
    if (bot)
    {
        if (!group || !group->IsMember(bot->GetObjectGuid()))
            return std::string(bot->GetName()) + " is not in your group.";
        if (!reviveOne(bot))
            return std::string(bot->GetName()) + " is already alive.";
        return std::string(bot->GetName()) + " revived.";
    }

    // Group-wide path: `.bot revive` (no bot specified)
    if (!group)
        return "You are not in a group. Use `.bot revive <name>` for a single bot.";

    uint32 revived = 0;
    for (auto const& slot : group->GetMemberSlots())
    {
        Player* member = sObjectMgr.GetPlayer(slot.guid);
        if (!member || member == master) continue;
        if (reviveOne(member))
            revived++;
    }
    std::ostringstream out;
    out << "Revived " << revived << " bot(s) in your group.";
    return out.str();
}

// `.bot bis <name>` — apply pre-raid BiS gear to a level-60 bot.
//
// Safe sequence (avoiding the heap-corruption pattern observed when inlined into
// PlayerbotFactory::InitEquipment):
//   1. CanEquipNewItem to validate the slot can accept the BiS itemId without
//      conflicting with currently-equipped items or class restrictions.
//   2. Destroy the existing item in that slot.
//   3. EquipNewItem (atomic create + equip in one call).
//
// Items missing from the BiS table (entries == 0) or missing from the DB
// (sObjectMgr.GetItemPrototype returns null) are skipped. Bots below lvl 60 also
// skipped — BiS sets are only defined for level 60 pre-raid right now.
std::string PlayerbotHolder::HandleBotBis(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "Bot is offline.";
    if (bot->GetLevel() < 60)
        return std::string(bot->GetName()) + " must be lvl 60 to apply BiS.";

    uint8 cls = bot->getClass();

    // Detect bot's talent spec via PlayerbotAI::GetTalentSpec(), which counts
    // talents per tab and picks the highest. The returned enum follows
    // ((class * 3) - 2) + tab, so we recover the tab index (0/1/2) by taking
    // (enum_value - 1) % 3 for classes 1-9, and (enum - 31) for druid (11).
    // SpecKey(cls, tab) is what PlayerbotBiS.h uses to differentiate
    // tank/heal/dps gear within the same class.
    uint8 spec = 0;
    if (PlayerbotAI* botAI = bot->GetPlayerbotAI())
    {
        PlayerTalentSpec ts = botAI->GetTalentSpec();
        uint8 raw = static_cast<uint8>(ts);
        if (raw >= 1 && raw <= 30)
            spec = (raw - 1) % 3;
        else if (raw >= 31 && raw <= 33)
            spec = raw - 31;  // druid tab block starts at 31
    }
    uint32 equipped = 0;
    uint32 skippedNoItem = 0;
    uint32 skippedNoSlot = 0;
    uint32 skippedSame = 0;

    for (uint8 slot = EQUIPMENT_SLOT_HEAD; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        uint32 itemId = mcwow_bis::GetItem(cls, spec, slot);
        if (!itemId) continue;
        ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
        if (!proto) { skippedNoItem++; continue; }

        // Skip if same item already equipped (avoid churning the same gear).
        if (Item* existing = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            if (existing->GetEntry() == itemId) { skippedSame++; continue; }

        // CanEquipNewItem (swap=true) returns dest as the destination FOR THE NEW ITEM.
        // The displaced item should be moved by the underlying inventory code. In practice
        // we observed cases where EquipNewItem returned a non-null Item* but the new entry
        // didn't actually land in the target equipment slot — the old item stayed in place
        // and the new item ended up somewhere in the bags (or worse, in a "limbo" state
        // that the periodic save layer then discarded). The BiS log claimed "equipped 17"
        // but `SELECT * FROM character_inventory WHERE slot=0` still showed the old gear.
        //
        // Robust approach:
        //   1. Pre-validate CanEquipNewItem(swap=true) BEFORE touching the existing
        //      item. If the bot can't equip the BiS item for any reason (class
        //      proficiency, allowable_class, missing item proto, etc.) we leave the
        //      slot untouched — observed naked hunters (Grunthar, Trollbane lost
        //      their ranged weapon entirely) when the previous "destroy first,
        //      check after" path failed validation on step 2.
        //   2. Only then destroy the old item.
        //   3. Re-call CanEquipNewItem with swap=false (now slot is empty).
        //   4. EquipNewItem into the empty slot.
        //   5. Verify the slot actually contains the new entry.
        uint16 dest = 0;
        InventoryResult pre = bot->CanEquipNewItem(slot, dest, itemId, true);
        if (pre != EQUIP_ERR_OK) { skippedNoSlot++; continue; }

        Item* oldItem = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (oldItem)
            bot->DestroyItem(oldItem->GetBagSlot(), oldItem->GetSlot(), true);

        InventoryResult res = bot->CanEquipNewItem(slot, dest, itemId, false);
        if (res != EQUIP_ERR_OK)
        {
            // Shouldn't happen — pre-check already returned OK and the slot is
            // now empty. Defensive only; log so we can spot it if it ever fires.
            sLog.outError("[BIS] %s: slot %u went from OK to err=%d after destroy (item=%u)",
                          bot->GetName(), (uint32)slot, (int)res, itemId);
            skippedNoSlot++;
            continue;
        }

        Item* newItem = bot->EquipNewItem(dest, itemId, true);

        // Post-equip verification: did the item ACTUALLY land in the destination slot?
        Item* nowInSlot = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (newItem && nowInSlot && nowInSlot->GetEntry() == itemId)
        {
            equipped++;
        }
        else
        {
            sLog.outError("[BIS] %s: post-equip mismatch slot=%u expected=%u actual=%u (newItem=%p)",
                          bot->GetName(), (uint32)slot, itemId,
                          nowInSlot ? nowInSlot->GetEntry() : 0u, (void*)newItem);
            skippedNoSlot++;
        }
    }

    std::ostringstream out;
    out << "BiS: equipped " << equipped << ", skipped "
        << skippedSame << " same, " << skippedNoItem << " missing, " << skippedNoSlot << " incompat";
    if (equipped > 0)
        sLog.outString("[BIS] %s: %s", bot->GetName(), out.str().c_str());

    // After BiS may have swapped the ranged weapon, the ammo in the quiver no longer
    // matches the new weapon type (e.g. init gave a gun + bullets, BiS replaced with
    // a bow → bullets are now useless and the hunter can't shoot). Re-run InitAmmo
    // to top up the right kind. InitAmmo no-ops for classes that don't use ammo, so
    // calling unconditionally is fine.
    {
        PlayerbotFactory ammoFactory(bot, bot->GetLevel());
        ammoFactory.InitAmmo();
    }
    return out.str();
}

// Shared overlay routine for all four resistance schools. The school determines
// which gear table is read and which prototype field contributes to the total.
// Returns a short summary string for the chat reply, and emits a [RES] log line.
static std::string OverlayResistSet(Player* bot, mcwow_resist::School school)
{
    if (!bot)
        return "Bot is offline.";
    if (bot->GetLevel() < 60)
        return std::string(bot->GetName()) + " must be lvl 60 for resist set.";

    const uint8 cls = bot->getClass();
    uint32 equipped = 0, skippedSame = 0, skippedNoItem = 0, skippedNoSlot = 0;
    uint32 totalRes = 0;

    auto resOf = [&](ItemPrototype const* p) -> uint32 {
        switch (school) {
            case mcwow_resist::School::Fire:   return p->FireRes;
            case mcwow_resist::School::Frost:  return p->FrostRes;
            case mcwow_resist::School::Nature: return p->NatureRes;
            case mcwow_resist::School::Shadow: return p->ShadowRes;
        }
        return 0;
    };

    for (uint8 slot = 0; slot < 15; ++slot)
    {
        uint32 itemId = mcwow_resist::GetItem(school, cls, 0, slot);
        if (!itemId) continue;
        ItemPrototype const* proto = sObjectMgr.GetItemPrototype(itemId);
        if (!proto) { skippedNoItem++; continue; }

        if (Item* existing = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            if (existing->GetEntry() == itemId) { skippedSame++; totalRes += resOf(proto); continue; }

        // Same pattern as HandleBotBis: pre-check CanEquipNewItem(swap=true) BEFORE
        // touching the existing item so we don't strip a valid old item when the new
        // one can't actually be equipped.
        uint16 dest = 0;
        InventoryResult pre = bot->CanEquipNewItem(slot, dest, itemId, true);
        if (pre != EQUIP_ERR_OK) { skippedNoSlot++; continue; }

        if (Item* old = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            bot->DestroyItem(old->GetBagSlot(), old->GetSlot(), true);

        InventoryResult res = bot->CanEquipNewItem(slot, dest, itemId, false);
        if (res != EQUIP_ERR_OK) { skippedNoSlot++; continue; }

        Item* newItem = bot->EquipNewItem(dest, itemId, true);
        Item* nowInSlot = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (newItem && nowInSlot && nowInSlot->GetEntry() == itemId)
        {
            equipped++;
            totalRes += resOf(proto);
        }
        else
        {
            skippedNoSlot++;
        }
    }

    const char* schoolName = mcwow_resist::SchoolName(school);
    std::ostringstream out;
    out << schoolName << " Res: equipped " << equipped << " (~" << totalRes << "), skipped "
        << skippedSame << " same, " << skippedNoItem << " missing, " << skippedNoSlot << " incompat";
    if (equipped > 0)
        sLog.outString("[RES-%s] %s: %s", schoolName, bot->GetName(), out.str().c_str());
    return out.str();
}

// Thin per-school aliases. The addon's Apply Resist picker maps each button
// (Fire/Frost/Nature/Shadow) to one of these so it can use the standard
// `.bot <cmd> <name>` dispatcher with `*` as the target. (The `.bot resist
// <school> <name>` form was attempted first but the dispatcher consumed
// "<school>" as the bot name — character not found.)
std::string PlayerbotHolder::HandleBotFR(Player* bot, Player* master, const std::string param)
{
    return OverlayResistSet(bot, mcwow_resist::School::Fire);
}
std::string PlayerbotHolder::HandleBotFrostRes(Player* bot, Player* master, const std::string param)
{
    return OverlayResistSet(bot, mcwow_resist::School::Frost);
}
std::string PlayerbotHolder::HandleBotNatRes(Player* bot, Player* master, const std::string param)
{
    return OverlayResistSet(bot, mcwow_resist::School::Nature);
}
std::string PlayerbotHolder::HandleBotShadowRes(Player* bot, Player* master, const std::string param)
{
    return OverlayResistSet(bot, mcwow_resist::School::Shadow);
}

// `.bot resist <fire|frost|nature|shadow> <name>` — generic resistance overlay. The
// addon's Apply Resist dropdown wires each choice through this command. Unknown school
// strings fall back to Fire (most common) with a hint in the reply.
std::string PlayerbotHolder::HandleBotResist(Player* bot, Player* master, const std::string param)
{
    // param contains the school name as the first whitespace-delimited token (the
    // dispatcher strips off the bot-name target already).
    mcwow_resist::School school = mcwow_resist::School::Fire;
    std::string token = param;
    size_t sp = token.find(' ');
    if (sp != std::string::npos) token = token.substr(0, sp);
    if (!token.empty() && !mcwow_resist::ParseSchool(token, school))
    {
        // Unknown school: tell the caller which ones we accept, don't apply anything.
        return "Usage: .bot resist <fire|frost|nature|shadow>";
    }
    return OverlayResistSet(bot, school);
}

// `.bot inspect <name>` — dump the bot's currently-equipped gear to chat.
// Replaces the vanilla right-click → Inspect flow which is gated on UnitInParty
// (only works for the 5-man party, not the whole 40-raid). Returns a multi-line
// summary slot-by-slot: name, quality letter, item level. Master gets the lines
// via the standard command-reply path; the bot dispatch already routes the return
// value into a system message visible to master only.
std::string PlayerbotHolder::HandleBotInspectGear(Player* bot, Player* master, const std::string param)
{
    if (!bot)
        return "Bot is offline.";

    static const char* slotNames[19] = {
        "Head", "Neck", "Shoulders", "Shirt", "Chest", "Waist",
        "Legs", "Feet", "Wrists", "Hands", "Finger1", "Finger2",
        "Trinket1", "Trinket2", "Back", "MH", "OH", "Ranged", "Tabard"
    };
    static const char qLetter[6] = { 'g', 'w', 'G', 'B', 'P', 'O' };  // grey/white/green/blue/purple/orange

    std::ostringstream out;
    out << bot->GetName() << " gear:";
    uint32 totalIL = 0;
    uint32 nFilled = 0;
    for (uint8 slot = 0; slot < 19; ++slot)
    {
        Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item) continue;
        ItemPrototype const* proto = item->GetProto();
        if (!proto) continue;
        char q = (proto->Quality < 6) ? qLetter[proto->Quality] : '?';
        out << "\n  [" << slotNames[slot] << "] " << proto->Name1
            << " (" << q << " ilvl " << (uint32)proto->ItemLevel << ")";
        totalIL += proto->ItemLevel;
        nFilled++;
    }
    out << "\n  -> " << nFilled << "/19 slots, avg ilvl "
        << (nFilled ? totalIL / nFilled : 0u);
    return out.str();
}

std::string PlayerbotHolder::GetCommandTexts(const std::string& command)
{
    auto texts = GetCommandTexts();
    auto it = texts.find(command);
    if (it != texts.end())
        return it->second;
    return "";
}

std::unordered_map<std::string, std::string> PlayerbotHolder::GetCommandTexts()
{
    return std::unordered_map<std::string, std::string>
    {
        // Holder commands (used with .(rnd)bot)
        {"list", "List all active player bots.\nUsage: .(rnd)bot list"},
        {"help", "Show help for commands.\nUsage: .(rnd)bot help <command>"},
        {"reload", "Reload the playerbot config (GM only).\nUsage: .(rnd)bot reload"},
        {"tweak", "Adjust the tweak value for testing (GM only).\nUsage: .(rnd)bot tweak"},
        {"self", "Enable self-bot mode for a player.\nUsage: .(rnd)bot self <playername>"},
        {"group", "Create 4 bots with complementary classes at master's level.\nUsage: .(rnd)bot group"},
        {"create", "Create a new bot character.\nUsage: .(rnd)bot create level=<n> class=<class> race=<race>"},
        {"spoof", "Spoof as another bot for command routing.\nUsage: .(rnd)bot spoof <botname>"},
        {"runtest", "Run bot tests.\nUsage: .rndbot runtest <testnamepart> [count]"},
        
        // Bot commands (used with .(rnd)bot <bot> ...)
        {"add", "Add a bot to the player's group.\nUsage: .(rnd)bot add <playername>"},
        {"login", "Add a bot to the player's group.\nUsage: .(rnd)bot login <playername>"},
        {"remove", "Remove a bot from the player's group.\nUsage: .(rnd)bot remove <botname>"},
        {"logout", "Remove a bot from the player's group.\nUsage: .(rnd)bot logout <botname>"},
        {"rm", "Remove a bot from the player's group.\nUsage: .(rnd)bot rm <botname>"},
        {"delete", "Delete a bot character.\nUsage: .(rnd)bot delete <botname>"},
        
        {"gear", "Equip best gear on bot.\nUsage: .(rnd)bot gear <bot> "},
        {"equip", "Equip best gear on bot.\nUsage: .(rnd)bot equip  <bot> "},
        
        {"train", "Train bot spells at trainer.\nUsage: .(rnd)bot train <bot> "},
        {"learn", "Train bot spells at trainer.\nUsage: .(rnd)bot learn <bot> "},
        
        {"food", "Buy food/drink for bot.\nUsage: .(rnd)bot food <bot> "},
        {"drink", "Buy food/drink for bot.\nUsage: .(rnd)bot drink <bot> "},
        
        {"potions", "Buy potions for bot.\nUsage: .(rnd)bot potions <bot> "},
        {"pots", "Buy potions for bot.\nUsage: .(rnd)bot pots <bot> "},
        
        {"consumes", "Buy all consumables for bot.\nUsage: .(rnd)bot consumes <bot> "},
        {"consumables", "Buy all consumables for bot.\nUsage: .(rnd)bot consumables <bot> "},
        
        {"regs", "Buy reagents for bot.\nUsage: .(rnd)bot regs <bot> "},
        {"reg", "Buy reagents for bot.\nUsage: .(rnd)bot reg <bot> "},
        {"reagents", "Buy reagents for bot.\nUsage: .(rnd)bot reagents  <bot> "},
        
        {"prepare", "Prepare bot (gear, food, pots, etc).\nUsage: .(rnd)bot prepare <bot> "},
        {"prep", "Prepare bot (gear, food, pots, etc).\nUsage: .(rnd)bot prep <bot>"},
        {"refresh", "Refresh bot gear and items.\nUsage: .(rnd)bot refresh <bot> "},
        
        {"init", "Initialize bot with default actions.\nUsage: .(rnd)bot init <bot> "},
        
        {"enchants", "Apply enchants to bot's gear.\nUsage: .(rnd)bot enchants <bot> "},
        
        {"ammo", "Buy ammo for bot.\nUsage: .(rnd)bot ammo <bot> "},
        
        {"pet", "Summon/dismiss pet for bot.\nUsage: .(rnd)bot pet <bot> "},
        
        {"levelup", "Level up bot.\nUsage: .(rnd)bot levelup <bot>"},
        {"level", "Level up bot.\nUsage: .(rnd)bot level <bot>"},
        
        {"random", "Randomize bot appearance and gear.\nUsage: .(rnd)bot random <bot>"},
        
        {"always", "Enable offline AI for a player.\nUsage: .(rnd)bot always <playername>"},
        
        {"debug", "Run debug commands on the bot (GM only).\nUsage: .(rnd)bot debug <bot> <command>"},
        
        {"c", "Execute a chat command on the bot.\nUsage: .(rnd)bot c <bot> <command>"},
        
        {"w", "Send a whisper.\nUsage: .(rnd)bot w <bot> <message> (while spoofing as sender)\nUsage: .(rnd)bot <sender> <reciever> "},
        
        {"p", "Send a party message as the bot.\nUsage: .(rnd)bot p <message> (while spoofing as sender)\n .(rnd)bot p <botname> <message>\nNote: No message = party info.\nExample: .rndbot p Dunpriest (shows party info)"},
        
        {"g", "Send a guild message as the bot.\nUsage: .(rnd)bot g <message> (while spoofing as sender)\n .(rnd)bot g <botname> <message>\nNote: No message = guild info.\nExample: .rndbot g Dunpriest (shows guild info)"},
        
        {"r", "Send a raid message as the bot.\nUsage: .(rnd)bot r <message> (while spoofing as sender)\n .(rnd)bot r <botname> <message>"},
        
        {"rl", "Transfer raid leadership.\nUsage: .(rnd)bot rl <message> (while spoofing as sender)\n .(rnd)bot rl <botname>"},
        
        {"do", "Execute a bot action (sync, immediate response).\nUsage: .(rnd)bot do <bot> <action>\nExample: .(rnd)bot do <bot> stats, where, quests, who"},
        
        {"cmd", "Execute a bot action (async, queued).\nUsage: .(rnd)bot cmd <bot> do <action>\nNote: Use with record to capture output."},
        
        {"record", "Enable message recording for async commands.\nUsage: .(rnd)bot record <bot> enable\nUsage: .(rnd)bot record <bot> disable"},
        
        {"read", "Get recorded async command output.\nUsage: .(rnd)bot read <bot>"},
        
        {"clear", "Clear recorded messages without retrieving.\nUsage: .(rnd)bot clear <bot>"},
        
        {"spoof", "Spoof as another bot for command routing.\nUsage: .(rnd)bot spoof <botname>\nUsage: .(rnd)bot spoof (to clear)"}
    };
}

std::list<std::string> PlayerbotHolder::HandleSpoof(Player* master, const std::string param, AccountTypes security)
{
    std::list<std::string> messages;
    
    if (param.empty())
    {
        // Clear the spoof
        if (m_spoofGuid)
        {
            std::string playerName;
            if (sObjectMgr.GetPlayerNameByGUID(m_spoofGuid, playerName))
            {
                messages.push_back("Spoof cleared. Was spoofing: " + playerName);
            }
            else
            {
                messages.push_back("Spoof cleared.");
            }
            m_spoofGuid = ObjectGuid();
        }
        else
        {
            messages.push_back("Spoof is not set.");
        }
        return messages;
    }
    
    // Look up player by name
    ObjectGuid guid = sObjectMgr.GetPlayerGuidByName(param);
    if (!guid)
    {
        messages.push_back("Player '" + param + "' not found.");
        return messages;
    }
    
    // Get the player to verify they exist
    Player* player = sObjectMgr.GetPlayer(guid, false);
    if (!player)
    {
        messages.push_back("Player '" + param + "' found but is not online.");
        return messages;
    }
    
    std::string playerName;
    sObjectMgr.GetPlayerNameByGUID(guid, playerName);
    m_spoofGuid = guid;
    
    messages.push_back("Spoof set to: " + playerName + " (" + std::to_string(guid.GetCounter()) + ")");
    return messages;
}
