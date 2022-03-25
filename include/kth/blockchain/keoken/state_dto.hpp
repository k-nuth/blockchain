// Copyright (c) 2016-2022 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_KEOKEN_STATE_DTO_HPP_
#define KTH_BLOCKCHAIN_KEOKEN_STATE_DTO_HPP_

#include <kth/domain/wallet/payment_address.hpp>

#include <kth/keoken/entities/asset.hpp>
#include <kth/keoken/primitives.hpp>

namespace kth::keoken {

struct get_assets_by_address_data {
    get_assets_by_address_data(asset_id_t asset_id, std::string asset_name, kth::domain::wallet::payment_address asset_creator, amount_t amount)
        : asset_id(asset_id)
        , asset_name(std::move(asset_name))
        , asset_creator(std::move(asset_creator))
        , amount(amount) {}

    asset_id_t asset_id;
    std::string asset_name;
    kth::domain::wallet::payment_address asset_creator;
    amount_t amount;
};

using get_assets_data = get_assets_by_address_data;

struct get_all_asset_addresses_data : get_assets_by_address_data {

    get_all_asset_addresses_data(asset_id_t asset_id, std::string asset_name, kth::domain::wallet::payment_address asset_creator, amount_t amount, kth::domain::wallet::payment_address amount_owner)
        : get_assets_by_address_data(asset_id, std::move(asset_name), std::move(asset_creator), amount)
        , amount_owner(std::move(amount_owner)) {}

    kth::domain::wallet::payment_address amount_owner;   //TODO(fernando): naming: quien es dueno del saldo
};

} // namespace kth::keoken

#endif //KTH_BLOCKCHAIN_KEOKEN_STATE_DTO_HPP_
