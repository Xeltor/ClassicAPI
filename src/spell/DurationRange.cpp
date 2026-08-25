// This file is part of ClassicAPI.
//
// ClassicAPI is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.

// Read-only SpellDuration range data. Combo finishers use the row's base and
// maximum as the zero/five-point endpoints; exposing both avoids localized
// tooltip parsing and lets addons project later combo counts generically.

#include "Game.h"
#include "Offsets.h"
#include "dbc/Lookup.h"
#include "spell/Arg.h"
#include "spell/Lookup.h"
#include "spell/Mod.h"

#include <cstdint>

namespace Spell::DurationRange {

namespace {

int __fastcall Script_GetSpellDurationRange(void *L) {
    const int spellId = Spell::Arg::ResolveSpellID(L, 1);
    const uint8_t *spell = Spell::Lookup::RecordForID(spellId);
    if (spell == nullptr) {
        Game::Lua::PushNil(L);
        return 1;
    }
    const int index = Game::Read<int>(
        spell, Offsets::OFF_SPELL_RECORD_DURATION_INDEX);
    const uint8_t *row = DBC::Record(
        Offsets::VAR_SPELL_DURATION_RECORDS,
        Offsets::VAR_SPELL_DURATION_COUNT, static_cast<uint32_t>(index));
    if (index <= 0 || row == nullptr) {
        Game::Lua::PushNil(L);
        return 1;
    }

    const int32_t rawBase = Game::Read<int32_t>(row, 0x4);
    const int32_t rawMax = Game::Read<int32_t>(row, 0xC);
    if (rawBase < 0 || rawMax <= 0) {
        Game::Lua::PushNumber(L, 0);
        Game::Lua::PushNumber(L, 0);
        Game::Lua::PushBool(L, false);
        return 3;
    }
    const float base = Spell::Mod::Apply(
        spell, Offsets::SPELLMOD_OP_DURATION, static_cast<float>(rawBase));
    const float maximum = Spell::Mod::Apply(
        spell, Offsets::SPELLMOD_OP_DURATION, static_cast<float>(rawMax));
    Game::Lua::PushNumber(L, (base > 0 ? base : 0) * 0.001);
    Game::Lua::PushNumber(L, (maximum > 0 ? maximum : 0) * 0.001);
    Game::Lua::PushBool(L, rawBase != rawMax);
    return 3;
}

void RegisterLuaFunctions() {
    Game::Lua::RegisterTableFunction(
        "C_Spell", "GetSpellDurationRange", &Script_GetSpellDurationRange);
}

const Game::ModuleAutoRegister _autoreg{&RegisterLuaFunctions};

} // namespace

} // namespace Spell::DurationRange
