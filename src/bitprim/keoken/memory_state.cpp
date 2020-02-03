// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <knuth/keoken/memory_state.hpp>

#include <algorithm>
#include <numeric>

using kth::data_chunk;
using kth::hash_digest;
using kth::wallet::payment_address;

namespace kth {
namespace keoken {

// memory_state::memory_state(asset_id_t asset_id_initial)
//     : asset_id_next_(asset_id_initial)
// {}

void memory_state::set_initial_asset_id(asset_id_t asset_id_initial) {
    asset_id_initial_ = asset_id_initial;
    asset_id_next_ = asset_id_initial;
}

// private
template <typename Predicate>
void memory_state::remove_balance_entries(Predicate const& pred) {
    // precondition: mutex must be locked

    // for(auto&& x : balance_) {
    //     auto& balance_entries = x.second;

    //     balance_entries.erase(
    //         std::remove_if(begin(balance_entries), end(balance_entries), pred), 
    //     end(balance_entries));
    // }

    auto f = begin(balance_); 
    auto l = end(balance_);

    while (f != l) {
        auto& balance_entries = f->second;

        balance_entries.erase(
            std::remove_if(begin(balance_entries), end(balance_entries), pred), 
        end(balance_entries));

        if (balance_entries.empty()) {
            f = balance_.erase(f);
        } else {
            ++f;
        }
    }
}

void memory_state::reset() {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);

    asset_id_next_ = asset_id_initial_;
    asset_list_.clear();
    balance_.clear();
}

struct rollback_pred {
    size_t height;

    explicit
    rollback_pred(size_t height)
        : height(height)
    {}

    template <typename Entry>
    bool operator()(Entry const& entry) const {
        return entry.block_height >= height;
    }
};

void memory_state::remove_up_to(size_t height) {
    boost::unique_lock<boost::shared_mutex> lock(mutex_);

    remove_balance_entries(rollback_pred{height});

    //TODO(fernando): could be done in a more efficient way using find from backwards... (or binary search, ..., is the data ordered?)
    asset_list_.erase(
        std::remove_if(begin(asset_list_), end(asset_list_), rollback_pred{height}), 
        end(asset_list_)
    );

    asset_id_next_ = asset_list_.empty() ? asset_id_initial_ : asset_list_.back().asset.id() + 1;
}


void memory_state::create_asset(std::string asset_name, amount_t asset_amount, 
                    payment_address owner,
                    size_t block_height, hash_digest const& txid) {

    entities::asset obj(asset_id_next_, std::move(asset_name), asset_amount, owner);

    boost::unique_lock<boost::shared_mutex> lock(mutex_);

    //TODO(fernando): emplace inside a lock? It is a good practice? is construct outside and push preferible?
    asset_list_.emplace_back(std::move(obj), block_height, txid);

    balance_.emplace(balance_key{asset_id_next_, std::move(owner)}, 
                     balance_value{balance_entry{asset_amount, block_height, txid}});
 
    ++asset_id_next_;
}

void memory_state::create_balance_entry(asset_id_t asset_id, amount_t asset_amount,
                            payment_address source,
                            payment_address target, 
                            size_t block_height, hash_digest const& txid) {

    boost::unique_lock<boost::shared_mutex> lock(mutex_);

    //TODO(fernando): emplace inside a lock? It is a good practice? is construct outside and push preferible?
    balance_[balance_key{asset_id, std::move(source)}].emplace_back(amount_t(-1) * asset_amount, block_height, txid);
    balance_[balance_key{asset_id, std::move(target)}].emplace_back(asset_amount, block_height, txid);
}

bool memory_state::asset_id_exists(asset_id_t id) const {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    return id < asset_id_next_;      // id > 0 ????
}

// private
amount_t memory_state::get_balance_internal(balance_value const& entries) const {
    // precondition: mutex_.lock_shared() called
    return std::accumulate(entries.begin(), entries.end(), amount_t(0), [](amount_t bal, balance_entry const& entry) {
        return bal + entry.amount;
    });
}

amount_t memory_state::get_balance(asset_id_t id, kth::wallet::payment_address const& addr) const {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    
    auto it = balance_.find(balance_key{id, addr});
    if (it == balance_.end()) {
        return amount_t(0);
    }

    return get_balance_internal(it->second);
}

memory_state::get_assets_by_address_list memory_state::get_assets_by_address(kth::wallet::payment_address const& addr) const {
    get_assets_by_address_list res;

    {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);
    for (auto const& entry : asset_list_) {
        // using balance_key = std::tuple<knuth::keoken::asset_id_t, kth::wallet::payment_address>;
        balance_key key {entry.asset.id(), addr};
        auto it = balance_.find(key);
        if (it != balance_.end()) {
            res.emplace_back(entry.asset.id(),
                             entry.asset.name(),
                             entry.asset.owner(),
                             get_balance_internal(it->second)
                            );
        }
    }
    }

    return res;
}

memory_state::get_assets_list memory_state::get_assets() const {
    get_assets_list res;

    {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto const& entry : asset_list_) {
        res.emplace_back(entry.asset.id(),
                            entry.asset.name(),
                            entry.asset.owner(),
                            entry.asset.amount()
                        );
    }
    }

    return res;
}

// private
entities::asset memory_state::get_asset_by_id(asset_id_t id) const {
    // precondition: id must exists in asset_list_
    // precondition: mutex_.lock_shared() called

    auto const cmp = [](asset_entry const& a, asset_id_t id) {
        return a.asset.id() < id;
    };

    auto it = std::lower_bound(asset_list_.begin(), asset_list_.end(), id, cmp);
    return it->asset;

    // if (it != asset_list_.end() && !cmp(id, *it)) {
}

memory_state::get_all_asset_addresses_list memory_state::get_all_asset_addresses() const {
    get_all_asset_addresses_list res;

    {
    boost::shared_lock<boost::shared_mutex> lock(mutex_);

    for (auto const& bal : balance_) {
        auto const& bal_key = bal.first;
        auto const& bal_value = bal.second;
        auto const& asset_id = std::get<0>(bal_key);
        auto const& amount_owner = std::get<1>(bal_key);

        auto asset = get_asset_by_id(asset_id);

        res.emplace_back(asset_id,
                         asset.name(),
                         asset.owner(),
                         get_balance_internal(bal_value),
                         amount_owner
                        );
    }
    }

    return res;
}

} // namespace keoken
} // namespace kth
