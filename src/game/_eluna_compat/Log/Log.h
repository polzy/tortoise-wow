#pragma once
// MCWoW Eluna compat shim
#include "../../../shared/Log.h"

// Eluna upstream calls `sLog->outErrorEluna(...)` and `sLog->outBasicEluna(...)`
// which are AzerothCore-flavored helpers that route Eluna's own diagnostics into
// a dedicated log file. Penqle's Log class only has outError / outBasic. Map the
// names here so we don't have to fork ElunaConfig.cpp / LuaEngine.cpp; the
// payload format is identical (printf-style).
#define outErrorEluna outError
#define outBasicEluna outBasic
