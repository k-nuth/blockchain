// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kth/blockchain/populate/populate_chain_state.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>

#include <tao/algorithm/partition/partition_point_variant.hpp>

#include <kth/domain.hpp>

#include <kth/blockchain/define.hpp>
#include <kth/blockchain/interface/fast_chain.hpp>
#include <kth/blockchain/pools/branch.hpp>
#include <kth/blockchain/settings.hpp>

namespace kth::blockchain {

using namespace kd::chain;

// This value should never be read, but may be useful in debugging.
static constexpr uint32_t unspecified = max_uint32;

// Database access is limited to:
// get_last_height
// block: { hash, bits, version, timestamp }

populate_chain_state::populate_chain_state(fast_chain const& chain, settings const& settings)
    :
#if defined(KTH_CURRENCY_BCH)
      settings_(settings),
#endif //KTH_CURRENCY_BCH
      configured_forks_(settings.enabled_forks())
    , checkpoints_(infrastructure::config::checkpoint::sort(settings.checkpoints))
    , fast_chain_(chain)
{}

inline 
bool is_transaction_pool(branch::const_ptr branch) {
    return branch->empty();
}

bool populate_chain_state::get_bits(uint32_t& out_bits, size_t height, branch::const_ptr branch) const {
    // branch returns false only if the height is out of range.
    return branch->get_bits(out_bits, height) || fast_chain_.get_bits(out_bits, height);
}

bool populate_chain_state::get_version(uint32_t& out_version, size_t height, branch::const_ptr branch) const {
    // branch returns false only if the height is out of range.
    return branch->get_version(out_version, height) || fast_chain_.get_version(out_version, height);
}

bool populate_chain_state::get_timestamp(uint32_t& out_timestamp, size_t height, branch::const_ptr branch) const {
    // branch returns false only if the height is out of range.
    return branch->get_timestamp(out_timestamp, height) || fast_chain_.get_timestamp(out_timestamp, height);
}

bool populate_chain_state::get_block_hash(hash_digest& out_hash, size_t height, branch::const_ptr branch) const {
    return branch->get_block_hash(out_hash, height) || fast_chain_.get_block_hash(out_hash, height);
}

bool populate_chain_state::populate_bits(chain_state::data& data, chain_state::map const& map, branch::const_ptr branch) const {
    auto& bits = data.bits.ordered;
    bits.resize(map.bits.count);
    auto height = map.bits.high - map.bits.count;

    for (auto& bit: bits) {
        if ( ! get_bits(bit, ++height, branch)) {
            return false;
        }
    }

    if (is_transaction_pool(branch)) {
        // This is an unused value.
        data.bits.self = work_limit(true);
        return true;
    }

    return get_bits(data.bits.self, map.bits_self, branch);
}

bool populate_chain_state::populate_versions(chain_state::data& data, chain_state::map const& map, branch::const_ptr branch) const {
    auto& versions = data.version.ordered;
    versions.resize(map.version.count);
    auto height = map.version.high - map.version.count;

    for (auto& version: versions) {
        if ( ! get_version(version, ++height, branch)) {
            return false;
        }
    }

    if (is_transaction_pool(branch)) {
        data.version.self = chain_state::signal_version(configured_forks_);
        return true;
    }

    return get_version(data.version.self, map.version_self, branch);
}

bool populate_chain_state::populate_timestamps(chain_state::data& data, chain_state::map const& map, branch::const_ptr branch) const {
    data.timestamp.retarget = unspecified;
    auto& timestamps = data.timestamp.ordered;
    timestamps.resize(map.timestamp.count);
    auto height = map.timestamp.high - map.timestamp.count;

    for (auto& timestamp: timestamps) {
        if ( ! get_timestamp(timestamp, ++height, branch)) {
            return false;
        }
    }

    // Retarget is required if timestamp_retarget is not unrequested.
    if (map.timestamp_retarget != chain_state::map::unrequested &&
// #ifdef LITECOIN
#ifdef KTH_CURRENCY_LTC
        ! get_timestamp(data.timestamp.retarget, map.timestamp_retarget != 0 ? map.timestamp_retarget - 1 : 0, branch))
#else
        ! get_timestamp(data.timestamp.retarget, map.timestamp_retarget, branch))
#endif //KTH_CURRENCY_LTC
    {
        return false;
    }

    if (is_transaction_pool(branch)) {
        data.timestamp.self = static_cast<uint32_t>(zulu_time());
        return true;
    }

    return get_timestamp(data.timestamp.self, map.timestamp_self, branch);
}

bool populate_chain_state::populate_collision(chain_state::data& data, chain_state::map const& map, branch::const_ptr branch) const {
    if (map.allow_collisions_height == chain_state::map::unrequested) {
        data.allow_collisions_hash = null_hash;
        return true;
    }

    if (is_transaction_pool(branch)) {
        data.allow_collisions_hash = null_hash;
        return true;
    }

    return get_block_hash(data.allow_collisions_hash, map.allow_collisions_height, branch);
}

bool populate_chain_state::populate_bip9_bit0(chain_state::data& data, chain_state::map const& map, branch::const_ptr branch) const {
    if (map.bip9_bit0_height == chain_state::map::unrequested) {
        data.bip9_bit0_hash = null_hash;
        return true;
    }

    return get_block_hash(data.bip9_bit0_hash, map.bip9_bit0_height, branch);
}

bool populate_chain_state::populate_bip9_bit1(chain_state::data& data,
    chain_state::map const& map, branch::const_ptr branch) const {
    if (map.bip9_bit1_height == chain_state::map::unrequested) {
        data.bip9_bit1_hash = null_hash;
        return true;
    }

    return get_block_hash(data.bip9_bit1_hash,
        map.bip9_bit1_height, branch);
}



bool populate_chain_state::populate_all(chain_state::data& data, branch::const_ptr branch) const {
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(mutex_);

    // Construct a map to inform chain state data population.
    auto const map = chain_state::get_map(data.height, checkpoints_, configured_forks_);

    return (
        populate_bits(data, map, branch)
        && populate_versions(data, map, branch)
        && populate_timestamps(data, map, branch)
        && populate_collision(data, map, branch)
        && populate_bip9_bit0(data, map, branch)
        && populate_bip9_bit1(data, map, branch)
    );
    ///////////////////////////////////////////////////////////////////////////
}

#if defined(KTH_CURRENCY_BCH)

chain_state::assert_anchor_block_info_t populate_chain_state::find_assert_anchor_block(size_t height, uint32_t forks, data const& data, branch_ptr branch) const {
    using tao::algorithm::partition_point_variant_n;

    auto const last_time_span = domain::chain::chain_state::median_time_past(data, 0, true);
    bool const asert_activated = domain::chain::chain_state::is_mtp_activated(last_time_span, settings_.axion_activation_time);

    if ( ! asert_activated) {
        return {};
    }
    auto const testnet = script::is_enabled(forks, domain::machine::rule_fork::easy_blocks);
    auto const from = testnet ? testnet_asert_anchor_lock_up_checkpoint.height() : mainnet_asert_anchor_lock_up_checkpoint.height();

    //TODO(fernando): get from branch, like ...
    // return branch->get_bits(out_xxxxxxx, height) || fast_chain_.get_bits(out_xxxxxxx, height);
    auto const headers = fast_chain_.get_headers(from, height);
    std::vector<uint32_t> timestamps(headers.size(), 0);
    std::transform(std::begin(headers), std::end(headers), std::begin(timestamps), [](auto const& h) { 
        return h.timestamp();
    });

    constexpr size_t subset_len = 11;
    auto const get_mtp = [=subset_len](auto const& it) {
        std::array<uint32_t, subset_len> subset = {};
        auto f = std::prev(it, subset_len - 1);
        auto l = std::next(it);
        std::copy_n(f, subset_len, std::begin(subset));
        std::sort(std::begin(subset), std::end(subset));
        auto mtp = subset[subset_len / 2];
        return mtp;
    };

    auto const is_axion_enabled = [this, &get_mtp](auto const& it) {
        auto mtp = get_mtp(it);
        return domain::chain::chain_state::is_mtp_activated(mtp, settings_.axion_activation_time);
    };
    auto p = partition_point_variant_n(std::next(std::begin(timestamps), subset_len), 
                                       std::size(timestamps), is_axion_enabled);

    //TODO(fernando): what to do if p == std::end(timestamps)?
    auto const d = std::distance(std::begin(timestamps), p);
    auto const& header = headers[d];
    auto const& prev_header = headers[d - 1];
    return {from + d, prev_header.timestamp(), header.bits()};
}
#endif

chain_state::ptr populate_chain_state::populate() const {
    size_t top;

    if ( ! fast_chain_.get_last_height(top)) {
        return {};
    }

    chain_state::data data;
    data.hash = null_hash;
    data.height = safe_add(top, size_t(1));

    auto branch_ptr = std::make_shared<branch>(top);

    // Use an empty branch to represent the transaction pool.
    if ( ! populate_all(data, branch_ptr)) {
        return {};
    }

#if defined(KTH_CURRENCY_BCH)
    auto anchor = find_assert_anchor_block(data.height, configured_forks_, data, branch_ptr);
#endif

    return std::make_shared<chain_state>(
        std::move(data)
        , configured_forks_
        , checkpoints_
#if defined(KTH_CURRENCY_BCH)
        , anchor
        , settings_.asert_half_life
        // , settings_.monolith_activation_time
        // , settings_.magnetic_anomaly_activation_time
        // , settings_.great_wall_activation_time
        // , settings_.graviton_activation_time
        // , phonon_t(settings_.phonon_activation_time)
        , axion_t(settings_.axion_activation_time)
#endif //KTH_CURRENCY_BCH
    );
}

chain_state::ptr populate_chain_state::populate(chain_state::ptr pool, branch::const_ptr branch) const {
    auto const block = branch->top();
    KTH_ASSERT(block);

    // If this is not a reorganization we can just promote the pool state.
    if (branch->size() == 1 && branch->top_height() == pool->height()) {
        return chain_state::from_pool_ptr(*pool, *block);
    }

    chain_state::data data;
    data.hash = block->hash();
    data.height = branch->top_height();

    // Caller must test result.
    if ( ! populate_all(data, branch)) {
        return {};
    }

    return std::make_shared<chain_state>(
        std::move(data)
        , configured_forks_
        , checkpoints_
#if defined(KTH_CURRENCY_BCH)
        , pool->assert_anchor_block_info()
        , settings_.asert_half_life
        // , settings_.monolith_activation_time
        // , settings_.magnetic_anomaly_activation_time
        // , settings_.great_wall_activation_time
        // , settings_.graviton_activation_time
        // , phonon_t(settings_.phonon_activation_time)
        , axion_t(settings_.axion_activation_time)
#endif //KTH_CURRENCY_BCH
    );
}

chain_state::ptr populate_chain_state::populate(chain_state::ptr top) const {
    // Create pool state from top block chain state.
    auto const state = std::make_shared<chain_state>(*top);

    // Invalidity is not possible unless next height is zero.
    // This can only happen when the chain size overflows size_t.
    KTH_ASSERT(state->is_valid());

    return state;
}

} // namespace kth::blockchain
