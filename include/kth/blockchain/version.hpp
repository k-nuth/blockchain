// Copyright (c) 2016-2022 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_VERSION_HPP_
#define KTH_BLOCKCHAIN_VERSION_HPP_

#ifdef KTH_PROJECT_VERSION
#define KTH_BLOCKCHAIN_VERSION KTH_PROJECT_VERSION
#else
#define KTH_BLOCKCHAIN_VERSION "0.0.0"
#endif

namespace kth::blockchain {

char const* version();

} // namespace kth::blockchain

#endif //KTH_BLOCKCHAIN_VERSION_HPP_
