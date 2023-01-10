// Copyright (c) 2016-2023 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_KEOKEN_MEMORY_STATE_HPP_
#define KTH_BLOCKCHAIN_KEOKEN_MEMORY_STATE_HPP_

#include <unordered_map>
#include <vector>

#include <kth/domain/wallet/payment_address.hpp>

#include <kth/keoken/asset_entry.hpp>
#include <kth/keoken/balance.hpp>
#include <kth/keoken/entities/asset.hpp>
#include <kth/keoken/message/create_asset.hpp>
#include <kth/keoken/message/send_tokens.hpp>
#include <kth/keoken/primitives.hpp>
#include <kth/keoken/state_dto.hpp>

namespace kth::keoken {

class memory_state {
public:
    using asset_list_t = std::vector<asset_entry>;
    using balance_value = std::vector<balance_entry>;
    using balance_t = std::unordered_map<balance_key, balance_value>;
    using payment_address = kth::domain::wallet::payment_address;

    using get_assets_by_address_list = std::vector<get_assets_by_address_data>;
    using get_assets_list = std::vector<get_assets_data>;
    using get_all_asset_addresses_list = std::vector<get_all_asset_addresses_data>;

    // explicit
    // state(asset_id_t asset_id_initial);

    memory_state() = default;

    // non-copyable and non-movable class
    memory_state(memory_state const&) = delete;
    memory_state operator=(memory_state const&) = delete;

    // Commands.
    // ---------------------------------------------------------------------------------
    void set_initial_asset_id(asset_id_t asset_id_initial);
    void reset();
    void remove_up_to(size_t height);

    void create_asset(std::string asset_name, amount_t asset_amount,
                      payment_address owner,
                      size_t block_height, kth::hash_digest const& txid);

    void create_balance_entry(asset_id_t asset_id, amount_t asset_amount,
                              payment_address source,
                              payment_address target,
                              size_t block_height, kth::hash_digest const& txid);



    // Queries.
    // ---------------------------------------------------------------------------------
    bool asset_id_exists(asset_id_t id) const;
    amount_t get_balance(asset_id_t id, payment_address const& addr) const;
    get_assets_by_address_list get_assets_by_address(kth::domain::wallet::payment_address const& addr) const;
    get_assets_list get_assets() const;
    get_all_asset_addresses_list get_all_asset_addresses() const;

private:
    template <typename Predicate>
    void remove_balance_entries(Predicate const& pred);

    entities::asset get_asset_by_id(asset_id_t id) const;
    amount_t get_balance_internal(balance_value const& entries) const;

    asset_id_t asset_id_initial_;
    asset_id_t asset_id_next_;
    asset_list_t asset_list_;
    balance_t balance_;

    // Synchronization
    mutable boost::shared_mutex mutex_;
};

} // namespace kth::keoken

#endif //KTH_BLOCKCHAIN_KEOKEN_MEMORY_STATE_HPP_
