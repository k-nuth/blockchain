/**
 * Copyright (c) 2016-2018 Bitprim Inc.
 *
 * This file is part of Bitprim.
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

#ifndef BITPRIM_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_TRANSACTIONS_HPP_
#define BITPRIM_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_TRANSACTIONS_HPP_

#include <bitprim/keoken/error.hpp>
#include <bitprim/keoken/transaction_processors/v0/create_asset.hpp>
#include <bitprim/keoken/transaction_processors/v0/send_tokens.hpp>

namespace bitprim {
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
} // namespace bitprim

#endif //BITPRIM_BLOCKCHAIN_KEOKEN_TRANSACTION_PROCESSORS_V0_TRANSACTIONS_HPP_
