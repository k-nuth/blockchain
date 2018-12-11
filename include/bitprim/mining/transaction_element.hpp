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
#ifndef BITPRIM_BLOCKCHAIN_MINING_TRANSACTION_ELEMENT_HPP_
#define BITPRIM_BLOCKCHAIN_MINING_TRANSACTION_ELEMENT_HPP_

#include <unordered_map>
#include <vector>

#include <bitcoin/bitcoin.hpp>

namespace libbitcoin {
namespace mining {

class transaction_element {
public:

    transaction_element(hash_digest const& txid,
#if ! defined(BITPRIM_CURRENCY_BCH)
                        hash_digest const& hash,
#endif
                        data_chunk const& raw,
                        uint64_t fee,
                        size_t sigops,
                        uint32_t output_count
                        // std::vector<chain::point> const& previous_outputs
                        )
        : txid_(txid)
#if ! defined(BITPRIM_CURRENCY_BCH)
        , hash_(hash)
#endif
        , raw_(raw)
        , fee_(fee)
        , sigops_(sigops)
        , output_count_(output_count)
        // , previous_outputs_(previous_outputs)
    {}

    transaction_element(hash_digest const& txid,
#if ! defined(BITPRIM_CURRENCY_BCH)
                        hash_digest const& hash,
#endif
                        data_chunk&& raw,
                        uint64_t fee,
                        size_t sigops,
                        uint32_t output_count
                        // std::vector<chain::point>&& previous_outputs
                        )
        : txid_(txid)
#if ! defined(BITPRIM_CURRENCY_BCH)
        , hash_(hash)
#endif
        , raw_(std::move(raw))
        , fee_(fee)
        , sigops_(sigops)
        , output_count_(output_count)
        // , previous_outputs_(std::move(previous_outputs))
    {}

    hash_digest const& txid() const {
        return txid_;
    }

#if ! defined(BITPRIM_CURRENCY_BCH)
    hash_digest const& hash() const {
        return hash_;
    }
#endif

    data_chunk const& raw() const {
        return raw_;
    }
    
    uint64_t fee() const {
        return fee_;
    }

    size_t sigops() const {
        return sigops_;
    }

    size_t size() const {
        return raw_.size();
    }

    uint32_t output_count() const {
        return output_count_;
    }

    // std::vector<chain::point> const& previous_outputs() const {
    //     return previous_outputs_;
    // }
private:
    hash_digest txid_;

#if ! defined(BITPRIM_CURRENCY_BCH)
    hash_digest hash_;
#endif

    data_chunk raw_;
    uint64_t fee_;
    size_t sigops_;
    uint32_t output_count_;
    // std::vector<chain::point> previous_outputs_;
};

}  // namespace mining
}  // namespace libbitcoin

#endif  //BITPRIM_BLOCKCHAIN_MINING_TRANSACTION_ELEMENT_HPP_
