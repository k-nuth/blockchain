// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef KTH_BLOCKCHAIN_BLOCK_ENTRY_HPP
#define KTH_BLOCKCHAIN_BLOCK_ENTRY_HPP

#include <iostream>
////#include <memory>
#include <boost/functional/hash_fwd.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/blockchain/define.hpp>

namespace kth {
namespace blockchain {

/// This class is not thread safe.
class BCB_API block_entry
{
public:
    ////typedef std::shared_ptr<transaction_entry> ptr;
    ////typedef std::vector<ptr> list;

    /// Construct an entry for the pool.
    /// Never store an invalid block in the pool.
    block_entry(block_const_ptr block);

    /// Use this construction only as a search key.
    block_entry(const hash_digest& hash);

    /// The block that the entry contains.
    block_const_ptr block() const;

    /// The hash table entry identity.
    const hash_digest& hash() const;

    /// The hash table entry's parent (preceding block) hash.
    const hash_digest& parent() const;

    /// The hash table entry's child (succeeding block) hashes.
    const hash_list& children() const;

    /// Add block to the list of children of this block.
    void add_child(block_const_ptr child) const;

    /// Serializer for debugging (temporary).
    friend std::ostream& operator<<(std::ostream& out, const block_entry& of);

    /// Operators.
    bool operator==(const block_entry& other) const;

private:
    // These are non-const to allow for default copy construction.
    hash_digest hash_;
    block_const_ptr block_;

    // TODO: could save some bytes here by holding the pointer in place of the
    // hash. This would allow navigation to the hash saving 24 bytes per child.
    // Children do not pertain to entry hash, so must be mutable.
    mutable hash_list children_;
};

} // namespace blockchain
} // namespace kth

// Standard (boost) hash.
//-----------------------------------------------------------------------------

namespace boost
{

// Extend boost namespace with our block_const_ptr hash function.
template <>
struct hash<bc::blockchain::block_entry>
{
    size_t operator()(const bc::blockchain::block_entry& entry) const
    {
        return boost::hash<bc::hash_digest>()(entry.hash());
    }
};

} // namespace boost

#endif
