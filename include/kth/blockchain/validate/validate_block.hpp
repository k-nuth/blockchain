// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_VALIDATE_BLOCK_HPP
#define KTH_BLOCKCHAIN_VALIDATE_BLOCK_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/blockchain/define.hpp>
#include <bitcoin/blockchain/interface/fast_chain.hpp>
#include <bitcoin/blockchain/pools/branch.hpp>
#include <bitcoin/blockchain/populate/populate_block.hpp>
#include <bitcoin/blockchain/settings.hpp>

#if defined(KTH_WITH_MEMPOOL)
#include <knuth/mining/mempool.hpp>
#endif

namespace kth {
namespace blockchain {

/// This class is NOT thread safe.
class BCB_API validate_block
{
public:
    typedef handle0 result_handler;

#if defined(KTH_WITH_MEMPOOL)
    validate_block(dispatcher& dispatch, const fast_chain& chain, const settings& settings, bool relay_transactions, mining::mempool const& mp);
#else
    validate_block(dispatcher& dispatch, const fast_chain& chain, const settings& settings, bool relay_transactions);
#endif    


    void start();
    void stop();

    void check(block_const_ptr block, result_handler handler) const;
    void accept(branch::const_ptr branch, result_handler handler) const;
    void connect(branch::const_ptr branch, result_handler handler) const;

protected:
    inline bool stopped() const
    {
        return stopped_;
    }

    float hit_rate() const;

private:
    typedef std::atomic<size_t> atomic_counter;
    typedef std::shared_ptr<atomic_counter> atomic_counter_ptr;

    static void dump(const code& ec, const chain::transaction& tx, uint32_t input_index, uint32_t forks, size_t height);

    void check_block(block_const_ptr block, size_t bucket, size_t buckets,
        result_handler handler) const;
    void handle_checked(const code& ec, block_const_ptr block,
        result_handler handler) const;
    void handle_populated(const code& ec, block_const_ptr block,
        result_handler handler) const;
    void accept_transactions(block_const_ptr block, size_t bucket,
        size_t buckets, atomic_counter_ptr sigops, bool bip16, bool bip141,
        result_handler handler) const;
    void handle_accepted(const code& ec, block_const_ptr block,
        atomic_counter_ptr sigops, bool bip141, result_handler handler) const;
    void connect_inputs(block_const_ptr block, size_t bucket,
        size_t buckets, result_handler handler) const;
    void handle_connected(const code& ec, block_const_ptr block,
        result_handler handler) const;

    // These are thread safe.
    std::atomic<bool> stopped_;
    const fast_chain& fast_chain_;
    dispatcher& priority_dispatch_;
    mutable atomic_counter hits_;
    mutable atomic_counter queries_;

    // Caller must not invoke accept/connect concurrently.
    populate_block block_populator_;
};

} // namespace blockchain
} // namespace kth

#endif
