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
#include <bitcoin/blockchain/validate/validate_transaction.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/bitcoin/multi_crypto_support.hpp>
#include <bitcoin/blockchain/interface/fast_chain.hpp>
#include <bitcoin/blockchain/pools/branch.hpp>
#include <bitcoin/blockchain/settings.hpp>
#include <bitcoin/blockchain/validate/validate_input.hpp>

namespace libbitcoin {
namespace blockchain {

using namespace bc::chain;
using namespace bc::machine;
using namespace std::placeholders;

#define NAME "validate_transaction"

// Database access is limited to: populator:
// spend: { spender }
// transaction: { exists, height, output }

validate_transaction::validate_transaction(dispatcher& dispatch, const fast_chain& chain, const settings& settings)
    : stopped_(true)
    , dispatch_(dispatch)
    , transaction_populator_(dispatch, chain)
    , fast_chain_(chain)
{}

// Start/stop sequences.
//-----------------------------------------------------------------------------

void validate_transaction::start() {
    stopped_ = false;
}

void validate_transaction::stop() {
    stopped_ = true;
}

// Check.
//-----------------------------------------------------------------------------
// These checks are context free.

void validate_transaction::check(transaction_const_ptr tx, result_handler handler) const {
    // Run context free checks.
    handler(tx->check());
}

void validate_transaction::check_v2(chainv2::transaction::const_ptr tx, result_handler handler) const {
    // Run context free checks.
    handler(tx->check());
}


// Accept sequence.
//-----------------------------------------------------------------------------
// These checks require chain and tx state (net height and enabled forks).

void validate_transaction::accept(transaction_const_ptr tx, result_handler handler) const {
    //std::cout << "validate_transaction::accept - 1" << std::endl;

    // Populate chain state of the next block (tx pool).
    tx->validation.state = fast_chain_.chain_state();

    //std::cout << "validate_transaction::accept - 2" << std::endl;

    if (!tx->validation.state) {
        //std::cout << "validate_transaction::accept - 3" << std::endl;
        handler(error::operation_failed_23);
        return;
    }

    //std::cout << "validate_transaction::accept - 4" << std::endl;
    transaction_populator_.populate(tx, std::bind(&validate_transaction::handle_populated, this, _1, tx, handler));
    //std::cout << "validate_transaction::accept - 5" << std::endl;
}

void validate_transaction::handle_populated(code const& ec, transaction_const_ptr tx, result_handler handler) const {
    //std::cout << "validate_transaction::handle_populated - 1" << std::endl;
    if (stopped()) {
        //std::cout << "validate_transaction::handle_populated - 2" << std::endl;
        handler(error::service_stopped);
        return;
    }

    //std::cout << "validate_transaction::handle_populated - 3" << std::endl;
    if (ec) {
        //std::cout << "validate_transaction::handle_populated - 4" << std::endl;
        handler(ec);
        return;
    }

    //std::cout << "validate_transaction::handle_populated - 5" << std::endl;
    BITCOIN_ASSERT(tx->validation.state);

    // Run contextual tx checks.
    //std::cout << "validate_transaction::handle_populated - 6" << std::endl;
    handler(tx->accept());
    //std::cout << "validate_transaction::handle_populated - 7" << std::endl;
}

void validate_transaction::accept_v2(chainv2::transaction::const_ptr tx, result_handler handler) const {
    // Populate chain state of the next block (tx pool).
    auto const state = fast_chain_.chain_state();

    if ( ! state) {
        handler(error::operation_failed_23);
        return;
    }

    // Chain state is for the next block, so always > 0.
    BITCOIN_ASSERT(state->height() > 0);
    const auto chain_height = state->height() - 1u;

    //*************************************************************************
    // CONSENSUS:
    // It is OK for us to restrict *pool* transactions to those that do not
    // collide with any in the chain (as well as any in the pool) as collision
    // will result in monetary destruction and we don't want to facilitate it.
    // We must allow collisions in *block* validation if that is configured as
    // otherwise will will not follow the chain when a collision is mined.
    //*************************************************************************
    auto const tx_duplicate = fast_chain_.get_is_unspent_transaction(tx->hash(), chain_height, false);

    // Because txs include no proof of work we much short circuit here.
    // Otherwise a peer can flood us with repeat transactions to validate.
    // if (tx->validation.duplicate) {
    if (tx_duplicate) {
        handler(error::unspent_duplicate);
        return;
    }    

    transaction_populator_.populate_v2(tx, state, std::bind(&validate_transaction::handle_populated_v2, this, _1, tx, tx_duplicate, handler));
}

void validate_transaction::handle_populated_v2(code const& ec, chainv2::transaction::const_ptr tx, bool tx_duplicate, result_handler handler) const {
    if (stopped()) {
        handler(error::service_stopped);
        return;
    }

    if (ec) {
        handler(ec);
        return;
    }

    auto const state = fast_chain_.chain_state();
    // BITCOIN_ASSERT(tx->validation.state);
    BITCOIN_ASSERT(state);

    // Run contextual tx checks.
    handler(tx->accept(*state, tx_duplicate, true)); //TODO(fernando): eliminate the 3rd actual argument when default value formal argument be added
}


// void validate_transaction::accept_sequential(transaction_const_ptr tx, result_handler handler) const {
code validate_transaction::accept_sequential(transaction_const_ptr tx) const {

    // Populate chain state of the next block (tx pool).
    tx->validation.state = fast_chain_.chain_state();

    if (!tx->validation.state) {
        // handler(error::operation_failed_23);
        return error::operation_failed_23;
    }

    //transaction_populator_.populate_sequential(tx, std::bind(&validate_transaction::handle_populated_sequential, this, _1, tx, handler));

    code ec = transaction_populator_.populate_sequential(tx);
    if (ec) {
        // handler(ec);
        return ec;
    }

    BITCOIN_ASSERT(tx->validation.state);

    // Run contextual tx checks.
    // handler(tx->accept());
    return tx->accept();
}

// void validate_transaction::handle_populated_sequential(code const& ec, transaction_const_ptr tx, result_handler handler) const {
//     if (stopped()) {
//         handler(error::service_stopped);
//         return;
//     }

//     if (ec) {
//         handler(ec);
//         return;
//     }

//     BITCOIN_ASSERT(tx->validation.state);

//     // Run contextual tx checks.
//     handler(tx->accept());
// }

// Connect sequence.
//-----------------------------------------------------------------------------
// These checks require chain state, block state and perform script validation.

void validate_transaction::connect(transaction_const_ptr tx, result_handler handler) const {
    BITCOIN_ASSERT(tx->validation.state);
    auto const total_inputs = tx->inputs().size();

    // Return if there are no inputs to validate (will fail later).
    if (total_inputs == 0) {
        handler(error::success);
        return;
    }

    auto const buckets = std::min(dispatch_.size(), total_inputs);
    auto const join_handler = synchronize(handler, buckets, NAME "_validate");
    BITCOIN_ASSERT(buckets != 0);

    // If the priority threadpool is shut down when this is called the handler
    // will never be invoked, resulting in a threadpool.join indefinite hang.
    for (size_t bucket = 0; bucket < buckets; ++bucket) {
        dispatch_.concurrent(&validate_transaction::connect_inputs, this, tx, bucket, buckets, join_handler);
    }
}

void validate_transaction::connect_inputs(transaction_const_ptr tx, size_t bucket, size_t buckets, result_handler handler) const {
    BITCOIN_ASSERT(bucket < buckets);
    code ec(error::success);
    auto const forks = tx->validation.state->enabled_forks();
    auto const& inputs = tx->inputs();

    for (auto input_index = bucket; input_index < inputs.size(); input_index = ceiling_add(input_index, buckets)) {
        if (stopped()) {
            ec = error::service_stopped;
            break;
        }

        auto const& prevout = inputs[input_index].previous_output();

        if (!prevout.validation.cache.is_valid()) {
            ec = error::missing_previous_output;
            break;
        }

        if ((ec = validate_input::verify_script(*tx, input_index, forks))) {
            break;
        }
    }

    handler(ec);
}

void validate_transaction::connect_v2(chainv2::transaction::const_ptr tx, result_handler handler) const {
    BITCOIN_ASSERT(tx->validation.state);
    const auto total_inputs = tx->inputs().size();

    // Return if there are no inputs to validate (will fail later).
    if (total_inputs == 0) {
        handler(error::success);
        return;
    }

    const auto buckets = std::min(dispatch_.size(), total_inputs);
    const auto join_handler = synchronize(handler, buckets, NAME "_validate");
    BITCOIN_ASSERT(buckets != 0);

    // If the priority threadpool is shut down when this is called the handler
    // will never be invoked, resulting in a threadpool.join indefinite hang.
    for (size_t bucket = 0; bucket < buckets; ++bucket) {
        dispatch_.concurrent(&validate_transaction::connect_inputs_v2, this, tx, bucket, buckets, join_handler);
    }
}

void validate_transaction::connect_inputs_v2(chainv2::transaction::const_ptr tx, size_t bucket, size_t buckets, result_handler handler) const {
    BITCOIN_ASSERT(bucket < buckets);
    code ec(error::success);

    auto const state = fast_chain_.chain_state();
    BITCOIN_ASSERT(state);
    
    // const auto forks = tx->validation.state->enabled_forks();
    const auto forks = state->enabled_forks();

    const auto& inputs = tx->inputs();

    for (auto input_index = bucket; input_index < inputs.size(); input_index = ceiling_add(input_index, buckets)) {
        if (stopped()) {
            ec = error::service_stopped;
            break;
        }

        const auto& prevout = inputs[input_index].previous_output();

        if ( ! prevout.validation.cache.is_valid()) {
            ec = error::missing_previous_output;
            break;
        }

        if ((ec = validate_input::verify_script(*tx, input_index, forks))) {
            break;
        }
    }

    handler(ec);
}




void validate_transaction::connect_sequential(transaction_const_ptr tx, result_handler handler) const {
    BITCOIN_ASSERT(tx->validation.state);
    auto const total_inputs = tx->inputs().size();

    // Return if there are no inputs to validate (will fail later).
    if (total_inputs == 0) {
        handler(error::success);
        return;
    }

    // auto const buckets = std::min(dispatch_.size(), total_inputs);
    size_t const buckets = 1;
    // auto const join_handler = synchronize(handler, buckets, NAME "_validate");
    BITCOIN_ASSERT(buckets != 0);

    // If the priority threadpool is shut down when this is called the handler
    // will never be invoked, resulting in a threadpool.join indefinite hang.
    for (size_t bucket = 0; bucket < buckets; ++bucket) {
        connect_inputs_sequential(tx, bucket, buckets);
    }

    handler(error::success);
}

void validate_transaction::connect_inputs_sequential(transaction_const_ptr tx, size_t bucket, size_t buckets) const {
    BITCOIN_ASSERT(bucket < buckets);
    code ec(error::success);
    auto const forks = tx->validation.state->enabled_forks();
    auto const& inputs = tx->inputs();

    for (auto input_index = bucket; input_index < inputs.size(); input_index = ceiling_add(input_index, buckets)) {
        if (stopped()) {
            ec = error::service_stopped;
            break;
        }

        auto const& prevout = inputs[input_index].previous_output();

        if (!prevout.validation.cache.is_valid()) {
            ec = error::missing_previous_output;
            break;
        }

        if ((ec = validate_input::verify_script(*tx, input_index, forks))) {
            break;
        }
    }
}

}} // namespace libbitcoin::blockchain
