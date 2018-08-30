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

#ifndef BITPRIM_BLOCKCHAIN_KEOKEN_DETAIL_DISPATCHER_UTILS_HPP_
#define BITPRIM_BLOCKCHAIN_KEOKEN_DETAIL_DISPATCHER_UTILS_HPP_

#include <bitprim/integer_sequence.hpp>
#include <bitprim/keoken/error.hpp>
#include <bitprim/keoken/transaction_processors/commons.hpp>
// #include <bitprim/keoken/message/create_asset.hpp>
// #include <bitprim/keoken/message/send_tokens.hpp>
// #include <bitprim/keoken/transaction_extractor.hpp>
// #include <bitprim/keoken/transaction_processors/v0/transactions.hpp>
#include <bitprim/tuple_element.hpp>

// #define Tuple typename

namespace bitprim {
namespace keoken {
namespace detail {

template <typename T, message_type_t x>
struct find_type {

    template <size_t... Is>
    constexpr 
    bool call_impl(bitprim::index_sequence<Is...>) const {
        return false;
    }

    template <size_t I, size_t... Is>
    constexpr 
    bool call_impl(bitprim::index_sequence<I, Is...>) const {
        return (x == bitprim::tuple_element_t<I, T>::message_type) ? 
                true :
                call_impl(bitprim::index_sequence<Is...>{});
    }

    constexpr 
    bool operator()() const {
        using idxs_t = bitprim::make_index_sequence<std::tuple_size<T>::value>;
        return call_impl(idxs_t{});
    }
};

template <typename T>
struct no_repeated_types {

    template <size_t... Is>
    constexpr 
    bool call_impl(bitprim::index_sequence<Is...>) const {
        return true;
    }

    template <size_t I, size_t... Is>
    constexpr 
    bool call_impl(bitprim::index_sequence<I, Is...>) const {
        using idxs_t = bitprim::index_sequence<Is...>;
        return find_type<T, bitprim::tuple_element_t<I, T>::message_type>{}.call_impl(idxs_t{}) ?
            false :
            call_impl(idxs_t{});
    }

    constexpr
    bool operator()() const {
        using idxs_t = bitprim::make_index_sequence<std::tuple_size<T>::value>;
        return call_impl(idxs_t{});
    }

};

} // namespace detail
} // namespace keoken
} // namespace bitprim

#endif //BITPRIM_BLOCKCHAIN_KEOKEN_DETAIL_DISPATCHER_UTILS_HPP_
