/*
 * Per-class resistance gear sets for raid encounters where one school dominates.
 *
 * Coverage rationale per raid (Polz's tw_world snapshot):
 *   Fire   — Molten Core, Onyxia (deep breath / fire phases)
 *   Frost  — AQ40 Princess Huhuran, Naxx Sapphiron + Kel'Thuzad frostbolt volley
 *   Nature — AQ20/AQ40 silithid trash + Princess Yauj, Hydraxian dailies
 *   Shadow — Naxx Four Horsemen, certain Maexxna phases
 *
 * Auto-selection rules (same as Fire set):
 *   - inventory_type matches the slot
 *   - item.class=ARMOR, subclass within class proficiency
 *   - <type>_res >= 10 (skip negligible pieces; rings/trinkets are the weakest, 15 minimum)
 *   - lvl <= 60, no rep/honor/skill gate
 *   - quality >= uncommon
 *
 * Per-class totals (raid soft-cap is 315; cloth classes typically undershoot which
 * is fine — healers in back row, frontline tanks/melee benefit most):
 *   Fire:   235-335 (BACK = Onyxia Scale Cloak 15138 — +16 fire res but the key
 *                   value is Deep Breath immunity, the largest single Onyxia mit)
 *   Frost:  340-373  (frost gear is plentiful in tw_world)
 *   Nature: 279-337
 *   Shadow: 240-280  (sparsest pool; cloth chest slots have several gaps)
 *
 * Onyxia Scale Cloak requires turning in 5 Onyxia Scales (drop from Onyxia).
 * EquipNewItem will silently fail until the bot has actually killed Onyxia and
 * obtained the scales, leaving the BiS BACK slot (Cloak of Rapid Regeneration
 * 55352) in place — so the first Onyxia attempt still benefits from BiS.
 */
#pragma once
#include "Common.h"
#include "Objects/Player.h"
#include <unordered_map>
#include <string>

namespace mcwow_resist
{
    enum class School : uint8 { Fire = 0, Frost = 1, Nature = 2, Shadow = 3 };

    inline uint32 SpecKey(uint8 cls, uint8 spec) { return uint32(cls) * 10 + spec; }

    struct ResistSet { uint32 items[15]; };  // slots 0..14 (HEAD..BACK; no weapons)

    // Parse user-facing string ("fire" / "frost" / "nature" / "shadow") to School enum.
    // Returns true on match.
    inline bool ParseSchool(const std::string& s, School& out)
    {
        if (s == "fire" || s == "fr") { out = School::Fire; return true; }
        if (s == "frost") { out = School::Frost; return true; }
        if (s == "nature" || s == "nat") { out = School::Nature; return true; }
        if (s == "shadow" || s == "sh") { out = School::Shadow; return true; }
        return false;
    }

    inline const char* SchoolName(School s)
    {
        switch (s)
        {
            case School::Fire:   return "Fire";
            case School::Frost:  return "Frost";
            case School::Nature: return "Nature";
            case School::Shadow: return "Shadow";
        }
        return "?";
    }

    inline const std::unordered_map<uint32, ResistSet>& Gear(School school)
    {
        // slots: HEAD NECK SHOUL BODY CHEST WAIST LEGS FEET WRIST HANDS R1 R2 T1 T2 BACK
        static const std::unordered_map<uint32, ResistSet> fire = {
            { SpecKey(1, 0), { { 19148, 17783, 16980, 0, 21527, 19149, 19433, 20039, 17014, 19164, 17982, 58209, 33153, 60668, 15138 } } },
            { SpecKey(2, 0), { { 19148, 17783, 16980, 0, 21527, 19149, 19433, 20039, 17014, 19164, 17982, 58209, 33153, 60668, 15138 } } },
            { SpecKey(3, 0), { { 16983, 17783, 16980, 0, 21527, 19149, 19433, 16984,     0, 16979, 17982, 58209, 33153, 60668, 15138 } } },
            { SpecKey(4, 0), { { 16983, 17783, 16980, 0, 21527, 19149, 15054, 16982,     0, 16979, 17982, 58209, 33153, 60668, 15138 } } },
            { SpecKey(5, 0), { { 14130, 17783, 16980, 0, 21527,     0, 19165, 13369,     0, 16979, 17982, 58209, 33153, 60668, 15138 } } },
            { SpecKey(7, 0), { { 16983, 17783, 16980, 0, 21527, 19149, 19433, 16984,     0, 16979, 17982, 58209, 33153, 60668, 15138 } } },
            { SpecKey(8, 0), { { 14130, 17783, 16980, 0, 21527,     0, 19165, 13369,     0, 16979, 17982, 58209, 33153, 60668, 15138 } } },
            { SpecKey(9, 0), { { 14130, 17783, 16980, 0, 21527,     0, 19165, 13369,     0, 16979, 17982, 58209, 33153, 60668, 15138 } } },
            { SpecKey(11,0), { { 16983, 17783, 16980, 0, 21527, 19149, 15054, 16982,     0, 16979, 17982, 58209, 33153, 60668, 15138 } } },
        };
        static const std::unordered_map<uint32, ResistSet> frost = {
            { SpecKey(1, 0), { { 23019, 83457, 22968, 0, 22669, 18547, 22699, 12419, 22671, 22670, 22707, 60389, 55087, 23042, 22658 } } },
            { SpecKey(2, 0), { { 23019, 83457, 22968, 0, 22669, 18547, 22699, 12419, 22671, 22670, 22707, 60389, 55087, 23042, 22658 } } },
            { SpecKey(3, 0), { { 23020, 83457, 22968, 0, 22652, 12416, 22700, 12419, 22665, 22654, 22707, 60389, 55087, 23042, 22658 } } },
            { SpecKey(4, 0), { { 23020, 83457, 22968, 0, 22652, 14502, 22700, 15071, 22663, 22654, 22707, 60389, 55087, 23042, 22658 } } },
            { SpecKey(5, 0), { { 23032, 83457, 22968, 0, 22652,     0, 22700, 60988, 22655, 22654, 22707, 60389, 55087, 23042, 22658 } } },
            { SpecKey(7, 0), { { 23020, 83457, 22968, 0, 22652, 12416, 22700, 12419, 22665, 22654, 22707, 60389, 55087, 23042, 22658 } } },
            { SpecKey(8, 0), { { 23032, 83457, 22968, 0, 22652,     0, 22700, 60988, 22655, 22654, 22707, 60389, 55087, 23042, 22658 } } },
            { SpecKey(9, 0), { { 23032, 83457, 22968, 0, 22652,     0, 22700, 60988, 22655, 22654, 22707, 60389, 55087, 23042, 22658 } } },
            { SpecKey(11,0), { { 23020, 83457, 22968, 0, 22652, 14502, 22700, 15071, 22663, 22654, 22707, 60389, 55087, 23042, 22658 } } },
        };
        static const std::unordered_map<uint32, ResistSet> nature = {
            { SpecKey(1, 0), { { 21614, 17783, 20633, 0, 21652, 20625, 21626, 20621, 21708, 21691, 84506, 20600, 83450, 65105, 20579 } } },
            { SpecKey(2, 0), { { 21614, 17783, 20633, 0, 21652, 20625, 21626, 20621, 21708, 21691, 84506, 20600, 83450, 65105, 20579 } } },
            { SpecKey(3, 0), { { 21614, 17783, 20633, 0, 33077, 20625, 21626, 20621, 21708, 20477, 84506, 20600, 83450, 65105, 20579 } } },
            { SpecKey(4, 0), { { 21614, 17783, 20633, 0, 33077, 20625, 20703, 22760, 21708, 18344, 84506, 20600, 83450, 65105, 20579 } } },
            { SpecKey(5, 0), { { 22757, 17783, 22758, 0, 33077, 20625, 20705, 21648,     0,     0, 84506, 20600, 83450, 65105, 20579 } } },
            { SpecKey(7, 0), { { 21614, 17783, 20633, 0, 33077, 20625, 21626, 20621, 21708, 20477, 84506, 20600, 83450, 65105, 20579 } } },
            { SpecKey(8, 0), { { 22757, 17783, 22758, 0, 33077, 20625, 20705, 21648,     0,     0, 84506, 20600, 83450, 65105, 20579 } } },
            { SpecKey(9, 0), { { 22757, 17783, 22758, 0, 33077, 20625, 20705, 21648,     0,     0, 84506, 20600, 83450, 65105, 20579 } } },
            { SpecKey(11,0), { { 21614, 17783, 20633, 0, 33077, 20625, 20703, 22760, 21708, 18344, 84506, 20600, 83450, 65105, 20579 } } },
        };
        static const std::unordered_map<uint32, ResistSet> shadow = {
            { SpecKey(1, 0), { { 20551, 33307, 50193, 0, 19439, 20539, 21530, 20537, 13528, 20549, 21687, 58209, 61563, 60551, 21627 } } },
            { SpecKey(2, 0), { { 20551, 33307, 50193, 0, 19439, 20539, 21530, 20537, 13528, 20549, 21687, 58209, 61563, 60551, 21627 } } },
            { SpecKey(3, 0), { { 33314, 33307, 50193, 0, 19439, 20539, 21530, 20537, 13528, 13525, 21687, 58209, 61563, 60551, 21627 } } },
            { SpecKey(4, 0), { { 33314, 33307, 50193, 0, 19439, 20539, 20538, 20537, 61021, 13525, 21687, 58209, 61563, 60551, 21627 } } },
            { SpecKey(5, 0), { { 60408, 33307,     0, 0, 21838, 20539, 20538, 20537, 12626, 13525, 21687, 58209, 61563, 60551, 21627 } } },
            { SpecKey(7, 0), { { 33314, 33307, 50193, 0, 19439, 20539, 21530, 20537, 13528, 13525, 21687, 58209, 61563, 60551, 21627 } } },
            { SpecKey(8, 0), { { 16914, 33307,     0, 0, 21838, 20539, 20538, 20537, 12626, 13525, 21687, 58209, 61563, 60551, 21627 } } },
            { SpecKey(9, 0), { { 16929, 33307,     0, 0, 21838, 20539, 20538, 20537, 12626, 13525, 21687, 58209, 61563, 60551, 21627 } } },
            { SpecKey(11,0), { { 33314, 33307, 50193, 0, 19439, 20539, 20538, 20537, 61021, 13525, 21687, 58209, 61563, 60551, 21627 } } },
        };
        switch (school)
        {
            case School::Frost:  return frost;
            case School::Nature: return nature;
            case School::Shadow: return shadow;
            case School::Fire:
            default:             return fire;
        }
    }

    inline uint32 GetItem(School school, uint8 cls, uint8 spec, uint8 slot)
    {
        const auto& g = Gear(school);
        auto it = g.find(SpecKey(cls, spec));
        if (it == g.end() || slot >= 15) return 0;
        return it->second.items[slot];
    }
}

// Backwards-compat: previous code used mcwow_fr::GetItem(...) for fire. Keep the
// alias so HandleBotFR keeps compiling without changes during the migration window.
namespace mcwow_fr
{
    inline uint32 GetItem(uint8 cls, uint8 spec, uint8 slot)
    {
        return mcwow_resist::GetItem(mcwow_resist::School::Fire, cls, spec, slot);
    }
}
