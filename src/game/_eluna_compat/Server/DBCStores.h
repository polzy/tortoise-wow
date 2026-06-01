#pragma once
// MCWoW Eluna compat shim — points at Penqle's actual location
#include "../../Database/DBCStores.h"
#include "../Globals/ObjectMgr.h"  // sibling shim

// Eluna expects `sFactionTemplateStore` at global scope (cmangos modern
// layout). Penqle/Turtle exposes the same data via
// `sObjectMgr.GetFactionTemplateEntry(id)`. PlayerBots has the same
// shim at `src/modules/PlayerBots/cmangos-compat-shim.h:182`; we
// duplicate it here so the game lib also exports the symbol.
//
// FactionTemplateEntry forward-declared since Eluna only ever takes
// `T const*` from this proxy and never instantiates.
#ifndef MCWOW_ELUNA_SFACTIONTEMPLATESTORE
#define MCWOW_ELUNA_SFACTIONTEMPLATESTORE
struct FactionTemplateEntry;
struct ElunaSFactionTemplateStoreProxy
{
    template<typename T = FactionTemplateEntry>
    T const* LookupEntry(uint32 id) const { return sObjectMgr.GetFactionTemplateEntry(id); }
    uint32 GetNumRows() const { return 1500; }  // Upper-bound stub.
};
inline ElunaSFactionTemplateStoreProxy sFactionTemplateStore;
#endif
