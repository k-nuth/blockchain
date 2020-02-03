// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_KEOKEN_STATE_DTO_HPP_
#define KTH_BLOCKCHAIN_KEOKEN_STATE_DTO_HPP_

#include <bitcoin/bitcoin/wallet/payment_address.hpp>

#include <knuth/keoken/entities/asset.hpp>
#include <knuth/keoken/primitives.hpp>

namespace bitprim {
namespace keoken {

struct get_assets_by_address_data {
    get_assets_by_address_data(asset_id_t asset_id, std::string asset_name, libbitcoin::wallet::payment_address asset_creator, amount_t amount)
        : asset_id(asset_id)
        , asset_name(std::move(asset_name))
        , asset_creator(std::move(asset_creator))
        , amount(amount)
    {}

    asset_id_t asset_id;
    std::string asset_name;
    libbitcoin::wallet::payment_address asset_creator;
    amount_t amount;
};

using get_assets_data = get_assets_by_address_data;

struct get_all_asset_addresses_data : get_assets_by_address_data {

    get_all_asset_addresses_data(asset_id_t asset_id, std::string asset_name, libbitcoin::wallet::payment_address asset_creator, amount_t amount, libbitcoin::wallet::payment_address amount_owner)
        : get_assets_by_address_data(asset_id, std::move(asset_name), std::move(asset_creator), amount)
        , amount_owner(std::move(amount_owner))
    {}

    libbitcoin::wallet::payment_address amount_owner;   //TODO(fernando): naming: quien es dueno del saldo
};

} // namespace keoken
} // namespace bitprim

#endif //KTH_BLOCKCHAIN_KEOKEN_STATE_DTO_HPP_
