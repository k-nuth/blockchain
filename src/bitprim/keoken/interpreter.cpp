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

#include <bitprim/keoken/interpreter.hpp>

#include <bitcoin/bitcoin/chain/output.hpp>
#include <bitcoin/bitcoin/utility/container_source.hpp>
#include <bitcoin/bitcoin/utility/istream_reader.hpp>

#include <bitprim/keoken/transaction_extractor.hpp>

using libbitcoin::chain::transaction;
using libbitcoin::chain::output;
using libbitcoin::data_chunk;
using libbitcoin::reader;
using libbitcoin::wallet::payment_address;

namespace bitprim {
namespace keoken {

using error::error_code_t;

// explicit
interpreter::interpreter(libbitcoin::blockchain::fast_chain& fast_chain, state& st)
    : fast_chain_(fast_chain)
    , state_(st)
{}

error_code_t interpreter::process(size_t block_height, transaction const& tx) {
    using libbitcoin::istream_reader;
    using libbitcoin::data_source;

    auto data = first_keoken_output(tx);
    if ( ! data.empty()) {
        data_source ds(data);
        istream_reader source(ds);

        return version_dispatcher(block_height, tx, source);
    }
    return error::not_keoken_tx;
}

error_code_t interpreter::version_dispatcher(size_t block_height, transaction const& tx, reader& source) {

    auto version = source.read_2_bytes_big_endian();
    if ( ! source) return error::invalid_version_number;

    switch (static_cast<version_t>(version)) {
        case version_t::zero:
            return version_0_type_dispatcher(block_height, tx, source);
    }
    return error::not_recognized_version_number;
}

error_code_t interpreter::version_0_type_dispatcher(size_t block_height, transaction const& tx, reader& source) {
    auto type = source.read_2_bytes_big_endian();
    if ( ! source) return error::invalid_type;

    switch (static_cast<message_type_t>(type)) {
        case message_type_t::create_asset:
            return process_create_asset_version_0(block_height, tx, source);
        case message_type_t::send_tokens:
            return process_send_tokens_version_0(block_height, tx, source);
    }
    return error::not_recognized_type;
}

payment_address interpreter::get_first_input_addr(transaction const& tx) const {
    auto const& owner_input = tx.inputs()[0];

    output out_output;
    size_t out_height;
    uint32_t out_median_time_past;
    bool out_coinbase;

    if ( ! fast_chain_.get_output(out_output, out_height, out_median_time_past, out_coinbase, 
                                  owner_input.previous_output(), libbitcoin::max_size_t, true)) {
        return payment_address{};
    }

    return out_output.address();
}

error_code_t interpreter::process_create_asset_version_0(size_t block_height, transaction const& tx, reader& source) {
    auto msg = message::create_asset::factory_from_data(source);
    if ( ! source) return error::invalid_create_asset_message;

    if (msg.amount() <= 0) {
        return error::invalid_asset_amount;
    }

    auto owner = get_first_input_addr(tx);
    if ( ! owner) {
        return error::invalid_asset_creator;
    }

    state_.create_asset(msg.name(), msg.amount(), std::move(owner), block_height, tx.hash());
    return error::success;
}

std::pair<payment_address, payment_address> interpreter::get_send_tokens_addrs(transaction const& tx) const {
    auto source = get_first_input_addr(tx);
    if ( ! source) {
        return {payment_address{}, payment_address{}};
    }

    auto it = std::find_if(tx.outputs().begin(), tx.outputs().end(), [&source](output const& o) {
        return o.address() && o.address() != source;
    });

    if (it == tx.outputs().end()) {
        return {std::move(source), payment_address{}};        
    }

    return {std::move(source), it->address()};
}

error_code_t interpreter::process_send_tokens_version_0(size_t block_height, transaction const& tx, reader& source) {
    auto msg = message::send_tokens::factory_from_data(source);
    if ( ! source) return error::invalid_send_tokens_message;

    if ( ! state_.asset_id_exists(msg.asset_id())) {
        return error::asset_id_does_not_exist;
    }

    if (msg.amount() <= 0) {
        return error::invalid_asset_amount;
    }
  
    auto wallets = get_send_tokens_addrs(tx);
    payment_address const& source_addr = wallets.first;
    payment_address const& target_addr = wallets.second;

    if ( ! source_addr) return error::invalid_source_address;
    if ( ! target_addr) return error::invalid_target_address;

    if (state_.get_balance(msg.asset_id(), source_addr) < msg.amount()) {
        return error::insufficient_money;
    }

    state_.create_balance_entry(msg.asset_id(), msg.amount(), source_addr, target_addr, block_height, tx.hash());
    return error::success;
}

} // namespace keoken
} // namespace bitprim
