// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_POPULATE_BASE_HPP
#define KTH_BLOCKCHAIN_POPULATE_BASE_HPP

#include <cstddef>
#include <cstdint>
#include <kth/bitcoin.hpp>
#include <kth/blockchain/define.hpp>
#include <kth/blockchain/interface/fast_chain.hpp>
#include <kth/blockchain/settings.hpp>

namespace kth {
namespace blockchain {

/// This class is NOT thread safe.
class BCB_API populate_base
{
protected:
    typedef handle0 result_handler;

    populate_base(dispatcher& dispatch, const fast_chain& chain);

    void populate_duplicate(size_t maximum_height,
        const chain::transaction& tx, bool require_confirmed) const;

    void populate_pooled(const chain::transaction& tx, uint32_t forks) const;

    void populate_prevout(size_t maximum_height, const chain::output_point& outpoint, bool require_confirmed) const;

    // This is thread safe.
    dispatcher& dispatch_;

    // The store is protected by caller not invoking populate concurrently.
    const fast_chain& fast_chain_;
};

} // namespace blockchain
} // namespace kth

#endif
