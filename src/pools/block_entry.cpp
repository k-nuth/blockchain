// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kth/blockchain/pools/block_entry.hpp>

#include <algorithm>
#include <iostream>
#include <kth/bitcoin.hpp>
#include <kth/blockchain/define.hpp>

namespace kth {
namespace blockchain {

block_entry::block_entry(block_const_ptr block)
  : hash_(block->hash()), block_(block)
{
}

// Create a search key.
block_entry::block_entry(const hash_digest& hash)
  : hash_(hash)
{
}

block_const_ptr block_entry::block() const
{
    return block_;
}

const hash_digest& block_entry::hash() const
{
    return hash_;
}

// Not callable if the entry is a search key.
const hash_digest& block_entry::parent() const
{
    BITCOIN_ASSERT(block_);
    return block_->header().previous_block_hash();
}

// Not valid if the entry is a search key.
const hash_list& block_entry::children() const
{
    ////BITCOIN_ASSERT(block_);
    return children_;
}

// This is not guarded against redundant entries.
void block_entry::add_child(block_const_ptr child) const
{
    children_.push_back(child->hash());
}

std::ostream& operator<<(std::ostream& out, const block_entry& of)
{
    out << encode_hash(of.hash_)
        << " " << encode_hash(of.parent())
        << " " << of.children_.size();
    return out;
}

// For the purpose of bimap identity only the tx hash matters.
bool block_entry::operator==(const block_entry& other) const
{
    return hash_ == other.hash_;
}

} // namespace blockchain
} // namespace kth
