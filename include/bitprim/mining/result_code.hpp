/**
 * Copyright (c) 2016-2017 Bitprim Inc.
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
#ifndef BITPRIM_BLOCKCHAIN_MINING_RESULT_CODE_HPP_
#define BITPRIM_BLOCKCHAIN_MINING_RESULT_CODE_HPP_

namespace libbitcoin {
namespace mining {

enum class result_code {
    success = 0,
    duplicated_transaction = 2,
    // duplicated_output = 3,
    double_spend_mempool = 4,
    double_spend_blockchain = 5,
    low_benefit_transaction = 6,
    
    other = 8
};

} // namespace mining
} // namespace libbitcoin

#endif // BITPRIM_BLOCKCHAIN_MINING_RESULT_CODE_HPP_
