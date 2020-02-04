// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef KTH_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_CREATE_ASSET_HPP_
#define KTH_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_CREATE_ASSET_HPP_

#include <kth/keoken/error.hpp>
#include <kth/keoken/transaction_processors/commons.hpp>

namespace kth {
namespace keoken {
namespace transaction_processors {
namespace v0 {

struct create_asset {
    static constexpr message_type_t message_type = message_type_t::create_asset;

#ifdef KTH_USE_DOMAIN
    template <typename State, typename Fastchain, Reader R, KTH_IS_READER(R)>
    error::error_code_t operator()(State& state, Fastchain const& fast_chain, size_t block_height, bc::chain::transaction const& tx, R& source) const {
#else
    template <typename State, typename Fastchain>
    error::error_code_t operator()(State& state, Fastchain const& fast_chain, size_t block_height, bc::chain::transaction const& tx, bc::reader& source) const {
#endif // KTH_USE_DOMAIN
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
} // namespace kth

#endif //KTH_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_CREATE_ASSET_HPP_
