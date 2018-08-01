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

#ifndef BITPRIM_BLOCKCHAIN_KEOKEN_INTERPRETER_HPP_
#define BITPRIM_BLOCKCHAIN_KEOKEN_INTERPRETER_HPP_

#include <bitcoin/blockchain/interface/fast_chain.hpp>

#include <bitcoin/bitcoin/chain/output.hpp>
#include <bitcoin/bitcoin/chain/transaction.hpp>
#include <bitcoin/bitcoin/utility/container_source.hpp>
#include <bitcoin/bitcoin/utility/istream_reader.hpp>
#include <bitcoin/bitcoin/utility/reader.hpp>

#include <bitprim/keoken/error.hpp>
#include <bitprim/keoken/message/base.hpp>
#include <bitprim/keoken/message/create_asset.hpp>
#include <bitprim/keoken/state.hpp>
#include <bitprim/keoken/transaction_extractor.hpp>

namespace bitprim {
namespace keoken {

enum class version_t {
    zero = 0
};

enum class message_type_t {
    create_asset = 0,
    send_tokens = 1
};

template <typename Fastchain>
class interpreter {
public:
    explicit
    interpreter(Fastchain& fast_chain, state& st);

    // non-copyable class
    interpreter(interpreter const&) = delete;
    interpreter& operator=(interpreter const&) = delete;

    error::error_code_t process(size_t block_height, bc::chain::transaction const& tx);

private:
    bc::wallet::payment_address get_first_input_addr(bc::chain::transaction const& tx) const;
    std::pair<bc::wallet::payment_address, bc::wallet::payment_address> get_send_tokens_addrs(bc::chain::transaction const& tx) const;
    
    error::error_code_t version_dispatcher(size_t block_height, bc::chain::transaction const& tx, bc::reader& source);
    error::error_code_t version_0_type_dispatcher(size_t block_height, bc::chain::transaction const& tx, bc::reader& source);
    error::error_code_t process_create_asset_version_0(size_t block_height, bc::chain::transaction const& tx, bc::reader& source);
    error::error_code_t process_send_tokens_version_0(size_t block_height, bc::chain::transaction const& tx, bc::reader& source);

    state& state_;
    Fastchain& fast_chain_;
};


using error::error_code_t;

// explicit
template <typename Fastchain>
interpreter<Fastchain>::interpreter(Fastchain& fast_chain, state& st)
    : fast_chain_(fast_chain)
    , state_(st)
{}

template <typename Fastchain>
error_code_t interpreter<Fastchain>::process(size_t block_height, bc::chain::transaction const& tx) {
    using bc::istream_reader;
    using bc::data_source;

    auto data = first_keoken_output(tx);
    if ( ! data.empty()) {
        data_source ds(data);
        istream_reader source(ds);

        return version_dispatcher(block_height, tx, source);
    }
    return error::not_keoken_tx;
}

template <typename Fastchain>
error_code_t interpreter<Fastchain>::version_dispatcher(size_t block_height, bc::chain::transaction const& tx, bc::reader& source) {

    auto version = source.read_2_bytes_big_endian();
    if ( ! source) return error::invalid_version_number;

    switch (static_cast<version_t>(version)) {
        case version_t::zero:
            return version_0_type_dispatcher(block_height, tx, source);
    }
    return error::not_recognized_version_number;
}

template <typename Fastchain>
error_code_t interpreter<Fastchain>::version_0_type_dispatcher(size_t block_height, bc::chain::transaction const& tx, bc::reader& source) {
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

template <typename Fastchain>
bc::wallet::payment_address interpreter<Fastchain>::get_first_input_addr(bc::chain::transaction const& tx) const {
    auto const& owner_input = tx.inputs()[0];

    bc::chain::output out_output;
    size_t out_height;
    uint32_t out_median_time_past;
    bool out_coinbase;

    if ( ! fast_chain_.get_output(out_output, out_height, out_median_time_past, out_coinbase, 
                                  owner_input.previous_output(), bc::max_size_t, true)) {
        return bc::wallet::payment_address{};
    }

    return out_output.address();
}

template <typename Fastchain>
error_code_t interpreter<Fastchain>::process_create_asset_version_0(size_t block_height, bc::chain::transaction const& tx, bc::reader& source) {
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

template <typename Fastchain>
std::pair<bc::wallet::payment_address, bc::wallet::payment_address> interpreter<Fastchain>::get_send_tokens_addrs(bc::chain::transaction const& tx) const {
    auto source = get_first_input_addr(tx);
    if ( ! source) {
        return {bc::wallet::payment_address{}, bc::wallet::payment_address{}};
    }

    auto it = std::find_if(tx.outputs().begin(), tx.outputs().end(), [&source](output const& o) {
        return o.address() && o.address() != source;
    });

    if (it == tx.outputs().end()) {
        return {std::move(source), bc::wallet::payment_address{}};        
    }

    return {std::move(source), it->address()};
}

template <typename Fastchain>
error_code_t interpreter<Fastchain>::process_send_tokens_version_0(size_t block_height, bc::chain::transaction const& tx, bc::reader& source) {
    auto msg = message::send_tokens::factory_from_data(source);
    if ( ! source) return error::invalid_send_tokens_message;

    if ( ! state_.asset_id_exists(msg.asset_id())) {
        return error::asset_id_does_not_exist;
    }

    if (msg.amount() <= 0) {
        return error::invalid_asset_amount;
    }
  
    auto wallets = get_send_tokens_addrs(tx);
    auto const& source_addr = wallets.first;
    auto const& target_addr = wallets.second;

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

#endif //BITPRIM_BLOCKCHAIN_KEOKEN_INTERPRETER_HPP_
