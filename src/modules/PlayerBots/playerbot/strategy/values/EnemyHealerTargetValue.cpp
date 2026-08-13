
#include "playerbot/playerbot.h"
#include "EnemyHealerTargetValue.h"
#include "playerbot/PlayerbotAIConfig.h"
#include "playerbot/ServerFacade.h"

using namespace ai;

Unit* EnemyHealerTargetValue::Calculate()
{
    std::string spell = qualifier;

    Unit* target = ai->GetAiObjectContext()->GetValue<Unit*>("current target")->Get();

    // Scan 'possible attack targets' first (bots that already have the healer
    // in their threat list — cheap path).
    std::list<ObjectGuid> attackers = ai->GetAiObjectContext()->GetValue<std::list<ObjectGuid>>("possible attack targets")->Get();
    for (std::list<ObjectGuid>::iterator i = attackers.begin(); i != attackers.end(); ++i)
    {
        Unit* unit = ai->GetUnit(*i);
        if (!unit || unit == target)
            continue;

        if (sServerFacade.GetDistance2d(bot, unit) > ai->GetRange("spell"))
            continue;

        if (!ai->IsInterruptableSpellCasting(unit, spell))
            continue;

        Spell* spell2 = unit->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (spell2 && IsPositiveSpell(spell2->m_spellInfo))
            return unit;

        spell2 = unit->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
        if (spell2 && IsPositiveSpell(spell2->m_spellInfo))
            return unit;
    }

    // Fallback: scan 'nearest adds' (hostile NPCs in spell range that haven't
    // attacked the bot personally — typical raid case where healer adds focus
    // the tank only, but the whole raid wants to interrupt their casts).
    // Without this fallback, only bots in the healer's threat list could fire
    // CS/pummel/kick/etc. → effectively only the MT.
    std::list<ObjectGuid> nearestHostile = ai->GetAiObjectContext()->GetValue<std::list<ObjectGuid>>("nearest adds")->Get();
    for (auto& guid : nearestHostile)
    {
        Unit* unit = ai->GetUnit(guid);
        if (!unit || unit == target) continue;
        if (sServerFacade.GetDistance2d(bot, unit) > ai->GetRange("spell")) continue;
        if (!ai->IsInterruptableSpellCasting(unit, spell)) continue;

        Spell* spell2 = unit->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (spell2 && IsPositiveSpell(spell2->m_spellInfo)) return unit;
        spell2 = unit->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
        if (spell2 && IsPositiveSpell(spell2->m_spellInfo)) return unit;
    }

    return NULL;
}
