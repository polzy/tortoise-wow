
#include "playerbot/playerbot.h"
#include "BossPhaseValues.h"
#include "Maps/GridNotifiers.h"
#include "Maps/GridNotifiersImpl.h"
#include "Maps/CellImpl.h"

using namespace ai;

float BossHpPctValue::Calculate()
{
    if (qualifier.empty()) return 100.0f;
    uint32 entry = (uint32)atoi(qualifier.c_str());
    if (!entry) return 100.0f;

    std::list<Unit*> units;
    MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, entry, 100.0f);
    MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
    Cell::VisitAllObjects(bot, searcher, 100.0f);
    for (Unit* u : units)
    {
        if (u && u->IsAlive() && u->GetMaxHealth() > 0)
            return (float)u->GetHealth() * 100.0f / (float)u->GetMaxHealth();
    }
    return 100.0f;
}

bool BossHasAuraValue::Calculate()
{
    if (qualifier.empty()) return false;
    size_t sep = qualifier.find(':');
    if (sep == std::string::npos) return false;

    uint32 entry = (uint32)atoi(qualifier.substr(0, sep).c_str());
    uint32 spellId = (uint32)atoi(qualifier.substr(sep + 1).c_str());
    if (!entry || !spellId) return false;

    std::list<Unit*> units;
    MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, entry, 100.0f);
    MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
    Cell::VisitAllObjects(bot, searcher, 100.0f);
    for (Unit* u : units)
    {
        if (u && u->IsAlive() && u->HasAura(spellId))
            return true;
    }
    return false;
}
