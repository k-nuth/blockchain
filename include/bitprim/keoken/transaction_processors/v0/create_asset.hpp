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

#ifndef BITPRIM_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_CREATE_ASSET_HPP_
#define BITPRIM_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_CREATE_ASSET_HPP_

#include <bitprim/keoken/error.hpp>
#include <bitprim/keoken/transaction_processors/commons.hpp>

namespace bitprim {
namespace keoken {
namespace transaction_processors {
namespace v0 {

struct create_asset {
    static constexpr message_type_t message_type = message_type_t::create_asset;

    template <typename State, typename Fastchain>
    error::error_code_t operator()(State& state, Fastchain const& fast_chain, size_t block_height, bc::chain::transaction const& tx, bc::reader& source) const {
        auto msg = message::create_asset::factory_from_data(source);
        if ( ! source) return error::invalid_create_asset_message;

        if (msg.amount() <= 0) {
            return error::invalid_asset_amount;
        }

        auto owner = get_first_input_addr(fast_chain, tx);
        if ( ! owner) {
            return error::invalid_asset_creator;
        }

        state.create_asset(msg.name(), msg.amount(), std::move(owner), block_height, tx.hash());
        return error::success;
    }
};

} // namespace v0
} // namespace transaction_processors
} // namespace keoken
} // namespace bitprim

#endif //BITPRIM_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_CREATE_ASSET_HPP_
