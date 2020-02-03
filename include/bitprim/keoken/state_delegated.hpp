// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_KEOKEN_STATE_DELEGATED_HPP_
#define KTH_BLOCKCHAIN_KEOKEN_STATE_DELEGATED_HPP_

// #include <unordered_map>
#include <vector>

#include <bitcoin/bitcoin/wallet/payment_address.hpp>

#include <knuth/keoken/state_dto.hpp>

namespace bitprim {
namespace keoken {

struct state_delegated {
    using payment_address = bc::wallet::payment_address;
    using get_assets_by_address_list = std::vector<get_assets_by_address_data>;
    using get_assets_list = std::vector<get_assets_data>;
    using get_all_asset_addresses_list = std::vector<get_all_asset_addresses_data>;

    using set_initial_asset_id_func = std::function<void(asset_id_t /*asset_id_initial*/)>;
    using reset_func = std::function<void()>;
    using remove_up_to_func = std::function<void(size_t /*height*/)>;
    using create_asset_func = std::function<void(std::string /*asset_name*/, amount_t /*asset_amount*/, bc::wallet::payment_address const& /*owner*/, size_t /*block_height*/, libbitcoin::hash_digest const& /*txid*/)>;
    using create_balance_entry_func = std::function<void(asset_id_t /*asset_id*/, amount_t /*asset_amount*/, bc::wallet::payment_address const& /*source*/, bc::wallet::payment_address const& /*target*/, size_t /*block_height*/, libbitcoin::hash_digest const& /*txid*/)>;
    using asset_id_exists_func = std::function<bool(asset_id_t /*id*/)>;
    using get_balance_func = std::function<amount_t(asset_id_t id, bc::wallet::payment_address const& addr)>;
    using get_assets_by_address_func = std::function<get_assets_by_address_list(bc::wallet::payment_address const& /*addr*/)>;
    using get_assets_func = std::function<get_assets_list()>;
    using get_all_asset_addresses_func = std::function<get_all_asset_addresses_list()>;

    state_delegated() = default;

    // non-copyable and non-movable class
    state_delegated(state_delegated const&) = delete;
    state_delegated operator=(state_delegated const&) = delete;

    // Commands.
    // ---------------------------------------------------------------------------------
    set_initial_asset_id_func set_initial_asset_id;
    reset_func reset;
    remove_up_to_func remove_up_to;

    create_asset_func create_asset;
    create_balance_entry_func create_balance_entry;

    // Queries.
    // ---------------------------------------------------------------------------------
    asset_id_exists_func asset_id_exists;
    get_balance_func get_balance;
    get_assets_by_address_func get_assets_by_address;
    get_assets_func get_assets;
    get_all_asset_addresses_func get_all_asset_addresses;
};

template <typename State>
void bind_to_state(State& st, state_delegated& st_del) {
    using std::placeholders::_1;
    using std::placeholders::_2;
    using std::placeholders::_3;
    using std::placeholders::_4;
    using std::placeholders::_5;
    using std::placeholders::_6;

    st_del.set_initial_asset_id = std::bind(&State::set_initial_asset_id, &st, _1);
    st_del.reset = std::bind(&State::reset, &st);
    st_del.remove_up_to = std::bind(&State::remove_up_to, &st, _1);

    st_del.create_asset         = std::bind(&State::create_asset, &st, _1, _2, _3, _4, _5);
    st_del.create_balance_entry = std::bind(&State::create_balance_entry, &st, _1, _2, _3, _4, _5, _6);

    st_del.asset_id_exists         = std::bind(&State::asset_id_exists, &st, _1);
    st_del.get_balance             = std::bind(&State::get_balance, &st, _1, _2);
    st_del.get_assets_by_address   = std::bind(&State::get_assets_by_address, &st, _1);
    st_del.get_assets              = std::bind(&State::get_assets, &st);
    st_del.get_all_asset_addresses = std::bind(&State::get_all_asset_addresses, &st);
}

} // namespace keoken
} // namespace bitprim

#endif //KTH_BLOCKCHAIN_KEOKEN_STATE_DELEGATED_HPP_
