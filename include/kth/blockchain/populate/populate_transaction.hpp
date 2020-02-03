// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_POPULATE_TRANSACTION_HPP
#define KTH_BLOCKCHAIN_POPULATE_TRANSACTION_HPP

#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/blockchain/define.hpp>
#include <bitcoin/blockchain/interface/fast_chain.hpp>
#include <bitcoin/blockchain/populate/populate_base.hpp>

#if defined(KTH_WITH_MEMPOOL)
#include <knuth/mining/mempool.hpp>
#endif

namespace kth {
namespace blockchain {

/// This class is NOT thread safe.
class BCB_API populate_transaction
    : public populate_base
{
public:

#if defined(KTH_WITH_MEMPOOL)
    populate_transaction(dispatcher& dispatch, const fast_chain& chain, mining::mempool const& mp);
#else
    populate_transaction(dispatcher& dispatch, const fast_chain& chain);
#endif

    /// Populate validation state for the transaction.
    void populate(transaction_const_ptr tx, result_handler&& handler) const;

protected:
    void populate_inputs(transaction_const_ptr tx, size_t chain_height, size_t bucket, size_t buckets, result_handler handler) const;

private:
#if defined(KTH_WITH_MEMPOOL)
    mining::mempool const& mempool_;
#endif
};

} // namespace blockchain
} // namespace kth

#endif
