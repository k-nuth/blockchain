// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_POPULATE_CHAIN_STATE_HPP
#define KTH_BLOCKCHAIN_POPULATE_CHAIN_STATE_HPP

#include <cstddef>
#include <cstdint>
#include <kth/bitcoin.hpp>
#include <kth/blockchain/define.hpp>
#include <kth/blockchain/interface/fast_chain.hpp>
#include <kth/blockchain/pools/branch.hpp>
#include <kth/blockchain/settings.hpp>

namespace kth { namespace blockchain {

/// This class is NOT thread safe.
class BCB_API populate_chain_state {
public:
    populate_chain_state(const fast_chain& chain, const settings& settings);

    /// Populate chain state for the tx pool (start).
    chain::chain_state::ptr populate() const;

    /// Populate chain state for the top block in the branch (try).
    chain::chain_state::ptr populate(chain::chain_state::ptr pool, branch::const_ptr branch) const;

    /// Populate pool state from the top block (organized).
    chain::chain_state::ptr populate(chain::chain_state::ptr top) const;

private:
    typedef branch::const_ptr branch_ptr;
    typedef chain::chain_state::map map;
    typedef chain::chain_state::data data;

    bool populate_all(data& data, branch_ptr branch) const;
    bool populate_bits(data& data, const map& map, branch_ptr branch) const;
    bool populate_versions(data& data, const map& map, branch_ptr branch) const;
    bool populate_timestamps(data& data, const map& map, branch_ptr branch) const;
    bool populate_collision(data& data, const map& map, branch_ptr branch) const;
    bool populate_bip9_bit0(data& data, const map& map, branch_ptr branch) const;
    bool populate_bip9_bit1(data& data, const map& map, branch_ptr branch) const;

    bool get_bits(uint32_t& out_bits, size_t height, branch_ptr branch) const;
    bool get_version(uint32_t& out_version, size_t height, branch_ptr branch) const;
    bool get_timestamp(uint32_t& out_timestamp, size_t height, branch_ptr branch) const;
    bool get_block_hash(hash_digest& out_hash, size_t height, branch_ptr branch) const;

#ifdef KTH_CURRENCY_BCH
    const settings& settings_;
#endif //KTH_CURRENCY_BCH

    // These are thread safe.
    const uint32_t configured_forks_;
    const config::checkpoint::list checkpoints_;

    // Populate is guarded against concurrent callers but because it uses the fast
    // chain it must not be invoked during chain writes.
    const fast_chain& fast_chain_;
    mutable shared_mutex mutex_;
};

}} // namespace kth::blockchain

#endif
