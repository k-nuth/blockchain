// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_VALIDATE_TRANSACTION_HPP
#define KTH_BLOCKCHAIN_VALIDATE_TRANSACTION_HPP

#include <atomic>
#include <cstddef>
#include <kth/domain.hpp>
#include <kth/blockchain/define.hpp>
#include <kth/blockchain/interface/fast_chain.hpp>
#include <kth/blockchain/pools/branch.hpp>
#include <kth/blockchain/populate/populate_transaction.hpp>
#include <kth/blockchain/settings.hpp>

#if defined(KTH_WITH_MEMPOOL)
#include <kth/mining/mempool.hpp>
#endif

namespace kth {
namespace blockchain {

/// This class is NOT thread safe.
class BCB_API validate_transaction
{
public:
    typedef handle0 result_handler;

#if defined(KTH_WITH_MEMPOOL)
    validate_transaction(dispatcher& dispatch, const fast_chain& chain, const settings& settings, mining::mempool const& mp);
#else
    validate_transaction(dispatcher& dispatch, const fast_chain& chain, const settings& settings);
#endif

    void start();
    void stop();

    void check(transaction_const_ptr tx, result_handler handler) const;
    void accept(transaction_const_ptr tx, result_handler handler) const;
    void connect(transaction_const_ptr tx, result_handler handler) const;

protected:
    inline bool stopped() const
    {
        return stopped_;
    }

private:
    void handle_populated(const code& ec, transaction_const_ptr tx,
        result_handler handler) const;
    void connect_inputs(transaction_const_ptr tx, size_t bucket,
        size_t buckets, result_handler handler) const;

    // These are thread safe.
    std::atomic<bool> stopped_;
    const bool retarget_;
    const fast_chain& fast_chain_;
    dispatcher& dispatch_;

    // Caller must not invoke accept/connect concurrently.
    populate_transaction transaction_populator_;
};

} // namespace blockchain
} // namespace kth

#endif
