// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_KEOKEN_BALANCE_HPP_
#define KTH_BLOCKCHAIN_KEOKEN_BALANCE_HPP_

#include <tuple>

#include <kth/keoken/entities/asset.hpp>
#include <kth/keoken/primitives.hpp>

namespace kth {
namespace keoken {

using balance_key = std::tuple<asset_id_t, kth::wallet::payment_address>;

} // namespace keoken
} // namespace kth


// Standard hash.
//-----------------------------------------------------------------------------

namespace std {
template <>
struct hash<knuth::keoken::balance_key> {
    size_t operator()(knuth::keoken::balance_key const& key) const {
        //Note: if we choose use boost::hash_combine we have to specialize kd::wallet::payment_address in the boost namespace
        // size_t seed = 0;
        // boost::hash_combine(seed, std::get<0>(key));
        // boost::hash_combine(seed, std::get<1>(key));
        // return seed;
        size_t h1 = std::hash<knuth::keoken::asset_id_t>{}(std::get<0>(key));
        size_t h2 = std::hash<kth::wallet::payment_address>{}(std::get<1>(key));
        return h1 ^ (h2 << 1u);
    }
};
} // namespace std
//-----------------------------------------------------------------------------

namespace kth {
namespace keoken {

struct balance_entry {
    balance_entry(amount_t amount, size_t block_height, kth::hash_digest const& txid)
        : amount(amount), block_height(block_height), txid(txid)
    {}

    // balance_entry() = default;
    // balance_entry(balance_entry const& x) = default;
    // balance_entry(balance_entry&& x) = default;
    // balance_entry& operator=(balance_entry const& x) = default;
    // balance_entry& operator=(balance_entry&& x) = default;

    amount_t amount;
    size_t block_height;
    kth::hash_digest txid;
};

} // namespace keoken
} // namespace kth

#endif //KTH_BLOCKCHAIN_KEOKEN_BALANCE_HPP_
