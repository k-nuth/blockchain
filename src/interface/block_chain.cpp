/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/blockchain/interface/block_chain.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <unordered_set>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/database.hpp>


#ifdef BITPRIM_USE_DOMAIN
#include <bitcoin/infrastructure/math/sip_hash.hpp>
#else
#include <bitcoin/bitcoin/math/sip_hash.hpp>
#endif // BITPRIM_USE_DOMAIN

#include <bitcoin/bitcoin/multi_crypto_support.hpp>
#include <bitcoin/blockchain/settings.hpp>
#include <bitcoin/blockchain/populate/populate_chain_state.hpp>
#include <boost/thread/latch.hpp>


namespace libbitcoin { namespace blockchain {

using spent_value_type = std::pair<hash_digest, uint32_t>;
//using spent_container = std::vector<spent_value_type>;
using spent_container = std::unordered_set<spent_value_type>;

}}

namespace std {

template <>
struct hash<libbitcoin::blockchain::spent_value_type> {
    size_t operator()(libbitcoin::blockchain::spent_value_type const& point) const {
        size_t seed = 0;
        boost::hash_combine(seed, point.first);
        boost::hash_combine(seed, point.second);
        return seed;
    }
};

} // namespace std

namespace libbitcoin {
namespace blockchain {

using namespace bc::config;
using namespace bc::message;
using namespace bc::database;
using namespace std::placeholders;

#define NAME "block_chain"

static auto const hour_seconds = 3600u;

block_chain::block_chain(threadpool& pool,
    const blockchain::settings& chain_settings,
    const database::settings& database_settings,  bool relay_transactions)
    : stopped_(true)
    , settings_(chain_settings)
    , notify_limit_seconds_(chain_settings.notify_limit_hours * hour_seconds)
    , chain_state_populator_(*this, chain_settings)
    , database_(database_settings)
    , validation_mutex_(database_settings.flush_writes && relay_transactions)
    , priority_pool_(thread_ceiling(chain_settings.cores)
    , priority(chain_settings.priority))
    , dispatch_(priority_pool_, NAME "_priority")
    , transaction_organizer_(validation_mutex_, dispatch_, pool, *this, chain_settings)
    , block_organizer_(validation_mutex_, dispatch_, pool, *this, chain_settings, relay_transactions)
#ifdef BITPRIM_WITH_MINING
    , chosen_size_(0)
    , chosen_sigops_(0)
    , chosen_unconfirmed_()
    , chosen_spent_()
    , gbt_mutex_()
    , gbt_ready_(true)
#endif // BITPRIM_WITH_MINING    
{
}

// ============================================================================
// FAST CHAIN
// ============================================================================

// Readers.
// ----------------------------------------------------------------------------

uint32_t get_clock_now() {
    auto const now = std::chrono::high_resolution_clock::now();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count());
}

#ifdef BITPRIM_DB_LEGACY
inline
void block_chain::prune_reorg_async() {}
/*
inline 
void block_chain::set_database_flags() {}
*/

bool block_chain::get_gaps(block_database::heights& out_gaps) const
{
    database_.blocks().gaps(out_gaps);
    return true;
}

bool block_chain::get_block_exists(hash_digest const& block_hash) const
{
    return database_.blocks().get(block_hash);
}

bool block_chain::get_block_exists_safe(hash_digest const& block_hash) const
{
    return get_block_exists(block_hash);
}

bool block_chain::get_block_hash(hash_digest& out_hash, size_t height) const
{
    auto const result = database_.blocks().get(height);

    if (!result)
        return false;

    out_hash = result.hash();
    return true;
}

bool block_chain::get_branch_work(uint256_t& out_work,
    const uint256_t& maximum, size_t from_height) const
{
    size_t top;
    if (!database_.blocks().top(top))
        return false;

    out_work = 0;
    for (auto height = from_height; height <= top && out_work < maximum;
        ++height)
    {
        auto const result = database_.blocks().get(height);
        if (!result)
            return false;

        out_work += chain::header::proof(result.bits());
    }

    return true;
}

bool block_chain::get_header(chain::header& out_header, size_t height) const
{
    auto result = database_.blocks().get(height);
    if (!result)
        return false;

    out_header = result.header();
    return true;
}

bool block_chain::get_height(size_t& out_height,
    hash_digest const& block_hash) const
{
    auto result = database_.blocks().get(block_hash);
    if (!result)
        return false;

    out_height = result.height();
    return true;
}

bool block_chain::get_bits(uint32_t& out_bits, size_t height) const {
    auto result = database_.blocks().get(height);
    if (!result)
        return false;

    out_bits = result.bits();
    return true;
}

bool block_chain::get_timestamp(uint32_t& out_timestamp, size_t height) const {
    auto result = database_.blocks().get(height);
    if (!result)
        return false;

    out_timestamp = result.timestamp();
    return true;
}

bool block_chain::get_version(uint32_t& out_version, size_t height) const {
    auto result = database_.blocks().get(height);
    if (!result)
        return false;

    out_version = result.version();
    return true;
}

bool block_chain::get_last_height(size_t& out_height) const
{
    return database_.blocks().top(out_height);
}



bool block_chain::get_output_is_confirmed(chain::output& out_output, size_t& out_height,
                             bool& out_coinbase, bool& out_is_confirmed, const chain::output_point& outpoint,
                             size_t branch_height, bool require_confirmed) const
{
    // This includes a cached value for spender height (or not_spent).
    // Get the highest tx with matching hash, at or below the branch height.
    return database_.transactions().get_output_is_confirmed(out_output, out_height,
                                               out_coinbase, out_is_confirmed, outpoint, branch_height, require_confirmed);
}

//TODO(fernando): check if can we do it just with the UTXO
bool block_chain::get_is_unspent_transaction(hash_digest const& hash, size_t branch_height, bool require_confirmed) const {
    auto const result = database_.transactions().get(hash, branch_height, require_confirmed);
    return result && !result.is_spent(branch_height);
}
#endif // BITPRIM_DB_LEGACY



#if defined(BITPRIM_DB_LEGACY) || defined(BITPRIM_DB_NEW_FULL) 

bool block_chain::get_output(chain::output& out_output, size_t& out_height,
    uint32_t& out_median_time_past, bool& out_coinbase,
    const chain::output_point& outpoint, size_t branch_height,
    bool require_confirmed) const
{

#if defined(BITPRIM_DB_LEGACY)

    // This includes a cached value for spender height (or not_spent).
    // Get the highest tx with matching hash, at or below the branch height.
    return database_.transactions().get_output(out_output, out_height,
        out_median_time_past, out_coinbase, outpoint, branch_height,
        require_confirmed);

#elif defined(BITPRIM_DB_NEW_FULL)

    auto const tx = database_.internal_db().get_transaction(outpoint.hash(), branch_height);

    if (!tx.is_valid()) {
        return false;
    }
    
    out_height = tx.height();
    out_coinbase = tx.position() == 0;
    out_median_time_past = tx.median_time_past();
    out_output = tx.transaction().outputs()[outpoint.index()];

    return true;
#endif

}

#endif



//Bitprim: We don't store spent information
/*#if defined(BITPRIM_DB_NEW_FULL)
//TODO(fernando): check if can we do it just with the UTXO
bool block_chain::get_is_unspent_transaction(hash_digest const& hash, size_t branch_height, bool require_confirmed) const {
    auto const result = database_.internal_db().get_transaction(hash, branch_height, require_confirmed);
    return result.is_valid() && ! result.is_spent(branch_height);
}
#endif
*/

#if defined(BITPRIM_DB_LEGACY) || defined(BITPRIM_DB_NEW_FULL)
bool block_chain::get_transaction_position(size_t& out_height, size_t& out_position, hash_digest const& hash, bool require_confirmed) const {

#if defined(BITPRIM_DB_LEGACY)    
    auto const result = database_.transactions().get(hash, max_size_t, require_confirmed);
    if ( ! result) {
        return false;
    }

    out_height = result.height();
    out_position = result.position();
    return true;

#else
    auto const result = database_.internal_db().get_transaction(hash, max_size_t);
    
    if ( result.is_valid() ) {
        out_height = result.height();
        out_position = result.position();
        return true;
    }   

    if (require_confirmed ) {
        return false;
    } 
    
    auto const result2 = database_.internal_db().get_transaction_unconfirmed(hash);
    if ( ! result2.is_valid() ) {
        return false;
    }

    out_height = result2.height();
    out_position = position_max;
    return true;

#endif

}

#endif // defined(BITPRIM_DB_LEGACY) || defined(BITPRIM_DB_NEW_FULL)

#ifdef BITPRIM_DB_NEW

void block_chain::prune_reorg_async() {
    if ( ! is_stale()) {
        dispatch_.concurrent([this](){
            database_.prune_reorg();
        });
    }
}
/*
void block_chain::set_database_flags() {
    bool stale = is_stale();
    database_.set_database_flags(stale);
}*/

// bool block_chain::get_gaps(block_database::heights& out_gaps) const {
//     database_.blocks().gaps(out_gaps);
//     return true;
// }

bool block_chain::get_block_exists(hash_digest const& block_hash) const {
    
    return database_.internal_db().get_header(block_hash).first.is_valid();
}

bool block_chain::get_block_exists_safe(hash_digest const& block_hash) const {
   
    return get_block_exists(block_hash);
}

bool block_chain::get_block_hash(hash_digest& out_hash, size_t height) const {
   
    auto const result = database_.internal_db().get_header(height);
    if ( ! result.is_valid()) return false;
    out_hash = result.hash();
    return true;
}

bool block_chain::get_branch_work(uint256_t& out_work, uint256_t const& maximum, size_t from_height) const {
    
    size_t top;
    if ( ! get_last_height(top)) return false;

    out_work = 0;
    for (uint32_t height = from_height; height <= top && out_work < maximum; ++height) {
        auto const result = database_.internal_db().get_header(height);
        if ( ! result.is_valid()) return false;
        out_work += chain::header::proof(result.bits());
    }

    return true;
}

bool block_chain::get_header(chain::header& out_header, size_t height) const {
    out_header = database_.internal_db().get_header(height);
    return out_header.is_valid();
}

bool block_chain::get_height(size_t& out_height, hash_digest const& block_hash) const {
   
    auto result = database_.internal_db().get_header(block_hash);
    if ( ! result.first.is_valid()) return false;
    out_height = result.second;
    return true;
}

bool block_chain::get_bits(uint32_t& out_bits, size_t height) const {
   
    auto result = database_.internal_db().get_header(height);
    if ( ! result.is_valid()) return false;
    out_bits = result.bits();
    return true;
}

bool block_chain::get_timestamp(uint32_t& out_timestamp, size_t height) const {
   
    auto result = database_.internal_db().get_header(height);
    if ( ! result.is_valid()) return false;
    out_timestamp = result.timestamp();
    return true;
}

bool block_chain::get_version(uint32_t& out_version, size_t height) const {
  
    auto result = database_.internal_db().get_header(height);
    if ( ! result.is_valid()) return false;
    out_version = result.version();
    return true;
}

bool block_chain::get_last_height(size_t& out_height) const {
 
    uint32_t temp;
    auto res = database_.internal_db().get_last_height(temp);
    out_height = temp;
    return succeed(res);
}

bool block_chain::get_utxo(chain::output& out_output, size_t& out_height, uint32_t& out_median_time_past, bool& out_coinbase, chain::output_point const& outpoint, size_t branch_height) const {
    auto entry = database_.internal_db().get_utxo(outpoint);
    if ( ! entry.is_valid()) return false;
    if (entry.height() > branch_height) return false;

    out_output = entry.output();
    out_height = entry.height();
    out_median_time_past = entry.median_time_past();
    out_coinbase = entry.coinbase();

    return true;
}

std::pair<bool, database::internal_database::utxo_pool_t> block_chain::get_utxo_pool_from(uint32_t from, uint32_t to) const {
    auto p = database_.internal_db().get_utxo_pool_from(from, to);

    if (p.first != result_code::success) {
        return {false, std::move(p.second)};
    }    
    return {true, std::move(p.second)};
}

#endif // BITPRIM_DB_NEW


////transaction_ptr block_chain::get_transaction(size_t& out_block_height,
////    hash_digest const& hash, bool require_confirmed) const
////{
////    auto const result = database_.transactions().get(hash, max_size_t,
////        require_confirmed);
////
////    if (!result)
////        return nullptr;
////
////    out_block_height = result.height();
////    return std::make_shared<transaction>(result.transaction());
////}

// Writers
// ----------------------------------------------------------------------------

#ifdef BITPRIM_DB_LEGACY
bool block_chain::begin_insert() const {
    return database_.begin_insert();
}

bool block_chain::end_insert() const {
    return database_.end_insert();
}
#endif // BITPRIM_DB_LEGACY

bool block_chain::insert(block_const_ptr block, size_t height) {
    return database_.insert(*block, height) == error::success;
}

void block_chain::push(transaction_const_ptr tx, dispatcher&, result_handler handler) {
    
    //TODO: (bitprim) dissabled this tx cache because we don't want special treatment for the last txn, it affects the explorer rpc methods
    //last_transaction_.store(tx);

    // Transaction push is currently sequential so dispatch is not used.
    handler(database_.push(*tx, chain_state()->enabled_forks()));
}

#ifdef BITPRIM_WITH_MINING
//Mark every previous output of the transaction as TEMPORARY SPENT
void block_chain::append_spend(transaction_const_ptr tx) {
    std::vector<prev_output> prev_outputs;
    for (auto const& input : tx->inputs()) {
        auto const& output_point = input.previous_output();
        prev_outputs.push_back({output_point.hash(), output_point.index()});
    }
    chosen_spent_.insert({tx->hash(), std::move(prev_outputs)});

}

// Once a transaction was mined, every previous output that was marked
// as TEMPORARY SPENT, needs to be removed
// (since it was permanetly added to the spent database)
void block_chain::remove_spend(libbitcoin::hash_digest const& hash){
    chosen_spent_.erase(hash);
}

// Search the TEMPORARY SPENT index for conflicts between previous output
std::set<libbitcoin::hash_digest> block_chain::get_double_spend_chosen_list(transaction_const_ptr tx) {

    std::set<libbitcoin::hash_digest> spent_conflict{};
    for (auto const& input : tx->inputs()) {
        for(auto const& spents : chosen_spent_){
            for(auto const& previous : spents.second){
                if(previous.output_hash == input.previous_output().hash() &&
                    previous.output_index == input.previous_output().index())
                {
                    spent_conflict.insert(spents.first);
                }
            }
        }
    }
    return spent_conflict;
}

// Search the spent database and the TEMPORARY SPENT for conflicts
bool block_chain::check_is_double_spend(transaction_const_ptr tx){
    bool is_double_spend = false;
    for(auto const& input : tx->inputs()){
        boost::latch latch(2);
        fetch_spend(input.previous_output(), [&](const libbitcoin::code &ec, libbitcoin::chain::input_point input) {
            if (ec == libbitcoin::error::success) {
                is_double_spend = true;
            }
            latch.count_down();
        });
        latch.count_down_and_wait();

        if(is_double_spend) return true;
    }
    return !get_double_spend_chosen_list(tx).empty();
}


// Insert to chosen transaction List, ordered by the benefit yielded by the transaction
bool block_chain::insert_to_chosen_list(transaction_const_ptr& tx, double benefit, size_t tx_size, size_t tx_sigops){
#ifdef BITPRIM_CURRENCY_BCH
    tx_benefit txnew{benefit, tx_sigops, tx_size, tx->fees(), tx->to_data(1), tx->hash()};
#else
    tx_benefit txnew{benefit, tx_sigops, tx_size, tx->fees(), tx->to_data(1), tx->hash(), tx->hash(true)};
#endif
    chosen_unconfirmed_.insert(
        std::upper_bound(begin(chosen_unconfirmed_), end(chosen_unconfirmed_), txnew, [](tx_benefit const& a, tx_benefit const& b) {
            return a.benefit < b.benefit;
    }), txnew);

    chosen_size_ += tx_size;
    chosen_sigops_ += tx_sigops;
    append_spend(tx);

    return true;
}

//Search how many transactions we need to remove from the transaction chosen list, if we want to add a new transaction
size_t block_chain::find_txs_to_remove_from_chosen(const size_t sigops_limit, const size_t tx_size,
        const size_t tx_sigops, const size_t tx_fees, const double benefit,
            size_t& acum_sigops, size_t& acum_size, double& acum_benefit)
{
    // How many transactions should I remove, while still gaining more benefit
    acum_benefit += (*chosen_unconfirmed_.begin()).benefit;
    acum_sigops += (*chosen_unconfirmed_.begin()).tx_sigops;
    acum_size += (*chosen_unconfirmed_.begin()).tx_size;

    size_t removed = 0;
    auto max_block_size = libbitcoin::get_max_block_size() - libbitcoin::coinbase_reserved_size;

    for(auto to_remove : chosen_unconfirmed_){
        if(benefit > acum_benefit){
        //As long as the new transaction generate more benefit than the old ones, keep trying to add to the list
            if((chosen_sigops_ + tx_sigops - acum_sigops < sigops_limit) &&
                (chosen_size_ - acum_size + tx_size < (max_block_size))){
                // If I reached a point where taking out some transaction, i will generate more benefit
                return 1 + removed;
            } else {
                acum_benefit += to_remove.benefit; // Benefit i will loose if i remove this transaction
                acum_sigops += to_remove.tx_sigops; // Sigops removed from list
                acum_size += to_remove.tx_size; // Total bytes removed from list
                removed++;
            }
        } else return 0;
    }

    return 0;

}

bool block_chain::get_transaction_is_confirmed(libbitcoin::hash_digest tx_hash){

#ifdef BITPRIM_CURRENCY_BCH
    bool witness = false;
#else
    bool witness = true;
#endif

    bool is_confirmed = true;
    boost::latch latch(2);
    fetch_transaction(tx_hash, true, witness,
        [&](const libbitcoin::code &ec, libbitcoin::transaction_const_ptr tx_ptr, size_t index,
                size_t height) {
        if (ec != libbitcoin::error::success) {
            is_confirmed = false;
        }
        latch.count_down();
    });
    latch.count_down_and_wait();
    return is_confirmed;

}

//Check if the new transaction can be added to the txs selection.
bool block_chain::add_to_chosen_list(transaction_const_ptr tx){
    std::lock_guard<std::mutex> lock(gbt_mutex_);

    //Dont allow dependencies
    for(auto const& input : tx->inputs()){
        if( !get_transaction_is_confirmed(input.previous_output().hash())){
            return false;
        }
    }

    //If is not double spend
    if (!check_is_double_spend(tx)){
        auto tx_size = tx->serialized_size(0);
        auto tx_sigops = tx->signature_operations();
        auto tx_fees = tx->fees();

        auto estimated_size = chosen_size_ + tx_size;
        auto max_block_size = libbitcoin::get_max_block_size() - libbitcoin::coinbase_reserved_size;
        auto next_size = (estimated_size > max_block_size)? max_block_size : (estimated_size);
        auto sigops_limit = (size_t(next_size / libbitcoin::one_million_bytes_block) + 1) * libbitcoin::sigops_per_million_bytes;
        double benefit = (double(tx_fees) / tx_size );

        // Check if the transaction can be added directly to the chosen transaction list
        if (((chosen_sigops_ + tx_sigops) > sigops_limit) || (estimated_size > max_block_size)){
            size_t acum_sigops = 0;
            size_t acum_size = 0;
            double acum_benefit = 0;

            size_t txs_to_remove_from_list = find_txs_to_remove_from_chosen(sigops_limit, tx_size, tx_sigops, tx_fees, benefit,
                acum_sigops, acum_size, acum_benefit);

            if(txs_to_remove_from_list != 0)
            {
                // Remove transactions
                chosen_sigops_ -= acum_sigops;
                chosen_size_ -= acum_size;
                while(txs_to_remove_from_list != 0)
                {
                    remove_spend((*chosen_unconfirmed_.begin()).tx_id);
                    chosen_unconfirmed_.pop_front();
                    txs_to_remove_from_list--;
                }
                // Add the new transaction ordered by benefit
                insert_to_chosen_list(tx, benefit, tx_size, tx_sigops);
            }
        } else {
            // Since there still space to add the transaction
            // Add the new transaction ordered by benefit
            insert_to_chosen_list(tx, benefit, tx_size, tx_sigops);
        }
    }

    return true;
} 

std::vector<block_chain::tx_benefit> block_chain::get_gbt_tx_list() const{
    if (stopped()) {
        return std::vector<block_chain::tx_benefit>();
    }

    if (!gbt_ready_) {
        return std::vector<block_chain::tx_benefit>();
    }

    std::lock_guard<std::mutex> lock(gbt_mutex_);
    std::vector<tx_benefit> txs_chosen_list{ std::begin(chosen_unconfirmed_), std::end(chosen_unconfirmed_) };

    return txs_chosen_list;
}


//When a new block arrives, we need to check every transaction on the chosen list
//to see if it was mined; and remove it if it was.
//using tx_benefit = std::tuple<double /*benefit*/, size_t /*tx_sigops*/, size_t /*tx_size*/, size_t /*tx_fees*/, libbitcoin::data_chunk /*tx_hex*/, libbitcoin::hash_digest /*tx_hash */> ;
void block_chain::remove_mined_txs_from_chosen_list(block_const_ptr blk){

    gbt_ready_= false;
    std::lock_guard<std::mutex> lock(gbt_mutex_);
    for(auto const& tx : blk->transactions()){
        //erase transactions by hash
        auto it = std::find_if (chosen_unconfirmed_.begin(), chosen_unconfirmed_.end(),
            [&](const tx_benefit& tx_chosen){
                return tx_chosen.tx_id == tx.hash();
              });
        if(it != chosen_unconfirmed_.end()){
          //If transaction was deleted by hash, remove spended outputs
          remove_spend(tx.hash());
          chosen_unconfirmed_.erase(it);
        } else {
            //If the tx mined hash wasnt on our chosen list
            //There may be another transaction wich uses the same prev output as this tx. (double spend attempt) 
            //This should be extremely rare
            libbitcoin::message::transaction msg_tx {tx};
            transaction_const_ptr msg_ptr = std::make_shared<const libbitcoin::message::transaction>(msg_tx);
            for(auto const& conflict_hash : get_double_spend_chosen_list(msg_ptr)){
                auto conflict = std::find_if (chosen_unconfirmed_.begin(), chosen_unconfirmed_.end(),
                [&](const tx_benefit& tx_chosen){
                    return (tx_chosen.tx_id == conflict_hash);
                  });
                if(conflict != chosen_unconfirmed_.end()){
                    chosen_unconfirmed_.erase(conflict);
                    remove_spend(conflict_hash); //Do it on get_souble_spend_mempool
                }
            }
        }
    }

    chosen_size_ = 0;
    chosen_sigops_ = 0;
    for(auto const& tx : chosen_unconfirmed_ ){
        chosen_sigops_ += tx.tx_sigops;
        chosen_size_ += tx.tx_size;
    }
    gbt_ready_ = true;

}
#endif // BITPRIM_WITH_MINING



void block_chain::reorganize(const checkpoint& fork_point,
    block_const_ptr_list_const_ptr incoming_blocks,
    block_const_ptr_list_ptr outgoing_blocks, dispatcher& dispatch,
    result_handler handler)
{
    if (incoming_blocks->empty())
    {
        handler(error::operation_failed_13);
        return;
    }

    // The top (back) block is used to update the chain state.
    auto const complete =
        std::bind(&block_chain::handle_reorganize,
            this, _1, incoming_blocks->back(), handler);

    database_.reorganize(fork_point, incoming_blocks, outgoing_blocks,
        dispatch, complete);
}

void block_chain::handle_reorganize(const code& ec, block_const_ptr top,
    result_handler handler)
{
    if (ec)
    {
        handler(ec);
        return;
    }

    if (!top->validation.state)
    {
        handler(error::operation_failed_14);
        return;
    }

    set_chain_state(top->validation.state);
    last_block_.store(top);

    handler(error::success);
}

// Properties.
// ----------------------------------------------------------------------------

// For tx validator, call only from inside validate critical section.
chain::chain_state::ptr block_chain::chain_state() const
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(pool_state_mutex_);

    // Initialized on start and updated after each successful organization.
    return pool_state_;
    ///////////////////////////////////////////////////////////////////////////
}

// For block validator, call only from inside validate critical section.
chain::chain_state::ptr block_chain::chain_state(
    branch::const_ptr branch) const
{
    // Promote from cache if branch is same height as pool (most typical).
    // Generate from branch/store if the promotion is not successful.
    // If the organize is successful pool state will be updated accordingly.
    return chain_state_populator_.populate(chain_state(), branch);
}

// private.
code block_chain::set_chain_state(chain::chain_state::ptr previous)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(pool_state_mutex_);

    pool_state_ = chain_state_populator_.populate(previous);
    return pool_state_ ? error::success : error::operation_failed_15;
    ///////////////////////////////////////////////////////////////////////////
}

// ============================================================================
// SAFE CHAIN
// ============================================================================

// Startup and shutdown.
// ----------------------------------------------------------------------------

bool block_chain::start()
{
    stopped_ = false;

    if (!database_.open())
        return false;

    //switch to fast mode if the database is stale
    //set_database_flags();

    // Initialize chain state after database start but before organizers.
    pool_state_ = chain_state_populator_.populate();

    return pool_state_ && transaction_organizer_.start() &&
        block_organizer_.start();
}

bool block_chain::stop()
{
    stopped_ = true;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    validation_mutex_.lock_high_priority();

    // This cannot call organize or stop (lock safe).
    auto result = transaction_organizer_.stop() && block_organizer_.stop();

    // The priority pool must not be stopped while organizing.
    priority_pool_.shutdown();

    validation_mutex_.unlock_high_priority();
    ///////////////////////////////////////////////////////////////////////////
    return result;
}

// Close is idempotent and thread safe.
// Optional as the blockchain will close on destruct.
bool block_chain::close()
{
    auto const result = stop();
    priority_pool_.join();
    return result && database_.close();
}

block_chain::~block_chain()
{
    close();
}

// Queries.
// ----------------------------------------------------------------------------
// Blocks are and transactions returned const because they don't change and
// this eliminates the need to copy the cached items.

#ifdef BITPRIM_DB_LEGACY
void block_chain::fetch_block(size_t height, bool witness,
    block_fetch_handler handler) const
{
#ifdef BITPRIM_CURRENCY_BCH
    witness = false;
#endif
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0);
        return;
    }

    auto const cached = last_block_.load();

    // Try the cached block first.
    if (cached && cached->validation.state &&
        cached->validation.state->height() == height)
    {
        handler(error::success, cached, height);
        return;
    }

    auto const block_result = database_.blocks().get(height);

    if (!block_result)
    {
        handler(error::not_found, nullptr, 0);
        return;
    }

    BITCOIN_ASSERT(block_result.height() == height);
    auto const tx_hashes = block_result.transaction_hashes();
    auto const& tx_store = database_.transactions();
    transaction::list txs;
    txs.reserve(tx_hashes.size());
    DEBUG_ONLY(size_t position = 0;)

    for (auto const& hash: tx_hashes)
    {
        auto const tx_result = tx_store.get(hash, max_size_t, true);

        if (!tx_result)
        {
            handler(error::operation_failed_16, nullptr, 0);
            return;
        }

        BITCOIN_ASSERT(tx_result.height() == height);
        BITCOIN_ASSERT(tx_result.position() == position++);
        txs.push_back(tx_result.transaction(witness));
    }

    auto message = std::make_shared<const block>(block_result.header(),
        std::move(txs));
    handler(error::success, message, height);
}

void block_chain::fetch_block(hash_digest const& hash, bool witness,
    block_fetch_handler handler) const
{
#ifdef BITPRIM_CURRENCY_BCH
    witness = false;
#endif
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0);
        return;
    }

    auto const cached = last_block_.load();

    // Try the cached block first.
    if (cached && cached->validation.state && cached->hash() == hash)
    {
        handler(error::success, cached, cached->validation.state->height());
        return;
    }

    auto const block_result = database_.blocks().get(hash);

    if (!block_result)
    {
        handler(error::not_found, nullptr, 0);
        return;
    }

    auto const height = block_result.height();
    auto const tx_hashes = block_result.transaction_hashes();
    auto const& tx_store = database_.transactions();
    transaction::list txs;
    txs.reserve(tx_hashes.size());
    DEBUG_ONLY(size_t position = 0;)

    for (auto const& hash: tx_hashes)
    {
        auto const tx_result = tx_store.get(hash, max_size_t, true);

        if (!tx_result)
        {
            handler(error::operation_failed_17, nullptr, 0);
            return;
        }

        BITCOIN_ASSERT(tx_result.height() == height);
        BITCOIN_ASSERT(tx_result.position() == position++);
        txs.push_back(tx_result.transaction(witness));
    }

    auto const message = std::make_shared<const block>(block_result.header(),
        std::move(txs));
    handler(error::success, message, height);
}

void block_chain::fetch_block_header_txs_size(hash_digest const& hash,
    block_header_txs_size_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0, std::make_shared<hash_list>(hash_list()),0);
        return;
    }

    auto const block_result = database_.blocks().get(hash);

    if (!block_result)
    {
        handler(error::not_found, nullptr, 0, std::make_shared<hash_list>(hash_list()),0);
        return;
    }

    auto const height = block_result.height();
    auto const message = std::make_shared<const header>(block_result.header());
    auto const tx_hashes = std::make_shared<hash_list>(block_result.transaction_hashes());
    //TODO encapsulate header and tx_list
    handler(error::success, message, height, tx_hashes, block_result.serialized_size());
}

void block_chain::fetch_block_hash_timestamp(size_t height, block_hash_time_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, null_hash, 0, 0);
        return;
    }

    auto const block_result = database_.blocks().get(height);

    if (!block_result)
    {
        handler(error::not_found, null_hash, 0, 0);
        return;
    }

    handler(error::success, block_result.hash(), block_result.timestamp(), height);

}


void block_chain::fetch_block_header(size_t height,
    block_header_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0);
        return;
    }

    auto const result = database_.blocks().get(height);

    if (!result)
    {
        handler(error::not_found, nullptr, 0);
        return;
    }

    auto const message = std::make_shared<header>(result.header());
    handler(error::success, message, result.height());
}

void block_chain::fetch_block_header(hash_digest const& hash,
    block_header_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0);
        return;
    }

    auto const result = database_.blocks().get(hash);

    if (!result)
    {
        handler(error::not_found, nullptr, 0);
        return;
    }

    auto const message = std::make_shared<header>(result.header());
    handler(error::success, message, result.height());
}

// void block_chain::fetch_merkle_block(size_t height, transaction_hashes_fetch_handler handler) const
void block_chain::fetch_merkle_block(size_t height, merkle_block_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0);
        return;
    }

    auto const result = database_.blocks().get(height);

    if (!result)
    {
        handler(error::not_found, nullptr, 0);
        return;
    }

    auto const merkle = std::make_shared<merkle_block>(result.header(),
        result.transaction_count(), result.transaction_hashes(), data_chunk{});
    handler(error::success, merkle, result.height());
}

void block_chain::fetch_merkle_block(hash_digest const& hash,
    merkle_block_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0);
        return;
    }

    auto const result = database_.blocks().get(hash);

    if (!result)
    {
        handler(error::not_found, nullptr, 0);
        return;
    }

    auto const merkle = std::make_shared<merkle_block>(result.header(),
        result.transaction_count(), result.transaction_hashes(), data_chunk{});
    handler(error::success, merkle, result.height());
}

void block_chain::fetch_compact_block(size_t height, compact_block_fetch_handler handler) const {
    // TODO (Mario): implement compact blocks.
    handler(error::not_implemented, {}, 0);
}

void block_chain::fetch_compact_block(hash_digest const& hash, compact_block_fetch_handler handler) const {
#ifdef BITPRIM_CURRENCY_BCH
    bool witness = false;
#else
    bool witness = true;
#endif
    if (stopped()) {
        handler(error::service_stopped, {},0);
        return;
    }
    
    fetch_block(hash, witness,[&handler](const code& ec, block_const_ptr message, size_t height) {
        if (ec == error::success) {
            auto blk_ptr = std::make_shared<compact_block>(compact_block::factory_from_block(*message));
            handler(error::success, blk_ptr, height);
        } else {
            handler(ec, nullptr, height);
        }
    });
}

void block_chain::fetch_block_height(hash_digest const& hash,
    block_height_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, {});
        return;
    }

    auto const result = database_.blocks().get(hash);

    if (!result)
    {
        handler(error::not_found, 0);
        return;
    }

    handler(error::success, result.height());
}

void block_chain::fetch_last_height(last_height_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, {});
        return;
    }

    size_t last_height;

    if (!database_.blocks().top(last_height))
    {
        handler(error::not_found, 0);
        return;
    }

    handler(error::success, last_height);
}

void block_chain::fetch_transaction(hash_digest const& hash,
    bool require_confirmed, bool witness,
    transaction_fetch_handler handler) const
{
#ifdef BITPRIM_CURRENCY_BCH
    witness = false;
#endif
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0, 0);
        return;
    }
//TODO: (bitprim) dissabled this tx cache because we don't want special treatment for the last txn, it affects the explorer rpc methods
//    // Try the cached block first if confirmation is not required.
//    if (!require_confirmed)
//    {
//        auto const cached = last_transaction_.load();
//
//        if (cached && cached->validation.state && cached->hash() == hash)
//        {
//            ////LOG_INFO(LOG_BLOCKCHAIN) << "TX CACHE HIT";
//
//            // Simulate the position and height overloading of the database.
//            handler(error::success, cached, transaction_database::unconfirmed,
//                cached->validation.state->height());
//            return;
//        }
//    }
  

    auto const result = database_.transactions().get(hash, max_size_t,
        require_confirmed);

    if (!result)
    {
        handler(error::not_found, nullptr, 0, 0);
        return;
    }

    auto const tx = std::make_shared<const transaction>(
        result.transaction(witness));
    handler(error::success, tx, result.position(), result.height());
}
#endif // BITPRIM_DB_LEGACY

#if defined(BITPRIM_DB_NEW_BLOCKS) || defined(BITPRIM_DB_NEW_FULL)

void block_chain::fetch_block(size_t height, bool witness,
    block_fetch_handler handler) const
{
#ifdef BITPRIM_CURRENCY_BCH
    witness = false;
#endif
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0);
        return;
    }

    auto const cached = last_block_.load();

    // Try the cached block first.
    if (cached && cached->validation.state &&
        cached->validation.state->height() == height)
    {
        handler(error::success, cached, height);
        return;
    }

    auto const block_result = database_.internal_db().get_block(height);

    if (!block_result.is_valid())
    {
        handler(error::not_found, nullptr, 0);
        return;
    }

    auto const result = std::make_shared<const block>(block_result);

    handler(error::success, result, height);
}

void block_chain::fetch_block(hash_digest const& hash, bool witness,
    block_fetch_handler handler) const
{
#ifdef BITPRIM_CURRENCY_BCH
    witness = false;
#endif
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0);
        return;
    }

    auto const cached = last_block_.load();

    // Try the cached block first.
    if (cached && cached->validation.state && cached->hash() == hash)
    {
        handler(error::success, cached, cached->validation.state->height());
        return;
    }

    auto const block_result = database_.internal_db().get_block(hash);

    if (!block_result.first.is_valid())
    {
        handler(error::not_found, nullptr, 0);
        return;
    }

    auto const height = block_result.second; 

    auto const result = std::make_shared<const block>(block_result.first);

    handler(error::success, result, height);
}

void block_chain::fetch_block_header_txs_size(hash_digest const& hash,
    block_header_txs_size_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0, std::make_shared<hash_list>(hash_list()),0);
        return;
    }

    auto const block_result = database_.internal_db().get_block(hash);

    if (!block_result.first.is_valid())
    {
        handler(error::not_found, nullptr, 0, std::make_shared<hash_list>(hash_list()),0);
        return;
    }

    auto const height = block_result.second; 
    auto const result = std::make_shared<const header>(block_result.first.header());
    auto const tx_hashes = std::make_shared<hash_list>(block_result.first.to_hashes());
    //TODO encapsulate header and tx_list
    handler(error::success, result, height, tx_hashes, block_result.first.serialized_size());
}


// void block_chain::fetch_merkle_block(size_t height, transaction_hashes_fetch_handler handler) const
void block_chain::fetch_merkle_block(size_t height, merkle_block_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0);
        return;
    }

    auto const block_result = database_.internal_db().get_block(height);

    if (!block_result.is_valid())
    {
        handler(error::not_found, nullptr, 0);
        return;
    }

    auto const merkle = std::make_shared<merkle_block>(block_result.header(),
        block_result.transactions().size(), block_result.to_hashes(), data_chunk{});
    handler(error::success, merkle, height);
}

void block_chain::fetch_merkle_block(hash_digest const& hash,
    merkle_block_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0);
        return;
    }

    auto const block_result = database_.internal_db().get_block(hash);

    if (!block_result.first.is_valid())
    {
        handler(error::not_found, nullptr, 0);
        return;
    }

    auto const merkle = std::make_shared<merkle_block>(block_result.first.header(),
        block_result.first.transactions().size(), block_result.first.to_hashes(), data_chunk{});
    handler(error::success, merkle, block_result.second);
}

void block_chain::fetch_compact_block(size_t height, compact_block_fetch_handler handler) const {
    // TODO (Mario): implement compact blocks.
    handler(error::not_implemented, {}, 0);
}

void block_chain::fetch_compact_block(hash_digest const& hash, compact_block_fetch_handler handler) const {
#ifdef BITPRIM_CURRENCY_BCH
    bool witness = false;
#else
    bool witness = true;
#endif
    if (stopped()) {
        handler(error::service_stopped, {},0);
        return;
    }
    
    fetch_block(hash, witness,[&handler](const code& ec, block_const_ptr message, size_t height) {
        if (ec == error::success) {
            auto blk_ptr = std::make_shared<compact_block>(compact_block::factory_from_block(*message));
            handler(error::success, blk_ptr, height);
        } else {
            handler(ec, nullptr, height);
        }
    });
}

// This may execute over 500 queries.
void block_chain::fetch_locator_block_hashes(get_blocks_const_ptr locator,
    hash_digest const& threshold, size_t limit,
    inventory_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    // This is based on the idea that looking up by block hash to get heights
    // will be much faster than hashing each retrieved block to test for stop.

    // Find the start block height.
    // If no start block is on our chain we start with block 0.
    uint32_t start = 0;
    for (auto const& hash: locator->start_hashes())
    {
        auto const result = database_.internal_db().get_block(hash);
        if (result.first.is_valid())
        {
            start = result.second;
            break;
        }
    }

    // The begin block requested is always one after the start block.
    auto begin = safe_add(start, uint32_t(1));

    // The maximum number of headers returned is 500.
    auto end = safe_add(begin, uint32_t(limit));

    // Find the upper threshold block height (peer-specified).
    if (locator->stop_hash() != null_hash)
    {
        // If the stop block is not on chain we treat it as a null stop.
        auto const result = database_.internal_db().get_block(locator->stop_hash());

        // Otherwise limit the end height to the stop block height.
        // If end precedes begin floor_subtract will handle below.
        if (result.first.is_valid())
            end = std::min(result.second, end);
    }

    // Find the lower threshold block height (self-specified).
    if (threshold != null_hash)
    {
        // If the threshold is not on chain we ignore it.
        auto const result = database_.internal_db().get_block(threshold);

        // Otherwise limit the begin height to the threshold block height.
        // If begin exceeds end floor_subtract will handle below.
        if (result.first.is_valid())
            begin = std::max(result.second, begin);
    }

    auto hashes = std::make_shared<inventory>();
    hashes->inventories().reserve(floor_subtract(end, begin));

    // Build the hash list until we hit end or the blockchain top.
    for (auto height = begin; height < end; ++height)
    {
        auto const result = database_.internal_db().get_block(height);

        // If not found then we are at our top.
        if (!result.is_valid())
        {
            hashes->inventories().shrink_to_fit();
            break;
        }

        static auto const id = inventory::type_id::block;
        hashes->inventories().emplace_back(id, result.header().hash());
    }

    handler(error::success, std::move(hashes));
}


#endif //BITPRIM_DB_NEW_BLOCKS || BITPRIM_DB_NEW_FULL


#if defined(BITPRIM_DB_NEW_FULL)

void block_chain::fetch_transaction(hash_digest const& hash, bool require_confirmed, bool witness, transaction_fetch_handler handler) const
{
#ifdef BITPRIM_CURRENCY_BCH
    witness = false;
#endif
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0, 0);
        return;
    }
//TODO: (bitprim) dissabled this tx cache because we don't want special treatment for the last txn, it affects the explorer rpc methods
//    // Try the cached block first if confirmation is not required.
//    if (!require_confirmed)
//    {
//        auto const cached = last_transaction_.load();
//
//        if (cached && cached->validation.state && cached->hash() == hash)
//        {
//            ////LOG_INFO(LOG_BLOCKCHAIN) << "TX CACHE HIT";
//
//            // Simulate the position and height overloading of the database.
//            handler(error::success, cached, transaction_database::unconfirmed,
//                cached->validation.state->height());
//            return;
//        }
//    }
  

    auto const result = database_.internal_db().get_transaction(hash, max_size_t);
    if ( result.is_valid() )
    {
        auto const tx = std::make_shared<const transaction>(result.transaction());
        handler(error::success, tx, result.position(), result.height());    
        return;
    }

    if (require_confirmed) {
        handler(error::not_found, nullptr, 0, 0);
        return;
    }

    auto const result2 = database_.internal_db().get_transaction_unconfirmed(hash);
    if (! result2.is_valid() )
    {
        handler(error::not_found, nullptr, 0, 0);
        return;
    }

    auto const tx = std::make_shared<const transaction>(result2.transaction());
    handler(error::success, tx, position_max, result2.height());
}

// This is same as fetch_transaction but skips deserializing the tx payload.
void block_chain::fetch_transaction_position(hash_digest const& hash, bool require_confirmed, transaction_index_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, 0, 0);
        return;
    }

    auto const result = database_.internal_db().get_transaction(hash, max_size_t);

    if ( result.is_valid() )
    {
        handler(error::success, result.position(), result.height());
        return;
    }

    if (require_confirmed) {
        handler(error::not_found, 0, 0);
        return;
    }

    auto const result2 = database_.internal_db().get_transaction_unconfirmed(hash);
    if (! result2.is_valid() ) {
        handler(error::not_found, 0, 0);
        return;
    }

    handler(error::success, position_max, result2.height());
}

#endif


//TODO (Mario) : Review and move to proper location
hash_digest generate_merkle_root(std::vector<chain::transaction> transactions) {
    if (transactions.empty())
        return null_hash;

    hash_list merkle, update;

    auto hasher = [&merkle](const transaction& tx)
    {
        merkle.push_back(tx.hash());
    };

    // Hash ordering matters, don't use std::transform here.
    std::for_each(transactions.begin(), transactions.end(), hasher);

    // Initial capacity is half of the original list (clear doesn't reset).
    update.reserve((merkle.size() + 1) / 2);

    while (merkle.size() > 1)
    {
        // If number of hashes is odd, duplicate last hash in the list.
        if (merkle.size() % 2 != 0)
            merkle.push_back(merkle.back());

        for (auto it = merkle.begin(); it != merkle.end(); it += 2)
            update.push_back(bitcoin_hash(build_chunk({ it[0], it[1] })));

        std::swap(merkle, update);
        update.clear();
    }

    // There is now only one item in the list.
    return merkle.front();
}

#ifdef BITPRIM_DB_NEW_FULL
//TODO(fernando): refactor!!!
std::vector<libbitcoin::blockchain::mempool_transaction_summary> block_chain::get_mempool_transactions(std::vector<std::string> const& payment_addresses, bool use_testnet_rules, bool witness) const {
/*          "    \"address\"  (string) The base58check encoded address\n"
            "    \"txid\"  (string) The related txid\n"
            "    \"index\"  (number) The related input or output index\n"
            "    \"satoshis\"  (number) The difference of satoshis\n"
            "    \"timestamp\"  (number) The time the transaction entered the mempool (seconds)\n"
            "    \"prevtxid\"  (string) The previous txid (if spending)\n"
            "    \"prevout\"  (string) The previous transaction output index (if spending)\n"
*/

#ifdef BITPRIM_CURRENCY_BCH
    witness = false;
#endif

    uint8_t encoding_p2kh;
    uint8_t encoding_p2sh;

    if (use_testnet_rules) {
        encoding_p2kh = libbitcoin::wallet::payment_address::testnet_p2kh;
        encoding_p2sh = libbitcoin::wallet::payment_address::testnet_p2sh;
    } else {
        encoding_p2kh = libbitcoin::wallet::payment_address::mainnet_p2kh;
        encoding_p2sh = libbitcoin::wallet::payment_address::mainnet_p2sh;
    }
    
    std::vector<libbitcoin::blockchain::mempool_transaction_summary> ret;
    
    std::unordered_set<libbitcoin::wallet::payment_address> addrs;
    for (auto const& payment_address : payment_addresses) {
        libbitcoin::wallet::payment_address address(payment_address);
        if (address){
            addrs.insert(address);
        }
    }

    auto const result = database_.internal_db().get_all_transaction_unconfirmed();
    
    for (auto const& tx_res : result) {
        auto const& tx = tx_res.transaction();
        //tx.recompute_hash();
        size_t i = 0;
        for (auto const& output : tx.outputs()) {
            auto const tx_addresses = libbitcoin::wallet::payment_address::extract(output.script(), encoding_p2kh, encoding_p2sh);
            for(auto const tx_address : tx_addresses) {
                if (tx_address && addrs.find(tx_address) != addrs.end()) {
                    ret.push_back
                            (libbitcoin::blockchain::mempool_transaction_summary
                                     (tx_address.encoded(), libbitcoin::encode_hash(tx.hash()), "",
                                      "", std::to_string(output.value()), i, tx_res.arrival_time()));
                }
            }
            ++i;
        }
        i = 0;
        for (auto const& input : tx.inputs()) {
            // TODO: payment_addrress::extract should use the prev_output script instead of the input script
            // see https://github.com/bitprim/bitprim-core/blob/v0.10.0/src/wallet/payment_address.cpp#L505
            auto const tx_addresses = libbitcoin::wallet::payment_address::extract(input.script(), encoding_p2kh, encoding_p2sh);
            for(auto const tx_address : tx_addresses)
            if (tx_address && addrs.find(tx_address) != addrs.end()) {
                boost::latch latch(2);
                fetch_transaction(input.previous_output().hash(), false, witness,
                                  [&](const libbitcoin::code &ec,
                                      libbitcoin::transaction_const_ptr tx_ptr, size_t index,
                                      size_t height) {
                                      if (ec == libbitcoin::error::success) {
                                          ret.push_back(libbitcoin::blockchain::mempool_transaction_summary
                                                                (tx_address.encoded(),
                                                                libbitcoin::encode_hash(tx.hash()),
                                                                libbitcoin::encode_hash(input.previous_output().hash()),
                                                                 std::to_string(input.previous_output().index()),
                                                                "-"+std::to_string(tx_ptr->outputs()[input.previous_output().index()].value()),
                                                                i,
                                                                tx_res.arrival_time()));
                                      }
                                      latch.count_down();
                                  });
                latch.count_down_and_wait();
            }
            ++i;
        }
    }

    return ret;
}

// Precondition: valid payment addresses
std::vector<chain::transaction> block_chain::get_mempool_transactions_from_wallets(std::vector<wallet::payment_address> const& payment_addresses, bool use_testnet_rules, bool witness) const {

#ifdef BITPRIM_CURRENCY_BCH
    witness = false;
#endif

    uint8_t encoding_p2kh;
    uint8_t encoding_p2sh;
    if (use_testnet_rules){
        encoding_p2kh = libbitcoin::wallet::payment_address::testnet_p2kh;
        encoding_p2sh = libbitcoin::wallet::payment_address::testnet_p2sh;
    } else {
        encoding_p2kh = libbitcoin::wallet::payment_address::mainnet_p2kh;
        encoding_p2sh = libbitcoin::wallet::payment_address::mainnet_p2sh;
    }

    std::vector<chain::transaction> ret;

    auto const result = database_.internal_db().get_all_transaction_unconfirmed();

    for (auto const& tx_res : result) { 
        auto const& tx = tx_res.transaction();
        //tx.recompute_hash();
        // Only insert the transaction once. Avoid duplicating the tx if serveral wallets are used in the same tx, and if the same wallet is the input and output addr.
        bool inserted = false;

        for (auto iter_output = tx.outputs().begin(); (iter_output != tx.outputs().end() && !inserted); ++iter_output) {
        
            const auto tx_addresses = libbitcoin::wallet::payment_address::extract((*iter_output).script(), encoding_p2kh, encoding_p2sh);

            for (auto iter_addr = tx_addresses.begin(); (iter_addr != tx_addresses.end() && !inserted); ++iter_addr) {
                if (*iter_addr) {
                    auto it = std::find(payment_addresses.begin(), payment_addresses.end(), *iter_addr);
                    if (it != payment_addresses.end()) {
                        ret.push_back(tx);
                        inserted = true;
                    }
                }
            }
        }

        for (auto iter_input = tx.inputs().begin(); (iter_input != tx.inputs().end() && !inserted); ++iter_input) {
            // TODO: payment_addrress::extract should use the prev_output script instead of the input script
            // see https://github.com/bitprim/bitprim-core/blob/v0.10.0/src/wallet/payment_address.cpp#L505
            const auto tx_addresses = libbitcoin::wallet::payment_address::extract((*iter_input).script(), encoding_p2kh, encoding_p2sh);
            for (auto iter_addr = tx_addresses.begin(); (iter_addr != tx_addresses.end() && !inserted); ++iter_addr) {
                if (*iter_addr) {
                    auto it = std::find(payment_addresses.begin(), payment_addresses.end(), *iter_addr);
                    if (it != payment_addresses.end()) {
                        ret.push_back(tx);
                        inserted = true;
                    }
                }
            }
        }
        
    }

    return ret;
}

void block_chain::fill_tx_list_from_mempool(message::compact_block const& block, size_t& mempool_count, std::vector<chain::transaction>& txn_available, std::unordered_map<uint64_t, uint16_t> const& shorttxids) const {
    
    std::vector<bool> have_txn(txn_available.size());
    
    auto header_hash = hash(block);
    auto k0 = from_little_endian_unsafe<uint64_t>(header_hash.begin());
    auto k1 = from_little_endian_unsafe<uint64_t>(header_hash.begin() + sizeof(uint64_t));

    auto const result = database_.internal_db().get_all_transaction_unconfirmed();

    for (auto const& tx_res : result) { 
        auto const& tx = tx_res.transaction();

#ifdef BITPRIM_CURRENCY_BCH
        bool witness = false;
#else
        bool witness = true;
#endif
        uint64_t shortid = sip_hash_uint256(k0, k1, tx.hash(witness)) & uint64_t(0xffffffffffff);
        
        auto idit = shorttxids.find(shortid);
        if (idit != shorttxids.end()) {
            if (!have_txn[idit->second]) {
                txn_available[idit->second] = tx;
                have_txn[idit->second] = true;
                ++mempool_count;
            } else {
                // If we find two mempool txn that match the short id, just
                // request it. This should be rare enough that the extra
                // bandwidth doesn't matter, but eating a round-trip due to
                // FillBlock failure would be annoying.
                if (txn_available[idit->second].is_valid()) {
                    //txn_available[idit->second].reset();
                    txn_available[idit->second] = chain::transaction{};
                    --mempool_count;
                }
            }
        }

        //TODO (Mario) :  break the loop
        // Though ideally we'd continue scanning for the
        // two-txn-match-shortid case, the performance win of an early exit
        // here is too good to pass up and worth the extra risk.
        /*if (mempool_count == shorttxids.size()) {
            return false;
        } else {
            return true;
        }*/
    }

}

safe_chain::mempool_mini_hash_map block_chain::get_mempool_mini_hash_map(message::compact_block const& block) const {
#ifdef BITPRIM_CURRENCY_BCH
     bool witness = false;
#else
     bool witness = true;
#endif

    if (stopped()) {
        return safe_chain::mempool_mini_hash_map();
    }
    
    auto header_hash = hash(block);

    auto k0 = from_little_endian_unsafe<uint64_t>(header_hash.begin());
    auto k1 = from_little_endian_unsafe<uint64_t>(header_hash.begin() + sizeof(uint64_t));

    safe_chain::mempool_mini_hash_map mempool;


    auto const result = database_.internal_db().get_all_transaction_unconfirmed();

    for (auto const& tx_res : result) { 
        auto const& tx = tx_res.transaction();

        auto sh = sip_hash_uint256(k0, k1, tx.hash(witness));
        
       /* to_little_endian()
        uint64_t pepe = 4564564;
        uint64_t pepe2 = pepe & 0x0000ffffffffffff;
            
        reinterpret_cast<uint8_t*>(pepe2);
        */
        //Drop the most significative bytes from the sh
        mini_hash short_id;
        mempool.emplace(short_id,tx);
    
    }

    return mempool;
}

std::vector<libbitcoin::blockchain::mempool_transaction_summary> block_chain::get_mempool_transactions(std::string const& payment_address, bool use_testnet_rules, bool witness) const{
    std::vector<std::string> addresses = {payment_address};
    return get_mempool_transactions(addresses, use_testnet_rules, witness);
}


void block_chain::fetch_unconfirmed_transaction(hash_digest const& hash, transaction_unconfirmed_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    auto const result = database_.internal_db().get_transaction_unconfirmed(hash);

    if ( ! result.is_valid())
    {
        handler(error::not_found, nullptr);
        return;
    }

    auto const tx = std::make_shared<const transaction>(result.transaction());
    handler(error::success, tx);
}


#endif // BITPRIM_DB_NEW_FULL

#ifdef BITPRIM_DB_TRANSACTION_UNCONFIRMED
//TODO(fernando): refactor!!!
std::vector<libbitcoin::blockchain::mempool_transaction_summary> block_chain::get_mempool_transactions(std::vector<std::string> const& payment_addresses, bool use_testnet_rules, bool witness) const {
/*          "    \"address\"  (string) The base58check encoded address\n"
            "    \"txid\"  (string) The related txid\n"
            "    \"index\"  (number) The related input or output index\n"
            "    \"satoshis\"  (number) The difference of satoshis\n"
            "    \"timestamp\"  (number) The time the transaction entered the mempool (seconds)\n"
            "    \"prevtxid\"  (string) The previous txid (if spending)\n"
            "    \"prevout\"  (string) The previous transaction output index (if spending)\n"
*/

#ifdef BITPRIM_CURRENCY_BCH
    witness = false;
#endif

    uint8_t encoding_p2kh;
    uint8_t encoding_p2sh;
    if (use_testnet_rules){
        encoding_p2kh = libbitcoin::wallet::payment_address::testnet_p2kh;
        encoding_p2sh = libbitcoin::wallet::payment_address::testnet_p2sh;
    } else {
        encoding_p2kh = libbitcoin::wallet::payment_address::mainnet_p2kh;
        encoding_p2sh = libbitcoin::wallet::payment_address::mainnet_p2sh;
    }
    std::vector<libbitcoin::blockchain::mempool_transaction_summary> ret;
    std::unordered_set<libbitcoin::wallet::payment_address> addrs;
    for (auto const & payment_address : payment_addresses) {
        libbitcoin::wallet::payment_address address(payment_address);
        if (address){
            addrs.insert(address);
        }
    }

    database_.transactions_unconfirmed().for_each_result([&](libbitcoin::database::transaction_unconfirmed_result const &tx_res) {
        auto tx = tx_res.transaction(witness);
        tx.recompute_hash();
        size_t i = 0;
        for (auto const& output : tx.outputs()) {
            auto const tx_addresses = libbitcoin::wallet::payment_address::extract(output.script(), encoding_p2kh, encoding_p2sh);
            for(auto const tx_address : tx_addresses) {
                if (tx_address && addrs.find(tx_address) != addrs.end()) {
                    ret.push_back
                            (libbitcoin::blockchain::mempool_transaction_summary
                                     (tx_address.encoded(), libbitcoin::encode_hash(tx.hash()), "",
                                      "", std::to_string(output.value()), i, tx_res.arrival_time()));
                }
            }
            ++i;
        }
        i = 0;
        for (auto const& input : tx.inputs()) {
            // TODO: payment_addrress::extract should use the prev_output script instead of the input script
            // see https://github.com/bitprim/bitprim-core/blob/v0.10.0/src/wallet/payment_address.cpp#L505
            auto const tx_addresses = libbitcoin::wallet::payment_address::extract(input.script(), encoding_p2kh, encoding_p2sh);
            for(auto const tx_address : tx_addresses)
            if (tx_address && addrs.find(tx_address) != addrs.end()) {
                boost::latch latch(2);
                fetch_transaction(input.previous_output().hash(), false, witness,
                                  [&](const libbitcoin::code &ec,
                                      libbitcoin::transaction_const_ptr tx_ptr, size_t index,
                                      size_t height) {
                                      if (ec == libbitcoin::error::success) {
                                          ret.push_back(libbitcoin::blockchain::mempool_transaction_summary
                                                                (tx_address.encoded(),
                                                                libbitcoin::encode_hash(tx.hash()),
                                                                libbitcoin::encode_hash(input.previous_output().hash()),
                                                                 std::to_string(input.previous_output().index()),
                                                                "-"+std::to_string(tx_ptr->outputs()[input.previous_output().index()].value()),
                                                                i,
                                                                tx_res.arrival_time()));
                                      }
                                      latch.count_down();
                                  });
                latch.count_down_and_wait();
            }
            ++i;
        }
        return true;
    });

    return ret;
}

// Precondition: valid payment addresses
std::vector<chain::transaction> block_chain::get_mempool_transactions_from_wallets(std::vector<wallet::payment_address> const& payment_addresses, bool use_testnet_rules, bool witness) const {

#ifdef BITPRIM_CURRENCY_BCH
    witness = false;
#endif

    uint8_t encoding_p2kh;
    uint8_t encoding_p2sh;
    if (use_testnet_rules){
        encoding_p2kh = libbitcoin::wallet::payment_address::testnet_p2kh;
        encoding_p2sh = libbitcoin::wallet::payment_address::testnet_p2sh;
    } else {
        encoding_p2kh = libbitcoin::wallet::payment_address::mainnet_p2kh;
        encoding_p2sh = libbitcoin::wallet::payment_address::mainnet_p2sh;
    }

    std::vector<chain::transaction> ret;

    database_.transactions_unconfirmed().for_each_result([&](libbitcoin::database::transaction_unconfirmed_result const &tx_res) {
        auto tx = tx_res.transaction(witness);
        tx.recompute_hash();
        
        // Only insert the transaction once. Avoid duplicating the tx if serveral wallets are used in the same tx, and if the same wallet is the input and output addr.
        bool inserted = false;

        for (auto iter_output = tx.outputs().begin(); (iter_output != tx.outputs().end() && !inserted); ++iter_output) {
        
            const auto tx_addresses = libbitcoin::wallet::payment_address::extract((*iter_output).script(), encoding_p2kh, encoding_p2sh);

            for (auto iter_addr = tx_addresses.begin(); (iter_addr != tx_addresses.end() && !inserted); ++iter_addr) {
                if (*iter_addr) {
                    auto it = std::find(payment_addresses.begin(), payment_addresses.end(), *iter_addr);
                    if (it != payment_addresses.end()) {
                        ret.push_back(tx);
                        inserted = true;
                    }
                }
            }
        }

        for (auto iter_input = tx.inputs().begin(); (iter_input != tx.inputs().end() && !inserted); ++iter_input) {
            // TODO: payment_addrress::extract should use the prev_output script instead of the input script
            // see https://github.com/bitprim/bitprim-core/blob/v0.10.0/src/wallet/payment_address.cpp#L505
            const auto tx_addresses = libbitcoin::wallet::payment_address::extract((*iter_input).script(), encoding_p2kh, encoding_p2sh);
            for (auto iter_addr = tx_addresses.begin(); (iter_addr != tx_addresses.end() && !inserted); ++iter_addr) {
                if (*iter_addr) {
                    auto it = std::find(payment_addresses.begin(), payment_addresses.end(), *iter_addr);
                    if (it != payment_addresses.end()) {
                        ret.push_back(tx);
                        inserted = true;
                    }
                }
            }
        }
        return true;
    });

    return ret;
}

/*
   def get_siphash_keys(self):
        header_nonce = self.header.serialize()
        header_nonce += struct.pack("<Q", self.nonce)
        hash_header_nonce_as_str = sha256(header_nonce)
        key0 = struct.unpack("<Q", hash_header_nonce_as_str[0:8])[0]
        key1 = struct.unpack("<Q", hash_header_nonce_as_str[8:16])[0]
        return [key0, key1]
*/


void block_chain::fill_tx_list_from_mempool(message::compact_block const& block, size_t& mempool_count, std::vector<chain::transaction>& txn_available, std::unordered_map<uint64_t, uint16_t> const& shorttxids) const {
    
    std::vector<bool> have_txn(txn_available.size());
    
    auto header_hash = hash(block);
    auto k0 = from_little_endian_unsafe<uint64_t>(header_hash.begin());
    auto k1 = from_little_endian_unsafe<uint64_t>(header_hash.begin() + sizeof(uint64_t));

        
    //LOG_INFO(LOG_BLOCKCHAIN)
    //<< "fill_tx_list_from_mempool header_hash ->  " << encode_hash(header_hash) 
    //<< " k0 " << k0
    //<< " k1 " << k1;
            

    database_.transactions_unconfirmed().for_each([&](chain::transaction const &tx) {
#ifdef BITPRIM_CURRENCY_BCH
        bool witness = false;
#else
        bool witness = true;
#endif
        uint64_t shortid = sip_hash_uint256(k0, k1, tx.hash(witness)) & uint64_t(0xffffffffffff);
        
      /*   LOG_INFO(LOG_BLOCKCHAIN)
            << "mempool tx ->  " << encode_hash(tx.hash()) 
            << " shortid " << shortid;
      */      
        auto idit = shorttxids.find(shortid);
        if (idit != shorttxids.end()) {
            if (!have_txn[idit->second]) {
                txn_available[idit->second] = tx;
                have_txn[idit->second] = true;
                ++mempool_count;
            } else {
                // If we find two mempool txn that match the short id, just
                // request it. This should be rare enough that the extra
                // bandwidth doesn't matter, but eating a round-trip due to
                // FillBlock failure would be annoying.
                if (txn_available[idit->second].is_valid()) {
                    //txn_available[idit->second].reset();
                    txn_available[idit->second] = chain::transaction{};
                    --mempool_count;
                }
            }
        }
        // Though ideally we'd continue scanning for the
        // two-txn-match-shortid case, the performance win of an early exit
        // here is too good to pass up and worth the extra risk.
        if (mempool_count == shorttxids.size()) {
            return false;
        } else {
            return true;
        }
    });
}

safe_chain::mempool_mini_hash_map block_chain::get_mempool_mini_hash_map(message::compact_block const& block) const {
#ifdef BITPRIM_CURRENCY_BCH
     bool witness = false;
#else
     bool witness = true;
#endif

    if (stopped()) {
        return safe_chain::mempool_mini_hash_map();
    }
    
    auto header_hash = hash(block);

    auto k0 = from_little_endian_unsafe<uint64_t>(header_hash.begin());
    auto k1 = from_little_endian_unsafe<uint64_t>(header_hash.begin() + sizeof(uint64_t));

    safe_chain::mempool_mini_hash_map mempool;
   
    database_.transactions_unconfirmed().for_each([&](chain::transaction const &tx) {
    
        auto sh = sip_hash_uint256(k0, k1, tx.hash(witness));
        
       /* to_little_endian()
        uint64_t pepe = 4564564;
        uint64_t pepe2 = pepe & 0x0000ffffffffffff;
            
        reinterpret_cast<uint8_t*>(pepe2);
        */
        //Drop the most significative bytes from the sh
        mini_hash short_id;
        mempool.emplace(short_id,tx);
        return true;
    });

    return mempool;
}

std::vector<libbitcoin::blockchain::mempool_transaction_summary> block_chain::get_mempool_transactions(std::string const& payment_address, bool use_testnet_rules, bool witness) const{
    std::vector<std::string> addresses = {payment_address};
    return get_mempool_transactions(addresses, use_testnet_rules, witness);
}



void block_chain::fetch_unconfirmed_transaction(hash_digest const& hash, transaction_unconfirmed_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    auto const result = database_.transactions_unconfirmed().get(hash);

    if (!result)
    {
        handler(error::not_found, nullptr);
        return;
    }

    auto const tx = std::make_shared<const transaction>(result.transaction());
    handler(error::success, tx);
}


#endif // BITPRIM_DB_TRANSACTION_UNCONFIRMED

#ifdef BITPRIM_DB_LEGACY
// This is same as fetch_transaction but skips deserializing the tx payload.
void block_chain::fetch_transaction_position(hash_digest const& hash,
    bool require_confirmed, transaction_index_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, 0, 0);
        return;
    }

    auto const result = database_.transactions().get(hash, max_size_t,
        require_confirmed);

    if (!result)
    {
        handler(error::not_found, 0, 0);
        return;
    }

    handler(error::success, result.position(), result.height());
}

// This may execute over 500 queries.
void block_chain::fetch_locator_block_hashes(get_blocks_const_ptr locator,
    hash_digest const& threshold, size_t limit,
    inventory_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    // This is based on the idea that looking up by block hash to get heights
    // will be much faster than hashing each retrieved block to test for stop.

    // Find the start block height.
    // If no start block is on our chain we start with block 0.
    size_t start = 0;
    for (auto const& hash: locator->start_hashes())
    {
        auto const result = database_.blocks().get(hash);
        if (result)
        {
            start = result.height();
            break;
        }
    }

    // The begin block requested is always one after the start block.
    auto begin = safe_add(start, size_t(1));

    // The maximum number of headers returned is 500.
    auto end = safe_add(begin, limit);

    // Find the upper threshold block height (peer-specified).
    if (locator->stop_hash() != null_hash)
    {
        // If the stop block is not on chain we treat it as a null stop.
        auto const result = database_.blocks().get(locator->stop_hash());

        // Otherwise limit the end height to the stop block height.
        // If end precedes begin floor_subtract will handle below.
        if (result)
            end = std::min(result.height(), end);
    }

    // Find the lower threshold block height (self-specified).
    if (threshold != null_hash)
    {
        // If the threshold is not on chain we ignore it.
        auto const result = database_.blocks().get(threshold);

        // Otherwise limit the begin height to the threshold block height.
        // If begin exceeds end floor_subtract will handle below.
        if (result)
            begin = std::max(result.height(), begin);
    }

    auto hashes = std::make_shared<inventory>();
    hashes->inventories().reserve(floor_subtract(end, begin));

    // Build the hash list until we hit end or the blockchain top.
    for (auto height = begin; height < end; ++height)
    {
        auto const result = database_.blocks().get(height);

        // If not found then we are at our top.
        if (!result)
        {
            hashes->inventories().shrink_to_fit();
            break;
        }

        static auto const id = inventory::type_id::block;
        hashes->inventories().emplace_back(id, result.header().hash());
    }

    handler(error::success, std::move(hashes));
}

// This may execute over 2000 queries.
void block_chain::fetch_locator_block_headers(get_headers_const_ptr locator,
    hash_digest const& threshold, size_t limit,
    locator_block_headers_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    // This is based on the idea that looking up by block hash to get heights
    // will be much faster than hashing each retrieved block to test for stop.

    // Find the start block height.
    // If no start block is on our chain we start with block 0.
    size_t start = 0;
    for (auto const& hash: locator->start_hashes())
    {
        auto const result = database_.blocks().get(hash);
        if (result)
        {
            start = result.height();
            break;
        }
    }

    // The begin block requested is always one after the start block.
    auto begin = safe_add(start, size_t(1));

    // The maximum number of headers returned is 2000.
    auto end = safe_add(begin, limit);

    // Find the upper threshold block height (peer-specified).
    if (locator->stop_hash() != null_hash)
    {
        // If the stop block is not on chain we treat it as a null stop.
        auto const result = database_.blocks().get(locator->stop_hash());

        // Otherwise limit the end height to the stop block height.
        // If end precedes begin floor_subtract will handle below.
        if (result)
            end = std::min(result.height(), end);
    }

    // Find the lower threshold block height (self-specified).
    if (threshold != null_hash)
    {
        // If the threshold is not on chain we ignore it.
        auto const result = database_.blocks().get(threshold);

        // Otherwise limit the begin height to the threshold block height.
        // If begin exceeds end floor_subtract will handle below.
        if (result)
            begin = std::max(result.height(), begin);
    }

    auto message = std::make_shared<headers>();
    message->elements().reserve(floor_subtract(end, begin));

    // Build the hash list until we hit end or the blockchain top.
    for (auto height = begin; height < end; ++height)
    {
        auto const result = database_.blocks().get(height);

        // If not found then we are at our top.
        if (!result)
        {
            message->elements().shrink_to_fit();
            break;
        }

        message->elements().push_back(result.header());
    }

    handler(error::success, std::move(message));
}

// This may generally execute 29+ queries.
void block_chain::fetch_block_locator(const block::indexes& heights, block_locator_fetch_handler handler) const {

    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    // Caller can cast get_headers down to get_blocks.
    auto message = std::make_shared<get_headers>();
    auto& hashes = message->start_hashes();
    hashes.reserve(heights.size());

    for (auto const height: heights)
    {
        auto const result = database_.blocks().get(height);

        if (!result)
        {
            handler(error::not_found, nullptr);
            break;
        }

        hashes.push_back(result.header().hash());
    }

    handler(error::success, message);
}
#endif // BITPRIM_DB_LEGACY

#ifdef BITPRIM_DB_NEW


void block_chain::fetch_block_hash_timestamp(size_t height, block_hash_time_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, null_hash, 0, 0);
        return;
    }

    auto const result = database_.internal_db().get_header(height);

    if ( ! result.is_valid() )
    {
        handler(error::not_found, null_hash, 0, 0);
        return;
    }

    handler(error::success, result.hash(), result.timestamp(), height);

}


void block_chain::fetch_block_height(hash_digest const& hash,
    block_height_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, {});
        return;
    }

    auto const result = database_.internal_db().get_header(hash);

    if ( ! result.first.is_valid() )
    {
        handler(error::not_found, 0);
        return;
    }

    handler(error::success, result.second);
}


void block_chain::fetch_block_header(size_t height,
    block_header_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0);
        return;
    }

    auto const result = database_.internal_db().get_header(height);

    if (!result.is_valid())
    {
        handler(error::not_found, nullptr, 0);
        return;
    }

    auto const message = std::make_shared<header>(std::move(result));
    handler(error::success, message, height);
}

void block_chain::fetch_block_header(hash_digest const& hash,
    block_header_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr, 0);
        return;
    }
    
    auto const result = database_.internal_db().get_header(hash);

    if ( ! result.first.is_valid() )
    {
        handler(error::not_found, nullptr, 0);
        return;
    }

    auto const message = std::make_shared<header>(std::move(result.first));
    handler(error::success, message, result.second);
}


void block_chain::fetch_last_height(last_height_fetch_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped, {});
        return;
    }

    uint32_t last_height;

    if ( database_.internal_db().get_last_height(last_height) != database::result_code::success )
    {
        handler(error::not_found, 0);
        return;
    }

    handler(error::success, last_height);
}



// This may execute over 2000 queries.
void block_chain::fetch_locator_block_headers(get_headers_const_ptr locator, hash_digest const& threshold, size_t limit, locator_block_headers_fetch_handler handler) const {
    if (stopped()) {
        handler(error::service_stopped, nullptr);
        return;
    }

    // This is based on the idea that looking up by block hash to get heights
    // will be much faster than hashing each retrieved block to test for stop.

    // Find the start block height.
    // If no start block is on our chain we start with block 0.
    size_t start = 0;
    for (auto const& hash: locator->start_hashes()) {
        auto const result = database_.internal_db().get_header(hash);
        if (result.first.is_valid()) {
            start = result.second; //result.height();
            break;
        }
    }

    // The begin block requested is always one after the start block.
    auto begin = safe_add(start, size_t(1));

    // The maximum number of headers returned is 2000.
    auto end = safe_add(begin, limit);

    // Find the upper threshold block height (peer-specified).
    if (locator->stop_hash() != null_hash) {
        // If the stop block is not on chain we treat it as a null stop.
        auto const result = database_.internal_db().get_header(locator->stop_hash());

        // Otherwise limit the end height to the stop block height.
        // If end precedes begin floor_subtract will handle below.
        if (result.first.is_valid()) {
            // end = std::min(result.height(), end);
            end = std::min(size_t(result.second), end);
        }
    }

    // Find the lower threshold block height (self-specified).
    if (threshold != null_hash) {
        // If the threshold is not on chain we ignore it.
        auto const result = database_.internal_db().get_header(threshold);

        // Otherwise limit the begin height to the threshold block height.
        // If begin exceeds end floor_subtract will handle below.
        if (result.first.is_valid()) {
            // begin = std::max(result.height(), begin);
            begin = std::max(size_t(result.second), begin);
        }
    }

    auto message = std::make_shared<headers>();
    message->elements().reserve(floor_subtract(end, begin));

    // Build the hash list until we hit end or the blockchain top.
    for (auto height = begin; height < end; ++height) {
        auto const result = database_.internal_db().get_header(height);

        // If not found then we are at our top.
        if ( ! result.is_valid()) {
            message->elements().shrink_to_fit();
            break;
        }
        message->elements().push_back(result);
    }
    handler(error::success, std::move(message));
}

// This may generally execute 29+ queries.
void block_chain::fetch_block_locator(block::indexes const& heights, block_locator_fetch_handler handler) const {
    
    if (stopped()) {
        handler(error::service_stopped, nullptr);
        return;
    }

    // Caller can cast get_headers down to get_blocks.
    auto message = std::make_shared<get_headers>();
    auto& hashes = message->start_hashes();
    hashes.reserve(heights.size());

    for (auto const height : heights) {
        auto const result = database_.internal_db().get_header(height);
        if ( ! result.is_valid()) {
            handler(error::not_found, nullptr);
            break;
        }
        hashes.push_back(result.hash());
    }

    handler(error::success, message);
}
#endif // BITPRIM_DB_NEW

// Server Queries.
//-----------------------------------------------------------------------------

#if defined(BITPRIM_DB_SPENDS) || defined(BITPRIM_DB_NEW_FULL)
void block_chain::fetch_spend(const chain::output_point& outpoint, spend_fetch_handler handler) const {
    if (stopped())
    {
        handler(error::service_stopped, {});
        return;
    }

    #if defined(BITPRIM_DB_SPENDS)
        auto point = database_.spends().get(outpoint);
    #else
        auto point = database_.internal_db().get_spend(outpoint);
    #endif

    if (point.hash() == null_hash)
    {
        handler(error::not_found, {});
        return;
    }

    handler(error::success, std::move(point));
}
#endif // BITPRIM_DB_SPENDS


#if defined(BITPRIM_DB_HISTORY) || defined(BITPRIM_DB_NEW_FULL)
void block_chain::fetch_history(short_hash const& address_hash, size_t limit, size_t from_height, history_fetch_handler handler) const {
    if (stopped()) {
        handler(error::service_stopped, {});
        return;
    }

#if defined(BITPRIM_DB_HISTORY)
    handler(error::success, database_.history().get(address_hash, limit, from_height));
#else
    handler(error::success, database_.internal_db().get_history(address_hash, limit, from_height));
#endif

}

void block_chain::fetch_confirmed_transactions(const short_hash& address_hash, size_t limit, size_t from_height, confirmed_transactions_fetch_handler handler) const {
    if (stopped()) {
        handler(error::service_stopped, {});
        return;
    }

#if defined(BITPRIM_DB_HISTORY)
    handler(error::success, database_.history().get_txns(address_hash, limit, from_height));
#else
    handler(error::success, database_.internal_db().get_history_txns(address_hash, limit, from_height));
#endif
}

#endif // BITPRIM_DB_HISTORY

#ifdef BITPRIM_DB_STEALTH
void block_chain::fetch_stealth(const binary& filter, size_t from_height, stealth_fetch_handler handler) const
{
    if (stopped()) {
        handler(error::service_stopped, {});
        return;
    }

    handler(error::success, database_.stealth().scan(filter, from_height));
}
#endif // BITPRIM_DB_STEALTH

// Transaction Pool.
//-----------------------------------------------------------------------------

// Same as fetch_mempool but also optimized for maximum possible block fee as
// limited by total bytes and signature operations.
void block_chain::fetch_template(merkle_block_fetch_handler handler) const
{
    transaction_organizer_.fetch_template(handler);
}

// Fetch a set of currently-valid unconfirmed txs in dependency order.
// All txs satisfy the fee minimum and are valid at the next chain state.
// The set of blocks is limited in count to size. The set may have internal
// dependencies but all inputs must be satisfied at the current height.
void block_chain::fetch_mempool(size_t count_limit, uint64_t minimum_fee,
    inventory_fetch_handler handler) const
{
    transaction_organizer_.fetch_mempool(count_limit, handler);
}

// Filters.
//-----------------------------------------------------------------------------

#ifdef BITPRIM_DB_LEGACY
// This may execute up to 500 queries.
// This filters against the block pool and then the block chain.
void block_chain::filter_blocks(get_data_ptr message, result_handler handler) const {
    
    if (stopped())
    {
        handler(error::service_stopped);
        return;
    }

    // Filter through block pool first.
    block_organizer_.filter(message);
    auto& inventories = message->inventories();
    auto const& blocks = database_.blocks();

    for (auto it = inventories.begin(); it != inventories.end();)
    {
        if (it->is_block_type() && blocks.get(it->hash()))
            it = inventories.erase(it);
        else
            ++it;
    }

    handler(error::success);
}

// This filters against all transactions (confirmed and unconfirmed).
void block_chain::filter_transactions(get_data_ptr message,
    result_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped);
        return;
    }

    auto& inventories = message->inventories();
    auto const& transactions = database_.transactions();

    for (auto it = inventories.begin(); it != inventories.end();)
    {
        //TODO(fernando): check how to replace it with UTXO
        if (it->is_transaction_type() &&
            get_is_unspent_transaction(it->hash(), max_size_t, false))
            it = inventories.erase(it);
        else
            ++it;
    }

    handler(error::success);
}
#endif // BITPRIM_DB_LEGACY

#if defined(BITPRIM_DB_NEW_FULL)

// This filters against all transactions (confirmed and unconfirmed).
void block_chain::filter_transactions(get_data_ptr message, result_handler handler) const
{
    if (stopped())
    {
        handler(error::service_stopped);
        return;
    }

    auto& inventories = message->inventories();
    //auto const& transactions = database_.transactions();

    size_t out_height;
    size_t out_position;

    for (auto it = inventories.begin(); it != inventories.end();)
    {
        //Bitprim: We don't store spent information
        if (it->is_transaction_type() 
            //&& get_is_unspent_transaction(it->hash(), max_size_t, false))
            && get_transaction_position(out_height, out_position, it->hash(), false))
            it = inventories.erase(it);
        else
            ++it;
    }

    handler(error::success);
}

#endif //BITPRIM_DB_NEW_FULL


#ifdef BITPRIM_DB_NEW
// This may execute up to 500 queries.
// This filters against the block pool and then the block chain.
void block_chain::filter_blocks(get_data_ptr message, result_handler handler) const {
    
    if (stopped()) {
        handler(error::service_stopped);
        return;
    }

    // Filter through block pool first.
    block_organizer_.filter(message);
    auto& inventories = message->inventories();
    auto const& internal_db = database_.internal_db();

    for (auto it = inventories.begin(); it != inventories.end();) {
        if (it->is_block_type() && internal_db.get_header(it->hash()).first.is_valid()) {
            it = inventories.erase(it);
        } else {
            ++it;
        }
    }

    handler(error::success);
}
#endif // BITPRIM_DB_NEW


// Subscribers.
//-----------------------------------------------------------------------------

void block_chain::subscribe_blockchain(reorganize_handler&& handler) {
    // Pass this through to the organizer, which issues the notifications.
    block_organizer_.subscribe(std::move(handler));
}

void block_chain::subscribe_transaction(transaction_handler&& handler) {
    // Pass this through to the tx organizer, which issues the notifications.
    transaction_organizer_.subscribe(std::move(handler));
}

void block_chain::unsubscribe() {
    block_organizer_.unsubscribe();
    transaction_organizer_.unsubscribe();
}

// Transaction Validation.
//-----------------------------------------------------------------------------

void block_chain::transaction_validate(transaction_const_ptr tx, result_handler handler) const {
    transaction_organizer_.transaction_validate(tx, handler);
}

// Organizers.
//-----------------------------------------------------------------------------

void block_chain::organize(block_const_ptr block, result_handler handler)
{
    // This cannot call organize or stop (lock safe).
    block_organizer_.organize(block, handler);
}

void block_chain::organize(transaction_const_ptr tx, result_handler handler)
{
    // This cannot call organize or stop (lock safe).
    transaction_organizer_.organize(tx, handler);
}


// Properties (thread safe).
// ----------------------------------------------------------------------------

bool block_chain::is_stale() const {
    // If there is no limit set the chain is never considered stale.
    if (notify_limit_seconds_ == 0) {
        return false;
    }

    auto const top = last_block_.load();

    // TODO(fernando): refactor this!
    // BITPRIM: get the last block if there is no cache
    uint32_t last_timestamp = 0;
    if ( ! top) {
        size_t last_height;
        if (get_last_height(last_height)) {
            chain::header last_header;
            if (get_header(last_header, last_height)) {
                last_timestamp = last_header.timestamp();
            }
        }
    }
    auto const timestamp = top ? top->header().timestamp() : last_timestamp;
    return timestamp < floor_subtract(zulu_time(), notify_limit_seconds_);
}

const settings& block_chain::chain_settings() const
{
    return settings_;
}

// protected
bool block_chain::stopped() const
{
    return stopped_;
}

} // namespace blockchain
} // namespace libbitcoin
