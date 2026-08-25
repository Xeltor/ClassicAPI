// This file is part of ClassicAPI.
//
// ClassicAPI is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.

// Exact local-player hit supplied by equipped, usable items. Vanilla stores
// these bonuses across cached item equip spells, random properties, and live
// per-instance enchantments; resolving all three natively avoids localized
// tooltip parsing and includes scopes/permanent enchants.

#include "Game.h"
#include "Offsets.h"
#include "item/CGItem.h"
#include "item/Location.h"
#include "item/Record.h"
#include "item/StatAccum.h"
#include "unit/Identity.h"

#include <cstdint>

namespace Player::EquippedHitBonuses {

namespace {

constexpr int kFirstEquipmentSlot = 1;
constexpr int kLastEquipmentSlot = 19;
constexpr int kEnchantSlots = 7;

bool IsBroken(const uint8_t *descriptor) {
    if (descriptor == nullptr)
        return false;
    const uint32_t maximum = Game::Read<uint32_t>(
        descriptor, Offsets::OFF_DESCRIPTOR_MAX_DURABILITY);
    const uint32_t current = Game::Read<uint32_t>(
        descriptor, Offsets::OFF_DESCRIPTOR_DURABILITY);
    return maximum > 0 && current == 0;
}

int __fastcall Script_GetEquippedHitBonuses(void *L) {
    if (Unit::Identity::PlayerObject() == nullptr) {
        Game::Lua::PushNil(L);
        return 1;
    }

    Item::StatAccum::Accum stats[Item::StatAccum::kCount];
    Item::StatAccum::Init(stats);
    int equipped = 0;
    for (int slot = kFirstEquipmentSlot; slot <= kLastEquipmentSlot; ++slot) {
        const uint8_t *item = Item::Location::ResolveEquipmentSlot(slot);
        if (item == nullptr)
            continue;
        ++equipped;
        const uint8_t *instance = Item::InstanceBlock(item);
        const uint8_t *descriptor = Item::ObjectFields(item);
        if (instance == nullptr || descriptor == nullptr) {
            Game::Lua::PushNil(L);
            return 1;
        }
        const uint32_t itemID = Game::Read<uint32_t>(
            instance, Offsets::OFF_INSTANCE_BLOCK_ITEM_ID);
        const uint8_t *record = Item::PeekRecord(itemID);
        if (itemID == 0 || record == nullptr) {
            // Partial equipment totals would silently understate hit. Hold the
            // whole capability until the ordinary equipped-item cache is warm.
            Game::Lua::PushNil(L);
            return 1;
        }
        if (IsBroken(descriptor))
            continue;

        Item::StatAccum::AccumulateRecord(stats, record, +1);
        const int suffix = Game::Read<int32_t>(
            descriptor, Offsets::OFF_DESCRIPTOR_RANDOM_PROPERTY);
        Item::StatAccum::ApplyRandomSuffix(stats, suffix, +1);
        for (int enchantSlot = 0; enchantSlot < kEnchantSlots; ++enchantSlot) {
            const uint32_t enchantID = Game::Read<uint32_t>(descriptor,
                Offsets::OFF_DESCRIPTOR_ENCHANTMENT_ID
                    + enchantSlot * Offsets::DESCRIPTOR_ENCHANTMENT_SLOT_STRIDE);
            Item::StatAccum::ApplyEnchant(stats, enchantID, +1);
        }
    }

    Game::Lua::PushNumber(L, static_cast<double>(Item::StatAccum::Value(
        stats, "ITEM_MOD_HIT_MELEE_RATING")));
    Game::Lua::PushNumber(L, static_cast<double>(Item::StatAccum::Value(
        stats, "ITEM_MOD_HIT_RANGED_RATING")));
    Game::Lua::PushNumber(L, static_cast<double>(Item::StatAccum::Value(
        stats, "ITEM_MOD_HIT_SPELL_RATING")));
    Game::Lua::PushNumber(L, static_cast<double>(equipped));
    return 4;
}

void RegisterLuaFunctions() {
    Game::Lua::RegisterTableFunction("C_PlayerInfo", "GetEquippedHitBonuses",
                                     &Script_GetEquippedHitBonuses);
}

const Game::ModuleAutoRegister _autoreg{&RegisterLuaFunctions};

} // namespace

} // namespace Player::EquippedHitBonuses
