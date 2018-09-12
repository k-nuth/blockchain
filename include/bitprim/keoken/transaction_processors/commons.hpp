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

#ifndef BITPRIM_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_COMMON_HPP_
#define BITPRIM_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_COMMON_HPP_

#include <bitcoin/bitcoin/wallet/payment_address.hpp>


namespace bitprim {
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
    auto source = get_first_input_addr(fast_chain, tx);
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
} // namespace bitprim

#endif //BITPRIM_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_COMMON_HPP_
