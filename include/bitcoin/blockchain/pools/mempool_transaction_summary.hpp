/**
 * Copyright (c) 2011-2018 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
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
#ifndef LIBBITCOIN_MEMPOOL_TRANSACTION_SUMMARY_HPP
#define LIBBITCOIN_MEMPOOL_TRANSACTION_SUMMARY_HPP

#include <string>

namespace libbitcoin {
namespace blockchain {

/// This class is not thread safe.
class mempool_transaction_summary
{
public:
    mempool_transaction_summary(std::string const& address, std::string const& hash, std::string const& previous_output_hash,
                                std::string const& previous_output_index, std::string const& satoshis, uint64_t index,
                                uint64_t timestamp);

    /// Associated wallet address
    std::string address() const;

    /// Unique identifier inside the blockchain
    std::string hash() const;

    /// Previous output hash
    std::string previous_output_hash() const;

    /// Previous output index
    std::string previous_output_index() const;

    /// Total value
    std::string satoshis() const;

    /// Transaction index
    uint64_t index() const;

    /// Transaction timestamp
    uint64_t timestamp() const;

private:
    std::string address_;

    std::string hash_;

    std::string previous_output_hash_;

    std::string previous_output_index_;

    std::string satoshis_;

    uint64_t index_;

    uint64_t timestamp_;
};

} // namespace blockchain
} // namespace libbitcoin

#endif //LIBBITCOIN_BLOCKCHAIN_FORK_HPP
