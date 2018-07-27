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
#ifndef BITPRIM_BLOCKCHAIN_KEOKEN_BALANCE_HPP_
#define BITPRIM_BLOCKCHAIN_KEOKEN_BALANCE_HPP_

#include <tuple>

#include <bitprim/keoken/entities/asset.hpp>
#include <bitprim/keoken/primitives.hpp>

namespace bitprim {
namespace keoken {

using balance_key = std::tuple<asset_id_t, libbitcoin::wallet::payment_address>;

} // namespace keoken
} // namespace bitprim


// Standard hash.
//-----------------------------------------------------------------------------

namespace std {
template <>
struct hash<bitprim::keoken::balance_key> {
    size_t operator()(bitprim::keoken::balance_key const& key) const {
        //Note: if we choose use boost::hash_combine we have to specialize bc::wallet::payment_address in the boost namespace
        // size_t seed = 0;
        // boost::hash_combine(seed, std::get<0>(key));
        // boost::hash_combine(seed, std::get<1>(key));
        // return seed;
        size_t h1 = std::hash<bitprim::keoken::asset_id_t>{}(std::get<0>(key));
        size_t h2 = std::hash<libbitcoin::wallet::payment_address>{}(std::get<1>(key));
        return h1 ^ (h2 << 1u);
    }
};
} // namespace std
//-----------------------------------------------------------------------------

namespace bitprim {
namespace keoken {

struct balance_entry {
    balance_entry(amount_t amount, size_t block_height, libbitcoin::hash_digest const& txid)
        : amount(amount), block_height(block_height), txid(txid)
    {}

    // balance_entry() = default;
    // balance_entry(balance_entry const& x) = default;
    // balance_entry(balance_entry&& x) = default;
    // balance_entry& operator=(balance_entry const& x) = default;
    // balance_entry& operator=(balance_entry&& x) = default;

    amount_t amount;
    size_t block_height;
    libbitcoin::hash_digest txid;
};

} // namespace keoken
} // namespace bitprim

#endif //BITPRIM_BLOCKCHAIN_KEOKEN_BALANCE_HPP_
