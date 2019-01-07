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

    transaction_element(chain::transaction const& tx)
        : transaction_(tx)
        , txid_(transaction_.hash())
#if ! defined(BITPRIM_CURRENCY_BCH)
        , hash_(transaction_.hash(true))
#endif
        , raw_(transaction_.to_data(true, BITPRIM_WITNESS_DEFAULT))
        , fee_(transaction_.fees())
        , sigops_(transaction_.signature_operations())
    {}

    transaction_element(chain::transaction&& tx)
        : transaction_(std::move(tx))
        , txid_(transaction_.hash())
#if ! defined(BITPRIM_CURRENCY_BCH)
        , hash_(transaction_.hash(true))
#endif
        , raw_(transaction_.to_data(true, BITPRIM_WITNESS_DEFAULT))
        , fee_(transaction_.fees())
        , sigops_(transaction_.signature_operations())
    {}

    chain::transaction const& transaction() const {
        return transaction_;
    }

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
        return transaction_.outputs().size();
    }

private:
    chain::transaction transaction_;
    hash_digest txid_;

#if ! defined(BITPRIM_CURRENCY_BCH)
    hash_digest hash_;
#endif

    data_chunk raw_;
    uint64_t fee_;
    size_t sigops_;
};

}  // namespace mining
}  // namespace libbitcoin

#endif  //BITPRIM_BLOCKCHAIN_MINING_TRANSACTION_ELEMENT_HPP_
