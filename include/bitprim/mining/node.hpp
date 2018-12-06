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
#ifndef BITPRIM_BLOCKCHAIN_MINING_NODE_HPP_
#define BITPRIM_BLOCKCHAIN_MINING_NODE_HPP_

#include <bitcoin/bitcoin.hpp>

#include <bitprim/mining/common.hpp>
#include <bitprim/mining/transaction_element.hpp>

namespace libbitcoin {
namespace mining {

class node {
public:

    node(transaction_element const& te) 
        : te_(te)
        , children_fees_(te.fee())
        , children_size_(te.size())
        , children_sigops_(te.sigops())
    {}

    node(transaction_element&& te) 
        : te_(std::move(te))
        , children_fees_(te.fee())
        , children_size_(te.size())
        , children_sigops_(te.sigops())
    {}

    hash_digest const& txid() const {
        return te_.txid();
    }

    uint64_t fee() const {
        return te_.fee();
    }

    size_t sigops() const {
        return te_.sigops();
    }

    size_t size() const {
        return te_.size();
    }

    uint64_t children_fees() const {
        return children_fees_;
    }

    size_t children_sigops() const {
        return children_sigops_;
    }

    size_t children_size() const {
        return children_size_;
    }

    index_t candidate_index() const {
        return candidate_index_;
    }


#ifdef BITPRIM_MINING_CTOR_ENABLED
    index_t candidate_ctor_index() const {
        return candidate_ctor_index_;
    }
#endif

    void set_candidate_index(index_t i) {
        candidate_index_ = i;
    }

#ifdef BITPRIM_MINING_CTOR_ENABLED
    void set_candidate_ctor_index(index_t i) {
        candidate_ctor_index_ = i;
    }
#endif

    std::vector<index_t> const& parents() const {
        return parents_;
    }

    std::vector<index_t>& parents() {
        return parents_;
    }    

    std::vector<index_t> const& children() const {
        return children_;
    }

    void add_child(index_t index, uint64_t fee, size_t size, size_t sigops) {
        children_.push_back(index);
        increment_values(fee, size, sigops);
    }

    void add_parent(index_t index) {
        parents_.push_back(index);
    }

    template <typename I>
    void add_parents(I f, I l) {
        parents_.insert(parents_.end(), f, l);
    }

    void increment_values(uint64_t fee, size_t size, size_t sigops) {
        children_fees_ += fee;
        children_size_ += size;
        children_sigops_ += sigops;
    }

    void decrement_values(uint64_t fee, size_t size, size_t sigops) {
        children_fees_ -= fee;
        children_size_ -= size;
        children_sigops_ -= sigops;
    }

private:
    // void update_values(uint64_t fee, size_t size, size_t sigops) {
    //     for (auto const pi : parents_) {
    //         auto& parent = all_transactions_[pi];
    //         parent.update_values(fee, size, sigops);
    //     }

    //     children_fees_ += fee;
    //     children_size_ += size;
    //     children_sigops_ += sigops;
    // }

    // static std::vector<node>& all_transactions_;

    transaction_element te_;
    std::vector<index_t> parents_;
    std::vector<index_t> children_;

    // size_t parent_fees_;
    // size_t parent_size_;    //self plus parents
    // size_t parent_sigops_;  //self plus parents

    size_t children_fees_;
    size_t children_size_;  //self plus children
    size_t children_sigops_;    //self plus children

    index_t candidate_index_;

#ifdef BITPRIM_MINING_CTOR_ENABLED
    index_t candidate_ctor_index_;
#endif

};

}  // namespace mining
}  // namespace libbitcoin

#endif  //BITPRIM_BLOCKCHAIN_MINING_NODE_HPP_
