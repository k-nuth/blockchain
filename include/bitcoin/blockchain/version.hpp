///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2015 libbitcoin-blockchain developers (see COPYING).
//
//        GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
//
///////////////////////////////////////////////////////////////////////////////
#ifndef LIBBITCOIN_BLOCKCHAIN_VERSION_HPP
#define LIBBITCOIN_BLOCKCHAIN_VERSION_HPP

/**
 * The semantic version of this repository as: [major].[minor].[patch]
 * For interpretation of the versioning scheme see: http://semver.org
 */

// #define LIBBITCOIN_BLOCKCHAIN_VERSION "0.3.0"
#define LIBBITCOIN_BLOCKCHAIN_MAJOR_VERSION 0
#define LIBBITCOIN_BLOCKCHAIN_MINOR_VERSION 3
#define LIBBITCOIN_BLOCKCHAIN_PATCH_VERSION 0

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define LIBBITCOIN_BLOCKCHAIN_VERSION STR(LIBBITCOIN_BLOCKCHAIN_MAJOR_VERSION) "." STR(LIBBITCOIN_BLOCKCHAIN_MINOR_VERSION) "." STR(LIBBITCOIN_BLOCKCHAIN_PATCH_VERSION)
#undef STR
#undef STR_HELPER

#endif
