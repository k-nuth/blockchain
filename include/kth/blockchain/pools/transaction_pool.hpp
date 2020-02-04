// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_TRANSACTION_POOL_HPP
#define KTH_BLOCKCHAIN_TRANSACTION_POOL_HPP

#include <cstddef>
#include <cstdint>
#include <kth/bitcoin.hpp>
#include <kth/blockchain/define.hpp>
#include <kth/blockchain/interface/safe_chain.hpp>
#include <kth/blockchain/settings.hpp>

namespace kth {
namespace blockchain {

/// TODO: this class is not implemented or utilized.
class BCB_API transaction_pool
{
public:
    typedef safe_chain::inventory_fetch_handler inventory_fetch_handler;
    typedef safe_chain::merkle_block_fetch_handler merkle_block_fetch_handler;

    transaction_pool(const settings& settings);

    void fetch_template(merkle_block_fetch_handler) const;
    void fetch_mempool(size_t maximum, inventory_fetch_handler) const;

////private:
////    const bool reject_conflicts_;
////    const uint64_t minimum_fee_;
};

} // namespace blockchain
} // namespace kth

#endif
