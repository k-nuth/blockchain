// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <bitcoin/blockchain/version.hpp>

namespace kth { namespace blockchain {

char const* version() {
    return KTH_BLOCKCHAIN_VERSION;
}

}} // namespace kth::blockchain

