// This file is part of ClassicAPI.
//
// ClassicAPI is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.

// Exact local combo-point ownership. Vanilla's GetComboPoints hides the count
// whenever the selected target differs; the client still stores both the count
// and owning GUID in CGPlayer. Exposing the pair lets decision addons preserve
// target identity without parsing names or guessing from a target swap.

#include "Game.h"
#include "Offsets.h"
#include "guid/Guid.h"
#include "unit/Identity.h"

#include <cstdint>

namespace Player::ComboPoints {

namespace {

int __fastcall Script_GetComboPointState(void *L) {
    const uint8_t *player = Unit::Identity::PlayerObject();
    if (player == nullptr) {
        Game::Lua::PushNil(L);
        return 1;
    }
    const uint8_t *info = Game::Read<const uint8_t *>(
        player, Offsets::OFF_CGPLAYER_INFO);
    if (info == nullptr) {
        Game::Lua::PushNil(L);
        return 1;
    }

    const uint8_t points = Game::Read<uint8_t>(
        info, Offsets::OFF_CGPLAYER_COMBO_POINTS);
    const uint64_t target = Game::Read<uint64_t>(
        info, Offsets::OFF_CGPLAYER_COMBO_TARGET);
    Game::Lua::PushNumber(L, static_cast<double>(points));
    if (points > 0 && target != 0) {
        char buffer[Guid::STRING_SIZE];
        Game::Lua::PushString(
            L, Guid::FormatAsString(target, buffer, sizeof buffer));
    } else {
        Game::Lua::PushNil(L);
    }
    return 2;
}

void RegisterLuaFunctions() {
    Game::Lua::RegisterTableFunction(
        "C_PlayerInfo", "GetComboPointState", &Script_GetComboPointState);
}

const Game::ModuleAutoRegister _autoreg{&RegisterLuaFunctions};

} // namespace

} // namespace Player::ComboPoints
