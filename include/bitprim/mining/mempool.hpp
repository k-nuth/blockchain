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
#ifndef BITPRIM_BLOCKCHAIN_MINING_MEMPOOL_HPP_
#define BITPRIM_BLOCKCHAIN_MINING_MEMPOOL_HPP_

#include <algorithm>
#include <chrono>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <vector>

// #include <boost/bimap.hpp>

#include <bitprim/mining/common.hpp>
#include <bitprim/mining/node.hpp>
#include <bitprim/mining/prioritizer.hpp>
// #include <bitprim/mining/result_code.hpp>

#include <bitcoin/bitcoin.hpp>


template <typename F> 
auto scope_guard(F&& f) {
    return std::unique_ptr<void, typename std::decay<F>::type>{(void*)1, std::forward<F>(f)};
}

namespace libbitcoin {
namespace mining {

// inline
// node make_node(chain::transaction const& tx) {
//     return node(transaction_element(tx));
// }

inline
node make_node(chain::transaction const& tx) {
    return node(
                transaction_element(tx.hash()
#if ! defined(BITPRIM_CURRENCY_BCH)
                                  , tx.hash(true)
#endif
                                  , tx.to_data(true, BITPRIM_WITNESS_DEFAULT)
                                  , tx.fees() 
                                  , tx.signature_operations()
                                  , tx.outputs().size())
                        );
}

#ifdef BITPRIM_MINING_STATISTICS_ENABLED
template <typename F>
void measure(F f, measurements_t& t) {
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();
    t += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    // auto time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    // t += double(time_ns);
}
#else
template <typename F>
void measure(F f, measurements_t&) {
    f();
}
#endif


template <typename F, typename Container, typename Cmp = std::greater<typename Container::value_type>>
std::set<typename Container::value_type, Cmp> to_ordered_set(F f, Container const& to_remove) {
    std::set<typename Container::value_type, Cmp> ordered;
    for (auto const& i : to_remove) {
        ordered.insert(f(i));
    }
    return ordered;
}



class mempool {
public:

    using to_insert_t = std::tuple<indexes_t, uint64_t, size_t, size_t>;
    using accum_t = std::tuple<uint64_t, size_t, size_t>;
    using all_transactions_t = std::vector<node>;
    using internal_utxo_set_t = std::unordered_map<chain::point, chain::output>;
    
    // using previous_outputs_t = boost::bimap<chain::point, index_t>;
    using previous_outputs_t = std::unordered_map<chain::point, index_t>;

    // using hash_index_t = std::unordered_map<hash_digest, index_t>;
    using hash_index_t = std::unordered_map<hash_digest, std::pair<index_t, chain::transaction>>;

    // using mutex_t = boost::shared_mutex;
    // using shared_lock_t = boost::shared_lock<mutex_t>;
    // using unique_lock_t = boost::unique_lock<mutex_t>;
    // using upgrade_lock_t = boost::upgrade_lock<mutex_t>;

    static constexpr size_t max_template_size_default = get_max_block_weight() - coinbase_reserved_size;

#if defined(BITPRIM_CURRENCY_BCH)
    static constexpr size_t mempool_size_multiplier_default = 10;
#elif defined(BITPRIM_CURRENCY_BTC)
    static constexpr size_t mempool_size_multiplier_default = 10;
#else
    static constexpr size_t mempool_size_multiplier_default = 10;
#endif 

    mempool(size_t max_template_size = max_template_size_default, size_t mempool_size_multiplier = mempool_size_multiplier_default) 
        : max_template_size_(max_template_size)
        // , mempool_size_multiplier_(mempool_size_multiplier)
        , mempool_total_size_(get_max_block_weight() * mempool_size_multiplier)
    {
        BOOST_ASSERT(max_template_size <= get_max_block_weight()); //TODO(fernando): what happend in BTC with SegWit.

        size_t const candidates_capacity = max_template_size_ / min_transaction_size_for_capacity;
        size_t const all_capacity = mempool_total_size_ / min_transaction_size_for_capacity;

        candidate_transactions_.reserve(candidates_capacity);
#ifdef BITPRIM_MINING_CTOR_ENABLED    
        candidate_transactions_ctor_.reserve(candidates_capacity);
#endif
        all_transactions_.reserve(all_capacity);
    }



    error::error_code_t add(chain::transaction const& tx) {
        //precondition: tx is fully validated: check() && accept() && connect()
        //              ! tx.is_coinbase()

        std::cout << encode_base16(tx.to_data(true, BITPRIM_WITNESS_DEFAULT)) << std::endl;

        return prioritizer_.low_job([this, &tx]{
            auto const index = all_transactions_.size();

            auto temp_node = make_node(tx);
            auto res = process_utxo_and_graph(tx, index, temp_node);
            if (res != error::success) {
                return res;
            }

            all_transactions_.push_back(std::move(temp_node));
            return add_node(index);
        });
    }

    void reindex_xxx(size_t index) {    //TODO: rename


        size_t i = index;
        while (i < all_transactions_.size()) {
            auto& node = all_transactions_[i];

            for (auto& ci : node.children()) {
                --ci;
            }

            for (auto& pi : node.parents()) {
                if (pi >= index) {
                    --pi;
                }
            }

            ++i;
        }
    }

    template <typename I>
    error::error_code_t remove(I f, I l, size_t non_coinbase_input_count = 0) {
        // precondition: [f, l) is a valid non-empty range
        //               there are no coinbase transactions in the range

        if (all_transactions_.empty()) {
            return error::success;
        }

        // std::cout << "Arrive Block -------------------------------------------------------------------" << std::endl;
        // std::cout << encode_base16(tx.to_data(true, BITPRIM_WITNESS_DEFAULT)) << std::endl;


        processing_block_ = true;
        auto unique = scope_guard([&](void*){ processing_block_ = false; });

        std::set<index_t, std::greater<index_t>> to_remove;
        std::vector<chain::point> outs;
        if (non_coinbase_input_count > 0) {
            outs.reserve(non_coinbase_input_count);   //TODO: unnecesary extra space
        }

        return prioritizer_.high_job([&f, l, &to_remove, &outs, this]{

            //TODO: temp code, remove
#ifndef NDEBUG
            auto old_transactions = all_transactions_;
#endif

            while (f != l) {
                auto const& tx = *f;
                auto it = hash_index_.find(tx.hash());
                if (it != hash_index_.end()) {
                    auto const index = it->second.first;
                    auto& node = all_transactions_[index];

                    //TODO(fernando): check if children() is the complete descendence of node or if it just the inmediate parents.
                    for (auto ci : node.children()) {
                        auto& child = all_transactions_[ci];
                        child.remove_parent(index);
                    }
                    to_remove.insert(index);
                } else {
                    for (auto const& i : tx.inputs()) {
                        outs.push_back(i.previous_output());
                    }
                }
                ++f;
            }

            find_double_spend_issues(to_remove, outs);


            //TODO: process batches of adjacent elements
            for (auto i : to_remove) {
                auto it = std::next(all_transactions_.begin(), i);
                hash_index_.erase(it->txid());
                remove_from_utxo(it->txid(), it->output_count());

                if (i < all_transactions_.size() - 1) {
                    reindex_xxx(i + 1);
                }

                all_transactions_.erase(it);
            }

            BOOST_ASSERT(all_transactions_.size() == hash_index_.size());

#ifndef NDEBUG
            auto diff = old_transactions.size() - all_transactions_.size();

            for (size_t i = 0; i < all_transactions_.size(); ++i) {
                auto& node = all_transactions_[i];
                auto& node_old = old_transactions[i + diff];

                indexes_t old_children;
                for (auto x : node_old.children()) {
                    if (x >= diff) {
                        old_children.push_back(x - diff);
                    }
                }

                assert(node_old.children().size() >= node.children().size());
                assert(old_children.size() == node.children().size());

                for (size_t j = 0; j < old_children.size(); ++j) {
                    assert(old_children[j] == node.children()[j]);
                }

                std::cout << std::endl;

                // if (node_old.parents().size() > 0) {
                //     std::cout << std::endl;
                // }

                
                indexes_t old_parents;
                for (auto x : node_old.parents()) {
                    if (x >= diff) {
                        old_parents.push_back(x - diff);
                    }
                }

                assert(node_old.parents().size() >= node.parents().size());
                assert(old_parents.size() == node.parents().size());

                for (size_t j = 0; j < old_parents.size(); ++j) {
                    assert(old_parents[j] == node.parents()[j]);
                }

            }
#endif


            candidate_transactions_.clear();
            previous_outputs_.clear();

            accum_fees_ = 0;
            accum_size_ = 0;
            accum_sigops_ = 0;
            
            for (size_t i = 0; i < all_transactions_.size(); ++i) {
                re_add_node(i);
            }
            return error::success;
        });
    }

    size_t capacity() const {
        // shared_lock_t lock(mutex_);
        return prioritizer_.low_job([this]{
            return max_template_size_;
        });
    }

    size_t all_transactions() const {
        // shared_lock_t lock(mutex_);
        return prioritizer_.low_job([this]{
            return all_transactions_.size();
        });
    }

    size_t candidate_transactions() const {
        // shared_lock_t lock(mutex_);
        return prioritizer_.low_job([this]{
            return candidate_transactions_.size();
        });
    }

    size_t candidate_bytes() const {
        // shared_lock_t lock(mutex_);
        return prioritizer_.low_job([this]{
            return accum_size_;
        });
    }

    size_t candidate_sigops() const {
        // shared_lock_t lock(mutex_);
        return prioritizer_.low_job([this]{
            return accum_sigops_;
        });
    }

    uint64_t candidate_fees() const {
        // shared_lock_t lock(mutex_);
        return prioritizer_.low_job([this]{
            return accum_fees_;
        });
    }

    //TODO:
    bool contains(hash_digest const& txid) const {
        // shared_lock_t lock(mutex_);
        return prioritizer_.low_job([&txid, this]{
            auto it = hash_index_.find(txid);
            return it != hash_index_.end();
        });
    }

    hash_index_t get_validated_txs_high() const {
        return prioritizer_.high_job([this]{
            return hash_index_;
        });
    }

    hash_index_t get_validated_txs_low() const {
        return prioritizer_.low_job([this]{
            return hash_index_;
        });
    }

    bool is_candidate(chain::transaction const& tx) const {
        // shared_lock_t lock(mutex_);
        return prioritizer_.low_job([&tx, this]{
            auto it = hash_index_.find(tx.hash());
            if (it == hash_index_.end()) {
                return false;
            }

            return all_transactions_[it->second.first].candidate_index() != null_index;
        });
    }

    index_t candidate_rank(chain::transaction const& tx) const {
        // shared_lock_t lock(mutex_);
        return prioritizer_.low_job([&tx, this]{
            auto it = hash_index_.find(tx.hash());
            if (it == hash_index_.end()) {
                return null_index;
            }

            return all_transactions_[it->second.first].candidate_index();
        });
    }

    std::pair<std::vector<transaction_element>, uint64_t> get_block_template() const {
        //TODO(fernando): implement a cache, outside

        if (processing_block_) {
            return {};
        } 

        auto copied_data = prioritizer_.high_job([this]{
            return make_tuple(candidate_transactions_, all_transactions_, accum_fees_);
        });

        auto& candidates = std::get<0>(copied_data);
        auto& all = std::get<1>(copied_data);
        auto accum_fees = std::get<2>(copied_data);

#if defined(BITPRIM_CURRENCY_BCH)

        auto const cmp = [this](index_t a, index_t b) {
            return ctor_cmp(a, b);
        };

        // start = chrono::high_resolution_clock::now();
        std::sort(std::begin(candidates), std::end(candidates), cmp);
        // end = chrono::high_resolution_clock::now();
        // auto sort_time = chrono::duration_cast<chrono::nanoseconds>(end - start).count();
#else
        //TODO: Sort LTOR the candidates
#endif

        std::vector<transaction_element> res;
        res.reserve(candidates.size());

        for (auto i : candidates) {
            res.push_back(std::move(all[i].element()));
        }

        return {std::move(res), accum_fees};
    }

    chain::output get_utxo(chain::point const& point) const {
        // shared_lock_t lock(mutex_);
        return prioritizer_.low_job([&point, this]{
            auto it = internal_utxo_set_.find(point);
            if (it != internal_utxo_set_.end()) {
                return it->second;
            } 

            return chain::output{};
        });
    }

private:

    void re_add_node(index_t index) {
        auto const& elem = all_transactions_[index];
        // auto const& tx = elem.transaction();
        // hash_index_.emplace(tx.txid(), index);

        auto it = hash_index_.find(elem.txid());
        if (it != hash_index_.end()) {
            
            it->second.first = index;
            auto const& tx = it->second.second;

            for (auto const& i : tx.inputs()) {
                // previous_outputs_.left.insert(previous_outputs_t::left_value_type(i.previous_output(), node_index));
                previous_outputs_.insert({i.previous_output(), index});
            }        
            add_node(index);
        } else {
            //No debería pasar por aca
            BOOST_ASSERT(false);
        }
    }

    error::error_code_t add_node(index_t index) {
        //TODO: what_to_insert_time
        auto to_insert = what_to_insert(index);

        if (candidate_transactions_.size() > 0 && ! has_room_for(std::get<2>(to_insert), std::get<3>(to_insert))) {
            //TODO: what_to_remove_time
            auto to_remove = what_to_remove(std::get<1>(to_insert), std::get<2>(to_insert), std::get<3>(to_insert));

            if (to_remove.empty()) {
                // ++low_benefit_tx_counter;
                return error::low_benefit_transaction;
            } 
            do_candidate_removal(to_remove);
        }
       
        do_candidates_insertion(to_insert);

        // check_indexes();
        return error::success;
    }    
    
    void clean_parents(mining::node const& node, index_t index) {
        for (auto pi : node.parents()) {
            auto& parent = all_transactions_[pi];
            parent.remove_child(index);
        }
    }

    void find_double_spend_issues(std::set<index_t, std::greater<index_t>>& to_remove, std::vector<chain::point> const& outs) {

        for (auto const& po : outs) {
            // auto it = previous_outputs_.left.find(po);
            // if (it != previous_outputs_.left.end()) {
            auto it = previous_outputs_.find(po);
            if (it != previous_outputs_.end()) {
                index_t index = it->second;
                auto const& node = all_transactions_[index];

                to_remove.insert(index);
                clean_parents(node, index);

                for (auto ci : node.children()) {
                    auto& child = all_transactions_[ci];
                    to_remove.insert(ci);
                    clean_parents(child, ci);
                }
            }
        }
    }    

    void remove_from_utxo(hash_digest const& txid, uint32_t output_count) {
        for (uint32_t i = 0; i < output_count; ++i) {
            internal_utxo_set_.erase(chain::point{txid, i});
        }

        //TODO(fernando): Do I have to insert the prevouts removed before??
    }

    bool fee_per_size_cmp(index_t a, index_t b) const {
        auto const& node_a = all_transactions_[a];
        auto const& node_b = all_transactions_[b];

        auto const value_a = static_cast<double>(node_a.children_fees()) / node_a.children_size();
        auto const value_b = static_cast<double>(node_b.children_fees()) / node_b.children_size();

        return value_b < value_a;
    }

#if defined(BITPRIM_CURRENCY_BCH)
    bool ctor_cmp(index_t a, index_t b) const {
        auto const& node_a = all_transactions_[a];
        auto const& node_b = all_transactions_[b];
        return std::lexicographical_compare(node_a.txid().rbegin(), node_a.txid().rend(),
                                            node_b.txid().rbegin(), node_b.txid().rend());
    }
#endif


    //TODO(fernando): replace tuple with a struct with names
    accum_t get_accum(removal_list_t& out_removed, index_t node_index, indexes_t const& children) {
        auto res = out_removed.insert(node_index);
        if ( ! res.second) {
            return {0, 0, 0};
        }
        
        auto const& node = all_transactions_[node_index];
        auto fee = node.fee();
        auto size = node.size();
        auto sigops = node.sigops();

        for (auto child_index : children) {
            auto const& child = all_transactions_[child_index];

            //To verify that the node is inside of the candidate list.
            if (child.candidate_index() != null_index) {
                auto res = out_removed.insert(child_index);
                if (res.second) {
                    fee += child.fee();
                    size += child.size();
                    sigops += child.sigops();
                }
            }
        }

        return {fee, size, sigops};
    }

    to_insert_t what_to_insert(index_t node_index) {
        indexes_t to_insert;

        auto const& node = all_transactions_[node_index];
        auto fees = node.fee();
        auto size = node.size();
        auto sigops = node.sigops();

        to_insert.push_back(node_index);

        for (auto pi : node.parents()) {
            auto const& parent = all_transactions_[pi];
            if (parent.candidate_index() == null_index) {
                fees += parent.fee();
                size += parent.size();
                sigops += parent.sigops();
                to_insert.push_back(pi);
            }
        }

        return {std::move(to_insert), fees, size, sigops};
    }

    removal_list_t what_to_remove(uint64_t fees, size_t size, size_t sigops) {
        //precondition: candidate_transactions_.size() > 0

        // auto const& node = all_transactions_[node_index];
        // auto node_benefit = static_cast<double>(node.fee()) / node.size();

        auto pack_benefit = static_cast<double>(fees) / size;

        auto it = candidate_transactions_.end() - 1;

        uint64_t fee_accum = 0;
        size_t size_accum = 0;
        
        auto next_size = accum_size_;
        auto next_sigops = accum_sigops_;

        removal_list_t removed;

        //TODO(fernando): check for end of range
        while (true) {
            auto elem_index = *it;
            auto const& elem = all_transactions_[elem_index];
            
            auto res = get_accum(removed, elem_index, elem.children());
            if (std::get<1>(res) != 0) {
                fee_accum += std::get<0>(res);
                size_accum += std::get<1>(res);

                auto to_remove_benefit = static_cast<double>(fee_accum) / size_accum;
                //El beneficio del elemento que voy a insertar es "peor" que el peor que el del peor elemento que tengo como candidato. Entonces, no sigo.
                if (pack_benefit <= to_remove_benefit) {
                    //El beneficio acumulado de los elementos a remover es "mejor" que el que tengo para insertar.
                    //No tengo que hacer nada.
                    // return candidate_transactions_.end();
                    return {};
                }

                next_size -= std::get<1>(res);
                next_sigops -= std::get<2>(res);

                if (next_size + size <= max_template_size_) {
                    auto const sigops_limit = get_allowed_sigops(next_size);
                    if (next_sigops + sigops <= sigops_limit) {
                        // return it;
                        return removed;
                    }
                }
            }

            if (it == candidate_transactions_.begin()) break;
            --it;
        }

        // return it;
        return removed;
    }

    void do_candidate_removal(removal_list_t const& to_remove) {
        // removed_tx_counter += to_remove.size();

        //TODO: remove_time
        // remove_nodes_v1(to_remove);
        remove_nodes(to_remove);
              

#ifdef BITPRIM_MINING_CTOR_ENABLED
        //TODO: remove_time_ctor
        remove_nodes_ctor(to_remove);
#endif
        //TODO: reindex_parents_quitar_time
        reindex_parents_for_removal(to_remove);
    }

    void do_candidates_insertion(to_insert_t const& to_insert) {

        for (auto i : std::get<0>(to_insert)) {
            insert_in_candidate(i);

#ifdef BITPRIM_MINING_CTOR_ENABLED
            insert_in_candidate_ctor(i);
#endif
        }

        accum_fees_ += std::get<1>(to_insert);
        accum_size_ += std::get<2>(to_insert);
        accum_sigops_ += std::get<3>(to_insert);
    }

    bool has_room_for(size_t size, size_t sigops) const {
        if (accum_size_ > max_template_size_ - size) {
            return false;
        }
        
        auto const next_size = accum_size_ + size;
        auto const sigops_limit = get_allowed_sigops(next_size);

        if (accum_sigops_ > sigops_limit - sigops) {
            return false;
        }

        return true;
    }

    error::error_code_t process_utxo_and_graph(chain::transaction const& tx, index_t node_index, node& new_node) {
        //TODO: evitar tratar de borrar en el UTXO Local, si el UTXO fue encontrado en la DB

        auto it = hash_index_.find(tx.hash());
        if (it != hash_index_.end()) {
            return error::duplicate_transaction;
        }

        auto res = check_double_spend(tx);
        if (res != error::success) {
            return res;
        }

        //--------------------------------------------------
        // Mutate the state

        insert_outputs_in_utxo(tx);
        hash_index_.emplace(tx.hash(), std::make_pair(node_index, tx));

        indexes_t parents;

        for (auto const& i : tx.inputs()) {
            if (i.previous_output().validation.from_mempool) {
                // Spend the UTXO
                internal_utxo_set_.erase(i.previous_output());

                auto it = hash_index_.find(i.previous_output().hash());
                index_t parent_index = it->second.first;
                parents.push_back(parent_index);
            }

            // previous_outputs_.left.insert(previous_outputs_t::left_value_type(i.previous_output(), node_index));
            previous_outputs_.insert({i.previous_output(), node_index});
        }

        if ( ! parents.empty()) {
            std::set<index_t, std::greater<index_t>> parents_temp(parents.begin(), parents.end());
            for (auto pi : parents) {
                auto const& parent = all_transactions_[pi];
                parents_temp.insert(parent.parents().begin(), parent.parents().end());
            }

            new_node.add_parents(parents_temp.begin(), parents_temp.end());

            for (auto pi : new_node.parents()) {
                auto& parent = all_transactions_[pi];
                // parent.add_child(node_index, new_node.fee(), new_node.size(), new_node.sigops());
                parent.add_child(node_index);
            }
        }

        return error::success;
    }

    error::error_code_t check_double_spend(chain::transaction const& tx) {
        for (auto const& i : tx.inputs()) {
            if (i.previous_output().validation.from_mempool) {
                auto it = internal_utxo_set_.find(i.previous_output());
                if (it == internal_utxo_set_.end()) {
                    return error::double_spend_mempool;
                }
            } else {
                // auto it = previous_outputs_.left.find(i.previous_output());
                // if (it != previous_outputs_.left.end()) {
                //     return error::double_spend_blockchain;
                // }
                auto it = previous_outputs_.find(i.previous_output());
                if (it != previous_outputs_.end()) {
                    return error::double_spend_blockchain;
                }
            }
        }
        return error::success;
    }

    // bool check_no_duplicated_outputs(chain::transaction const& tx) {
    //     uint32_t index = 0;
    //     for (auto const& o : tx.outputs()) {
    //         auto it = internal_utxo_set_.find(chain::point{tx.hash(), index});
    //         if (it != internal_utxo_set_.end()) {
    //             return false;
    //         }
    //         ++index;
    //     }
    //     return true;
    // }

    void insert_outputs_in_utxo(chain::transaction const& tx) {
        //precondition: there are no duplicates outputs between tx.outputs() and internal_utxo_set_
        uint32_t index = 0;
        for (auto const& o : tx.outputs()) {
            internal_utxo_set_.emplace(chain::point{tx.hash(), index}, o);
            ++index;
        }
    }

    template <typename I>
    void reindex_decrement(I f, I l) {
        //precondition: f != l
        std::for_each(f, l, [this](size_t i) {
            auto& n = all_transactions_[i];
            n.set_candidate_index(n.candidate_index() - 1);
        });
    }

    template <typename I>
    void reindex_increment(I f, I l) {
        //precondition: f != l
        std::for_each(f, l, [this](size_t i) {
            auto& n = all_transactions_[i];
            n.set_candidate_index(n.candidate_index() + 1);
        });
    }

#ifdef BITPRIM_MINING_CTOR_ENABLED
    template <typename I>
    void reindex_decrement_ctor(I f, I l) {
        //precondition: f != l
        std::for_each(f, l, [this](size_t i) {
            auto& n = all_transactions_[i];
            n.set_candidate_ctor_index(n.candidate_ctor_index() - 1);
        });
    }

    template <typename I>
    void reindex_increment_ctor(I f, I l) {
        //precondition: f != l
        std::for_each(f, l, [this](size_t i) {
            auto& n = all_transactions_[i];
            n.set_candidate_ctor_index(n.candidate_ctor_index() + 1);
        });
    }
#endif    

    void remove_and_reindex(index_t i) {
        //precondition: TODO?
        auto it = std::next(std::begin(candidate_transactions_), i);
        auto& node = all_transactions_[*it];
        node.set_candidate_index(null_index);

        accum_size_ -= node.size();
        accum_sigops_ -= node.sigops();
        accum_fees_ -= node.fee();

        reindex_decrement(std::next(it), std::end(candidate_transactions_));
        candidate_transactions_.erase(it);
    }

    template <typename RO, typename RU, typename F>
    void remove_nodes_gen(removal_list_t const& to_remove, RO remove_ordered_fun, RU remove_unordered_fun, F f) {
        constexpr auto thresold = 1; //??
        if (to_remove.size() <= thresold) {
            remove_unordered_fun(to_remove);
        } else {
            auto ordered = to_ordered_set(f, to_remove);
            remove_ordered_fun(ordered);
        }
    }

    index_t get_candidate_index(index_t index) const {
        auto const& node = all_transactions_[index];
        return node.candidate_index();
    }

#ifdef BITPRIM_MINING_CTOR_ENABLED
    index_t get_candidate_ctor_index(index_t index) const {
        auto const& node = all_transactions_[index];
        return node.candidate_ctor_index();
    }
#endif

    void remove_nodes(removal_list_t const& to_remove) {
        auto const remove_ordered = [this](auto s){
            std::for_each(std::begin(s), std::end(s), [this](index_t i) {
                remove_and_reindex(i);
            });
        };

        auto const remove_unordered = [this](auto s){
            std::for_each(std::begin(s), std::end(s), [this](index_t i){
                remove_and_reindex(get_candidate_index(i));
            });
        };

        remove_nodes_gen(to_remove, remove_ordered, remove_unordered, [this](index_t i){
            return get_candidate_index(i);
        });
    }


#ifdef BITPRIM_MINING_CTOR_ENABLED
    void remove_and_reindex_ctor(index_t i) {
        //precondition: TODO?
        auto it = std::next(std::begin(candidate_transactions_ctor_), i);
        auto& node = all_transactions_[*it];
        node.set_candidate_ctor_index(null_index);
        reindex_decrement_ctor(std::next(it), std::end(candidate_transactions_ctor_));
        candidate_transactions_ctor_.erase(it);
    }

    void remove_nodes_ctor(removal_list_t const& to_remove) {

        auto const remove_ordered = [this](auto s){
            std::for_each(std::begin(s), std::end(s), [this](index_t i) {
                remove_and_reindex_ctor(i);
            });
        };

        auto const remove_unordered = [this](auto s){
            std::for_each(std::begin(s), std::end(s), [this](index_t i){
                remove_and_reindex_ctor(get_candidate_ctor_index(i));
            });
        };

        remove_nodes_gen(to_remove, remove_ordered, remove_unordered, [this](index_t i){
            return get_candidate_ctor_index(i);
        });
    }
#endif
        
    void reindex_parent_for_removal(mining::node const& node, mining::node& parent, index_t parent_index) {
        // cout << "reindex_parent_quitar\n";
        auto node_benefit = static_cast<double>(node.fee()) / node.size();
        auto accum_benefit = static_cast<double>(parent.children_fees()) / parent.children_size();

        // reduce_values(node, parent);
        parent.decrement_values(node.fee(), node.size(), node.sigops());

        auto accum_benefit_new = static_cast<double>(parent.children_fees()) / parent.children_size();

        if (node_benefit == accum_benefit) {
            return;
        }

        auto it = std::next(std::begin(candidate_transactions_), parent.candidate_index());
        // auto child_it = std::next(std::begin(candidate_transactions_), node.candidate_index());

        auto const cmp = [this](index_t a, index_t b) {
            return fee_per_size_cmp(a, b);
        };

        if (node_benefit > accum_benefit) {
            assert(accum_benefit_new < accum_benefit);

            // El hijo mejoraba al padre, por lo tanto, quitar al hijo significa empeorar al padre
            // EMPEORA EL PADRE, POR LO TANTO TENGO QUE MOVERLO A LA DERECHA

            auto from = it + 1;
            auto to = std::end(candidate_transactions_);


            auto it2 = std::upper_bound(from, to, parent_index, cmp);
            reindex_decrement(from, it2);
            it = std::rotate(it, it + 1, it2);
            
            parent.set_candidate_index(std::distance(std::begin(candidate_transactions_), it));

        } else {
            assert(accum_benefit_new > accum_benefit);

            // El hijo empeoraba al padre, por lo tanto, quitar al hijo significa mejorar al padre
            // MEJORA EL PADRE, POR LO TANTO TENGO QUE MOVERLO A LA IZQUIERDA

            auto from = std::begin(candidate_transactions_);
            auto to = it;

            auto it2 = std::upper_bound(from, to, parent_index, cmp);
            reindex_increment(it2, it);
            std::rotate(it2, it, it + 1);
            parent.set_candidate_index(std::distance(std::begin(candidate_transactions_), it2));
        }
    }

    void reindex_parents_for_removal(removal_list_t const& removed_elements) {
        for (auto i : removed_elements) {
            auto const& node = all_transactions_[i];
            for (auto pi : node.parents()) {
                auto& parent = all_transactions_[pi];
                if (parent.candidate_index() != null_index) {
                    reindex_parent_for_removal(node, parent, pi);
                }
            }
        }
    }

    void reindex_parent_from_insertion(mining::node const& node, mining::node& parent, index_t parent_index) {
        auto node_benefit = static_cast<double>(node.fee()) / node.size();
        auto accum_benefit = static_cast<double>(parent.children_fees()) / parent.children_size();

        if (node_benefit == accum_benefit) {
            return;
        }

        auto it = std::next(std::begin(candidate_transactions_), parent.candidate_index());
        auto child_it = std::next(std::begin(candidate_transactions_), node.candidate_index());

        auto const cmp = [this](index_t a, index_t b) {
            return fee_per_size_cmp(a, b);
        };

        if (node_benefit < accum_benefit) {

            // assert(accum_benefit_new < accum_benefit);

            // EMPEORA EL PADRE, POR LO TANTO TENGO QUE MOVERLO A LA DERECHA
            // (FALSO) Sé que el hijo recién insertado está a la derecha del padre y va a permanecer a su derecha.


            if (parent.candidate_index() < node.candidate_index()) {
                auto from = it + 1;
                auto to = child_it;       // to = std::end(candidate_transactions_);

                auto it2 = std::upper_bound(from, to, parent_index, cmp);
                reindex_decrement(from, it2);
                it = std::rotate(it, it + 1, it2);
                parent.set_candidate_index(std::distance(std::begin(candidate_transactions_), it));
            } else {
                auto from = it + 1;
                auto to = std::end(candidate_transactions_);

                auto it2 = std::upper_bound(from, to, parent_index, cmp);
                reindex_decrement(from, it2);
                it = std::rotate(it, it + 1, it2);
                parent.set_candidate_index(std::distance(std::begin(candidate_transactions_), it));
            }


        } else {
            // assert(accum_benefit_new > accum_benefit);

            // MEJORA EL PADRE, POR LO TANTO TENGO QUE MOVERLO A LA IZQUIERDA
            // (FALSO) Sé que el hijo recién insertado está a la izquierda del padre y va a permanecer a su izquierda.

            if (node.candidate_index() < parent.candidate_index()) {
                auto from = child_it + 1;
                auto to = it;    // to = std::end(candidate_transactions_);

                auto it2 = std::upper_bound(from, to, parent_index, cmp);
                reindex_increment(it2, it);
                std::rotate(it2, it, it + 1);
                parent.set_candidate_index(std::distance(std::begin(candidate_transactions_), it2));
            } else {
                auto from = std::begin(candidate_transactions_);
                auto to = it;

                auto it2 = std::upper_bound(from, to, parent_index, cmp);
                reindex_increment(it2, it);
                std::rotate(it2, it, it + 1);
                parent.set_candidate_index(std::distance(std::begin(candidate_transactions_), it2));
            }
        


        }
    }

    void reindex_parents_from_insertion(mining::node const& node) {
        //precondition: candidate_transactions_.size() > 0

        for (auto pi : node.parents()) {
            auto& parent = all_transactions_[pi];
            parent.increment_values(node.fee(), node.size(), node.sigops());
            
            if (parent.candidate_index() != null_index) {
                reindex_parent_from_insertion(node, parent, pi);
            }
        }
    }

    void insert_in_candidate(index_t node_index) {
        auto& node = all_transactions_[node_index];

        // auto start = std::chrono::high_resolution_clock::now();

        auto const cmp = [this](index_t a, index_t b) {
            return fee_per_size_cmp(a, b);
        };

        auto it = std::upper_bound(std::begin(candidate_transactions_), std::end(candidate_transactions_), node_index, cmp);

        // auto end = std::chrono::high_resolution_clock::now();
        // auto time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        // binary_search_time += time_ns;


        if (it == std::end(candidate_transactions_)) {
            node.set_candidate_index(candidate_transactions_.size());
        } else {
            node.set_candidate_index(distance(std::begin(candidate_transactions_), it));

            // start = std::chrono::high_resolution_clock::now();
            reindex_increment(it, std::end(candidate_transactions_));
            // end = std::chrono::high_resolution_clock::now();
            // time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            // insert_reindex_time += time_ns;
        }

        // start = std::chrono::high_resolution_clock::now();
        candidate_transactions_.insert(it, node_index);
        // end = std::chrono::high_resolution_clock::now();
        // time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        // insert_time += time_ns;

        reindex_parents_from_insertion(node);
    }

#ifdef BITPRIM_MINING_CTOR_ENABLED
    void insert_in_candidate_ctor(index_t node_index) {
        auto& node = all_transactions_[node_index];

        // auto start = std::chrono::high_resolution_clock::now();

        auto const cmp = [this](index_t a, index_t b) {
            return ctor_cmp(a, b);
        };

        auto it = std::upper_bound(std::begin(candidate_transactions_ctor_), std::end(candidate_transactions_ctor_), node_index, cmp);
        // auto end = std::chrono::high_resolution_clock::now();
        // auto time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        // binary_search_time_ctor += time_ns;


        if (it == std::end(candidate_transactions_ctor_)) {
            node.set_candidate_ctor_index(candidate_transactions_ctor_.size());
        } else {
            node.set_candidate_ctor_index(distance(std::begin(candidate_transactions_ctor_), it));

            // start = std::chrono::high_resolution_clock::now();
            reindex_increment_ctor(it, std::end(candidate_transactions_ctor_));
            // end = std::chrono::high_resolution_clock::now();
            // time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            // insert_reindex_time_ctor += time_ns;
        }

        // start = std::chrono::high_resolution_clock::now();
        candidate_transactions_ctor_.insert(it, node_index);
        // end = std::chrono::high_resolution_clock::now();
        // time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        // insert_time_ctor += time_ns;
    }
#endif


    size_t const max_template_size_;
    size_t const mempool_total_size_;
    size_t accum_size_ = 0;
    size_t accum_sigops_ = 0;
    uint64_t accum_fees_ = 0;
    

    //TODO: race conditions, LOCK!
    //TODO: chequear el anidamiento de TX con su máximo (25??) y si es regla de consenso.

    internal_utxo_set_t internal_utxo_set_;
    all_transactions_t all_transactions_;
    hash_index_t hash_index_;
    indexes_t candidate_transactions_;        //Por Ponderacion
#if defined(BITPRIM_CURRENCY_BCH)    
    indexes_t candidate_transactions_ctor_;   //Por CTOR, solamente para BCH...
#endif

    // std::unordered_map<chain::point, index_t> previous_outputs_;
    previous_outputs_t previous_outputs_;

    // mutable mutex_t mutex_;
    prioritizer prioritizer_;

    std::atomic<bool> processing_block_{false};
};

}  // namespace mining
}  // namespace libbitcoin

#endif  //BITPRIM_BLOCKCHAIN_MINING_MEMPOOL_HPP_


//TODO: check if these examples are OK

//insert: fee: 6, size: 10, benf: 0.5
// fee    |010|009|008|007|005|
// size   |010|010|010|010|010|
// fee/s  |1.0|0.9|0.8|0.7|0.5|
//                          ^

//insert: fee: 11, size: 20, benf: 0.55
// fee    |010|009|008|007|005|
// size   |010|010|010|010|010|
// fee/s  |1.0|0.9|0.8|0.7|0.5|
// ------------------------------

//insert: fee: 13, size: 20, benf: 0.65
// fee    |010|009|008|007|005|
// size   |010|010|010|010|010|
// fee/s  |1.0|0.9|0.8|0.7|0.5|
//                      ^



// limits: size: 50, 2 sigops per 10
//insert: fee: 6, size: 12, benf: 0.65, sigops = 5
// fee    |010|009|008|007|005|
// size   |010|010|009|010|010| = 49
// fee/s  |1.0|0.9|0.8|0.7|0.5|
// sigops |002|002|002|002|002|
//                      ^

