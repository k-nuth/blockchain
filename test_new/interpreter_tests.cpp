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

class BCB_API fast_chain_dummy_return_false
{
public:
    /// Get the output that is referenced by the outpoint.
    bool get_output(libbitcoin::chain::output& out_output, size_t& out_height,
        uint32_t& out_median_time_past, bool& out_coinbase, 
        const libbitcoin::chain::output_point& outpoint, size_t branch_height,
        bool require_confirmed) const { 
            return false;
        }
};


class BCB_API fast_chain_dummy_return_true
{
public:
    
    fast_chain_dummy_return_true(libbitcoin::chain::transaction& tx)
        :tx_(tx) {

    }

    /// Get the output that is referenced by the outpoint.
    bool get_output(libbitcoin::chain::output& out_output, size_t& out_height,
        uint32_t& out_median_time_past, bool& out_coinbase, 
        const libbitcoin::chain::output_point& outpoint, size_t branch_height,
        bool require_confirmed) const { 
            
            out_output = tx_.outputs()[0];
            out_height = 123;
            out_median_time_past = 456;
            out_coinbase = false;
            
            return true;
        }
private:
    libbitcoin::chain::transaction& tx_;
};


TEST_CASE("[interpreter_tx_without_output] ") {

    using blk_t = fast_chain_dummy_return_false;

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



TEST_CASE("[interpreter_tx_create_asset_invalid] ") {

    using blk_t = fast_chain_dummy_return_false;

    blk_t chain_;
    state state_(1);
    interpreter<blk_t> interpreter_(chain_, state_);

    data_chunk raw_tx = to_chunk(base16_literal("01000000016ef955ef813fd167438ef35d862d9dcb299672b22ccbc20da598f5ddc59d69aa000000006a473044022056f0511deaaf7485d7f17ec953ad7f6ede03a73c957f98629d290f890aee165602207f1f1a4c04eadeafcd3f4eacd0bb85a45803ef715bfc9a3375fed472212b67fb4121036735a1fe1b39fbe39e629a6dd680bf00b13aefe40d9f3bb6f863d2c4094ddd0effffffff02a007052a010000001976a9140ef6dfde07323619edd2440ca0a54d311df1ee8b88ac00000000000000001b6a0400004b5014000000004269747072696d0000000000000f424000000000"));

    libbitcoin::chain::transaction tx;
    tx.from_data(raw_tx);

    REQUIRE(interpreter_.process(1550,tx) == error_code_t::invalid_asset_creator);
}

TEST_CASE("[interpreter_tx_create_asset_valid] ") {

    using blk_t = fast_chain_dummy_return_true;

    data_chunk raw_tx = to_chunk(base16_literal("01000000016ef955ef813fd167438ef35d862d9dcb299672b22ccbc20da598f5ddc59d69aa000000006a473044022056f0511deaaf7485d7f17ec953ad7f6ede03a73c957f98629d290f890aee165602207f1f1a4c04eadeafcd3f4eacd0bb85a45803ef715bfc9a3375fed472212b67fb4121036735a1fe1b39fbe39e629a6dd680bf00b13aefe40d9f3bb6f863d2c4094ddd0effffffff02a007052a010000001976a9140ef6dfde07323619edd2440ca0a54d311df1ee8b88ac00000000000000001b6a0400004b5014000000004269747072696d0000000000000f424000000000"));

    libbitcoin::chain::transaction tx;
    tx.from_data(raw_tx);

    blk_t chain_(tx);
    state state_(1);
    interpreter<blk_t> interpreter_(chain_, state_);

    auto const& ret = state_.get_assets();
    REQUIRE(ret.size() == 0);

    REQUIRE(interpreter_.process(1550,tx) == error_code_t::success);

    auto const& ret2 = state_.get_assets();
    REQUIRE(ret2.size() == 1);
}

TEST_CASE("[interpreter_tx_send_token_insufficient_money] ") {

    using blk_t = fast_chain_dummy_return_true;
    
    state state_(2);

    data_chunk raw_tx = to_chunk(base16_literal("01000000016ef955ef813fd167438ef35d862d9dcb299672b22ccbc20da598f5ddc59d69aa000000006a473044022056f0511deaaf7485d7f17ec953ad7f6ede03a73c957f98629d290f890aee165602207f1f1a4c04eadeafcd3f4eacd0bb85a45803ef715bfc9a3375fed472212b67fb4121036735a1fe1b39fbe39e629a6dd680bf00b13aefe40d9f3bb6f863d2c4094ddd0effffffff02a007052a010000001976a9140ef6dfde07323619edd2440ca0a54d311df1ee8b88ac00000000000000001b6a0400004b5014000000004269747072696d0000000000000f424000000000"));
    libbitcoin::chain::transaction tx;
    tx.from_data(raw_tx);

    blk_t chain1_(tx);
    interpreter<blk_t> interpreter1_(chain1_, state_);
    interpreter1_.process(1550,tx);

    data_chunk raw_send_tx = to_chunk(base16_literal("01000000011e572671f2cff67190785b52e72dc221b1c3a092159b70ec14bc2f433c4dcb2f000000006b48304502210084c05aa0d2a60f69045b46179cff207fde8003ea07a90a75d934ec35d6a46a3a02205b328724e736d9400b3f13ac6e0e49462048dfc2c9a7bd1be9944aa9baa455144121036735a1fe1b39fbe39e629a6dd680bf00b13aefe40d9f3bb6f863d2c4094ddd0effffffff03204e0000000000001976a914071ed73aa65c19f86c88a29a789210fafc8d675188ac606b042a010000001976a9140ef6dfde07323619edd2440ca0a54d311df1ee8b88ac0000000000000000176a0400004b50100000000100000002000000000000006400000000"));
    libbitcoin::chain::transaction tx_send;
    tx_send.from_data(raw_send_tx);

    blk_t chain2_(tx_send);
    interpreter<blk_t> interpreter2_(chain2_, state_);

    REQUIRE(interpreter2_.process(1550,tx_send) == error_code_t::insufficient_money);
}
