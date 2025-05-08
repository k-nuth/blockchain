// Copyright (c) 2016-2024 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kth/blockchain/interface/block_chain.hpp>

namespace kth::blockchain {

void block_chain::for_each_transaction(size_t from, size_t to, for_each_tx_handler const& handler) const {
    while (from <= to) {

        if (stopped()) {
            handler(error::service_stopped, 0, domain::chain::transaction{});
            return;
        }

        auto const block_result = database_.get_block(from);

        if ( ! block_result) {
            handler(error::not_found, 0, domain::chain::transaction{});
            return;
        }

        if ( ! block_result->is_valid()) {
            handler(error::not_found, 0, domain::chain::transaction{});
            return;
        }

        //KTH_ASSERT(block_result->height() == from);
        //auto const tx_hashes = block_result->transaction_hashes();

        for_each_tx_valid(
            block_result->transactions().begin(),
            block_result->transactions().end(),
            from,
            handler
        );

        ++from;
    }
}

void block_chain::for_each_transaction_non_coinbase(size_t from, size_t to, for_each_tx_handler const& handler) const {
    //auto const& tx_store = database_.transactions();

    while (from <= to) {

        if (stopped()) {
            handler(error::service_stopped, 0, domain::chain::transaction{});
            return;
        }

        auto const block_result = database_.get_block(from);

        if ( ! block_result) {
            handler(error::not_found, 0, domain::chain::transaction{});
            return;
        }
        if ( ! block_result->is_valid()) {
            handler(error::not_found, 0, domain::chain::transaction{});
            return;
        }
        //KTH_ASSERT(block_result.height() == from);
        auto const tx_hashes = block_result->transactions();

        for_each_tx_valid(
            std::next(tx_hashes.begin()),
            tx_hashes.end(),
            from,
            handler
        );

        ++from;
    }
}

} // namespace kth::blockchain
