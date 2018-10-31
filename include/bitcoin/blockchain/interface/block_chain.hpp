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
#ifndef LIBBITCOIN_BLOCKCHAIN_BLOCK_CHAIN_HPP
#define LIBBITCOIN_BLOCKCHAIN_BLOCK_CHAIN_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <functional>
#include <vector>
#include <bitcoin/database.hpp>
#include <bitcoin/blockchain/define.hpp>
#include <bitcoin/blockchain/interface/fast_chain.hpp>
#include <bitcoin/blockchain/interface/safe_chain.hpp>
#include <bitcoin/blockchain/pools/block_organizer.hpp>
#include <bitcoin/blockchain/pools/transaction_organizer.hpp>
#include <bitcoin/blockchain/populate/populate_chain_state.hpp>
#include <bitcoin/blockchain/settings.hpp>

#if WITH_BLOCKCHAIN_REQUESTER
#include <bitcoin/protocol/requester.hpp>
#endif

namespace libbitcoin {
namespace blockchain {


/// The fast_chain interface portion of this class is not thread safe.
class BCB_API block_chain
  : public safe_chain, public fast_chain, noncopyable
{
public:
    /// Relay transactions is network setting that is passed through to block
    /// population as an optimization. This can be removed once there is an
    /// in-memory cache of tx pool metadata, as the costly query will go away.
    block_chain(threadpool& pool,
        const blockchain::settings& chain_settings,
        const database::settings& database_settings,
        bool relay_transactions=true);

    /// The database is closed on destruct, threads must be joined.
    ~block_chain();

    // ========================================================================
    // FAST CHAIN
    // ========================================================================

    // Readers.
    // ------------------------------------------------------------------------
    // Thread safe, unprotected by sequential lock.

#ifdef BITPRIM_DB_LEGACY
    /// Get the set of block gaps in the chain.
    bool get_gaps(database::block_database::heights& out_gaps) const override;

    /// Get the output that is referenced by the outpoint.
    bool get_output(chain::output& out_output, size_t& out_height, uint32_t& out_median_time_past, bool& out_coinbase, const chain::output_point& outpoint, size_t branch_height, bool require_confirmed) const override;

    bool get_output_is_confirmed(chain::output& out_output, size_t& out_height, bool& out_coinbase, bool& out_is_confirmed, const chain::output_point& outpoint, size_t branch_height, bool require_confirmed) const;

    //TODO(fernando): check how to replace it with UTXO
    /// Determine if an unspent transaction exists with the given hash.
    bool get_is_unspent_transaction(const hash_digest& hash, size_t branch_height, bool require_confirmed) const override;

    /// Get position data for a transaction.
    bool get_transaction_position(size_t& out_height, size_t& out_position, const hash_digest& hash, bool require_confirmed) const override;
#endif// BITPRIM_DB_LEGACY

#ifdef BITPRIM_DB_NEW

    /// Get the output that is referenced by the outpoint in the UTXO Set.
    bool get_utxo(chain::output& out_output, size_t& out_height, uint32_t& out_median_time_past, bool& out_coinbase, chain::output_point const& outpoint, size_t branch_height) const override;
#endif// BITPRIM_DB_NEW


    /// Get a determination of whether the block hash exists in the store.
    bool get_block_exists(const hash_digest& block_hash) const override;

    /// Get a determination of whether the block hash exists in the store.
    bool get_block_exists_safe(const hash_digest& block_hash) const override;

    /// Get the work of the branch starting at the given height.
    bool get_branch_work(uint256_t& out_work, const uint256_t& maximum, size_t height) const override;

    /// Get the header of the block at the given height.
    bool get_header(chain::header& out_header, size_t height) const override;

    /// Get the height of the block with the given hash.
    bool get_height(size_t& out_height, const hash_digest& block_hash) const override;

    /// Get the bits of the block with the given height.
    bool get_bits(uint32_t& out_bits, size_t height) const override;

    /// Get the timestamp of the block with the given height.
    bool get_timestamp(uint32_t& out_timestamp, size_t height) const override;

    /// Get the version of the block with the given height.
    bool get_version(uint32_t& out_version, size_t height) const override;

    /// Get height of latest block.
    bool get_last_height(size_t& out_height) const override;


    void prune_reorg_async() override;

    /// Get the hash of the block if it exists.
    bool get_block_hash(hash_digest& out_hash, size_t height) const override;


    /////// Get the transaction of the given hash and its block height.
    ////transaction_ptr get_transaction(size_t& out_block_height,
    ////    const hash_digest& hash, bool require_confirmed) const;

    // Writers.
    // ------------------------------------------------------------------------
    // Thread safe, insert does not set sequential lock.

#ifdef BITPRIM_DB_LEGACY
    /// Create flush lock if flush_writes is true, and set sequential lock.
    bool begin_insert() const override;

    /// Clear flush lock if flush_writes is true, and clear sequential lock.
    bool end_insert() const override;
#endif // BITPRIM_DB_LEGACY

    /// Insert a block to the blockchain, height is checked for existence.
    /// Reads and reorgs are undefined when chain is gapped.
    bool insert(block_const_ptr block, size_t height) override;

    /// Push an unconfirmed transaction to the tx table and index outputs.
    void push(transaction_const_ptr tx, dispatcher& dispatch,
        result_handler handler) override;

    /// Swap incoming and outgoing blocks, height is validated.
    void reorganize(const config::checkpoint& fork_point,
        block_const_ptr_list_const_ptr incoming_blocks,
        block_const_ptr_list_ptr outgoing_blocks, dispatcher& dispatch,
        result_handler handler) override;

    // Properties
    // ------------------------------------------------------------------------

    /// Get forks chain state relative to chain top.
    chain::chain_state::ptr chain_state() const override;

    /// Get full chain state relative to the branch top.
    chain::chain_state::ptr chain_state(branch::const_ptr branch) const override;

    // ========================================================================
    // SAFE CHAIN
    // ========================================================================
    // Thread safe.

    // Startup and shutdown.
    // ------------------------------------------------------------------------
    // Thread safe except start.

    /// Start the block pool and the transaction pool.
    bool start() override;

    /// Signal pool work stop, speeds shutdown with multiple threads.
    bool stop() override;

    /// Unmaps all memory and frees the database file handles.
    /// Threads must be joined before close is called (or by destruct).
    bool close() override;

    // Node Queries.
    // ------------------------------------------------------------------------

#if defined(BITPRIM_DB_LEGACY) || defined(BITPRIM_DB_NEW_BLOCKS) 
    /// fetch a block by height.
    void fetch_block(size_t height, bool witness, block_fetch_handler handler) const override;

    /// fetch a block by hash.
    void fetch_block(const hash_digest& hash, bool witness, block_fetch_handler handler) const override;

    void fetch_block_header_txs_size(const hash_digest& hash, block_header_txs_size_fetch_handler handler) const override;

    /// fetch hashes of transactions for a block, by block height.
    void fetch_merkle_block(size_t height, merkle_block_fetch_handler handler) const override;

    /// fetch hashes of transactions for a block, by block hash.
    void fetch_merkle_block(const hash_digest& hash, merkle_block_fetch_handler handler) const override;

    /// fetch compact block by block height.
    void fetch_compact_block(size_t height, compact_block_fetch_handler handler) const override;

    /// fetch compact block by block hash.
    void fetch_compact_block(const hash_digest& hash, compact_block_fetch_handler handler) const override;

    /// fetch the set of block hashes indicated by the block locator.
    void fetch_locator_block_hashes(get_blocks_const_ptr locator, const hash_digest& threshold, size_t limit, inventory_fetch_handler handler) const override;

#endif // BITPRIM_DB_LEGACY || BITPRIM_DB_NEW_BLOCKS

#ifdef BITPRIM_DB_LEGACY

    /// fetch transaction by hash.
    void fetch_transaction(const hash_digest& hash, bool require_confirmed, bool witness, transaction_fetch_handler handler) const override;

    std::vector<chain::transaction> get_mempool_transactions_from_wallets(std::vector<wallet::payment_address> const& payment_addresses, bool use_testnet_rules, bool witness) const;

    /// fetch position and height within block of transaction by hash.
    void fetch_transaction_position(const hash_digest& hash, bool require_confirmed, transaction_index_fetch_handler handler) const override;

#endif

    /// fetch the set of block headers indicated by the block locator.
    void fetch_locator_block_headers(get_headers_const_ptr locator, const hash_digest& threshold, size_t limit, locator_block_headers_fetch_handler handler) const override;

    /// fetch a block locator relative to the current top and threshold.
    void fetch_block_locator(const chain::block::indexes& heights, block_locator_fetch_handler handler) const override;

    /// fetch height of latest block.
    void fetch_last_height(last_height_fetch_handler handler) const override;

        /// fetch block header by height.
    void fetch_block_header(size_t height, block_header_fetch_handler handler) const override;

    /// fetch block header by hash.
    void fetch_block_header(const hash_digest& hash, block_header_fetch_handler handler) const override;

    /// fetch height of block by hash.
    void fetch_block_height(const hash_digest& hash, block_height_fetch_handler handler) const override;
    
    void fetch_block_hash_timestamp(size_t height, block_hash_time_fetch_handler handler) const override;


#ifdef BITPRIM_DB_TRANSACTION_UNCONFIRMED
    /// fetch unconfirmed transaction by hash.
    void fetch_unconfirmed_transaction(const hash_digest& hash, transaction_unconfirmed_fetch_handler handler) const;

    std::vector<mempool_transaction_summary> get_mempool_transactions(std::vector<std::string> const& payment_addresses, bool use_testnet_rules, bool witness) const override;
    std::vector<mempool_transaction_summary> get_mempool_transactions(std::string const& payment_address, bool use_testnet_rules, bool witness) const override;
#endif // BITPRIM_DB_TRANSACTION_UNCONFIRMED


#ifdef BITPRIM_DB_LEGACY
    // Bitprim non-virtual functions.
    //-------------------------------------------------------------------------
    template <typename I>
    void for_each_tx_hash(I f, I l, database::transaction_database const& tx_store, size_t height, bool witness, for_each_tx_handler handler) const {
    #ifdef BITPRIM_CURRENCY_BCH
        witness = false;    //TODO(fernando): check what to do here. I dont like it
    #endif
        while (f != l) {
            auto const& hash = *f;
            auto const tx_result = tx_store.get(hash, max_size_t, true);

            if ( ! tx_result) {
                handler(error::operation_failed_16, 0, chain::transaction{});
                return;
            }
            BITCOIN_ASSERT(tx_result.height() == height);
            handler(error::success, height, tx_result.transaction(witness));
            ++f;
        }
    }
#endif // BITPRIM_DB_LEGACY    

    void for_each_transaction(size_t from, size_t to, bool witness, for_each_tx_handler const& handler) const;

    void for_each_transaction_non_coinbase(size_t from, size_t to, bool witness, for_each_tx_handler const& handler) const;


    // Server Queries.
    //-------------------------------------------------------------------------

#ifdef BITPRIM_DB_SPENDS
    /// fetch the inpoint (spender) of an outpoint.
    void fetch_spend(const chain::output_point& outpoint, spend_fetch_handler handler) const override;
#endif // BITPRIM_DB_SPENDS

#ifdef BITPRIM_DB_HISTORY
    /// fetch outputs, values and spends for an address_hash.
    void fetch_history(const short_hash& address_hash, size_t limit, size_t from_height, history_fetch_handler handler) const override;

    /// Fetch all the txns used by the wallet
    void fetch_confirmed_transactions(const short_hash& address_hash, size_t limit, size_t from_height, confirmed_transactions_fetch_handler handler) const override;
#endif // BITPRIM_DB_HISTORY


#ifdef BITPRIM_DB_STEALTH
    /// fetch stealth results.
    void fetch_stealth(const binary& filter, size_t from_height, stealth_fetch_handler handler) const override;
#endif // BITPRIM_DB_STEALTH

    // Transaction Pool.
    //-------------------------------------------------------------------------

    /// Fetch a merkle block for the maximal fee block template.
    void fetch_template(merkle_block_fetch_handler handler) const override;

    /// Fetch an inventory vector for a rational "mempool" message response.
    void fetch_mempool(size_t count_limit, uint64_t minimum_fee, inventory_fetch_handler handler) const override;


#ifdef BITPRIM_DB_TRANSACTION_UNCONFIRMED
    mempool_mini_hash_map get_mempool_mini_hash_map(message::compact_block const& block) const override;
    void fill_tx_list_from_mempool(message::compact_block const& block, size_t& mempool_count, std::vector<chain::transaction>& txn_available, std::unordered_map<uint64_t, uint16_t> const& shorttxids) const override;
#endif // BITPRIM_DB_TRANSACTION_UNCONFIRMED



    // Filters.
    //-------------------------------------------------------------------------

    /// Filter out block by hash that exist in the block pool or store.
    void filter_blocks(get_data_ptr message, result_handler handler) const override;

#ifdef BITPRIM_DB_LEGACY
    /// Filter out confirmed and unconfirmed transactions by hash.
    void filter_transactions(get_data_ptr message, result_handler handler) const override;
#endif // BITPRIM_DB_LEGACY

    // Subscribers.
    //-------------------------------------------------------------------------

    /// Subscribe to blockchain reorganizations, get branch/height.
    void subscribe_blockchain(reorganize_handler&& handler) override;

    /// Subscribe to memory pool additions, get transaction.
    void subscribe_transaction(transaction_handler&& handler) override;

    /// Send null data success notification to all subscribers.
    void unsubscribe() override;

    // Transaction Validation.
    //-----------------------------------------------------------------------------

    void transaction_validate(transaction_const_ptr tx, result_handler handler) const override;
    
    // Organizers.
    //-------------------------------------------------------------------------

    /// Organize a block into the block pool if valid and sufficient.
    void organize(block_const_ptr block, result_handler handler) override;

    /// Store a transaction to the pool if valid.
    void organize(transaction_const_ptr tx, result_handler handler) override;

    // Properties.
    //-------------------------------------------------------------------------

    /// True if the blockchain is stale based on configured age limit.
    bool is_stale() const override;

    /// Get a reference to the blockchain configuration settings.
    const settings& chain_settings() const;

#ifdef BITPRIM_WITH_MINING
    struct tx_benefit {
        double benefit;
        size_t tx_sigops;
        size_t tx_size;
        size_t tx_fees;
        libbitcoin::data_chunk tx_hex;
        libbitcoin::hash_digest tx_id;

#ifndef BITPRIM_CURRENCY_BCH
        libbitcoin::hash_digest tx_hash;
#endif // BITPRIM_CURRENCY_BCH
    };

    struct prev_output {
        libbitcoin::hash_digest output_hash;
        uint32_t output_index;
    };

    std::vector<block_chain::tx_benefit> get_gbt_tx_list() const;
    bool add_to_chosen_list(transaction_const_ptr tx) override;
    void remove_mined_txs_from_chosen_list(block_const_ptr blk) override;
#endif // BITPRIM_WITH_MINING

#ifdef WITH_KEOKEN    
    virtual void fetch_keoken_history(const short_hash& address_hash, size_t limit,
        size_t from_height, keoken_history_fetch_handler handler) const override;

    virtual void fetch_block_keoken(const hash_digest& hash, bool witness,
        block_keoken_fetch_handler handler) const override;

    virtual void convert_to_keo_transaction(const hash_digest& hash,
      std::shared_ptr<std::vector<transaction_const_ptr>> keoken_txs) const override;
#endif

protected:

    /// Determine if work should terminate early with service stopped code.
    bool stopped() const;

private:
#if WITH_BLOCKCHAIN_REQUESTER
    const settings& settings_;
    mutable protocol::requester requester_;

    std::string reorganize_subscription;
    std::string reorganize_transaction;
#else
    typedef database::data_base::handle handle;

    // Locking helpers.
    // ------------------------------------------------------------------------

    template <typename Reader>
    void read_serial(const Reader& reader) const;

    template <typename Handler, typename... Args>
    bool finish_read(handle sequence, Handler handler, Args... args) const;

    // Utilities.
    //-------------------------------------------------------------------------

    code set_chain_state(chain::chain_state::ptr previous);
    void handle_transaction(const code& ec, transaction_const_ptr tx,
        result_handler handler) const;
    void handle_block(const code& ec, block_const_ptr block,
        result_handler handler) const;
    void handle_reorganize(const code& ec, block_const_ptr top,
        result_handler handler);

    // These are thread safe.
    std::atomic<bool> stopped_;
    const settings& settings_;
    const time_t notify_limit_seconds_;
    bc::atomic<block_const_ptr> last_block_;
    bc::atomic<transaction_const_ptr> last_transaction_;
    const populate_chain_state chain_state_populator_;
    database::data_base database_;

    // This is protected by mutex.
    chain::chain_state::ptr pool_state_;
    mutable shared_mutex pool_state_mutex_;

    // These are thread safe.
    mutable prioritized_mutex validation_mutex_;
    mutable threadpool priority_pool_;
    mutable dispatcher dispatch_;
    transaction_organizer transaction_organizer_;
    block_organizer block_organizer_;

#ifdef BITPRIM_WITH_MINING
    bool get_transaction_is_confirmed(libbitcoin::hash_digest tx_hash);
    void append_spend(transaction_const_ptr tx);
    void remove_spend(libbitcoin::hash_digest const& hash);
    bool check_is_double_spend(transaction_const_ptr tx);
    std::set<libbitcoin::hash_digest> get_double_spend_chosen_list(transaction_const_ptr tx);
    bool insert_to_chosen_list(transaction_const_ptr& tx, double benefit, size_t tx_size, size_t tx_sigops);
    size_t find_txs_to_remove_from_chosen(const size_t sigops_limit, const size_t tx_size,
        const size_t tx_sigops, const size_t tx_fees, const double benefit,
            size_t& acum_sigops, size_t& acum_size, double& acum_benefit);


    uint64_t chosen_size_; // Size in bytes of the chosen unconfirmed transaction list
    uint64_t chosen_sigops_; // Total Amount of sigops in the chosen unconfirmed transaction list
    std::list <tx_benefit> chosen_unconfirmed_; // Chosen unconfirmed transaction list
    std::unordered_map<hash_digest, std::vector<prev_output>> chosen_spent_;
    mutable std::mutex gbt_mutex_; // Protect chosen unconfirmed transaction list
    std::atomic_bool gbt_ready_; // Getblocktemplate ready
#endif // BITPRIM_WITH_MINING

#endif // WITH_BLOCKCHAIN_REQUESTER
};

} // namespace blockchain
} // namespace libbitcoin

#endif // LIBBITCOIN_BLOCKCHAIN_BLOCK_CHAIN_HPP
