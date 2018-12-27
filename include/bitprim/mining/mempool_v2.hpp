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
#ifndef BITPRIM_BLOCKCHAIN_MINING_MEMPOOL_V2_HPP_
#define BITPRIM_BLOCKCHAIN_MINING_MEMPOOL_V2_HPP_

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

constexpr
double benefit(uint64_t fee, size_t size) {
    return static_cast<double>(fee) / size;
}

struct fee_per_size_cmp {
    bool operator()(node const& a, node const& b) const {
        auto const value_a = benefit(a.children_fees(), a.children_size());
        auto const value_b = benefit(b.children_fees(), b.children_size());
        return value_b < value_a;
    }
};


#if defined(BITPRIM_CURRENCY_BCH)
struct ctor_cmp {
    bool operator()(node const& a, node const& b) const {
        return std::lexicographical_compare(a.txid().rbegin(), a.txid().rend(),
                                            b.txid().rbegin(), b.txid().rend());
    }
};
#endif


class mempool {
public:
    using internal_utxo_set_t = std::unordered_map<chain::point, chain::output>;
    using previous_outputs_t = std::unordered_map<chain::point, index_t>;
    using hash_index_t = std::unordered_map<hash_digest, std::pair<index_t, chain::transaction>>;
    using triple_t = std::tuple<uint64_t, size_t, size_t>;
    using to_insert_t = std::tuple<indexes_t, uint64_t, size_t, size_t>;

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
        , mempool_total_size_(get_max_block_weight() * mempool_size_multiplier)
    {
        BOOST_ASSERT(max_template_size <= get_max_block_weight()); //TODO(fernando): what happend in BTC with SegWit.

        size_t const candidates_capacity = max_template_size_ / min_transaction_size_for_capacity;
        size_t const all_capacity = mempool_total_size_ / min_transaction_size_for_capacity;

        data_.reserve(all_capacity, candidates_capacity);
    }

    error::error_code_t add(chain::transaction const& tx) {
        //precondition: tx is fully validated: check() && accept() && connect()
        //              ! tx.is_coinbase()

        std::cout << encode_base16(tx.to_data(true, BITPRIM_WITNESS_DEFAULT)) << std::endl;

        return prioritizer_.low_job([this, &tx]{
            auto const index = data_.all_size();

            auto temp_node = make_node(tx);
            auto res = process_utxo_and_graph(tx, index, temp_node);
            if (res != error::success) {
                return res;
            }

            bool inserted = data_.insert(std::move(temp_node));
            if ( ! inserted) {
                return error::low_benefit_transaction;
            }
            return error::success;
        });
    }
 
//     template <typename I>
//     error::error_code_t remove(I f, I l, size_t non_coinbase_input_count = 0) {
//         // precondition: [f, l) is a valid non-empty range
//         //               there are no coinbase transactions in the range

//         if (all_transactions_.empty()) {
//             return error::success;
//         }

//         std::cout << "Arrive Block -------------------------------------------------------------------" << std::endl;
//         // std::cout << encode_base16(tx.to_data(true, BITPRIM_WITNESS_DEFAULT)) << std::endl;


//         processing_block_ = true;
//         auto unique = scope_guard([&](void*){ processing_block_ = false; });

//         std::set<index_t, std::greater<index_t>> to_remove;
//         std::vector<chain::point> outs;
//         if (non_coinbase_input_count > 0) {
//             outs.reserve(non_coinbase_input_count);   //TODO: unnecesary extra space
//         }

//         return prioritizer_.high_job([&f, l, &to_remove, &outs, this]{

//             //TODO: temp code, remove
// // #ifndef NDEBUG
// //             auto old_transactions = all_transactions_;
// // #endif

//             while (f != l) {
//                 auto const& tx = *f;
//                 auto it = hash_index_.find(tx.hash());
//                 if (it != hash_index_.end()) {
//                     auto const index = it->second.first;
//                     auto& node = all_transactions_[index];

//                     //TODO(fernando): check if children() is the complete descendence of node or if it just the inmediate parents.
//                     for (auto ci : node.children()) {
//                         auto& child = all_transactions_[ci];
//                         child.remove_parent(index);
//                     }
//                     to_remove.insert(index);
//                 } else {
//                     for (auto const& i : tx.inputs()) {
//                         outs.push_back(i.previous_output());
//                     }
//                 }
//                 ++f;
//             }

//             find_double_spend_issues(to_remove, outs);


//             //TODO: process batches of adjacent elements
//             for (auto i : to_remove) {
//                 auto it = std::next(all_transactions_.begin(), i);
//                 hash_index_.erase(it->txid());
//                 remove_from_utxo(it->txid(), it->output_count());

//                 if (i < all_transactions_.size() - 1) {
//                     reindex_relatives(i + 1);
//                 }

//                 all_transactions_.erase(it);
//             }

//             BOOST_ASSERT(all_transactions_.size() == hash_index_.size());

// // #ifndef NDEBUG
// //             auto diff = old_transactions.size() - all_transactions_.size();

// //             for (size_t i = 0; i < all_transactions_.size(); ++i) {
// //                 auto& node = all_transactions_[i];
// //                 auto& node_old = old_transactions[i + diff];

// //                 indexes_t old_children;
// //                 for (auto x : node_old.children()) {
// //                     if (x >= diff) {
// //                         old_children.push_back(x - diff);
// //                     }
// //                 }

// //                 assert(node_old.children().size() >= node.children().size());
// //                 assert(old_children.size() == node.children().size());

// //                 for (size_t j = 0; j < old_children.size(); ++j) {
// //                     assert(old_children[j] == node.children()[j]);
// //                 }

// //                 std::cout << std::endl;

// //                 // if (node_old.parents().size() > 0) {
// //                 //     std::cout << std::endl;
// //                 // }

                
// //                 indexes_t old_parents;
// //                 for (auto x : node_old.parents()) {
// //                     if (x >= diff) {
// //                         old_parents.push_back(x - diff);
// //                     }
// //                 }

// //                 assert(node_old.parents().size() >= node.parents().size());
// //                 assert(old_parents.size() == node.parents().size());

// //                 for (size_t j = 0; j < old_parents.size(); ++j) {
// //                     assert(old_parents[j] == node.parents()[j]);
// //                 }

// //             }
// // #endif


//             candidate_transactions_.clear();
//             previous_outputs_.clear();

//             accum_fees_ = 0;
//             accum_size_ = 0;
//             accum_sigops_ = 0;
            
//             for (size_t i = 0; i < all_transactions_.size(); ++i) {
//                 all_transactions_[i].set_candidate_index(null_index);
//                 all_transactions_[i].reset_children_values();
//             }

// #ifndef NDEBUG
//             check_invariant();
// #endif

//             for (size_t i = 0; i < all_transactions_.size(); ++i) {
//                 re_add_node(i);
// #ifndef NDEBUG
//                 check_invariant();
// #endif
//             }
//             return error::success;
//         });
//     }

    size_t capacity() const {
        // shared_lock_t lock(mutex_);
        return prioritizer_.low_job([this]{
            return max_template_size_;
        });
    }

    size_t all_transactions() const {
        // shared_lock_t lock(mutex_);
        return prioritizer_.low_job([this]{
            return data_.all_size();
        });
    }

    size_t candidate_transactions() const {
        // shared_lock_t lock(mutex_);
        return prioritizer_.low_job([this]{
            return data_.candidates_size();
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

//     std::pair<std::vector<transaction_element>, uint64_t> get_block_template() const {
//         //TODO(fernando): implement a cache, outside

//         if (processing_block_) {
//             return {};
//         } 

//         auto copied_data = prioritizer_.high_job([this]{
//             return make_tuple(candidate_transactions_, all_transactions_, accum_fees_);
//         });

//         auto& candidates = std::get<0>(copied_data);
//         auto& all = std::get<1>(copied_data);
//         auto accum_fees = std::get<2>(copied_data);

// #if defined(BITPRIM_CURRENCY_BCH)

//         auto const cmp = [this](index_t a, index_t b) {
//             return ctor_cmp(a, b);
//         };

//         // start = chrono::high_resolution_clock::now();
//         std::sort(std::begin(candidates), std::end(candidates), cmp);
//         // end = chrono::high_resolution_clock::now();
//         // auto sort_time = chrono::duration_cast<chrono::nanoseconds>(end - start).count();
// #else
//         //TODO: Sort LTOR the candidates
// #endif

//         std::vector<transaction_element> res;
//         res.reserve(candidates.size());

//         for (auto i : candidates) {
//             res.push_back(std::move(all[i].element()));
//         }

//         return {std::move(res), accum_fees};
//     }

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


// ----------------------------------------------------------------------------------------
//  Invariant Checks
// ----------------------------------------------------------------------------------------

    template <typename Getter>
    bool check_children_accum(main_index_t node_index, Getter getter) const {

        removal_list_t out_removed;
        auto res = out_removed.insert(node_index);
        if ( ! res.second) {
            return true;
        }
        
        // auto const& node = all_elements_[node_index];
        auto node = getter(node_index);
        auto fee = node.second.fee();
        auto size = node.second.size();
        auto sigops = node.second.sigops();

        // if (node.candidate_index() != null_index) {
        if (node.first) {
            for (auto child_index : node.second.children()) {
                // auto const& child = all_elements_[child_index];
                auto child = getter(child_index);

                //To verify that the node is inside of the candidate list.
                // if (child.candidate_index() != null_index) {
                if (child.first) {
                    auto res = out_removed.insert(child_index);
                    if (res.second) {
                        fee += child.second.fee();
                        size += child.second.size();
                        sigops += child.second.sigops();
                    }
                }
            }
        }

        // if (node.second.children_fees() != fee) {
        //     std::cout << "node_index:           " << node_index << std::endl;
        //     std::cout << "node.second.children_fees(): " << node.second.children_fees() << std::endl;
        //     std::cout << "fee:                  " << fee << std::endl;

        //     std::cout << "Removed:  ";
        //     for (auto i : out_removed) {
        //         std::cout << i << ", ";
        //     }
        //     std::cout << std::endl;
        // }

        // if (node.second.children_size() != size) {
        //     std::cout << "node_index:           " << node_index << std::endl;
        //     std::cout << "node.second.children_size(): " << node.second.children_size() << std::endl;
        //     std::cout << "size:                 " << size << std::endl;

        //     std::cout << "Removed:  ";
        //     for (auto i : out_removed) {
        //         std::cout << i << ", ";
        //     }
        //     std::cout << std::endl;
        // }

        // if (node.second.children_sigops() != sigops) {
        //     std::cout << "node_index:             " << node_index << std::endl;
        //     std::cout << "node.second.children_sigops(): " << node.second.children_sigops() << std::endl;
        //     std::cout << "sigops:                 " << sigops << std::endl;


        //     std::cout << "Removed:  ";
        //     for (auto i : out_removed) {
        //         std::cout << i << ", ";
        //     }
        //     std::cout << std::endl;
        // }

        // BITCOIN_ASSERT(node.second.children_fees() == fee);
        // BITCOIN_ASSERT(node.second.children_size() == size);
        // BITCOIN_ASSERT(node.second.children_sigops() == sigops);

        return node.second.children_fees() == fee
            && node.second.children_size() == size
            && node.second.children_sigops() == sigops;
    }

    template <typename Getter, typename Bounds>
    bool check_node(main_index_t node_index, Getter getter, Bounds bounds) const {
        auto node = getter(node_index);
        {
            for (auto ci : node.second.children()) {
                if ( ! bounds(ci)) {
                    // BOOST_ASSERT(false);
                    return false;
                }
            }
        }

        {
            for (auto pi : node.second.parents()) {
                if ( ! bounds(pi)) {
                    // BOOST_ASSERT(false);
                    return false;
                }

                if (node.first) {
                    auto const& parent = getter(pi);
                    // BOOST_ASSERT(parent.index() != null_index_);
                    // BOOST_ASSERT(parent.first);
                    if ( ! parent.first) {
                        return false;
                    }
                }
            }
        }        

        return check_children_accum(node_index, getter);
    }

// ----------------------------------------------------------------------------------------
//  Invariant Checks (End)
// ----------------------------------------------------------------------------------------

//    void reindex_relatives(size_t index) {

//         for (auto& node : all_transactions_) {

//             for (auto& ci : node.children()) {
//                 if (ci >= index) {
//                     --ci;
//                 }
//             }

//             for (auto& pi : node.parents()) {
//                 if (pi >= index) {
//                     --pi;
//                 }
//             }            
//         }
//     }

    // void re_add_node(index_t index) {
    //     auto const& elem = all_transactions_[index];
    //     // auto const& tx = elem.transaction();
    //     // hash_index_.emplace(tx.txid(), index);

    //     auto it = hash_index_.find(elem.txid());
    //     if (it != hash_index_.end()) {
            
    //         it->second.first = index;
    //         auto const& tx = it->second.second;

    //         for (auto const& i : tx.inputs()) {
    //             // previous_outputs_.left.insert(previous_outputs_t::left_value_type(i.previous_output(), node_index));
    //             previous_outputs_.insert({i.previous_output(), index});
    //         }        
    //         add_node(index);
    //     } else {
    //         //No debería pasar por aca
    //         BOOST_ASSERT(false);
    //     }
    // }


    // void clean_parents(mining::node const& node, index_t index) {
    //     for (auto pi : node.parents()) {
    //         auto& parent = all_transactions_[pi];
    //         parent.remove_child(index);
    //     }
    // }

    // void find_double_spend_issues(std::set<index_t, std::greater<index_t>>& to_remove, std::vector<chain::point> const& outs) {

    //     for (auto const& po : outs) {
    //         // auto it = previous_outputs_.left.find(po);
    //         // if (it != previous_outputs_.left.end()) {
    //         auto it = previous_outputs_.find(po);
    //         if (it != previous_outputs_.end()) {
    //             index_t index = it->second;
    //             auto const& node = all_transactions_[index];

    //             to_remove.insert(index);
    //             clean_parents(node, index);

    //             for (auto ci : node.children()) {
    //                 auto& child = all_transactions_[ci];
    //                 to_remove.insert(ci);
    //                 clean_parents(child, ci);
    //             }
    //         }
    //     }
    // }    

    // void remove_from_utxo(hash_digest const& txid, uint32_t output_count) {
    //     for (uint32_t i = 0; i < output_count; ++i) {
    //         internal_utxo_set_.erase(chain::point{txid, i});
    //     }

    //     //TODO(fernando): Do I have to insert the prevouts removed before??
    // }


// ------------------------------------------------------------------------------------------------
// State
// ------------------------------------------------------------------------------------------------

    bool has_room_for(node const& x) const {
        if (accum_size_ > max_template_size_ - x.size()) {
            return false;
        }
        
        auto const next_size = accum_size_ + x.size();
        auto const sigops_limit = get_allowed_sigops(next_size);

        if (accum_sigops_ > sigops_limit - x.sigops()) {
            return false;
        }

        return true;
    }

    void accumulate(node const& x) {
        accum_fees_ += x.fee();
        accum_size_ += x.size();
        accum_sigops_ += x.sigops();
    }

    //TODO(fernando): replace tuple with a struct with names
    template <typename Getter>
    triple_t get_accum(removal_list_t& out_removed, node const& x, main_index_t node_index, Getter getter) {
        auto res = out_removed.insert(node_index);
        if ( ! res.second) {
            return {0, 0, 0};
        }
        
        auto fee = x.fee();
        auto size = x.size();
        auto sigops = x.sigops();

        for (auto ci : x.children()) {
            // auto const& child = all_transactions_[ci];
            auto child = getter(ci);

            //To verify that the node is inside of the candidate list.
            // if (child.candidate_index() != null_index) {
            if (child.first) {
                auto res = out_removed.insert(ci);
                if (res.second) {
                    fee += child.second.fee();
                    size += child.second.size();
                    sigops += child.second.sigops();
                }
            }
        }

        return {fee, size, sigops};
    }

    template <typename Getter>
    to_insert_t what_to_insert(node const& x, main_index_t node_index, Getter getter) {
        
        auto fees = x.fee();
        auto size = x.size();
        auto sigops = x.sigops();
        indexes_t to_insert {node_index};

        for (auto pi : x.parents()) {
            // auto const& parent = all_transactions_[pi];
            // if (parent.candidate_index() == null_index) {
            auto parent = getter(pi);
            if ( ! parent.first) {
                fees += parent.second.fee();
                size += parent.second.size();
                sigops += parent.second.sigops();
                to_insert.push_back(pi);
            }
        }
        return {std::move(to_insert), fees, size, sigops};
    }

    template <typename ReSortLeft, typename ReSortRight>
    void re_sort_element_removal(mining::node const& node, mining::node& parent, main_index_t parent_index, ReSortLeft re_sort_left, ReSortRight re_sort_right) {
        auto node_benefit = benefit(node.fee(), node.size());
        auto accum_benefit = benefit(parent.children_fees(), parent.children_size());

        parent.decrement_values(node.fee(), node.size(), node.sigops());

        if (node_benefit == accum_benefit) {
            return;
        }

        if (node_benefit > accum_benefit) {
            re_sort_right(parent_index);
        } else {
            re_sort_left(parent_index);
        }
    }

    template <typename Getter, typename ReSortLeft, typename ReSortRight>
    void re_sort_elements_removal(node const& x, Getter getter, ReSortLeft re_sort_left, ReSortRight re_sort_right) {
        for (auto pi : x.parents()) {
            auto parent = getter(pi);
            if (parent.first) {
                re_sort_element_removal(x, parent.second, pi, re_sort_left, re_sort_right);
            } else {
                BOOST_ASSERT(false); // ????
            }
        }
    }

    template <typename ReSortToEnd, typename ReSort, typename ReSortFromBegin>
    void re_sort_element_insertion(mining::node const& node, mining::node& parent, main_index_t node_index, main_index_t parent_index, ReSortToEnd re_sort_to_end, ReSort re_sort, ReSortFromBegin re_sort_from_begin) {

        auto node_benefit = benefit(node.fee(), node.size());                          //a
        auto accum_benefit = benefit(parent.children_fees(), parent.children_size());  //b
        auto node_accum_benefit = benefit(node.children_fees(), node.children_size()); //c
        auto old_accum_benefit = benefit(parent.children_fees() - node.fee(), parent.children_size() - node.size());  //d?

        if (old_accum_benefit == accum_benefit) {
            return;
        }
    
        if (old_accum_benefit > accum_benefit) {
//  P               P'

            if (old_accum_benefit < node_accum_benefit) {
// C    P               P'
                re_sort_to_end(parent_index, parent_index);
            } else {
                if (accum_benefit < node_accum_benefit) {
//  P        C     P'      
                    re_sort_to_end(node_index, parent_index);
                } else {
//  P              P'      C
                    re_sort(parent_index, node_index, parent_index);
                }
            }
        }  else {
//  P'              P

            if (accum_benefit < node_accum_benefit) {

// C    P'              P
                re_sort(node_index, parent_index, parent_index);
            } else {
                if (old_accum_benefit < node_accum_benefit) {
//  P'        C       P
                    re_sort_from_begin(node_index, parent_index);
                } else {
//  P'        P         C

                    re_sort_from_begin(parent_index, parent_index);
                }
            }
        }
    }

    template <typename Getter, typename ReSortToEnd, typename ReSort, typename ReSortFromBegin>
    void re_sort_elements_insertion(mining::node const& node, main_index_t node_index, indexes_t to_insert, Getter getter, ReSortToEnd re_sort_to_end, ReSort re_sort, ReSortFromBegin re_sort_from_begin) {
        //precondition: candidate_transactions_.size() > 0

        for (auto pi : node.parents()) {
            auto parent = getter(pi);
            if (parent.first) {
                parent.second.increment_values(node.fee(), node.size(), node.sigops());
                re_sort_element_insertion(node, parent.second, node_index, pi, re_sort_to_end, re_sort, re_sort_from_begin);

            } else {
                auto it = std::find(std::begin(to_insert), std::end(to_insert), pi);
                if (it != std::end(to_insert)) {
                    parent.second.increment_values(node.fee(), node.size(), node.sigops());
                }
            }
        }
    }    

    template <typename Remover, typename Getter, typename ReSortLeft, typename ReSortRight>
    void do_remove(main_index_t index, Remover remover, Getter getter, ReSortLeft re_sort_left, ReSortRight re_sort_right) {
        remover(index);

        auto node = getter(index);

        accum_size_ -= node.second.size();
        accum_sigops_ -= node.second.sigops();
        accum_fees_ -= node.second.fee();

        re_sort_elements_removal(node.second, getter, re_sort_left, re_sort_right);
    }

    template <typename Inserter, typename Getter, typename ReSortToEnd, typename ReSort, typename ReSortFromBegin>
    void do_insert(main_index_t index, indexes_t to_insert, Inserter inserter, Getter getter, ReSortToEnd re_sort_to_end, ReSort re_sort, ReSortFromBegin re_sort_from_begin) {
        inserter(index);
        auto node = getter(index);
        re_sort_elements_insertion(node.second, index, to_insert, getter, re_sort_to_end, re_sort, re_sort_from_begin);
    }

    bool shares_parents(mining::node const& to_insert_node, main_index_t remove_candidate_index) const {
        auto const& parents = to_insert_node.parents();
        auto it = std::find(parents.begin(), parents.end(), remove_candidate_index);
        return it != parents.end();
    }

    // removal_list_t what_to_remove(uint64_t fees, size_t size, size_t sigops) {
    template <typename Reverser, typename Remover, typename Getter, typename Inserter, typename ReSortLeft, typename ReSortRight, typename ReSortToEnd, typename ReSort, typename ReSortFromBegin>
    bool remove_insert_several(node const& x, main_index_t node_index, Reverser reverser, Remover remover, Getter getter, Inserter inserter, ReSortLeft re_sort_left, ReSortRight re_sort_right, ReSortToEnd re_sort_to_end, ReSort re_sort, ReSortFromBegin re_sort_from_begin) {   
        //precondition: candidate_transactions_.size() > 0

        auto [to_insert, fees, size, sigops] = what_to_insert(x, node_index, getter);
        auto pack_benefit = benefit(fees, size);

        uint64_t fee_accum = 0;
        size_t size_accum = 0;
        auto next_size = accum_size_;
        auto next_sigops = accum_sigops_;

        removal_list_t removed;

        do {
            auto elem = reverser.next();

            //TODO(fernando): Do I have to check if elem_idex is any of the to_insert elements
            bool shares = shares_parents(elem.second, elem.first);

            if ( ! shares) {
                auto res = get_accum(removed, elem.second, elem.first, getter);
                if (std::get<1>(res) != 0) {
                    fee_accum += std::get<0>(res);
                    size_accum += std::get<1>(res);

                    auto to_remove_benefit = benefit(fee_accum, size_accum);
                    if (pack_benefit <= to_remove_benefit) {
                        return false;
                    }

                    next_size -= std::get<1>(res);
                    next_sigops -= std::get<2>(res);

                    if (next_size + size <= max_template_size_) {
                        auto const sigops_limit = get_allowed_sigops(next_size);
                        if (next_sigops + sigops <= sigops_limit) {
                            break;
                        }
                    }
                }
            }
        } while(reverser.has_next());

        for(auto i : removed) {
            do_remove(i, remover, getter, re_sort_left, re_sort_right);
        }

        for (auto index : to_insert) {
            auto const& parent = getter(index);
            if ( ! parent.first) {
                do_insert(index, to_insert, inserter, getter, re_sort_to_end, re_sort, re_sort_from_begin);
                //accum
            } else {
                BOOST_ASSERT(false);
            }
        }

        accum_fees_ += fees;
        accum_size_ += size;
        accum_sigops_ += sigops;

        return true;
    }

// ------------------------------------------------------------------------------------------------
// State (End)
// ------------------------------------------------------------------------------------------------


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

    void insert_outputs_in_utxo(chain::transaction const& tx) {
        //precondition: there are no duplicates outputs between tx.outputs() and internal_utxo_set_
        uint32_t index = 0;
        for (auto const& o : tx.outputs()) {
            internal_utxo_set_.emplace(chain::point{tx.hash(), index}, o);
            ++index;
        }
    }


    size_t const max_template_size_;
    size_t const mempool_total_size_;
    size_t accum_size_ = 0;
    size_t accum_sigops_ = 0;
    uint64_t accum_fees_ = 0;
   
    //TODO: chequear el anidamiento de TX con su máximo (25??) y si es regla de consenso.

    // has_room_for roomer(120);
    // partially_indexed<node, fee_per_size_cmp, has_room_for> data{fee_per_size_cmp{}, roomer};

    partially_indexed<node, fee_per_size_cmp, has_room_for> data_;

    internal_utxo_set_t internal_utxo_set_;
    hash_index_t hash_index_;

    // std::unordered_map<chain::point, index_t> previous_outputs_;
    previous_outputs_t previous_outputs_;

    // mutable mutex_t mutex_;
    prioritizer prioritizer_;

    std::atomic<bool> processing_block_{false};
};

}  // namespace mining
}  // namespace libbitcoin

#endif  //BITPRIM_BLOCKCHAIN_MINING_MEMPOOL_V2_HPP_

