// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin/blockchain/populate/populate_base.hpp>

#include <algorithm>
#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/blockchain/interface/fast_chain.hpp>

namespace libbitcoin {
namespace blockchain {

using namespace bc::chain;
using namespace bc::database;

#define NAME "populate_base"


// Database access is limited to:
// spend: { spender }
// transaction: { exists, height, position, output }

populate_base::populate_base(dispatcher& dispatch, const fast_chain& chain)
    : dispatch_(dispatch)
    , fast_chain_(chain)
{}

// This is the only necessary file system read in block/tx validation.
void populate_base::populate_duplicate(size_t branch_height, const chain::transaction& tx, bool require_confirmed) const {

#if defined(KTH_DB_LEGACY)    
    tx.validation.duplicate = fast_chain_.get_is_unspent_transaction(tx.hash(), branch_height, require_confirmed);
#else
    //Bitprim: We are not validating tx duplication    
    tx.validation.duplicate = false;
#endif // KTH_DB_LEGACY
}

void populate_base::populate_pooled(const chain::transaction& tx, uint32_t forks) const {
    size_t height;
    size_t position;

#if defined(KTH_DB_LEGACY) || defined(KTH_DB_NEW_FULL)
    if (fast_chain_.get_transaction_position(height, position, tx.hash(), false) 
        
#if defined(KTH_DB_LEGACY)        
        && (position == transaction_database::unconfirmed)) {
#else 
        && (position == position_max)) {
#endif
        
        tx.validation.pooled = true;
        tx.validation.current = (height == forks);
        return;
    }
#endif // KTH_DB_LEGACY  || KTH_DB_NEW_FULL

    tx.validation.pooled = false;
    tx.validation.current = false;
}

// Unspent outputs are cached by the store. If the cache is large enough this
// may never hit the file system. However on high RAM systems the file system
// is faster than the cache due to reduced paging of the memory-mapped file.
void populate_base::populate_prevout(size_t branch_height, output_point const& outpoint, bool require_confirmed) const {
    // The previous output will be cached on the input's outpoint.
    auto& prevout = outpoint.validation;

    prevout.spent = false;
    prevout.confirmed = false;
    prevout.cache = chain::output{};
    prevout.from_mempool = false;

    // If the input is a coinbase there is no prevout to populate.
    if (outpoint.is_null()) {
        return;
    }

#if defined(KTH_DB_NEW)
    //TODO(fernando): check the value of the parameters: branch_height and require_confirmed
    if ( ! fast_chain_.get_utxo(prevout.cache, prevout.height, prevout.median_time_past, prevout.coinbase, outpoint, branch_height)) {
        return;
    }
    


#elif defined(KTH_DB_LEGACY)
    // Get the prevout/cache (and spender height) and its metadata.
    // The output (prevout.cache) is populated only if the return is true.
    if ( ! fast_chain_.get_output(prevout.cache, prevout.height,
        prevout.median_time_past, prevout.coinbase, outpoint, branch_height,
        require_confirmed)) {
        return;
    }

    //*************************************************************************
    // CONSENSUS: The genesis block coinbase may not be spent. This is the
    // consequence of satoshi not including it in the utxo set for block
    // database initialization. Only he knows why, probably an oversight.
    //*************************************************************************
    if (prevout.height == 0) {
        return;
    }
#endif


    // BUGBUG: Spends are not marked as spent by unconfirmed transactions.
    // So tx pool transactions currently have no double spend limitation.
    // The output is spent only if by a spend at or below the branch height.
    auto const spend_height = prevout.cache.validation.spender_height;

    // The previous output has already been spent (double spend).
    if ((spend_height <= branch_height) && (spend_height != output::validation::not_spent)) {
        prevout.spent = true;
        prevout.confirmed = true;
        prevout.cache = chain::output{};
    }
}

} // namespace blockchain
} // namespace kth
