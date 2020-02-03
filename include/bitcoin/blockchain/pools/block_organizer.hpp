// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_BLOCK_ORGANIZER_HPP
#define KTH_BLOCKCHAIN_BLOCK_ORGANIZER_HPP

#include <atomic>
#include <cstddef>
#include <future>
#include <memory>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/blockchain/define.hpp>
#include <bitcoin/blockchain/interface/fast_chain.hpp>
#include <bitcoin/blockchain/interface/safe_chain.hpp>
#include <bitcoin/blockchain/pools/block_pool.hpp>
#include <bitcoin/blockchain/pools/branch.hpp>
#include <bitcoin/blockchain/settings.hpp>
#include <bitcoin/blockchain/validate/validate_block.hpp>

namespace kth {
namespace blockchain {

/// This class is thread safe.
/// Organises blocks via the block pool to the blockchain.
class BCB_API block_organizer
{
public:
    typedef handle0 result_handler;
    typedef std::shared_ptr<block_organizer> ptr;
    typedef safe_chain::reorganize_handler reorganize_handler;
    typedef resubscriber<code, size_t, block_const_ptr_list_const_ptr, block_const_ptr_list_const_ptr> reorganize_subscriber;

    /// Construct an instance.
#if defined(KTH_WITH_MEMPOOL)
    block_organizer(prioritized_mutex& mutex, dispatcher& dispatch, threadpool& thread_pool, fast_chain& chain, const settings& settings, bool relay_transactions, mining::mempool& mp);
#else
    block_organizer(prioritized_mutex& mutex, dispatcher& dispatch, threadpool& thread_pool, fast_chain& chain, const settings& settings, bool relay_transactions);
#endif

    bool start();
    bool stop();

    void organize(block_const_ptr block, result_handler handler);
    void subscribe(reorganize_handler&& handler);
    void unsubscribe();

    /// Remove all message vectors that match block hashes.
    void filter(get_data_ptr message) const;

protected:
    bool stopped() const;

private:
    // Utility.
    bool set_branch_height(branch::ptr branch);

    // Verify sub-sequence.
    void handle_check(const code& ec, block_const_ptr block, result_handler handler);
    void handle_accept(const code& ec, branch::ptr branch, result_handler handler);
    void handle_connect(const code& ec, branch::ptr branch, result_handler handler);
    void organized(branch::ptr branch, result_handler handler);
    void handle_reorganized(const code& ec, branch::const_ptr branch, block_const_ptr_list_ptr outgoing, result_handler handler);
    void signal_completion(const code& ec);

#if defined(KTH_WITH_MEMPOOL)
    void populate_prevout_1(branch::const_ptr branch, chain::output_point const& outpoint, bool require_confirmed) const;
    void populate_prevout_2(branch::const_ptr branch, chain::output_point const& outpoint, local_utxo_set_t const& branch_utxo) const;
    void populate_transaction_inputs(branch::const_ptr branch, chain::input::list const& inputs, local_utxo_set_t const& branch_utxo) const;
    void populate_transactions(branch::const_ptr branch, chain::block const& block, local_utxo_set_t const& branch_utxo) const;
    // void organize_mempool(branch::const_ptr branch, block_const_ptr_list_const_ptr const& incoming_blocks, block_const_ptr_list_ptr const& outgoing_blocks, local_utxo_set_t const& branch_utxo);
    void organize_mempool(branch::const_ptr branch, block_const_ptr_list_const_ptr const& incoming_blocks, block_const_ptr_list_ptr const& outgoing_blocks);
#endif

#ifdef KTH_DB_NEW
    bool is_branch_double_spend(branch::ptr const& branch) const;
#endif

    // Subscription.
    void notify(size_t branch_height, block_const_ptr_list_const_ptr branch, block_const_ptr_list_const_ptr original);

    // This must be protected by the implementation.
    fast_chain& fast_chain_;

    // These are thread safe.
    prioritized_mutex& mutex_;
    std::atomic<bool> stopped_;
    std::promise<code> resume_;
    dispatcher& dispatch_;
    block_pool block_pool_;
    validate_block validator_;
    reorganize_subscriber::ptr subscriber_;

#if defined(KTH_WITH_MEMPOOL)
    mining::mempool& mempool_;
#endif
};

} // namespace blockchain
} // namespace kth

#endif
