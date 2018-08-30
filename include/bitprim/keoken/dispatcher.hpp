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

#ifndef BITPRIM_BLOCKCHAIN_KEOKEN_DISPATCHER_HPP_
#define BITPRIM_BLOCKCHAIN_KEOKEN_DISPATCHER_HPP_

#include <bitprim/integer_sequence.hpp>
#include <bitprim/keoken/detail/dispatcher_utils.hpp>
#include <bitprim/keoken/error.hpp>
#include <bitprim/keoken/message/create_asset.hpp>
#include <bitprim/keoken/message/send_tokens.hpp>
#include <bitprim/keoken/transaction_extractor.hpp>
#include <bitprim/keoken/transaction_processors/v0/transactions.hpp>
#include <bitprim/tuple_element.hpp>

// #define Tuple typename

namespace bitprim {
namespace keoken {
namespace v0 {

template <typename T>
struct dispatcher {
    template <typename State, typename Fastchain, size_t... Is>
    constexpr 
    error::error_code_t call_impl(message_type_t, State&, Fastchain const&, size_t, bc::chain::transaction const&, bc::reader&, bitprim::index_sequence<Is...>) const {
        return error::not_recognized_type;
    }

    template <typename State, typename Fastchain, size_t I, size_t... Is>
    constexpr 
    error::error_code_t call_impl(message_type_t mt, State& state, Fastchain const& fast_chain, size_t block_height, bc::chain::transaction const& tx, bc::reader& source, bitprim::index_sequence<I, Is...>) const {
        using msg_t = bitprim::tuple_element_t<I, T>;
        using idxs_t = bitprim::index_sequence<Is...>;
        return mt == msg_t::message_type 
                ? msg_t{}(state, fast_chain, block_height, tx, source) 
                : call_impl(mt, state, fast_chain, block_height, tx, source, idxs_t{});
    }

    template <typename State, typename Fastchain>
    constexpr 
    error::error_code_t operator()(message_type_t mt, State& state, Fastchain const& fast_chain, size_t block_height, bc::chain::transaction const& tx, bc::reader& source) const {
        static_assert(detail::no_repeated_types<T>{}(), "repeated transaction types in transaction list");
        using idxs_t = bitprim::make_index_sequence<std::tuple_size<T>::value>;
        return call_impl(mt, state, fast_chain, block_height, tx, source, idxs_t{});
    }
};

} // namespace v0
} // namespace keoken
} // namespace bitprim

#endif //BITPRIM_BLOCKCHAIN_KEOKEN_DISPATCHER_HPP_
