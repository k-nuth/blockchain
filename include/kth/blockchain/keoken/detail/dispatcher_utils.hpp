// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef KTH_BLOCKCHAIN_KEOKEN_DETAIL_DISPATCHER_UTILS_HPP_
#define KTH_BLOCKCHAIN_KEOKEN_DETAIL_DISPATCHER_UTILS_HPP_

#include <kth/keoken/error.hpp>
#include <kth/keoken/transaction_processors/commons.hpp>
// #include <kth/keoken/message/create_asset.hpp>
// #include <kth/keoken/message/send_tokens.hpp>
// #include <kth/keoken/transaction_extractor.hpp>
// #include <kth/keoken/transaction_processors/v0/transactions.hpp>

// #define Tuple typename

namespace kth::keoken {
namespace detail {

template <typename T, message_type_t x>
struct find_type {

    template <size_t... Is>
    constexpr 
    bool call_impl(knuth::index_sequence<Is...>) const {
        return false;
    }

    template <size_t I, size_t... Is>
    constexpr 
    bool call_impl(knuth::index_sequence<I, Is...>) const {
        return (x == knuth::tuple_element_t<I, T>::message_type) ? 
                true :
                call_impl(knuth::index_sequence<Is...>{});
    }

    constexpr 
    bool operator()() const {
        using idxs_t = knuth::make_index_sequence<std::tuple_size<T>::value>;
        return call_impl(idxs_t{});
    }
};

template <typename T>
struct no_repeated_types {

    template <size_t... Is>
    constexpr 
    bool call_impl(knuth::index_sequence<Is...>) const {
        return true;
    }

    template <size_t I, size_t... Is>
    constexpr 
    bool call_impl(knuth::index_sequence<I, Is...>) const {
        using idxs_t = knuth::index_sequence<Is...>;
        return find_type<T, knuth::tuple_element_t<I, T>::message_type>{}.call_impl(idxs_t{}) ?
            false :
            call_impl(idxs_t{});
    }

    constexpr
    bool operator()() const {
        using idxs_t = knuth::make_index_sequence<std::tuple_size<T>::value>;
        return call_impl(idxs_t{});
    }

};

} // namespace detail
} // namespace keoken
} // namespace kth

#endif //KTH_BLOCKCHAIN_KEOKEN_DETAIL_DISPATCHER_UTILS_HPP_
