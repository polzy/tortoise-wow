#include "playerbot/playerbot.h"
#include "BotStatusBroadcaster.h"
#include "PlayerbotAI.h"
#include "strategy/Engine.h"
#include "strategy/Action.h"

#include <sstream>

// Build a snapshot of the bot's CURRENT engine's strategies (the engine that
// is actively driving the bot — combat/non-combat/reaction/dead). For the
// addon panel, what the bot is doing RIGHT NOW is what matters; per-state
// breakdown is left for a future verbose-mode payload.
//
// Format: "<state>:strat1,strat2,strat3"
//   e.g. "COMBAT:onyxia,dps assist,reach spell"
//        "NON_COMBAT:passive,follow master"
static std::string BuildSnapshot(PlayerbotAI* botAi)
{
    if (!botAi)
        return std::string();

    ai::Engine* eng = botAi->GetCurrentEngine();
    if (!eng)
        return std::string();

    std::ostringstream out;
    out << PlayerbotAI::BotStateToString(botAi->GetState()) << ':';

    bool first = true;
    for (std::string_view const& name : eng->GetStrategies())
    {
        if (!first) out << ',';
        out << name;
        first = false;
    }
    return out.str();
}

void BotStatusBroadcaster::SendToMaster(PlayerbotAI* botAi, std::string const& payload)
{
    if (!botAi)
        return;
    Player* bot = botAi->GetBot();
    if (!bot)
        return;
    Player* master = botAi->GetMaster();
    if (!master || master == bot)
        return;
    if (!master->GetSession())
        return;

    // Build a CHAT_MSG_WHISPER packet directly so we can stamp the bot as
    // the sender. The addon side filters on the "MCWBS\t" prefix and
    // suppresses the chat display, so the master never sees raw payloads.
    std::string msg = "MCWBS\t" + payload;

    WorldPacket data;
    ChatHandler::BuildChatPacket(
        data,
        CHAT_MSG_WHISPER,
        msg.c_str(),
        LANG_UNIVERSAL,
        bot->GetChatTag(),
        bot->GetObjectGuid(),
        bot->GetName());
    master->GetSession()->SendPacket(data);
}

bool BotStatusBroadcaster::BroadcastStrategies(PlayerbotAI* botAi, std::string& lastSentSnapshot)
{
    std::string snap = BuildSnapshot(botAi);
    if (snap == lastSentSnapshot)
        return false;
    lastSentSnapshot = snap;

    Player* bot = botAi ? botAi->GetBot() : nullptr;
    if (!bot)
        return false;

    std::ostringstream payload;
    payload << "S|" << bot->GetName() << '|' << snap;
    SendToMaster(botAi, payload.str());
    return true;
}

void BotStatusBroadcaster::BroadcastTarget(PlayerbotAI* botAi, std::string const& targetName)
{
    Player* bot = botAi ? botAi->GetBot() : nullptr;
    if (!bot)
        return;

    std::ostringstream payload;
    payload << "T|" << bot->GetName() << '|' << targetName;
    SendToMaster(botAi, payload.str());
}

bool BotStatusBroadcaster::BroadcastAction(PlayerbotAI* botAi, std::string& lastSentAction)
{
    if (!botAi)
        return false;
    Player* bot = botAi->GetBot();
    if (!bot)
        return false;

    // Pull the last executed action from the CURRENT engine — the one driving
    // the bot right now. State-engine pairs that aren't current are tracked
    // separately by their own engines. getName() is non-const in this fork's
    // AiNamedObject so we cast away const — the call is read-only by inspection.
    const ai::Action* act = nullptr;
    if (ai::Engine* eng = botAi->GetCurrentEngine())
        act = eng->GetLastExecutedAction();

    std::string actName = act ? const_cast<ai::Action*>(act)->getName() : std::string();
    if (actName == lastSentAction)
        return false;
    lastSentAction = actName;

    std::ostringstream payload;
    payload << "A|" << bot->GetName() << '|' << actName;
    SendToMaster(botAi, payload.str());
    return true;
}
