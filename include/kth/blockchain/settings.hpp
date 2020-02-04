// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_SETTINGS_HPP
#define KTH_BLOCKCHAIN_SETTINGS_HPP

#include <cstdint>
#include <boost/filesystem.hpp>
#include <kth/domain.hpp>

#ifdef KTH_USE_DOMAIN
#include <kth/infrastructure/config/endpoint.hpp>
#else
#include <kth/bitcoin/config/endpoint.hpp>
#endif // KTH_USE_DOMAIN

#include <kth/blockchain/define.hpp>

namespace kth { namespace blockchain {

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

#ifdef KTH_CURRENCY_BCH
    // size_t uahf_height;                             //2017-Aug-01 hard fork, defaults to 478559 (Mainnet)
    // size_t daa_height;                              //2017-Nov-13 hard fork, defaults to 504031 (Mainnet)
    // uint64_t monolith_activation_time;              //2018-May-15 hard fork, defaults to 1526400000
    // uint64_t magnetic_anomaly_activation_time;      //2018-Nov-15 hard fork, defaults to 1542300000
    uint64_t great_wall_activation_time;               //2019-May-15 hard fork, defaults to 1557921600: Wed, 15 May 2019 12:00:00 UTC hard fork
    uint64_t graviton_activation_time;                 //2019-May-15 hard fork, defaults to 1573819200: Nov 15, 2019 12:00:00 UTC protocol upgrade

        // // August 1, 2017 hard fork
        // consensus.uahfHeight = 478558;

        // // November 13, 2017 hard fork
        // consensus.daaHeight = 504031;

        // // November 15, 2018 hard fork
        // consensus.magneticAnomalyHeight = 556766;

        // // Wed, 15 May 2019 12:00:00 UTC hard fork
        // consensus.greatWallActivationTime = 1557921600;

        // // Nov 15, 2019 12:00:00 UTC protocol upgrade
        // consensus.gravitonActivationTime = 1573819200;


                // // UAHF fork block.
                // {478558, uint256S("0000000000000000011865af4122fe3b144e2cbeea86"
                //                   "142e8ff2fb4107352d43")},
                // // Nov, 13 DAA activation block.
                // {504031, uint256S("0000000000000000011ebf65b60d0a3de80b8175be70"
                //                   "9d653b4c1a1beeb6ab9c")},
                // // Monolith activation.
                // {530359, uint256S("0000000000000000011ada8bd08f46074f44a8f15539"
                //                   "6f43e38acf9501c49103")},
                // // Magnetic anomaly activation.
                // {556767, uint256S("0000000000000000004626ff6e3b936941d341c5932e"
                //                   "ce4357eeccac44e6d56c")},

#endif //KTH_CURRENCY_BCH


    bool bip141;
    bool bip143;
    bool bip147;

#if defined(KTH_WITH_MEMPOOL)
    size_t mempool_max_template_size;
    size_t mempool_size_multiplier;
#endif
};

}} // namespace kth::blockchain

#endif
