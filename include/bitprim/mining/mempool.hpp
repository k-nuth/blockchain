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
#include <vector>

#include <bitprim/mining/common.hpp>
#include <bitprim/mining/node.hpp>

namespace libbitcoin {
namespace mining {

node make_node(chain::transaction const& tx) {
    return node(
                transaction_element(tx.hash()
#if ! defined(BITPRIM_CURRENCY_BCH)
                                  , tx.hash(true)
#endif
                                  , tx.to_data(BITPRIM_WITNESS_DEFAULT) , tx.fees() , tx.signature_operations())
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





// template <typename I, typename F>
// void remove_indexes(I f, I l, F remove) {
//     while (f != l) {
//         auto const i = *f;
//         remove(i);
//         ++f;
//     }
// }


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
    // using excedent_t = std::pair<size_t, size_t>;

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

    // new_block() {}


    bool add(chain::transaction const& tx) {
        //precondition: tx is fully validated: check() && accept() && connect()

        auto new_node = make_node(tx);
        auto const node_index = all_transactions_.size();

        process_utxo_and_graph(tx, node_index, new_node);
        
        all_transactions_.push_back(std::move(new_node));
        hash_index_.emplace(tx.hash(), node_index);

        if (candidate_transactions_.size() > 0 && ! has_room_for(new_node)) {

            //TODO: what_to_remove_time
            auto to_remove = what_to_remove(node_index);

            if (to_remove.empty()) {
                // ++low_benefit_tx_counter;
                return false;
            } 
            do_candidate_removal(to_remove);
        }
        do_candidate_insertion(node_index, new_node.size(), new_node.sigops());
        // check_indexes();
        return true;
    }

    // gbt() const;

    // output get_utxo(point) const {
    //     auto it = internal_utxo_set_.find(point);
    //     return it->second;
    // }

private:

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
    std::tuple<uint64_t, size_t, size_t> get_accum(removal_list_t& out_removed, index_t node_index, indexes_t const& children) {
        auto res = out_removed.insert(node_index);
        if ( ! res.second) {
            return {0, 0, 0};
        }
        
        auto const& node = all_transactions_[node_index];
        auto fee = node.fee();
        auto size = node.size();
        auto sigops = node.sigops();

        for (auto child_index : children) {
            auto res = out_removed.insert(child_index);
            if (res.second) {
                auto const& child = all_transactions_[child_index];

                //To verify that the node is inside of the candidate list.
                if (child.candidate_index() != null_index) {
                    fee += child.fee();
                    size += child.size();
                    sigops += child.sigops();
                }
            }
        }

        return {fee, size, sigops};
    }

    removal_list_t what_to_remove(index_t node_index) {
        //precondition: candidate_transactions_.size() > 0

        auto const& node = all_transactions_[node_index];
        auto node_benefit = static_cast<double>(node.fee()) / node.size();

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
                if (node_benefit <= to_remove_benefit) {
                    //El beneficio acumulado de los elementos a remover es "mejor" que el que tengo para insertar.
                    //No tengo que hacer nada.
                    // return candidate_transactions_.end();
                    return {};
                }

                next_size -= std::get<1>(res);
                next_sigops -= std::get<2>(res);

                if (next_size + node.size() <= max_template_size_) {
                    auto const sigops_limit = get_allowed_sigops(next_size);
                    if (next_sigops + node.sigops() <= sigops_limit) {
                        // return it;
                        return removed;
                    }
                }
            }

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

    void do_candidate_insertion(index_t node_index, size_t size, size_t sigops) {
        insert_in_candidate(node_index);

#ifdef BITPRIM_MINING_CTOR_ENABLED
        insert_in_candidate_ctor(node_index);
#endif

        accum_size_ += size;
        accum_sigops_ += sigops;
    }

    bool has_room_for(node const& node) const {
        if (accum_size_ > max_template_size_ - node.size()) {
            return false;
        }
        
        auto const next_size = accum_size_ + node.size();
        auto const sigops_limit = get_allowed_sigops(next_size);

        if (accum_sigops_ > sigops_limit - node.sigops()) {
            return false;
        }

        return true;
    }

    void process_utxo_and_graph(chain::transaction const& tx, index_t new_node_index, node& new_node) {
        //TODO: evitar tratar de borrar en el UTXO Local, si el UTXO fue encontrado en la DB

        indexes_t parents;

        for (auto const& i : tx.inputs()) {

            if (i.previous_output().validation.from_mempool) {
                //TODO: the search in the hash table was done twice. (one in get_utxo, the other here (erase))
                
                // Spend the UTXO
                internal_utxo_set_.erase(i.previous_output());

                auto it = hash_index_.find(i.previous_output().hash());
                index_t parent_index = it->second;

                // auto const& parent = all_transactions_[parent_index];
                
                // parent.add_child(self, new_node.fee(), new_node.size(), new_node.sigops());
                // new_node.add_parent(parent);
                parents.push_back(parent_index);
            }
        }

        if ( ! parents.empty()) {
            std::set<index_t> parents_temp(parents.begin(), parents.end());
            for (auto pi : parents) {
                auto const& parent = all_transactions_[pi];
                parents_temp.insert(parent.parents().begin(), parent.parents().end());
            }

            new_node.add_parents(parents_temp.begin(), parents_temp.end());

            for (auto pi : new_node.parents()) {
                auto& parent = all_transactions_[pi];
                parent.add_child(new_node_index, new_node.fee(), new_node.size(), new_node.sigops());
            }
        }

        insert_outputs_in_utxo(tx);
    }

    void insert_outputs_in_utxo(chain::transaction const& tx) {
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

        reindex_decrement(std::next(it), std::end(candidate_transactions_));
        candidate_transactions_.erase(it);
    }


    // template <typename I>
    // void remove_ordered(I f, I l) {
    //     std::for_each(f, l, [this](index_t i){
    //         remove_and_reindex(i);
    //     });
    // }

    // template <typename I>
    // void remove_unordered(I f, I l) {
    //     std::for_each(f, l, [this](index_t i){
    //         auto const& node = all_transactions_[i];
    //         remove_and_reindex(node.candidate_index());
    //     });
    // }

    
    // void remove_ordered(std::setI f, I l) {
    // }

    // template <typename I>
    // void remove_unordered(I f, I l) {
    //     std::for_each(f, l, [this](index_t i){
    //         auto const& node = all_transactions_[i];
    //         remove_and_reindex(node.candidate_index());
    //     });
    // }


    template <typename RO, typename RU, typename F>
    void remove_nodes_gen(removal_list_t const& to_remove, RO remove_ordered_fun, RU remove_unordered_fun, F f) {
        constexpr auto thresold = 1; //??
        if (to_remove.size() <= thresold) {
            // remove_unordered_fun(std::begin(to_remove), std::end(to_remove));
            remove_unordered_fun(to_remove);
        } else {
            auto ordered = to_ordered_set(f, to_remove);
            // remove_ordered_fun(std::begin(ordered), std::end(ordered));
            remove_ordered_fun(ordered);
        }
    }

    index_t get_candidate_index(index_t index) const {
        auto const& node = all_transactions_[index];
        return node.candidate_index();
    }

    index_t get_candidate_ctor_index(index_t index) const {
        auto const& node = all_transactions_[index];
        return node.candidate_ctor_index();
    }

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


    // void remove_nodes(removal_list_t const& to_remove) {
    //     constexpr auto thresold = 1; //??
    //     if (to_remove.size() <= thresold) {
    //         remove_unordered(std::begin(to_remove), std::end(to_remove));
    //     } else {
    //         auto ordered = to_ordered_set([](index_t i){
    //             auto const& node = all_transactions_[i];
    //             return node.candidate_index();
    //         }, to_remove);

    //         remove_ordered(std::begin(ordered), std::end(ordered));
    //     }
    // }


    // void remove_nodes_ctor(removal_list_t const& to_remove) {
    //     constexpr auto thresold = 1; //??
    //     if (to_remove.size() <= thresold) {
    //         remove_unordered_ctor(std::begin(to_remove), std::end(to_remove));
    //     } else {
    //         auto ordered = to_ordered_set([](index_t i){
    //             auto const& node = all_transactions_[i];
    //             return node.candidate_ctor_index();
    //         }, to_remove);

    //         remove_ordered_ctor(std::begin(ordered), std::end(ordered));
    //     }
    // }


#ifdef BITPRIM_MINING_CTOR_ENABLED
    void remove_and_reindex_ctor(index_t i) {
        //precondition: TODO?
        auto it = std::next(std::begin(candidate_transactions_ctor_), i);
        auto& node = all_transactions_[*it];
        node.set_candidate_ctor_index(null_index);
        reindex_decrement_ctor(std::next(it), std::end(candidate_transactions_ctor_));
        candidate_transactions_ctor_.erase(it);
    }


    // template <typename I>
    // void remove_ordered_ctor(I f, I l) {
    //     while (f != l) {
    //         auto const i = *f;
    //         remove_and_reindex_ctor(i);
    //         ++f;
    //     }
    // }

    // template <typename I>
    // void remove_unordered_ctor(I f, I l) {
    //     while (f != l) {
    //         auto const i = *f;
    //         auto const& node = all_transactions_[i];
    //         remove_and_reindex_ctor(node.candidate_ctor_index());
    //         ++f;
    //     }
    // }

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



    void accum_values(mining::node const& node, mining::node& parent) {
        // parent.children_fees_ += node.self_fee_;
        // parent.children_size_ += node.self_size_;
        // parent.children_sigops_ += node.self_sigops_;
        parent.increment_values(node.fee(), node.size(), node.sigops());

    }

    void reduce_values(mining::node const& node, mining::node& parent) {
        parent.decrement_values(node.fee(), node.size(), node.sigops());
    }

        
    void reindex_parent_for_removal(mining::node const& node, mining::node& parent, index_t parent_index) {
        // cout << "reindex_parent_quitar\n";
        auto node_benefit = static_cast<double>(node.fee()) / node.size();
        auto accum_benefit = static_cast<double>(parent.children_fees()) / parent.children_size();

        reduce_values(node, parent);

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
                } else {
                    //Could not happend! (??)
                    assert(false);
                }
            }
        }
    }


    void reindex_parent_from_insertion(mining::node const& node, mining::node& parent, index_t parent_index) {
        auto node_benefit = static_cast<double>(node.fee()) / node.size();
        auto accum_benefit = static_cast<double>(parent.children_fees()) / parent.children_size();

        accum_values(node, parent);

        auto accum_benefit_new = static_cast<double>(parent.children_fees()) / parent.children_size();

        if (node_benefit == accum_benefit) {
            return;
        }

        auto it = std::next(std::begin(candidate_transactions_), parent.candidate_index());
        auto child_it = std::next(std::begin(candidate_transactions_), node.candidate_index());

        auto const cmp = [this](index_t a, index_t b) {
            return fee_per_size_cmp(a, b);
        };

        if (node_benefit < accum_benefit) {

            assert(accum_benefit_new < accum_benefit);

            // EMPEORA EL PADRE, POR LO TANTO TENGO QUE MOVERLO A LA DERECHA
            // Sé que el hijo recién insertado está a la derecha del padre y va a permanecer a su derecha.
            auto from = it + 1;
            auto to = child_it;       // to = std::end(candidate_transactions_);

            auto it2 = std::upper_bound(from, to, parent_index, cmp);
            reindex_decrement(from, it2);
            it = std::rotate(it, it + 1, it2);
            
            parent.set_candidate_index(std::distance(std::begin(candidate_transactions_), it));

        } else {
            assert(accum_benefit_new > accum_benefit);

            // MEJORA EL PADRE, POR LO TANTO TENGO QUE MOVERLO A LA IZQUIERDA
            // Sé que el hijo recién insertado está a la izquierda del padre y va a permanecer a su izquierda.

            auto from = child_it + 1;
            auto to = it;    // to = std::end(candidate_transactions_);

            auto it2 = std::upper_bound(from, to, parent_index, cmp);
            reindex_increment(it2, it);
            std::rotate(it2, it, it + 1);
            parent.set_candidate_index(std::distance(std::begin(candidate_transactions_), it2));
        }
    }

    void reindex_parents_from_insertion(mining::node const& node) {
        //precondition: candidate_transactions_.size() > 0

        for (auto pi : node.parents()) {
            auto& parent = all_transactions_[pi];
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
    // size_t const mempool_size_multiplier_;
    size_t const mempool_total_size_;
    size_t accum_size_ = 0;
    size_t accum_sigops_ = 0;

    //TODO: race conditions, LOCK!
    //TODO: chequear el anidamiento de TX con su máximo (25??) y si es regla de consenso.

    std::unordered_map<chain::point, chain::output> internal_utxo_set_;
    std::vector<node> all_transactions_;
    std::unordered_map<hash_digest, index_t> hash_index_;
    indexes_t candidate_transactions_;        //Por Ponderacion
#if defined(BITPRIM_CURRENCY_BCH)    
    indexes_t candidate_transactions_ctor_;   //Por CTOR, solamente para BCH...
#endif
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

