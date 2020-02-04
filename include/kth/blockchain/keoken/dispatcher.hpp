// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef KTH_BLOCKCHAIN_KEOKEN_DISPATCHER_HPP_
#define KTH_BLOCKCHAIN_KEOKEN_DISPATCHER_HPP_

#include <kth/keoken/detail/dispatcher_utils.hpp>
#include <kth/keoken/error.hpp>
#include <kth/keoken/message/create_asset.hpp>
#include <kth/keoken/message/send_tokens.hpp>
#include <kth/keoken/transaction_extractor.hpp>
#include <kth/keoken/transaction_processors/v0/transactions.hpp>

// #define Tuple typename

namespace kth {
namespace keoken {
namespace v0 {

template <typename T>
struct dispatcher {


#ifdef KTH_USE_DOMAIN
    // template <Reader R, KTH_IS_READER(R)>
    template <typename State, typename Fastchain, Reader R, size_t... Is>
    constexpr 
    error::error_code_t call_impl(message_type_t, State&, Fastchain const&, size_t, bc::chain::transaction const&, R&, knuth::index_sequence<Is...>) const {
#else
    template <typename State, typename Fastchain, size_t... Is>
    constexpr 
    error::error_code_t call_impl(message_type_t, State&, Fastchain const&, size_t, bc::chain::transaction const&, bc::reader&, knuth::index_sequence<Is...>) const {
#endif // KTH_USE_DOMAIN
        return error::not_recognized_type;
    }


#ifdef KTH_USE_DOMAIN
    // template <Reader R, KTH_IS_READER(R)>
    template <typename State, typename Fastchain, size_t I, Reader R, size_t... Is>
    constexpr 
    error::error_code_t call_impl(message_type_t mt, State& state, Fastchain const& fast_chain, size_t block_height, bc::chain::transaction const& tx, R& source, knuth::index_sequence<I, Is...>) const {
#else
    template <typename State, typename Fastchain, size_t I, size_t... Is>
    constexpr 
    error::error_code_t call_impl(message_type_t mt, State& state, Fastchain const& fast_chain, size_t block_height, bc::chain::transaction const& tx, bc::reader& source, knuth::index_sequence<I, Is...>) const {
#endif // KTH_USE_DOMAIN

        using msg_t = knuth::tuple_element_t<I, T>;
        using idxs_t = knuth::index_sequence<Is...>;
        return mt == msg_t::message_type 
                ? msg_t{}(state, fast_chain, block_height, tx, source) 
                : call_impl(mt, state, fast_chain, block_height, tx, source, idxs_t{});
    }


#ifdef KTH_USE_DOMAIN
    template <typename State, typename Fastchain, Reader R, KTH_IS_READER(R)>
    constexpr 
    error::error_code_t operator()(message_type_t mt, State& state, Fastchain const& fast_chain, size_t block_height, bc::chain::transaction const& tx, R& source) const {
#else
    template <typename State, typename Fastchain>
    constexpr 
    error::error_code_t operator()(message_type_t mt, State& state, Fastchain const& fast_chain, size_t block_height, bc::chain::transaction const& tx, bc::reader& source) const {
#endif // KTH_USE_DOMAIN

        static_assert(detail::no_repeated_types<T>{}(), "repeated transaction types in transaction list");
        using idxs_t = knuth::make_index_sequence<std::tuple_size<T>::value>;
        return call_impl(mt, state, fast_chain, block_height, tx, source, idxs_t{});
    }
};

} // namespace v0
} // namespace keoken
} // namespace kth

#endif //KTH_BLOCKCHAIN_KEOKEN_DISPATCHER_HPP_
