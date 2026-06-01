
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
    if (sep == std::string::npos)
    {
        // Malformed qualifier — caller passed something like "15263" (entry
        // only) where ":<spellId>" is required. Log so the wiring author
        // catches it on first encounter test.
        sLog.outError("BossHasAuraValue: malformed qualifier '%s' — expected '<entry>:<spellId>'", qualifier.c_str());
        return false;
    }

    uint32 entry = (uint32)atoi(qualifier.substr(0, sep).c_str());
    uint32 spellId = (uint32)atoi(qualifier.substr(sep + 1).c_str());
    if (!entry || !spellId)
    {
        sLog.outError("BossHasAuraValue: parsed entry=%u spellId=%u from '%s' — both must be non-zero", entry, spellId, qualifier.c_str());
        return false;
    }

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

bool BossIsCastingValue::Calculate()
{
    if (qualifier.empty()) return false;
    size_t sep = qualifier.find(':');
    if (sep == std::string::npos)
    {
        sLog.outError("BossIsCastingValue: malformed qualifier '%s' — expected '<entry>:<spellId>'", qualifier.c_str());
        return false;
    }

    uint32 entry   = (uint32)atoi(qualifier.substr(0, sep).c_str());
    uint32 spellId = (uint32)atoi(qualifier.substr(sep + 1).c_str());
    if (!entry || !spellId)
    {
        sLog.outError("BossIsCastingValue: parsed entry=%u spellId=%u from '%s' — both must be non-zero", entry, spellId, qualifier.c_str());
        return false;
    }

    std::list<Unit*> units;
    MaNGOS::AllCreaturesOfEntryInRangeCheck check(bot, entry, 100.0f);
    MaNGOS::UnitListSearcher<MaNGOS::AllCreaturesOfEntryInRangeCheck> searcher(units, check);
    Cell::VisitAllObjects(bot, searcher, 100.0f);
    for (Unit* u : units)
    {
        if (!u || !u->IsAlive()) continue;
        // Check all 3 cast slots. CURRENT_MELEE_SPELL is autoattack —
        // skipped because mainhand swings aren't "cast" events worth
        // reacting to.
        if (Spell* s = u->GetCurrentSpell(CURRENT_GENERIC_SPELL))
            if (s->m_spellInfo && s->m_spellInfo->Id == spellId)
                return true;
        if (Spell* s = u->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
            if (s->m_spellInfo && s->m_spellInfo->Id == spellId)
                return true;
        if (Spell* s = u->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL))
            if (s->m_spellInfo && s->m_spellInfo->Id == spellId)
                return true;
    }
    return false;
}
