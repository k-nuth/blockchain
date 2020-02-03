// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_SAFE_CHAIN_HPP
#define KTH_BLOCKCHAIN_SAFE_CHAIN_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/blockchain/define.hpp>
#include <bitcoin/blockchain/pools/mempool_transaction_summary.hpp>

namespace kth {
namespace blockchain {

/// This interface is thread safe.
/// A high level interface for encapsulation of the blockchain database.
/// Implementations are expected to be thread safe.
class BCB_API safe_chain
{
public:
    typedef handle0 result_handler;

    /// Object fetch handlers.
    typedef handle1<size_t> last_height_fetch_handler;
    typedef handle1<size_t> block_height_fetch_handler;
    typedef handle1<chain::output> output_fetch_handler;
    typedef handle1<chain::input_point> spend_fetch_handler;
    typedef handle1<chain::history_compact::list> history_fetch_handler;
    typedef handle1<chain::stealth_compact::list> stealth_fetch_handler;
    typedef handle2<size_t, size_t> transaction_index_fetch_handler;
#ifdef KTH_WITH_KEOKEN
    typedef std::function<void (const code&, const std::shared_ptr <std::vector <kth::transaction_const_ptr>> ) > keoken_history_fetch_handler;
    typedef std::function<void (const code&,  header_const_ptr, size_t,  const std::shared_ptr <std::vector <kth::transaction_const_ptr>> , uint64_t, size_t ) > block_keoken_fetch_handler;
    virtual void fetch_keoken_history(const short_hash& address_hash, size_t limit,
        size_t from_height, keoken_history_fetch_handler handler) const = 0;

    virtual void fetch_block_keoken(const hash_digest& hash, bool witness,
      block_keoken_fetch_handler handler) const = 0;

    virtual void convert_to_keo_transaction(const hash_digest& hash,
      std::shared_ptr<std::vector<transaction_const_ptr>> keoken_txs) const = 0;
#endif
    typedef handle1<std::vector<hash_digest>> confirmed_transactions_fetch_handler;

    // Smart pointer parameters must not be passed by reference.
    typedef std::function<void(const code&, block_const_ptr, size_t)>
        block_fetch_handler;

    typedef std::function<void(const code&, header_const_ptr, size_t,  const std::shared_ptr<hash_list>, uint64_t)>
        block_header_txs_size_fetch_handler;

    typedef std::function<void(const code&, const hash_digest&, uint32_t, size_t)>
        block_hash_time_fetch_handler;

    typedef std::function<void(const code&, merkle_block_ptr, size_t)>
        merkle_block_fetch_handler;
    typedef std::function<void(const code&, compact_block_ptr, size_t)>
        compact_block_fetch_handler;
    typedef std::function<void(const code&, header_ptr, size_t)>
        block_header_fetch_handler;
    typedef std::function<void(const code&, transaction_const_ptr, size_t,
        size_t)> transaction_fetch_handler;

#if defined(KTH_DB_TRANSACTION_UNCONFIRMED) || defined(KTH_DB_NEW_FULL)
    using transaction_unconfirmed_fetch_handler = std::function<void(const code&, transaction_const_ptr)>;
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

    typedef std::function<void(const code&, headers_ptr)>
        locator_block_headers_fetch_handler;
    typedef std::function<void(const code&, get_headers_ptr)>
        block_locator_fetch_handler;
    typedef std::function<void(const code&, inventory_ptr)>
        inventory_fetch_handler;

    /// Subscription handlers.
    typedef std::function<bool(code, size_t, block_const_ptr_list_const_ptr,
        block_const_ptr_list_const_ptr)> reorganize_handler;
    typedef std::function<bool(code, transaction_const_ptr)>
        transaction_handler;

    using for_each_tx_handler = std::function<void(code const&, size_t, chain::transaction const&)>;

    using mempool_mini_hash_map = std::unordered_map<mini_hash, chain::transaction>;

    // Startup and shutdown.
    // ------------------------------------------------------------------------

    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual bool close() = 0;

    // Node Queries.
    // ------------------------------------------------------------------------


    virtual void fetch_block(size_t height, bool witness, block_fetch_handler handler) const = 0;

    virtual void fetch_block(const hash_digest& hash, bool witness, block_fetch_handler handler) const = 0;

    virtual void fetch_locator_block_hashes(get_blocks_const_ptr locator, const hash_digest& threshold, size_t limit, inventory_fetch_handler handler) const = 0;

#if defined(KTH_DB_LEGACY) || defined(KTH_DB_NEW_BLOCKS) || defined(KTH_DB_NEW_FULL)
    virtual void fetch_merkle_block(size_t height, merkle_block_fetch_handler handler) const = 0;

    virtual void fetch_merkle_block(const hash_digest& hash, merkle_block_fetch_handler handler) const = 0;

    virtual void fetch_compact_block(size_t height, compact_block_fetch_handler handler) const = 0;

    virtual void fetch_compact_block(const hash_digest& hash, compact_block_fetch_handler handler) const = 0;

    virtual void fetch_block_header_txs_size(const hash_digest& hash, block_header_txs_size_fetch_handler handler) const = 0;
#endif // KTH_DB_LEGACY || KTH_DB_NEW_BLOCKS || KTH_DB_NEW_FULL


#if defined(KTH_DB_LEGACY) || defined(KTH_DB_NEW_FULL)
    
    virtual void fetch_transaction(const hash_digest& hash, bool require_confirmed, bool witness, transaction_fetch_handler handler) const = 0;

    virtual void fetch_transaction_position(const hash_digest& hash, bool require_confirmed, transaction_index_fetch_handler handler) const = 0;

    virtual void for_each_transaction(size_t from, size_t to, bool witness, for_each_tx_handler const& handler) const = 0;

    virtual void for_each_transaction_non_coinbase(size_t from, size_t to, bool witness, for_each_tx_handler const& handler) const = 0;

#endif 


    virtual void fetch_locator_block_headers(get_headers_const_ptr locator, const hash_digest& threshold, size_t limit, locator_block_headers_fetch_handler handler) const = 0;

    virtual void fetch_block_locator(const chain::block::indexes& heights, block_locator_fetch_handler handler) const = 0;

    virtual void fetch_last_height(last_height_fetch_handler handler) const = 0;

    virtual void fetch_block_header(size_t height, block_header_fetch_handler handler) const = 0;

    virtual void fetch_block_header(const hash_digest& hash, block_header_fetch_handler handler) const = 0;

    virtual bool get_block_hash(hash_digest& out_hash, size_t height) const = 0;

    virtual void fetch_block_height(const hash_digest& hash, block_height_fetch_handler handler) const = 0;

    virtual void fetch_block_hash_timestamp(size_t height, block_hash_time_fetch_handler handler) const = 0;

    // Server Queries.
    //-------------------------------------------------------------------------

#if defined(KTH_DB_SPENDS) || defined(KTH_DB_NEW_FULL)
    virtual void fetch_spend(const chain::output_point& outpoint, spend_fetch_handler handler) const = 0;
#endif 

#if defined(KTH_DB_HISTORY) || defined(KTH_DB_NEW_FULL)
    virtual void fetch_history(const short_hash& address_hash, size_t limit, size_t from_height, history_fetch_handler handler) const = 0;
    virtual void fetch_confirmed_transactions(const short_hash& address_hash, size_t limit, size_t from_height, confirmed_transactions_fetch_handler handler) const = 0;
#endif 

#ifdef KTH_DB_STEALTH
    virtual void fetch_stealth(const binary& filter, size_t from_height, stealth_fetch_handler handler) const = 0;
#endif // KTH_DB_STEALTH

    // Transaction Pool.
    //-------------------------------------------------------------------------

    virtual void fetch_template(merkle_block_fetch_handler handler) const = 0;
    virtual void fetch_mempool(size_t count_limit, uint64_t minimum_fee, inventory_fetch_handler handler) const = 0;

#if defined(KTH_DB_TRANSACTION_UNCONFIRMED) || defined(KTH_DB_NEW_FULL)
    virtual std::vector<mempool_transaction_summary> get_mempool_transactions(std::vector<std::string> const& payment_addresses, bool use_testnet_rules, bool witness) const = 0;
    virtual std::vector<mempool_transaction_summary> get_mempool_transactions(std::string const& payment_address, bool use_testnet_rules, bool witness) const = 0;
    
    virtual std::vector<chain::transaction> get_mempool_transactions_from_wallets(std::vector<wallet::payment_address> const& payment_addresses, bool use_testnet_rules, bool witness) const = 0;

    virtual void fetch_unconfirmed_transaction(const hash_digest& hash, transaction_unconfirmed_fetch_handler handler) const = 0;

    virtual mempool_mini_hash_map get_mempool_mini_hash_map(message::compact_block const& block) const = 0;
    virtual void fill_tx_list_from_mempool(message::compact_block const& block, size_t& mempool_count, std::vector<chain::transaction>& txn_available, std::unordered_map<uint64_t, uint16_t> const& shorttxids) const = 0;
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

  
    // Filters.
    //-------------------------------------------------------------------------

    virtual void filter_blocks(get_data_ptr message, result_handler handler) const = 0;

#if defined(KTH_DB_LEGACY) || defined(KTH_DB_NEW_FULL) || defined(KTH_WITH_MEMPOOL)
    virtual void filter_transactions(get_data_ptr message, result_handler handler) const = 0;
#endif 

    // Subscribers.
    //-------------------------------------------------------------------------

    virtual void subscribe_blockchain(reorganize_handler&& handler) = 0;
    virtual void subscribe_transaction(transaction_handler&& handler) = 0;
    virtual void unsubscribe() = 0;


    // Transaction Validation.
    //-----------------------------------------------------------------------------

    virtual void transaction_validate(transaction_const_ptr tx, result_handler handler) const = 0;

    // Organizers.
    //-------------------------------------------------------------------------

    virtual void organize(block_const_ptr block, result_handler handler) = 0;
    virtual void organize(transaction_const_ptr tx, result_handler handler) = 0;

    // Properties
    // ------------------------------------------------------------------------

    virtual bool is_stale() const = 0;

    //TODO(Mario) temporary duplication 
    /// Get a determination of whether the block hash exists in the store.
    virtual bool get_block_exists_safe(hash_digest const & block_hash) const = 0;

};

} // namespace blockchain
} // namespace kth

#endif
