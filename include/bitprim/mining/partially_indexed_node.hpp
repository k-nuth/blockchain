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
#ifndef BITPRIM_BLOCKCHAIN_MINING_PARTIALLY_INDEXED_NODE_HPP_
#define BITPRIM_BLOCKCHAIN_MINING_PARTIALLY_INDEXED_NODE_HPP_

#include <list>
#include <vector>

#include <bitprim/mining/common.hpp>

#include <bitcoin/bitcoin.hpp>

namespace libbitcoin {
namespace mining {

template <typename I, typename T>
    // requires(Regular<T>)
class partially_indexed_node {
public:
    using value_type = T;

    partially_indexed_node(I index, T const& x) 
        : index_(index)
        , x_(x)
    {}

    partially_indexed_node(I index, T&& x) 
        : index_(index)
        , x_(std::move(x))
    {}

    // template <typename... Args>
    // partially_indexed_node(std::piecewise_construct_t, I index, Args&&... args)
    //     : index_(index)
    //     , x_(std::forward<Args>(args)...)
    // {}

    template <typename... Args>
    partially_indexed_node(I index, Args&&... args)
        : index_(index)
        , x_(std::forward<Args>(args)...)
    {}

    I const& index() const {
        return index_;
    }

    T& element() & {
        return x_;
    }

    T&& element() && {
        return std::move(x_);
    }

    T const& element() const& {
        return x_;
    }

    void set_index(I index) {
        index_ = index;
    }

    void set_element(T const& x) {
        x_ = x;
    }

    void set_element(T&& x) {
        x_ = std::move(x);
    }

private:
    I index_;
    T x_;
};

}  // namespace mining
}  // namespace libbitcoin

#endif  //BITPRIM_BLOCKCHAIN_MINING_PARTIALLY_INDEXED_NODE_HPP_
