/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
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
#include <bitcoin/blockchain/populate/populate_block.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/blockchain/interface/fast_chain.hpp>
#include <bitcoin/blockchain/pools/branch.hpp>

namespace libbitcoin {
namespace blockchain {

using namespace bc::chain;
using namespace bc::machine;

#define NAME "populate_block"

// Database access is limited to calling populate_base.

#if defined(BITPRIM_WITH_MINING)
populate_block::populate_block(dispatcher& dispatch, fast_chain const& chain, bool relay_transactions, mining::mempool const& mp)
#else
populate_block::populate_block(dispatcher& dispatch, fast_chain const& chain, bool relay_transactions)
#endif
    : populate_base(dispatch, chain)
    , relay_transactions_(relay_transactions)
#if defined(BITPRIM_WITH_MINING)
    , mempool_(mp)
#endif
{}

local_utxo_t create_local_utxo_set(chain::block const& block) {
    //TODO(fernando): confirm if there is a validation to check that the coinbase tx is not spend, before this.
    //                we avoid to insert the coinbase in the local utxo set

    local_utxo_t res;
    res.reserve(block.transactions().size());
    for (auto const& tx : block.transactions()) {
        auto const& outputs = tx.outputs();
        for (uint32_t idx = 0; idx < outputs.size(); ++idx) {
            auto const& output = outputs[idx];
            res.emplace(output_point{tx.hash(), idx}, std::addressof(output));
        }
    }
    return res;
}

std::vector<local_utxo_t> create_branch_utxo_set(branch::const_ptr const& branch) {
    auto blocks = *branch->blocks();

    std::vector<local_utxo_t> res;
    res.reserve(branch->size());

    for (auto const& block : blocks) {
        res.push_back(create_local_utxo_set(*block));
    }

    return res;
}


void populate_block::populate(branch::const_ptr branch, result_handler&& handler) const {
    auto const block = branch->top();
    BITCOIN_ASSERT(block);

    auto const state = block->validation.state;
    BITCOIN_ASSERT(state);

    // Return if this blocks is under a checkpoint, block state not requried.
    if (state->is_under_checkpoint()) {
        handler(error::success);
        return;
    }

    // Handle the coinbase as a special case tx.
    populate_coinbase(branch, block);

    auto const non_coinbase_inputs = block->total_inputs(false);

    // Return if there are no non-coinbase inputs to validate.
    if (non_coinbase_inputs == 0) {
        handler(error::success);
        return;
    }

    auto const buckets = std::min(dispatch_.size(), non_coinbase_inputs);
    auto const join_handler = synchronize(std::move(handler), buckets, NAME);
    BITCOIN_ASSERT(buckets != 0);
    // LOG_INFO(LOG_BLOCKCHAIN) << "populate_block::populate - buckets:  " << buckets;

    // auto local_utxo = create_local_utxo_set(*block);
    auto branch_utxo = create_branch_utxo_set(branch);

    for (size_t bucket = 0; bucket < buckets; ++bucket) {
        dispatch_.concurrent(&populate_block::populate_transactions, this, branch, bucket, buckets, branch_utxo, join_handler);
    }
}

// Initialize the coinbase input for subsequent validation.
void populate_block::populate_coinbase(branch::const_ptr branch, block_const_ptr block) const {
    auto const& txs = block->transactions();
    auto const state = block->validation.state;
    BITCOIN_ASSERT(!txs.empty());

    auto const& coinbase = txs.front();
    BITCOIN_ASSERT(coinbase.is_coinbase());

    // A coinbase tx guarantees exactly one input.
    auto const& input = coinbase.inputs().front();
    auto& prevout = input.previous_output().validation;

    // A coinbase input cannot be a double spend since it originates coin.
    prevout.spent = false;

    // A coinbase is confirmed as long as its block is valid (context free).
    prevout.confirmed = true;

    // A coinbase does not spend a previous output so these are unused/default.
    prevout.cache = chain::output{};
    prevout.coinbase = false;
    prevout.height = 0;
    prevout.median_time_past = 0;

    //*************************************************************************
    // CONSENSUS: Satoshi implemented allow collisions in Nov 2015. This is a
    // hard fork that destroys unspent outputs in case of hash collision.
    // The tx duplicate check must apply to coinbase txs, handled here.
    //*************************************************************************
    if ( ! state->is_enabled(rule_fork::allow_collisions)) {
        populate_base::populate_duplicate(branch->height(), coinbase, true);
        ////populate_duplicate(branch, coinbase);
    }
}

////void populate_block::populate_duplicate(branch::const_ptr branch,
////    const chain::transaction& tx) const
////{
////    if (!tx.validation.duplicate)
////        branch->populate_duplicate(tx);
////}


#ifdef BITPRIM_DB_NEW
populate_block::utxo_pool_t populate_block::get_reorg_subset_conditionally(size_t first_height, size_t& out_chain_top) const {

    // auto temp1 = branch->top_height();

    out_chain_top;
    if ( ! fast_chain_.get_last_height(out_chain_top)) {
        out_chain_top = 0;
        return {};
    }

    if (first_height > out_chain_top) {
        return {};
    }

    auto p = fast_chain_.get_utxo_pool_from(first_height, out_chain_top);
    if ( ! p.first) {
        return {};
    }
    
    return std::move(p.second);
}
#endif // BITPRIM_DB_NEW


void populate_block::populate_transactions(branch::const_ptr branch, size_t bucket, size_t buckets, std::vector<local_utxo_t> const& branch_utxo, result_handler handler) const {

    // TODO(fernando): check how to replace it with UTXO
    // asm("int $3");  //TODO(fernando): remover

    BITCOIN_ASSERT(bucket < buckets);
    auto const block = branch->top();
    auto const branch_height = branch->height();
    auto const& txs = block->transactions();
    size_t input_position = 0;

    auto const state = block->validation.state;
    auto const forks = state->enabled_forks();
    auto const collide = state->is_enabled(rule_fork::allow_collisions);

    // Must skip coinbase here as it is already accounted for.
    auto const first = bucket == 0 ? buckets : bucket;

    for (auto position = first; position < txs.size(); position = ceiling_add(position, buckets)) {
        auto const& tx = txs[position];

        //---------------------------------------------------------------------
        // This prevents output validation and full tx deposit respectively.
        // The tradeoff is a read per tx that may not be cached. This is
        // bypassed by checkpoints. This will be optimized using the tx pool.
        // Until that time this is a material population performance hit.
        // However the hit is necessary in preventing store tx duplication
        // unless tx relay is disabled. In that case duplication is unlikely.
        //---------------------------------------------------------------------

        //TODO(fernando): check again why this is not implemented?
#ifdef BITPRIM_DB_LEGACY
        if (relay_transactions_) {
            populate_base::populate_pooled(tx, forks);
        }
#endif

        //*********************************************************************
        // CONSENSUS: Satoshi implemented allow collisions in Nov 2015. This is
        // a hard fork that destroys unspent outputs in case of hash collision.
        //*********************************************************************
        //TODO(fernando): check again why this is not implemented?
#ifdef BITPRIM_DB_LEGACY
        if ( ! collide) {
            populate_base::populate_duplicate(branch->height(), tx, true);
            ////populate_duplicate(branch, coinbase);
        }
#endif
    }

#ifdef BITPRIM_DB_NEW
    size_t first_height = branch_height + 1u;
    size_t chain_top;
    auto reorg_subset = get_reorg_subset_conditionally(first_height, /*out*/ chain_top);
#endif // BITPRIM_DB_NEW


    // Must skip coinbase here as it is already accounted for.
    for (auto tx = txs.begin() + 1; tx != txs.end(); ++tx) {
        if ( ! mempool_.contains(tx->hash())) {

            auto const& inputs = tx->inputs();

            for (size_t input_index = 0; input_index < inputs.size(); ++input_index, ++input_position) {
                if (input_position % buckets != bucket) {
                    continue;
                }

                auto const& input = inputs[input_index];
                auto const& prevout = input.previous_output();
                populate_base::populate_prevout(branch_height, prevout, true);  //Populate from Database
                populate_prevout(branch, prevout, branch_utxo);                 //Populate from the Blocks in the Branch

#ifdef BITPRIM_DB_NEW
                if (first_height <= chain_top) {
                    populate_from_reorg_subset(prevout, reorg_subset);
                }
#endif // BITPRIM_DB_NEW
            }
        } else {
            tx->validation.validated = true;
        }
    }

    handler(error::success);
}

#ifdef BITPRIM_DB_NEW
void populate_block::populate_from_reorg_subset(output_point const& outpoint, utxo_pool_t const& reorg_subset) const {
    if (outpoint.validation.cache.is_valid()) {
        return;
    }

    auto it = reorg_subset.find(outpoint);
    if (it != reorg_subset.end()) {
        auto& val = outpoint.validation;
        auto const& entry = it->second;
        val.height = entry.height();
        val.median_time_past = entry.median_time_past();
        val.cache = entry.output();
        val.coinbase = entry.coinbase();
    }

}
#endif // BITPRIM_DB_NEW

void populate_block::populate_prevout(branch::const_ptr branch, output_point const& outpoint, std::vector<local_utxo_t> const& branch_utxo) const {
    if ( ! outpoint.validation.spent) {
        branch->populate_spent(outpoint);
    }

    // Populate the previous output even if it is spent.
    if ( ! outpoint.validation.cache.is_valid()) {
        branch->populate_prevout(outpoint, branch_utxo);
    }
}

} // namespace blockchain
} // namespace libbitcoin
