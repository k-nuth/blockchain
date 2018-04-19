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
#ifndef LIBBITCOIN_BLOCKCHAIN_POPULATE_BASE_HPP
#define LIBBITCOIN_BLOCKCHAIN_POPULATE_BASE_HPP

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/blockchain/define.hpp>
#include <bitcoin/blockchain/interface/fast_chain.hpp>
#include <bitcoin/blockchain/settings.hpp>
#include <bitcoin/bitcoin/chainv2/transaction.hpp>

namespace libbitcoin { namespace blockchain {

/// This class is NOT thread safe.
class BCB_API populate_base
{
protected:
    typedef handle0 result_handler;

    populate_base(dispatcher& dispatch, const fast_chain& chain);

    void populate_duplicate(size_t maximum_height,
        const chain::transaction& tx, bool require_confirmed) const;

    void populate_pooled(const chain::transaction& tx, uint32_t forks) const;

    void populate_prevout(size_t maximum_height, chain::output_point const& outpoint, bool require_confirmed) const;
    // void populate_prevout_v2(size_t maximum_height, chainv2::output_point const& outpoint, bool require_confirmed) const;

    // This is thread safe.
    dispatcher& dispatch_;

    // The store is protected by caller not invoking populate concurrently.
    const fast_chain& fast_chain_;
};

}} // namespace libbitcoin::blockchain

#endif
