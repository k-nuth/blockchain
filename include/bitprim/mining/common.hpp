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
#ifndef BITPRIM_BLOCKCHAIN_MINING_COMMON_HPP_
#define BITPRIM_BLOCKCHAIN_MINING_COMMON_HPP_

#include <unordered_set>
#include <vector>

#include <bitcoin/bitcoin/constants.hpp>

// #ifndef BITPRIM_MINING_DONT_INCLUDE_CTOR
// #define BITPRIM_MINING_CTOR_ENABLED
// #endif

#if defined(BITPRIM_CURRENCY_BCH)
#define BITPRIM_WITNESS_DEFAULT false
#else
#define BITPRIM_WITNESS_DEFAULT true
#endif

namespace libbitcoin {
namespace mining {


#if defined(BITPRIM_CURRENCY_BCH)
constexpr size_t min_transaction_size_for_capacity = min_transaction_size;
#else
 //TODO(fernando): check if it is possible to be a TX size less than this...
constexpr size_t min_transaction_size_for_capacity = 61;
#endif  //BITPRIM_CURRENCY_BCH



//TODO(fernando): put in a common file
using index_t = size_t;
using measurements_t = double;
using removal_list_t = std::unordered_set<index_t>;
using indexes_t = std::vector<index_t>;

constexpr index_t null_index = max_size_t;

}  // namespace mining
}  // namespace libbitcoin

#endif  //BITPRIM_BLOCKCHAIN_MINING_COMMON_HPP_
