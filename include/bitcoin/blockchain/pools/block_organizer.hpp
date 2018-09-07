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
#ifndef LIBBITCOIN_BLOCKCHAIN_BLOCK_ORGANIZER_HPP
#define LIBBITCOIN_BLOCKCHAIN_BLOCK_ORGANIZER_HPP

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

namespace libbitcoin {
namespace blockchain {

#ifdef WITH_MEASUREMENTS
    struct block_measurement_elem_t {
        // hash_digest hash;
        size_t height = 0;
        size_t stage = 0;
        std::chrono::time_point<std::chrono::high_resolution_clock> t0 = {};     // organize first step
        std::chrono::time_point<std::chrono::high_resolution_clock> t1 = {};     // block_organizer::handle_check
        std::chrono::time_point<std::chrono::high_resolution_clock> t2 = {};     // block_organizer::handle_check BEFORE calling `validator_.accept(branch, accept_handler);`
        std::chrono::time_point<std::chrono::high_resolution_clock> t3 = {};     // block_organizer::handle_accept
        std::chrono::time_point<std::chrono::high_resolution_clock> t4 = {};     // block_organizer::handle_connect
        std::chrono::time_point<std::chrono::high_resolution_clock> t5 = {};     // block_organizer::handle_connect BEFORE calling `fast_chain_.reorganize`
        std::chrono::time_point<std::chrono::high_resolution_clock> t6 = {};     // block_organizer::handle_reorganized
        std::chrono::time_point<std::chrono::high_resolution_clock> t7 = {};     // block_organizer::handle_reorganized BEFORE calling `handler(error::success);`
        std::chrono::time_point<std::chrono::high_resolution_clock> t8 = {};
        std::chrono::time_point<std::chrono::high_resolution_clock> t9 = {};
        std::chrono::time_point<std::chrono::high_resolution_clock> t10 = {};
        std::chrono::time_point<std::chrono::high_resolution_clock> t11 = {};
        std::chrono::time_point<std::chrono::high_resolution_clock> t12 = {};
    };
#endif // WITH_MEASUREMENTS

/// This class is thread safe.
/// Organises blocks via the block pool to the blockchain.
class BCB_API block_organizer
{
public:
    typedef handle0 result_handler;
    typedef std::shared_ptr<block_organizer> ptr;
    typedef safe_chain::reorganize_handler reorganize_handler;
    typedef resubscriber<code, size_t, block_const_ptr_list_const_ptr,
        block_const_ptr_list_const_ptr> reorganize_subscriber;

    /// Construct an instance.
    block_organizer(prioritized_mutex& mutex, dispatcher& dispatch,
        threadpool& thread_pool, fast_chain& chain, const settings& settings,
        bool relay_transactions);

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
    void handle_check(const code& ec, block_const_ptr block,
        result_handler handler);
    void handle_accept(const code& ec, branch::ptr branch,
        result_handler handler);
    void handle_connect(const code& ec, branch::ptr branch,
        result_handler handler);
    void organized(branch::ptr branch, result_handler handler);
    void handle_reorganized(const code& ec, branch::const_ptr branch,
        block_const_ptr_list_ptr outgoing, result_handler handler);
    void signal_completion(const code& ec);

    // Subscription.
    void notify(size_t branch_height, block_const_ptr_list_const_ptr branch,
        block_const_ptr_list_const_ptr original);

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


#ifdef WITH_MEASUREMENTS
    // std::vector<block_measurement_elem_t> block_measurement_data;
    // std::unordered_map<hash_digest, block_measurement_elem_t> block_measurement_data_;
    // std::atomic<size_t> height_counter_ = 0;
    // std::mutex measurement_mutex_;
    size_t height_counter_ = 0;
    // block_const_ptr block_organizing_;
    hash_digest block_hash_;
    block_measurement_elem_t block_measurement_elem_;

    // void measurement_create_entry(block_const_ptr block) {
    void measurement_create_entry(hash_digest const& hash) {
        // block_organizing_ = block;
        block_hash_ = hash;
        block_measurement_elem_.height = height_counter_;
        height_counter_++;
        // block_measurement_elem_.stage = 0;
        block_measurement_elem_.t0 = std::chrono::high_resolution_clock::now();
    }

    template <typename F>
    void measurement_update_if(F f) {
        f(block_measurement_elem_);
    }
   
#endif // WITH_MEASUREMENT

};

} // namespace blockchain
} // namespace libbitcoin

#endif
