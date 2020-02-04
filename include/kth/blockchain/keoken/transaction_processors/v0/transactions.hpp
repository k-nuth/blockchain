// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef KTH_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_TRANSACTIONS_HPP_
#define KTH_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_TRANSACTIONS_HPP_

#include <kth/keoken/error.hpp>
#include <kth/keoken/transaction_processors/v0/create_asset.hpp>
#include <kth/keoken/transaction_processors/v0/send_tokens.hpp>

namespace kth {
namespace keoken {
namespace transaction_processors {
namespace v0 {

//TODO(fernando): see how could generate the list automatically from the .hpp files...
using transactions = std::tuple<
      create_asset 
    , send_tokens
>;

} // namespace v0
} // namespace transaction_processors
} // namespace keoken
} // namespace kth

#endif //KTH_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_TRANSACTIONS_HPP_
