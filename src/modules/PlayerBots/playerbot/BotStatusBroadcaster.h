#pragma once
#include <string>

class PlayerbotAI;
class Player;

// Lightweight server→addon channel for the MCWoWBots companion addon.
//
// Encoding: the server whispers the bot's master with a payload prefixed
// by the magic tag "MCWBS\t". The addon hooks ChatFrame_OnEvent to
// intercept these whispers, parse the payload, and SUPPRESS the chat
// display so the master never sees raw status spam.
//
// Why whisper-with-prefix instead of CHAT_MSG_ADDON?
//   1.12 does not natively route CHAT_MSG_ADDON to client Lua events the
//   way 2.x+ does. Whisper-with-prefix is the standard 1.12 pattern used
//   by raid-frame addons of the era — zero protocol hack, the only cost
//   is the addon-side filter (cheap).
//
// Payload formats (kept short to limit per-tick bandwidth on a 40-bot raid):
//   "MCWBS\tS|<botname>|<state>:<comma-sep-strategies>"  strategy snapshot
//   "MCWBS\tT|<botname>|<targetname>"                    current target name
//   "MCWBS\tA|<botname>|<action name>"                   last executed action
//
// Throttling: BroadcastStrategies fires only when the snapshot string CHANGES
// compared to the last sent value (kept per-bot via a string ref the caller
// owns).
class BotStatusBroadcaster
{
public:
    static bool BroadcastStrategies(PlayerbotAI* ai, std::string& lastSentSnapshot);
    static void BroadcastTarget(PlayerbotAI* ai, std::string const& targetName);
    // Send "A|name|<action>" if the current engine's last executed action
    // differs from lastSentAction. Updates lastSentAction in place.
    static bool BroadcastAction(PlayerbotAI* ai, std::string& lastSentAction);

private:
    static void SendToMaster(PlayerbotAI* ai, std::string const& payload);
};
