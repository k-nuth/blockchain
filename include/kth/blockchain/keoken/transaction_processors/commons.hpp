// Copyright (c) 2016-2022 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef KTH_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_COMMON_HPP_
#define KTH_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_COMMON_HPP_

#include <kth/domain/wallet/payment_address.hpp>

namespace kth::keoken {

enum class message_type_t {
    create_asset = 0,
    send_tokens = 1
};


template <typename Fastchain>
kd::wallet::payment_address get_first_input_addr(Fastchain const& fast_chain, kd::domain::chain::transaction const& tx, bool testnet = false) {
    auto const& owner_input = tx.inputs()[0];

    kd::domain::chain::output out_output;
    size_t out_height;
    uint32_t out_median_time_past;
    bool out_coinbase;

    if ( ! fast_chain.get_output(out_output, out_height, out_median_time_past, out_coinbase,
                                  owner_input.previous_output(), kd::max_size_t, true)) {
        return kd::wallet::payment_address{};
    }

    return out_output.address(testnet);
}

template <typename Fastchain>
std::pair<kd::wallet::payment_address, kd::wallet::payment_address> get_send_tokens_addrs(Fastchain const& fast_chain, kd::domain::chain::transaction const& tx, bool testnet = false) {
    auto source = get_first_input_addr(fast_chain, tx, testnet);
    if ( ! source) {
        return {kd::wallet::payment_address{}, kd::wallet::payment_address{}};
    }

    auto it = std::find_if(tx.outputs().begin(), tx.outputs().end(), [&source, &testnet](kd::domain::chain::output const& o) {
        return o.address(testnet) && o.address(testnet) != source;
    });

    if (it == tx.outputs().end()) {
        return {std::move(source), kd::wallet::payment_address{}};
    }

    return {std::move(source), it->address(testnet)};
}

} // namespace keoken
} // namespace kth

#endif //KTH_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_COMMON_HPP_
