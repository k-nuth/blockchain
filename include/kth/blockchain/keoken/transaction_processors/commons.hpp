// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef KTH_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_COMMON_HPP_
#define KTH_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_COMMON_HPP_

#include <bitcoin/bitcoin/wallet/payment_address.hpp>


namespace kth {
namespace keoken {

enum class message_type_t {
    create_asset = 0,
    send_tokens = 1
};


template <typename Fastchain>
bc::wallet::payment_address get_first_input_addr(Fastchain const& fast_chain, bc::chain::transaction const& tx, bool testnet = false) {
    auto const& owner_input = tx.inputs()[0];

    bc::chain::output out_output;
    size_t out_height;
    uint32_t out_median_time_past;
    bool out_coinbase;

    if ( ! fast_chain.get_output(out_output, out_height, out_median_time_past, out_coinbase, 
                                  owner_input.previous_output(), bc::max_size_t, true)) {
        return bc::wallet::payment_address{};
    }

    return out_output.address(testnet);
}

template <typename Fastchain>
std::pair<bc::wallet::payment_address, bc::wallet::payment_address> get_send_tokens_addrs(Fastchain const& fast_chain, bc::chain::transaction const& tx, bool testnet = false) {
    auto source = get_first_input_addr(fast_chain, tx, testnet);
    if ( ! source) {
        return {bc::wallet::payment_address{}, bc::wallet::payment_address{}};
    }

    auto it = std::find_if(tx.outputs().begin(), tx.outputs().end(), [&source, &testnet](bc::chain::output const& o) {
        return o.address(testnet) && o.address(testnet) != source;
    });

    if (it == tx.outputs().end()) {
        return {std::move(source), bc::wallet::payment_address{}};        
    }

    return {std::move(source), it->address(testnet)};
}

} // namespace keoken
} // namespace kth

#endif //KTH_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_COMMON_HPP_
