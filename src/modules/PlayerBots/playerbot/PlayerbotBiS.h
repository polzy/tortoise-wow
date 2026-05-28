/*
 * BiS (Best-in-Slot) gear table for lvl-60 bots — applied via `.bot bis` after
 * `.bot init` so it overlays the random gear path with class-coherent T2 sets.
 *
 * Strategy: prioritize the full 8-piece T2 set per class (BWL-tier equivalents in
 * Penqle's tw_world, ilvl 76-92) so the bot benefits from set bonuses, then fill
 * the remaining slots (neck, rings, trinkets, back, MH/OH/ranged) with the
 * highest-stat-weight individual item the class can wear.
 *
 * T2 set name per class (Penqle IDs 70xxx, all 8 pieces verified in DB):
 *   Warrior  -> Battlegear of Wrath   (HEAD/SHOUL/CHEST/HANDS/WAIST/LEGS/FEET/WRIST)
 *   Paladin  -> Judgement Armor
 *   Hunter   -> Dragonstalker         (kept 16xxx vanilla — Penqle preserved these)
 *   Rogue    -> Bloodfang             (kept 16xxx vanilla)
 *   Priest   -> Vestments of Transcendence
 *   Shaman   -> The Ten Storms
 *   Mage     -> Netherwind
 *   Warlock  -> Nemesis Raiment
 *   Druid    -> Stormrage Raiment
 *
 * Set bonuses (vanilla canonical, present on each class's 8-piece T2):
 *   - 2-piece: minor stat
 *   - 4-piece: spell crit / ability cost reduction
 *   - 6-piece: ability proc
 *   - 8-piece: signature ability (e.g. Warrior 'Defiance', Hunter 'Aimed Shot crit')
 *
 * Non-set slots (NECK/RINGS/TRINKETS/BACK/MH/OH/RANGED) are kept from the earlier
 * auto-curated picks — those are individual ilvl-92-96 epic items with proper
 * armor proficiency + class restrictions. They do NOT interfere with the T2 set
 * bonus (which only counts the 8 set pieces).
 *
 * Slot order (struct GearSet::items[19]):
 *   0=HEAD 1=NECK 2=SHOULDERS 3=BODY(shirt) 4=CHEST 5=WAIST 6=LEGS 7=FEET
 *   8=WRISTS 9=HANDS 10=FINGER1 11=FINGER2 12=TRINKET1 13=TRINKET2 14=BACK
 *   15=MAINHAND 16=OFFHAND 17=RANGED 18=TABARD
 */
#pragma once
#include "Common.h"
#include "Objects/Player.h"
#include <unordered_map>

namespace mcwow_bis
{
    inline uint32 SpecKey(uint8 cls, uint8 spec) { return uint32(cls) * 10 + spec; }

    struct GearSet { uint32 items[19]; };

    inline const std::unordered_map<uint32, GearSet>& Gear()
    {
        static const std::unordered_map<uint32, GearSet> data = {
            // WARRIOR (T2 Battlegear of Wrath, 8 pieces) + 2H sword + shield + thrown
            { SpecKey(1, 0), { {
                70687, 47275, 70688, 0, 70689, 70692, 70693, 70694,
                70690, 70691, 55353, 55516, 55124, 55131, 55352,
                21797, 55349, 55510, 0
            } } },

            // PALADIN (T2 Judgement Armor) + 2H sword + shield + no ranged
            { SpecKey(2, 0), { {
                70525, 47065, 70526, 0, 70527, 70530, 70531, 70532,
                70528, 70529, 55353, 55516, 55124, 55131, 55352,
                21797, 55349, 0, 0
            } } },

            // HUNTER (T2 Dragonstalker, 8 pieces) + 2H sword + holdable + bow
            { SpecKey(3, 0), { {
                16939, 47323, 16937, 0, 16942, 16936, 16938, 16941,
                16935, 16940, 55353, 55516, 55124, 55131, 55352,
                21797, 55350, 55346, 0
            } } },

            // ROGUE (T2 Bloodfang, 8 pieces) + 1H mace + 1H mace OH + bow
            { SpecKey(4, 0), { {
                16908, 47329, 16832, 0, 16905, 16910, 16909, 16906,
                16911, 16907, 55353, 55516, 55124, 55131, 55352,
                55347, 23242, 55346, 0
            } } },

            // PRIEST (T2 Vestments of Transcendence) + 1H mace + holdable + wand
            { SpecKey(5, 0), { {
                70657, 33176, 70658, 0, 70659, 70662, 70663, 70664,
                70660, 70661, 55353, 55516, 55124, 55131, 55352,
                55347, 55350, 22821, 0
            } } },

            // SHAMAN (T2 The Ten Storms) + 1H mace + shield + no ranged
            { SpecKey(7, 0), { {
                70724, 47185, 70725, 0, 70726, 70729, 70730, 70731,
                70727, 70728, 55353, 55516, 55124, 55131, 55352,
                55347, 55349, 0, 0
            } } },

            // MAGE (T2 Netherwind) + staff + holdable + wand
            { SpecKey(8, 0), { {
                70613, 47113, 70614, 0, 70615, 70618, 70619, 70620,
                70616, 70617, 55353, 55516, 55124, 55131, 55352,
                55348, 55350, 22821, 0
            } } },

            // WARLOCK (T2 Nemesis Raiment) + staff + holdable + wand
            { SpecKey(9, 0), { {
                70754, 47311, 70755, 0, 70756, 70759, 70760, 70761,
                70757, 70758, 55353, 55516, 55124, 55131, 55352,
                55348, 55350, 22821, 0
            } } },

            // DRUID (T2 Stormrage Raiment) + 1H mace + holdable + no ranged
            { SpecKey(11, 0), { {
                70792, 47395, 70793, 0, 70794, 70797, 70798, 70799,
                70795, 70796, 55353, 55516, 55124, 55131, 55352,
                55347, 55350, 0, 0
            } } },
        };
        return data;
    }

    inline uint32 GetItem(uint8 cls, uint8 spec, uint8 slot)
    {
        auto& g = Gear();
        auto it = g.find(SpecKey(cls, spec));
        if (it == g.end() || slot >= 19) return 0;
        return it->second.items[slot];
    }
}
