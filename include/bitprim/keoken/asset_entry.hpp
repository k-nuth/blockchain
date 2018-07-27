/**
 * Copyright (c) 2016-2018 Bitprim Inc.
 *
 * This file is part of Bitprim.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef BITPRIM_BLOCKCHAIN_KEOKEN_ASSET_ENTRY_HPP_
#define BITPRIM_BLOCKCHAIN_KEOKEN_ASSET_ENTRY_HPP_

#include <bitprim/keoken/entities/asset.hpp>
#include <bitprim/keoken/primitives.hpp>

namespace bitprim {
namespace keoken {

struct asset_entry {
    asset_entry(entities::asset asset, size_t block_height, libbitcoin::hash_digest const& txid)
        : asset(std::move(asset))
        , block_height(block_height)
        , txid(txid)
    {}

    // asset_entry() = default;
    // asset_entry(asset_entry const& x) = default;
    // asset_entry(asset_entry&& x) = default;
    // asset_entry& operator=(asset_entry const& x) = default;
    // asset_entry& operator=(asset_entry&& x) = default;

    entities::asset asset;
    size_t block_height;
    libbitcoin::hash_digest txid;
};

} // namespace keoken
} // namespace bitprim

#endif //BITPRIM_BLOCKCHAIN_KEOKEN_ASSET_ENTRY_HPP_
