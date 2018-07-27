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
#ifndef BITPRIM_BLOCKCHAIN_KEOKEN_STATE_DTO_HPP_
#define BITPRIM_BLOCKCHAIN_KEOKEN_STATE_DTO_HPP_

#include <bitcoin/bitcoin/wallet/payment_address.hpp>

#include <bitprim/keoken/entities/asset.hpp>
#include <bitprim/keoken/primitives.hpp>

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

#endif //BITPRIM_BLOCKCHAIN_KEOKEN_STATE_DTO_HPP_
