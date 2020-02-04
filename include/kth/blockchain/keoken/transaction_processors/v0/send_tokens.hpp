// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef KTH_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_SEND_TOKENS_HPP_
#define KTH_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_SEND_TOKENS_HPP_

#include <kth/keoken/error.hpp>
#include <kth/keoken/transaction_processors/commons.hpp>

namespace kth {
namespace keoken {
namespace transaction_processors {
namespace v0 {

struct send_tokens {
    static constexpr message_type_t message_type = message_type_t::send_tokens;

#ifdef KTH_USE_DOMAIN
    template <typename State, typename Fastchain, Reader R, KTH_IS_READER(R)>
    error::error_code_t operator()(State& state, Fastchain const& fast_chain, size_t block_height, bc::chain::transaction const& tx, R& source) const {
#else
    template <typename State, typename Fastchain>
    error::error_code_t operator()(State& state, Fastchain const& fast_chain, size_t block_height, bc::chain::transaction const& tx, bc::reader& source) const {
#endif // KTH_USE_DOMAIN


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
} // namespace kth

#endif //KTH_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_SEND_TOKENS_HPP_
