// Host-side glue for bot lifecycle and dispatch. Implements:
//   - Player::{Create,Remove}Playerbot{AI,Mgr}, Player::isRealPlayer
//   - Player::UpdatePlayerbotHooks (per-Player tick)
//   - World::{Update,Init}Playerbots* (world-tick driver, startup init)
//   - Player_DispatchBotOutgoing{Packet,ChatCommand} (free functions called
//     from WorldSession; the bot-AI null-check happens here so the host
//     call sites stay unconditional)
//
// Lives in the bot module so it sees both the host headers and the bot
// module's full PlayerbotAI / PlayerbotMgr types — the host declares the
// methods, only the bot module satisfies the linker with real bodies. The
// matching BUILD_PLAYERBOTS=OFF stubs live in src/game/PlayerbotStubs.cpp.

#include "playerbot/playerbot.h"
#include "Objects/Player.h"
#include "World.h"
#include "playerbot/RandomPlayerbotMgr.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "BotDiagnostics.h"

void Player::CreatePlayerbotAI()
{
    if (!m_playerbotAI)
        m_playerbotAI = new PlayerbotAI(this);
}

void Player::RemovePlayerbotAI()
{
    if (m_playerbotAI)
    {
        delete m_playerbotAI;
        m_playerbotAI = nullptr;
    }
}

void Player::CreatePlayerbotMgr()
{
    if (!m_playerbotMgr)
        m_playerbotMgr = new PlayerbotMgr(this);
}

void Player::RemovePlayerbotMgr()
{
    if (m_playerbotMgr)
    {
        // Log out the master's alt bots first; otherwise their PlayerbotAI
        // outlives the mgr and they linger in-world with a dangling master.
        m_playerbotMgr->LogoutAllBots();
        delete m_playerbotMgr;
        m_playerbotMgr = nullptr;
    }
}

bool Player::isRealPlayer() const
{
    return !m_playerbotAI || m_playerbotAI->IsRealPlayer();
}

// World-tick driver. RandomPlayerbotMgr ticks both the login queue and every
// active bot's PlayerbotAI.
void World::UpdatePlayerbotsTick(uint32 diff)
{
    if (!sPlayerbotAIConfig.enabled)
        return;

    static bool firstTick = true;
    static uint32 crashCount = 0;
    static uint32 ticksSinceLastCrash = 0;

    if (firstTick)
    {
        sLog.outString("[PLAYERBOTS] First update tick starting...");
        firstTick = false;
    }

    __try
    {
        sRandomPlayerbotMgr.UpdateAI(diff);
        ticksSinceLastCrash++;
        if (crashCount > 0 && ticksSinceLastCrash > 100)
            crashCount = 0;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        extern uint32 RandomPlayerbotMgr_GetCrashPhase();
        crashCount++;
        ticksSinceLastCrash = 0;
        if (crashCount <= 5 || crashCount % 1000 == 0)
            sLog.outError("[PLAYERBOTS] UpdateAI crash #%u at phase %u - will retry next tick.", crashCount, RandomPlayerbotMgr_GetCrashPhase());
    }
}

// Per-Player bot tick:
//  - if `this` is a bot (m_playerbotAI != null), tick the AI
//  - if `this` is a real master driving bots (m_playerbotMgr != null), tick
//    the mgr so its alt-bot squad responds to the master's actions
//
// SC_PHASE tags around each call site let the crash handler identify which
// call faulted when bots self-destruct mid-tick (logout, far-teleport).
// Engine helpers — defined in Engine.cpp. Used here to gate UpdateAI on AI initialization
// state so we never enter UpdateAI for an AI whose OnBotLogin hasn't completed yet
// (that produces use-of-half-built-context AVs → heap corruption).
extern bool Engine_IsAIReady(PlayerbotAI* ai);

static bool SafeCall_BotAI(PlayerbotAI* ai, uint32 diff)
{
    // Skip AI ticks entirely for bots still mid-OnBotLogin. Their context/strategies are
    // half-built; calling UpdateAI on them produces AVs whose SEH catches the symptom but
    // leaves the heap inconsistent. Eliminating these AVs at root removes the 0xc0000374
    // we've been chasing for hours.
    if (!Engine_IsAIReady(ai))
        return true;
    __try { ai->UpdateAI(diff); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
static bool SafeCall_BotMgr(PlayerbotMgr* mgr, uint32 diff)
{
    __try { mgr->UpdateAI(diff); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}

void Player::UpdatePlayerbotHooks(uint32 diff)
{
    if (!sPlayerbotAIConfig.enabled)
        return;
    if (m_playerbotAI)
    {
        if (!SafeCall_BotAI(m_playerbotAI, diff))
        {
            extern uint32 PlayerbotAI_GetCrashPhase();
            extern const char* Engine_GetLastActionName();
            static uint32 botCrashLogCount = 0;
            if (++botCrashLogCount <= 10)
                sLog.outError("[PLAYERBOTS] Bot %s AI crash phase=%u action='%s'", GetName(), PlayerbotAI_GetCrashPhase(), Engine_GetLastActionName());
        }
    }
    if (m_playerbotMgr)
    {
        if (!SafeCall_BotMgr(m_playerbotMgr, diff))
            sLog.outError("[PLAYERBOTS] Player %s bot manager tick crashed - continuing", GetName());
    }
}

// One-shot startup init. Singleton bot managers (RandomPlayerbotMgr,
// PlayerBotLoginMgr, etc.) lazy-instantiate on first reference; we just
// need the config file loaded here. No-op when AiPlayerbot.Enabled=0.
void World::InitPlayerbotsAtStartup()
{
    __try
    {
        sPlayerbotAIConfig.Initialize();
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        sLog.outError("[PLAYERBOTS] CRASH during Initialize()! Bot init incomplete but system stays enabled.");
    }
}

// Outgoing-packet interceptor (called from WorldSession::SendPacket). For a
// real player returns false → packet goes to the network normally. For a
// bot Player returns true after handing the packet to the AI to react to
// (group invites → auto-accept, BG status, vendor errors, ...); the AI
// suppresses the network send.
bool Player_DispatchBotOutgoingPacket(Player* player, WorldPacket const& packet)
{
    if (!player) return false;
    PlayerbotAI* ai = player->GetPlayerbotAI();
    if (!ai) return false;
    ai->HandleBotOutgoingPacket(packet);
    return true;
}

static void SafeCall_PlayerbotMgrOnLogin(PlayerbotMgr* mgr, Player* player)
{
    __try { mgr->OnPlayerLogin(player); }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        sLog.outError("[PLAYERBOTS] CRASH in PlayerbotMgr::OnPlayerLogin for %s", player ? player->GetName() : "?");
    }
}

static void SafeCall_RndPlayerbotMgrOnLogin(Player* player)
{
    __try { sRandomPlayerbotMgr.OnPlayerLogin(player); }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        sLog.outError("[PLAYERBOTS] CRASH in RandomPlayerbotMgr::OnPlayerLogin for %s", player ? player->GetName() : "?");
    }
}

void Player_OnPlayerbotLogin(Player* player)
{
    if (!player || !sPlayerbotAIConfig.enabled)
        return;

    PlayerbotMgr* mgr = player->GetPlayerbotMgr();
    if (mgr)
        SafeCall_PlayerbotMgrOnLogin(mgr, player);

    SafeCall_RndPlayerbotMgrOnLogin(player);
}

// Chat dispatcher: feeds the master's chat to every bot that listens. Without
// this, in-party "+heal" / "stay" / "co" commands don't reach any bot. Bots
// owned by the master and matching random bots both get the message.
void Player_DispatchBotChatCommand(Player* master, uint32 type, std::string const& msg, uint32 lang)
{
    if (!master || !sPlayerbotAIConfig.enabled)
        return;

    if (PlayerbotMgr* mgr = master->GetPlayerbotMgr())
        mgr->HandleCommand(type, msg, lang);

    sRandomPlayerbotMgr.HandleCommand(type, msg, *master, "", master->GetTeam(), lang);
}
