// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_KEOKEN_ASSET_ENTRY_HPP_
#define KTH_BLOCKCHAIN_KEOKEN_ASSET_ENTRY_HPP_

#include <knuth/keoken/entities/asset.hpp>
#include <knuth/keoken/primitives.hpp>

namespace kth {
namespace keoken {

struct asset_entry {
    asset_entry(entities::asset asset, size_t block_height, kth::hash_digest const& txid)
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
    kth::hash_digest txid;
};

} // namespace keoken
} // namespace kth

#endif //KTH_BLOCKCHAIN_KEOKEN_ASSET_ENTRY_HPP_
