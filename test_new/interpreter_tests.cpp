/**
 * Copyright (c) 2018 Bitprim developers (see AUTHORS)
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

#include "doctest.h"
/*
#include <cstddef>
#include <bitcoin/database.hpp>
#include <bitcoin/blockchain/define.hpp>
#include <bitcoin/blockchain/pools/branch.hpp>
*/
#include <bitcoin/bitcoin/chain/transaction.hpp>
#include <bitcoin/bitcoin/formats/base_16.hpp>
#include <bitcoin/bitcoin/utility/data.hpp>
#include <bitcoin/bitcoin/utility/container_source.hpp>
#include <bitcoin/bitcoin/utility/istream_reader.hpp>

#include <bitprim/keoken/transaction_extractor.hpp>

#include <bitprim/keoken/state.hpp>
#include <bitprim/keoken/interpreter.hpp>

using namespace bitprim::keoken;
using libbitcoin::hash_digest;
using libbitcoin::hash_literal;
using libbitcoin::wallet::payment_address;

using libbitcoin::data_chunk;
using libbitcoin::to_chunk;
using libbitcoin::base16_literal;
using libbitcoin::data_source;
using libbitcoin::istream_reader;

class BCB_API fast_chain_dummy
{
public:
    // This avoids conflict with the result_handler in safe_chain.
    typedef libbitcoin::handle0 complete_handler;

    // Readers.
    // ------------------------------------------------------------------------
/*
    /// Get the set of block gaps in the chain.
    bool get_gaps(
        libbitcoin::database::block_database::heights& out_gaps) const {
            return true;
        }

    /// Get a determination of whether the block hash exists in the store.
    bool get_block_exists(const hash_digest& block_hash) const {
        return true;
    }

    /// Get the hash of the block if it exists.
    bool get_block_hash(hash_digest& out_hash, size_t height) const {
            return true;
    }

    /// Get the work of the branch starting at the given height.
    bool get_branch_work(uint256_t& out_work,
        const uint256_t& maximum, size_t from_height) const {
            return true;
         }

    /// Get the header of the block at the given height.
    bool get_header(chain::header& out_header,
        size_t height) const {
            return true;
         }

    /// Get the height of the block with the given hash.
    bool get_height(size_t& out_height,
        const hash_digest& block_hash) const {
            return true;
         }

    /// Get the bits of the block with the given height.
    bool get_bits(uint32_t& out_bits, const size_t& height) const {
        return true;
     }

    /// Get the timestamp of the block with the given height.
    bool get_timestamp(uint32_t& out_timestamp,
        const size_t& height) const { 
            return true;
        }

    /// Get the version of the block with the given height.
    bool get_version(uint32_t& out_version,
        const size_t& height) const { 
            return true;
        }

    /// Get height of latest block.
    bool get_last_height(size_t& out_height) const {
        return true;
     }
*/
    /// Get the output that is referenced by the outpoint.
    bool get_output(libbitcoin::chain::output& out_output, size_t& out_height,
        uint32_t& out_median_time_past, bool& out_coinbase, 
        const libbitcoin::chain::output_point& outpoint, size_t branch_height,
        bool require_confirmed) const { 
            return false;
        }
        
/*
    /// Determine if an unspent transaction exists with the given hash.
    bool get_is_unspent_transaction(const hash_digest& hash,
        size_t branch_height, bool require_confirmed) const {
            return true;
         }

    /// Get position data for a transaction.
    bool get_transaction_position(size_t& out_height,
        size_t& out_position, const hash_digest& hash,
        bool require_confirmed) const {
            return true;
         }

    /////// Get the transaction of the given hash and its block height.
    ////transaction_ptr get_transaction(size_t& out_block_height,
    ////    const hash_digest& hash, bool require_confirmed) const { }

    // Writers.
    // ------------------------------------------------------------------------

    /// Create flush lock if flush_writes is true, and set sequential lock.
    bool begin_insert() const {
        return true;
     }

    /// Clear flush lock if flush_writes is true, and clear sequential lock.
    bool end_insert() const {
        return true;
     }

    /// Insert a block to the blockchain, height is checked for existence.
    bool insert(block_const_ptr block, size_t height) {
        return true;
    }

    /// Push an unconfirmed transaction to the tx table and index outputs.
    void push(transaction_const_ptr tx, dispatcher& dispatch,
        complete_handler handler) {
            
        }

    /// Swap incoming and outgoing blocks, height is validated.
    void reorganize(const config::checkpoint& fork_point,
        block_const_ptr_list_const_ptr incoming_blocks,
        block_const_ptr_list_ptr outgoing_blocks, dispatcher& dispatch,
        complete_handler handler) {
            
        }

    // Properties
    // ------------------------------------------------------------------------

    /// Get a reference to the chain state relative to the next block.
    chain::chain_state::ptr chain_state() const {
        return nullptr;
     }

    /// Get a reference to the chain state relative to the next block.
    chain::chain_state::ptr chain_state(
        branch::const_ptr branch) const {
            return nullptr;
         }
         */
};



TEST_CASE("[interpreter_tx_without_output] ") {

    using blk_t = fast_chain_dummy;

    blk_t chain_;
    state state_(1);
    interpreter<blk_t> interpreter_(chain_, state_);

     data_chunk raw_tx = to_chunk(base16_literal(
        "0100000001f08e44a96bfb5ae63eda1a6620adae37ee37ee4777fb0336e1bbbc"
        "4de65310fc010000006a473044022050d8368cacf9bf1b8fb1f7cfd9aff63294"
        "789eb1760139e7ef41f083726dadc4022067796354aba8f2e02363c5e510aa7e"
        "2830b115472fb31de67d16972867f13945012103e589480b2f746381fca01a9b"
        "12c517b7a482a203c8b2742985da0ac72cc078f2ffffffff02f0c9c467000000"
        "001976a914d9d78e26df4e4601cf9b26d09c7b280ee764469f88ac80c4600f00"
        "0000001976a9141ee32412020a324b93b1a1acfdfff6ab9ca8fac288ac000000"
        "00"));

    libbitcoin::chain::transaction tx;
    tx.from_data(raw_tx);

    REQUIRE(interpreter_.process(1550,tx) == error_code_t::not_keoken_tx);
}



TEST_CASE("[interpreter_tx_create_asset] ") {

    using blk_t = fast_chain_dummy;

    blk_t chain_;
    state state_(1);
    interpreter<blk_t> interpreter_(chain_, state_);

    data_chunk raw_tx = to_chunk(base16_literal("01000000016ef955ef813fd167438ef35d862d9dcb299672b22ccbc20da598f5ddc59d69aa000000006a473044022056f0511deaaf7485d7f17ec953ad7f6ede03a73c957f98629d290f890aee165602207f1f1a4c04eadeafcd3f4eacd0bb85a45803ef715bfc9a3375fed472212b67fb4121036735a1fe1b39fbe39e629a6dd680bf00b13aefe40d9f3bb6f863d2c4094ddd0effffffff02a007052a010000001976a9140ef6dfde07323619edd2440ca0a54d311df1ee8b88ac00000000000000001b6a0400004b5014000000004269747072696d0000000000000f424000000000"));

    libbitcoin::chain::transaction tx;
    tx.from_data(raw_tx);

    REQUIRE(interpreter_.process(1550,tx) == error_code_t::invalid_asset_creator);
}

