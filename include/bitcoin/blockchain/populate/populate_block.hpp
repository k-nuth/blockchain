/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
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
#ifndef LIBBITCOIN_BLOCKCHAIN_POPULATE_BLOCK_HPP
#define LIBBITCOIN_BLOCKCHAIN_POPULATE_BLOCK_HPP

#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/blockchain/define.hpp>
#include <bitcoin/blockchain/interface/fast_chain.hpp>
#include <bitcoin/blockchain/pools/branch.hpp>
#include <bitcoin/blockchain/populate/populate_base.hpp>

#if defined(BITPRIM_WITH_MEMPOOL)
#include <bitprim/mining/mempool.hpp>
#endif


namespace libbitcoin {
namespace blockchain {

// using local_utxo_t = std::unordered_map<point, output>;
using local_utxo_t = std::unordered_map<chain::point, chain::output const*>;

/// This class is NOT thread safe.
class BCB_API populate_block  : public populate_base {
public:
#ifdef BITPRIM_DB_NEW
    using utxo_pool_t = database::internal_database::utxo_pool_t;
#endif    

#if defined(BITPRIM_WITH_MEMPOOL)
    populate_block(dispatcher& dispatch, fast_chain const& chain, bool relay_transactions, mining::mempool const& mp);
#else
    populate_block(dispatcher& dispatch, fast_chain const& chain, bool relay_transactions);
#endif

    /// Populate validation state for the top block.
    void populate(branch::const_ptr branch, result_handler&& handler) const;

protected:
    using branch_ptr = branch::const_ptr;

    void populate_coinbase(branch::const_ptr branch, block_const_ptr block) const;

#ifdef BITPRIM_DB_NEW
    utxo_pool_t get_reorg_subset_conditionally(size_t first_height, size_t& out_chain_top) const;
    void populate_from_reorg_subset(chain::output_point const& outpoint, utxo_pool_t const& reorg_subset) const;
#endif

    ////void populate_duplicate(branch_ptr branch, const chain::transaction& tx) const;

#if defined(BITPRIM_WITH_MEMPOOL)
    void populate_transactions(branch::const_ptr branch, size_t bucket, size_t buckets, std::vector<local_utxo_t> const& branch_utxo, mining::mempool::hash_index_t const& validated_txs, result_handler handler) const;
#else
    void populate_transactions(branch::const_ptr branch, size_t bucket, size_t buckets, std::vector<local_utxo_t> const& branch_utxo, result_handler handler) const;
#endif

    void populate_prevout(branch_ptr branch, chain::output_point const& outpoint, std::vector<local_utxo_t> const& branch_utxo) const;

private:
    bool const relay_transactions_;

#if defined(BITPRIM_WITH_MEMPOOL)
    mining::mempool const& mempool_;
#endif

};

} // namespace blockchain
} // namespace libbitcoin

#endif
