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
#include "playerbot/AiFactory.h"
#include "playerbot/strategy/actions/ChangeTalentsAction.h"

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
            // extern "C" linkage block must be at file scope; we use a
            // forward declaration with implicit C++ linkage here. The
            // implementation in Engine.cpp is marked `extern "C"` but the
            // declaration site can match name-only (different scopes, same
            // mangled symbol since extern "C" disables mangling).
            extern const char* PlayerbotAI_GetLastTriggerName();
            static uint32 botCrashLogCount = 0;
            if (++botCrashLogCount <= 10)
            {
                uint32 phase = PlayerbotAI_GetCrashPhase();
                // Phase 9331 = ProcessTriggers loop. The last-set trigger
                // name is the culprit. For other phases, last action is more
                // useful (set behind, move, etc.).
                if (phase == 9331)
                    sLog.outError("[PLAYERBOTS] Bot %s AI crash phase=%u trigger='%s' lastAction='%s'",
                        GetName(), phase, PlayerbotAI_GetLastTriggerName(), Engine_GetLastActionName());
                else
                    sLog.outError("[PLAYERBOTS] Bot %s AI crash phase=%u action='%s'",
                        GetName(), phase, Engine_GetLastActionName());
            }
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

uint8 Playerbot_GetAllowedRoles(Player* bot)
{
    if (!bot || !bot->GetPlayerbotAI())
        return 0;

    // BotRoles and LFT_ROLE_* share their bit values - tank 1, healer 2, dps 4 -
    // so the mask carries over unchanged.
    return uint8(AiFactory::GetPlayerRoles(bot));
}

void Playerbot_SetForcedRole(Player* bot, uint8 role)
{
    if (!bot)
        return;

    PlayerbotAI* ai = bot->GetPlayerbotAI();
    if (!ai)
        return;

    if (ai->GetForcedRole() == role)
        return;

    ai->SetForcedRole(role);

    // Strategies alone are not enough. A balance druid handed the tank slot
    // keeps balance talents and tanks in caster form; AutoSelectTalents does
    // know about roles, but only when picking a spec for the first time - with
    // one already stored it continues that one and the role never comes up.
    //
    // So when the tree cannot fill the role at all, drop the stored choice and
    // let it choose again with the role in hand. Only for random bots: someone
    // else's bot keeps the spec its owner gave it. And only on a real
    // contradiction - a feral druid does not respec just because it alternates
    // between tanking and dps.
    if (role != BotRoles::BOT_ROLE_NONE &&
        bot->GetLevel() >= 10 &&
        sRandomPlayerbotMgr.IsRandomBot(bot))
    {
        BotRoles const current = AiFactory::GetPlayerRoles(bot);

        // And only when the class can actually reach the role. AutoSelectTalents
        // does not give up if no premade spec matches: it falls back to every
        // spec of the class and picks one. So a shaman told to tank had its
        // talents wiped and came back as restoration - worse off than before,
        // and still not a tank. Ask first, and leave the bot alone if the answer
        // is no.
        bool const reachable =
            !ChangeTalentsAction::getPremadePaths(bot->getClass(), "", (BotRoles)role).empty();

        if (!(current & role) && reachable)
        {
            sRandomPlayerbotMgr.SetValue(bot->GetGUIDLow(), "specNo", 0);
            sRandomPlayerbotMgr.SetValue(bot->GetGUIDLow(), "specLink", 0, "");

            bot->ResetTalents(true);

            std::ostringstream out;
            ChangeTalentsAction::AutoSelectTalents(bot, &out, (BotRoles)role);

            sLog.outBasic("LFT: %s respecced for role %u: %s",
                bot->GetName(), uint32(role), out.str().c_str());
        }
    }

    ai->ResetStrategies();
}
