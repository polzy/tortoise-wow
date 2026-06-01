#pragma once
// MCWoW Eluna compat shim — vanilla 1.18.1 stub for ProcEventInfo /
// DamageInfo / HealInfo, which are TBC+ types Eluna references
// unconditionally in its SpellHooks.cpp + ElunaSpellWrapper.cpp.
//
// Vanilla's proc system doesn't expose this structured info — proc
// callbacks get raw params. We stub these classes so the Eluna code
// compiles; the SpellHooks paths that call them are still wired but
// will operate on default-constructed stub instances at runtime.
// Lua scripts that depend on these proc fields won't see useful
// data on vanilla, but they also won't crash.
//
// To make the runtime path safe, every getter returns a sensible
// default. Mutators are no-ops.

// Pull canonical types via the normal game-lib include path. We can't
// use relative paths here — _eluna_compat/Spells is "rooted" at
// _eluna_compat/, so ../../X would land outside src/game. Let CMake
// resolve via -I src/shared / -I src/game / etc.
#include "Common.h"
#include "SharedDefines.h"

class Unit;
class Spell;
class SpellInfo;

#ifndef MCWOW_ELUNA_PROCEVENTINFO_STUB
#define MCWOW_ELUNA_PROCEVENTINFO_STUB

class DamageInfo
{
public:
    uint32 GetDamage() const { return 0; }
    DamageEffectType GetDamageType() const { return DIRECT_DAMAGE; }
    WeaponAttackType GetAttackType() const { return BASE_ATTACK; }
    uint32 GetAbsorb() const { return 0; }
    uint32 GetResist() const { return 0; }
    uint32 GetBlock() const { return 0; }
    SpellInfo const* GetSpellInfo() const { return nullptr; }
    SpellSchoolMask GetSchoolMask() const { return SPELL_SCHOOL_MASK_NORMAL; }
    void ModifyDamage(int32) {}
    void AbsorbDamage(uint32) {}
    void ResistDamage(uint32) {}
    void BlockDamage(uint32) {}
};

class HealInfo
{
public:
    uint32 GetHeal() const { return 0; }
    uint32 GetEffectiveHeal() const { return 0; }
    uint32 GetAbsorb() const { return 0; }
    SpellInfo const* GetSpellInfo() const { return nullptr; }
    SpellSchoolMask GetSchoolMask() const { return SPELL_SCHOOL_MASK_NORMAL; }
    void AbsorbHeal(uint32) {}
};

class ProcEventInfo
{
public:
    Unit* GetActor() const { return nullptr; }
    Unit* GetActionTarget() const { return nullptr; }
    uint32 GetTypeMask() const { return 0; }
    uint32 GetSpellTypeMask() const { return 0; }
    uint32 GetSpellPhaseMask() const { return 0; }
    uint32 GetHitMask() const { return 0; }
    Spell const* GetProcSpell() const { return nullptr; }
    SpellInfo const* GetSpellInfo() const { return nullptr; }
    SpellSchoolMask GetSchoolMask() const { return SPELL_SCHOOL_MASK_NORMAL; }
    DamageInfo* GetDamageInfo() const { return nullptr; }
    HealInfo* GetHealInfo() const { return nullptr; }
};

#endif // MCWOW_ELUNA_PROCEVENTINFO_STUB
