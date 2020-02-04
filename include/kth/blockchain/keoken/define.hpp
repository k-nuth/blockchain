// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_KEOKEN_DEFINE_HPP_
#define KTH_BLOCKCHAIN_KEOKEN_DEFINE_HPP_

#include <kth/domain.hpp>

// Now we use the generic helper definitions to
// define BBK_API and BBK_INTERNAL.
// BBK_API is used for the public API symbols. It either DLL imports or
// DLL exports (or does nothing for static build)
// BBK_INTERNAL is used for non-api symbols.

#if defined BBK_STATIC
    #define BBK_API
    #define BBK_INTERNAL
#elif defined BBK_DLL
    #define BBK_API      BC_HELPER_DLL_EXPORT
    #define BBK_INTERNAL BC_HELPER_DLL_LOCAL
#else
    #define BBK_API      BC_HELPER_DLL_IMPORT
    #define BBK_INTERNAL BC_HELPER_DLL_LOCAL
#endif

// Log name.
#define LOG_KEOKEN "keoken"

#endif //KTH_BLOCKCHAIN_KEOKEN_DEFINE_HPP_
