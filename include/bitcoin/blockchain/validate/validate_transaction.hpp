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
#ifndef LIBBITCOIN_BLOCKCHAIN_VALIDATE_TRANSACTION_HPP
#define LIBBITCOIN_BLOCKCHAIN_VALIDATE_TRANSACTION_HPP

#include <atomic>
#include <cstddef>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/blockchain/define.hpp>
#include <bitcoin/blockchain/interface/fast_chain.hpp>
#include <bitcoin/blockchain/pools/branch.hpp>
#include <bitcoin/blockchain/populate/populate_transaction.hpp>
#include <bitcoin/blockchain/settings.hpp>
#include <bitcoin/bitcoin/chainv2/transaction.hpp>

namespace libbitcoin { namespace blockchain {

/// This class is NOT thread safe.
class BCB_API validate_transaction {
public:
    typedef handle0 result_handler;

    validate_transaction(dispatcher& dispatch, const fast_chain& chain, const settings& settings);

    void start();
    void stop();

    void check(transaction_const_ptr tx, result_handler handler) const;
    void accept(transaction_const_ptr tx, result_handler handler) const;
    void connect(transaction_const_ptr tx, result_handler handler) const;

    void check_v2(chainv2::transaction::const_ptr tx, result_handler handler) const;
    void accept_v2(chainv2::transaction::const_ptr tx, result_handler handler) const;
    void connect_v2(chainv2::transaction::const_ptr tx, result_handler handler) const;

    void accept_sequential(transaction_const_ptr tx, result_handler handler) const;
    void connect_sequential(transaction_const_ptr tx, result_handler handler) const;

 
protected:
    inline bool stopped() const {
        return stopped_;
    }

private:
    void handle_populated(const code& ec, transaction_const_ptr tx, result_handler handler) const;
    void connect_inputs(transaction_const_ptr tx, size_t bucket, size_t buckets, result_handler handler) const;

    void handle_populated_v2(const code& ec, chainv2::transaction::const_ptr tx, bool tx_duplicate, result_handler handler) const;
    void connect_inputs_v2(chainv2::transaction::const_ptr tx, size_t bucket, size_t buckets, result_handler handler) const;

    void handle_populated_sequential(code const& ec, transaction_const_ptr tx, result_handler handler) const;
    void connect_inputs_sequential(transaction_const_ptr tx, size_t bucket, size_t buckets) const;

    // These are thread safe.
    std::atomic<bool> stopped_;
    const fast_chain& fast_chain_;
    dispatcher& dispatch_;

    // Caller must not invoke accept/connect concurrently.
    populate_transaction transaction_populator_;
};

}} // namespace libbitcoin::blockchain

#endif
