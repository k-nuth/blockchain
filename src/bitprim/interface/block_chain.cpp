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
#include <bitcoin/blockchain/interface/block_chain.hpp>

#ifdef BITPRIM_WITH_KEOKEN
#include <bitprim/keoken/transaction_extractor.hpp>
#endif

namespace libbitcoin {
namespace blockchain {

#ifdef BITPRIM_DB_LEGACY
void block_chain::for_each_transaction(size_t from, size_t to, bool witness, for_each_tx_handler const& handler) const {
#ifdef BITPRIM_CURRENCY_BCH
    witness = false;    //TODO(fernando): see what to do with those things!
#endif
    auto const& tx_store = database_.transactions();

    while (from <= to) {

        if (stopped()) {
            handler(error::service_stopped, 0, chain::transaction{});
            return;
        }
    
        auto const block_result = database_.blocks().get(from);

        if ( ! block_result) {
            handler(error::not_found, 0, chain::transaction{});
            return;
        }
        BITCOIN_ASSERT(block_result.height() == from);
        auto const tx_hashes = block_result.transaction_hashes();

        for_each_tx_hash(block_result.transaction_hashes().begin(), 
                         block_result.transaction_hashes().end(), 
                         tx_store, from, witness, handler);

        ++from;
    }
}

void block_chain::for_each_transaction_non_coinbase(size_t from, size_t to, bool witness, for_each_tx_handler const& handler) const {
#ifdef BITPRIM_CURRENCY_BCH
    witness = false;    //TODO(fernando): see what to do with those things!
#endif
    auto const& tx_store = database_.transactions();

    while (from <= to) {

        if (stopped()) {
            handler(error::service_stopped, 0, chain::transaction{});
            return;
        }
    
        auto const block_result = database_.blocks().get(from);

        if ( ! block_result) {
            handler(error::not_found, 0, chain::transaction{});
            return;
        }
        BITCOIN_ASSERT(block_result.height() == from);
        auto const tx_hashes = block_result.transaction_hashes();

        for_each_tx_hash(std::next(block_result.transaction_hashes().begin()), 
                         block_result.transaction_hashes().end(), 
                         tx_store, from, witness, handler);

        ++from;
    }
}
#endif // BITPRIM_DB_LEGACY


#ifdef BITPRIM_DB_NEW_FULL
void block_chain::for_each_transaction(size_t from, size_t to, bool witness, for_each_tx_handler const& handler) const {
#ifdef BITPRIM_CURRENCY_BCH
    witness = false;    //TODO(fernando): see what to do with those things!
#endif
    
    while (from <= to) {

        if (stopped()) {
            handler(error::service_stopped, 0, chain::transaction{});
            return;
        }
    
        auto const block_result = database_.internal_db().get_block(from);

        if ( ! block_result.is_valid()) {
            handler(error::not_found, 0, chain::transaction{});
            return;
        }

        //BITCOIN_ASSERT(block_result.height() == from);
        //auto const tx_hashes = block_result.transaction_hashes();

        for_each_tx(block_result.transactions().begin(), 
                         block_result.transactions().end(), from, witness, handler);

        ++from;
    }
}

void block_chain::for_each_transaction_non_coinbase(size_t from, size_t to, bool witness, for_each_tx_handler const& handler) const {
#ifdef BITPRIM_CURRENCY_BCH
    witness = false;    //TODO(fernando): see what to do with those things!
#endif
    //auto const& tx_store = database_.transactions();

    while (from <= to) {

        if (stopped()) {
            handler(error::service_stopped, 0, chain::transaction{});
            return;
        }
    
        auto const block_result = database_.internal_db().get_block(from);

        if ( ! block_result.is_valid()) {
            handler(error::not_found, 0, chain::transaction{});
            return;
        }
        //BITCOIN_ASSERT(block_result.height() == from);
        auto const tx_hashes = block_result.transactions();

        for_each_tx(std::next(tx_hashes.begin()), 
                         tx_hashes.end(), from, witness, handler);

        ++from;
    }
}
#endif // BITPRIM_DB_NEW_FULL

#if defined(BITPRIM_WITH_KEOKEN)

void block_chain::convert_to_keo_transaction(const libbitcoin::hash_digest& hash, std::shared_ptr<std::vector<transaction_const_ptr>> keoken_txs) const {
   fetch_transaction(hash, true, false,
              [&](const libbitcoin::code &ec,
                  libbitcoin::transaction_const_ptr tx_ptr, size_t index,
                  size_t height) {
                  if (ec == libbitcoin::error::success) {
                      auto keoken_data = bitprim::keoken::first_keoken_output(*tx_ptr);
                      if (!keoken_data.empty()) {
                          (*keoken_txs).push_back(tx_ptr);
                      }
                  }
          });
}


#if defined(BITPRIM_DB_LEGACY)

void block_chain::fetch_keoken_history(const short_hash& address_hash, size_t limit,
    size_t from_height, keoken_history_fetch_handler handler) const
{
    auto keoken_txs = std::make_shared<std::vector<transaction_const_ptr>>();
    if (stopped())
    {
        handler(error::service_stopped, keoken_txs);
        return;
    }

//TODO (rama): RESTRICT MINIMUM HEIGHT (define bitprim::starting_keoken_height)
/*
    if(from_height < bitprim::starting_keoken_height)
        from_height = bitprim::starting_keoken_height;
*/
    auto history_compact_list =  database_.history().get(address_hash, limit, from_height);

    for (const auto & history : history_compact_list) {
        if ((*keoken_txs).empty()) {
            convert_to_keo_transaction(history.point.hash(), keoken_txs);
        }
        else
            if (history.point.hash() != keoken_txs->back()->hash()) {
                convert_to_keo_transaction(history.point.hash(), keoken_txs);
            }
    }

    handler(error::success, keoken_txs);
}


void block_chain::fetch_block_keoken(const hash_digest& hash, bool witness,
    block_keoken_fetch_handler handler) const
{
#ifdef BITPRIM_CURRENCY_BCH
    witness = false;
#endif

    auto keoken_txs = std::make_shared<std::vector<transaction_const_ptr>>();
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0, keoken_txs , 0, 0);
        return;
    }

    const auto block_result = database_.blocks().get(hash);

    if (!block_result)
    {
        handler(error::not_found, nullptr, 0, keoken_txs , 0, 0);
        return;
    }

    const auto height = block_result.height();
    const auto message = std::make_shared<const libbitcoin::message::header>(block_result.header());
    const auto tx_hashes = block_result.transaction_hashes();
    const auto& tx_store = database_.transactions();
    DEBUG_ONLY(size_t position = 0;)



    for (const auto& hash: tx_hashes)
    {
        const auto tx_result = tx_store.get(hash, max_size_t, true);

        if (!tx_result)
        {
            handler(error::operation_failed_17, nullptr, 0,keoken_txs, 0, 0);
            return;
        }

        BITCOIN_ASSERT(tx_result.height() == height);
        BITCOIN_ASSERT(tx_result.position() == position++);
        const libbitcoin::chain::transaction& tx_ptr = tx_result.transaction(witness);
        auto keoken_data = bitprim::keoken::first_keoken_output(tx_ptr);
        if (!keoken_data.empty()) {
            (*keoken_txs).push_back(std::make_shared<const libbitcoin::message::transaction>(tx_result.transaction(witness)));
        }
    }

    handler(error::success, message, height, keoken_txs, block_result.serialized_size(), tx_hashes.size());

}

#endif //defined(BITPRIM_DB_LEGACY)


#if defined(BITPRIM_DB_NEW_FULL)

void block_chain::fetch_keoken_history(const short_hash& address_hash, size_t limit,
    size_t from_height, keoken_history_fetch_handler handler) const
{
    auto keoken_txs = std::make_shared<std::vector<transaction_const_ptr>>();
    if (stopped())
    {
        handler(error::service_stopped, keoken_txs);
        return;
    }

//TODO (rama): RESTRICT MINIMUM HEIGHT (define bitprim::starting_keoken_height)
/*
    if(from_height < bitprim::starting_keoken_height)
        from_height = bitprim::starting_keoken_height;
*/
    auto history_compact_list =  database_.internal_db().get_history(address_hash, limit, from_height);

    for (const auto & history : history_compact_list) {
        if ((*keoken_txs).empty()) {
            convert_to_keo_transaction(history.point.hash(), keoken_txs);
        }
        else
            if (history.point.hash() != keoken_txs->back()->hash()) {
                convert_to_keo_transaction(history.point.hash(), keoken_txs);
            }
    }

    handler(error::success, keoken_txs);
}


void block_chain::fetch_block_keoken(const hash_digest& hash, bool witness,
    block_keoken_fetch_handler handler) const
{
#ifdef BITPRIM_CURRENCY_BCH
    witness = false;
#endif

    auto keoken_txs = std::make_shared<std::vector<transaction_const_ptr>>();
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0, keoken_txs , 0, 0);
        return;
    }

    const auto block_result = database_.internal_db().get_block(hash);

    if (!block_result.first.is_valid())
    {
        handler(error::not_found, nullptr, 0, keoken_txs , 0, 0);
        return;
    }

    const auto height = block_result.second;
    const auto message = std::make_shared<const libbitcoin::message::header>(block_result.first.header());
    //const auto tx_hashes = block_result.first.transaction_hashes();
    
    DEBUG_ONLY(size_t position = 0;)

    for (const auto& tx_result : block_result.first.transactions())
    {
        //const auto tx_result = database_.internal_db().get_transaction(hash, max_size_t, true);

        if (!tx_result.is_valid())
        {
            handler(error::operation_failed_17, nullptr, 0,keoken_txs, 0, 0);
            return;
        }

        //BITCOIN_ASSERT(tx_result.height() == height);
        //BITCOIN_ASSERT(tx_result.position() == position++);
        const libbitcoin::chain::transaction& tx_ptr = tx_result;
        auto keoken_data = bitprim::keoken::first_keoken_output(tx_ptr);
        if (!keoken_data.empty()) {
            (*keoken_txs).push_back(std::make_shared<const libbitcoin::message::transaction>(tx_result));
        }
    }

    handler(error::success, message, height, keoken_txs, block_result.first.serialized_size(), block_result.first.transactions().size());

}

#endif //defined(BITPRIM_DB_LEGACY)

#endif //WITH_KEOKEN

} // namespace blockchain
} // namespace libbitcoin
