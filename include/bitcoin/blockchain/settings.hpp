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
#ifndef LIBBITCOIN_BLOCKCHAIN_SETTINGS_HPP
#define LIBBITCOIN_BLOCKCHAIN_SETTINGS_HPP

#include <cstdint>
#include <boost/filesystem.hpp>
#include <bitcoin/bitcoin.hpp>

#ifdef BITPRIM_USE_DOMAIN
#include <bitcoin/infrastructure/config/endpoint.hpp>
#else
#include <bitcoin/bitcoin/config/endpoint.hpp>
#endif // BITPRIM_USE_DOMAIN

#include <bitcoin/blockchain/define.hpp>

namespace libbitcoin { namespace blockchain {

/// Common blockchain configuration settings, properties not thread safe.
class BCB_API settings {
public:
    settings();
    settings(config::settings context);

    /// Fork flags combiner.
    uint32_t enabled_forks() const;

    /// Properties.
    uint32_t cores;
    bool priority;
    float byte_fee_satoshis;
    float sigop_fee_satoshis;
    uint64_t minimum_output_satoshis;
    uint32_t notify_limit_hours;
    uint32_t reorganization_limit;
    config::checkpoint::list checkpoints;
    bool allow_collisions;
    bool easy_blocks;
    bool retarget;
    bool bip16;
    bool bip30;
    bool bip34;
    bool bip66;
    bool bip65;
    bool bip90;

#if defined(WITH_REMOTE_BLOCKCHAIN)
    config::endpoint replier;
#endif // defined(WITH_REMOTE_BLOCKCHAIN)

    bool bip68;
    bool bip112;
    bool bip113;

#ifdef BITPRIM_CURRENCY_BCH
    // size_t uahf_height;                             //2017-Aug-01 hard fork, defaults to 478559 (Mainnet)
    // size_t daa_height;                              //2017-Nov-13 hard fork, defaults to 504031 (Mainnet)
    // uint64_t monolith_activation_time;              //2018-May-15 hard fork, defaults to 1526400000
    uint64_t magnetic_anomaly_activation_time;         //2018-Nov-15 hard fork, defaults to 1542300000
    uint64_t great_wall_activation_time;               //2019-May-15 hard fork, defaults to 1557921600
#endif //BITPRIM_CURRENCY_BCH


    bool bip141;
    bool bip143;
    bool bip147;

#if defined(BITPRIM_WITH_MINING)
    size_t mempool_max_template_size;
    size_t mempool_size_multiplier;
#endif
};

}} // namespace libbitcoin::blockchain

#endif
