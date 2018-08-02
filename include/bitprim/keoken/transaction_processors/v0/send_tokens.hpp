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

#ifndef BITPRIM_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_SEND_TOKENS_HPP_
#define BITPRIM_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_SEND_TOKENS_HPP_

#include <bitprim/keoken/error.hpp>
#include <bitprim/keoken/transaction_processors/commons.hpp>

namespace bitprim {
namespace keoken {
namespace transaction_processors {
namespace v0 {

struct send_tokens {
    static constexpr message_type_t message_type = message_type_t::send_tokens;

    template <typename State, typename Fastchain>
    error::error_code_t operator()(State& state, Fastchain const& fast_chain, size_t block_height, bc::chain::transaction const& tx, bc::reader& source) const {
        auto msg = message::send_tokens::factory_from_data(source);
        if ( ! source) return error::invalid_send_tokens_message;

        if ( ! state.asset_id_exists(msg.asset_id())) {
            return error::asset_id_does_not_exist;
        }

        if (msg.amount() <= 0) {
            return error::invalid_asset_amount;
        }
    
        auto wallets = get_send_tokens_addrs(fast_chain, tx);
        auto const& source_addr = wallets.first;
        auto const& target_addr = wallets.second;

        if ( ! source_addr) return error::invalid_source_address;
        if ( ! target_addr) return error::invalid_target_address;

        if (state.get_balance(msg.asset_id(), source_addr) < msg.amount()) {
            return error::insufficient_money;
        }

        state.create_balance_entry(msg.asset_id(), msg.amount(), source_addr, target_addr, block_height, tx.hash());
        return error::success;
    }
};


} // namespace v0
} // namespace transaction_processors
} // namespace keoken
} // namespace bitprim

#endif //BITPRIM_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_SEND_TOKENS_HPP_
