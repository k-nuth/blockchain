// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_SETTINGS_HPP
#define KTH_BLOCKCHAIN_SETTINGS_HPP

#include <cstdint>
#include <filesystem>

// #include <boost/filesystem.hpp>

#include <kth/domain.hpp>
#include <kth/infrastructure/config/endpoint.hpp>

#include <kth/blockchain/define.hpp>

namespace kth::blockchain {

/// Common blockchain configuration settings, properties not thread safe.
class BCB_API settings {
public:
    settings() = default;
    settings(infrastructure::config::settings context);

    /// Fork flags combiner.
    uint32_t enabled_forks() const;

    /// Properties.
    uint32_t cores = 0;
    bool priority = true;
    float byte_fee_satoshis = 0.1f;
    float sigop_fee_satoshis= 100.0f;
    uint64_t minimum_output_satoshis = 500;
    uint32_t notify_limit_hours = 24;
    uint32_t reorganization_limit = 256;
    infrastructure::config::checkpoint::list checkpoints;
    bool allow_collisions = true;
    bool easy_blocks = false;
    bool retarget = true;
    bool bip16 = true;
    bool bip30 = true;
    bool bip34 = true;
    bool bip66 = true;
    bool bip65 = true;
    bool bip90 = true;
    bool bip68 = true;
    bool bip112 = true;
    bool bip113 = true;

#ifdef KTH_CURRENCY_BCH

    bool bch_uahf = true;
    bool bch_daa_cw144 = true;
    bool bch_monolith = true;
    bool bch_magnetic_anomaly = true;
    bool bch_great_wall = true;
    bool bch_graviton = true;
    bool bch_phonon = false;      // 2020-May
    bool bch_axion = false;       // 2020-Nov
    // bool bch_unnamed = false;     // 2021-May
    
    ////2017-Aug-01 hard fork, defaults to 478559 (Mainnet)
    // size_t uahf_height = 478559;                             

    ////2017-Nov-13 hard fork, defaults to 504031 (Mainnet)
    // size_t daa_height = 504031;                              
    
    ////2018-May-15 hard fork, defaults to 1526400000
    // uint64_t monolith_activation_time = bch_monolith_activation_time;

    ////2018-Nov-15 hard fork, defaults to 1542300000
    // uint64_t magnetic_anomaly_activation_time = bch_magnetic_anomaly_activation_time;

    // //2019-May-15 hard fork, defaults to 1557921600: Wed, 15 May 2019 12:00:00 UTC protocol upgrade
    // uint64_t great_wall_activation_time = bch_great_wall_activation_time;               
    
    // //2019-May-15 hard fork, defaults to 1573819200: Nov 15, 2019 12:00:00 UTC protocol upgrade
    // uint64_t graviton_activation_time = bch_graviton_activation_time;

    //2020-May-15 hard fork, defaults to 1589544000: May 15, 2020 12:00:00 UTC protocol upgrade
    uint64_t phonon_activation_time = to_underlying(bch_phonon_activation_time);
    
    //2020-Nov-15 hard fork, defaults to 1605441600: Nov 15, 2020 12:00:00 UTC protocol upgrade
    uint64_t axion_activation_time = to_underlying(bch_axion_activation_time);

    // //2021-May-15 hard fork, defaults to 9999999999: ???May 15, 2020 12:00:00 UTC protocol upgrade
    // uint64_t unnamed_activation_time = to_underlying(bch_unnamed_activation_time);

#else
    // Just for Segwit coins
    bool bip141 = true;
    bool bip143 = true;
    bool bip147 = true;
#endif //KTH_CURRENCY_BCH

#if defined(KTH_WITH_MEMPOOL)
    size_t mempool_max_template_size = mining::mempool::max_template_size_default;
    size_t mempool_size_multiplier = mining::mempool::mempool_size_multiplier_default;
#endif

};

} // namespace kth::blockchain

#endif
