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
#include <bitcoin/blockchain/interface/block_chain.hpp>

namespace libbitcoin {
namespace blockchain {

void block_chain::for_each_transaction(size_t from, size_t to, bool witness, for_each_tx_handler const& handler) const {
#ifdef BITPRIM_CURRENCY_BCH
    witness = false;    //TODO(fernando): see what to do with those things!
#endif
    auto const& tx_store = database_.transactions();

    while (from <= to) {

        if (stopped()) {
            handler(error::service_stopped, 0, chain::transaction{});
            return;
        }
    
        auto const block_result = database_.blocks().get(from);

        if ( ! block_result) {
            handler(error::not_found, 0, chain::transaction{});
            return;
        }
        BITCOIN_ASSERT(block_result.height() == from);
        auto const tx_hashes = block_result.transaction_hashes();

        for_each_tx_hash(block_result.transaction_hashes().begin(), 
                         block_result.transaction_hashes().end(), 
                         tx_store, from, witness, handler);

        ++from;
    }
}

void block_chain::for_each_transaction_non_coinbase(size_t from, size_t to, bool witness, for_each_tx_handler const& handler) const {
#ifdef BITPRIM_CURRENCY_BCH
    witness = false;    //TODO(fernando): see what to do with those things!
#endif
    auto const& tx_store = database_.transactions();

    while (from <= to) {

        if (stopped()) {
            handler(error::service_stopped, 0, chain::transaction{});
            return;
        }
    
        auto const block_result = database_.blocks().get(from);

        if ( ! block_result) {
            handler(error::not_found, 0, chain::transaction{});
            return;
        }
        BITCOIN_ASSERT(block_result.height() == from);
        auto const tx_hashes = block_result.transaction_hashes();

        for_each_tx_hash(std::next(block_result.transaction_hashes().begin()), 
                         block_result.transaction_hashes().end(), 
                         tx_store, from, witness, handler);

        ++from;
    }
}

} // namespace blockchain
} // namespace libbitcoin
