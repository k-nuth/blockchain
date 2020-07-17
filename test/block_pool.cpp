// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test_helpers.hpp>

#include <utility>
#include <kth/blockchain.hpp>

using namespace kth;
using namespace kth::blockchain;

// Start Boost Suite: block pool tests

// Access to protected members.
class block_pool_fixture : public block_pool {
public:
    block_pool_fixture(size_t maximum_depth)
        : block_pool(maximum_depth)
    {}

    void prune(size_t top_height)
    {
        block_pool::prune(top_height);
    }

    bool exists(block_const_ptr candidate_block) const
    {
        return block_pool::exists(candidate_block);
    }

    block_const_ptr parent(block_const_ptr block) const
    {
        return block_pool::parent(block);
    }

    size_t maximum_depth() const
    {
        return maximum_depth_;
    }

    block_entries& blocks()
    {
        return blocks_;
    }
};

block_const_ptr make_block(uint32_t id, size_t height,
    hash_digest const& parent)
{
    auto const block = std::make_shared<const domain::message::block>(domain::message::block
    {
        domain::chain::header{ id, parent, null_hash, 0, 0, 0 }, {}
    });

    block->header().validation.height = height;
    return block;
}

block_const_ptr make_block(uint32_t id, size_t height, block_const_ptr parent) {
    return make_block(id, height, parent->hash());
}

block_const_ptr make_block(uint32_t id, size_t height) {
    return make_block(id, height, null_hash);
}

// construct

TEST_CASE("block pool  construct  zero depth  sets  maximum value", "[block pool tests]") {
    block_pool_fixture instance(0);
    REQUIRE(instance.maximum_depth() == max_size_t);
}

TEST_CASE("block pool  construct  nonzero depth  round trips", "[block pool tests]") {
    static const size_t expected = 42;
    block_pool_fixture instance(expected);
    REQUIRE(instance.maximum_depth() == expected);
}

// add1

TEST_CASE("block pool  add1  one  single", "[block pool tests]") {
    block_pool_fixture instance(0);
    static const size_t height = 42;
    auto const block1 = make_block(1, height);

    instance.add(block1);
    instance.add(block1);
    REQUIRE(instance.size() == 1u);

    auto const entry = instance.blocks().right.find(height);
    REQUIRE(entry != instance.blocks().right.end());
    REQUIRE(entry->second.block() == block1);
    REQUIRE(entry->first == height);
}

TEST_CASE("block pool  add1  twice  single", "[block pool tests]") {
    block_pool instance(0);
    auto const block = std::make_shared<const domain::message::block>();

    instance.add(block);
    instance.add(block);
    REQUIRE(instance.size() == 1u);
}

TEST_CASE("block pool  add1  two different blocks with same hash  first retained", "[block pool tests]") {
    block_pool_fixture instance(0);
    static const size_t height1a = 42;
    auto const block1a = make_block(1, height1a);
    auto const block1b = make_block(1, height1a + 1u);

    // The blocks have the same hash value, so second will not be added.
    REQUIRE(block1a->hash() == block1b->hash());

    instance.add(block1a);
    instance.add(block1b);
    REQUIRE(instance.size() == 1u);

    auto const entry = instance.blocks().right.find(height1a);
    REQUIRE(entry != instance.blocks().right.end());
    REQUIRE(entry->second.block() == block1a);
}

TEST_CASE("block pool  add1  two distinct hash  two", "[block pool tests]") {
    block_pool_fixture instance(0);
    static const size_t height1 = 42;
    static const size_t height2 = height1 + 1u;
    auto const block1 = make_block(1, height1);
    auto const block2 = make_block(2, height2);

    // The blocks do not have the same hash value, so both will be added.
    REQUIRE(block1->hash() != block2->hash());

    instance.add(block1);
    instance.add(block2);
    REQUIRE(instance.size() == 2u);

    auto const& entry1 = instance.blocks().right.find(height1);
    REQUIRE(entry1 != instance.blocks().right.end());
    REQUIRE(entry1->second.block() == block1);

    auto const& entry2 = instance.blocks().right.find(height2);
    REQUIRE(entry2 != instance.blocks().right.end());
    REQUIRE(entry2->second.block() == block2);
}

// add2

TEST_CASE("block pool  add2  empty  empty", "[block pool tests]") {
    block_pool instance(0);
    instance.add(std::make_shared<const block_const_ptr_list>());
    REQUIRE(instance.size() == 0u);
}

TEST_CASE("block pool  add2  distinct  expected", "[block pool tests]") {
    block_pool_fixture instance(0);
    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 43);
    block_const_ptr_list blocks{ block1, block2 };

    // The blocks do not have the same hash value, so both will be added.
    REQUIRE(block1->hash() != block2->hash());

    instance.add(std::make_shared<const block_const_ptr_list>(std::move(blocks)));
    REQUIRE(instance.size() == 2u);

    auto const entry1 = instance.blocks().right.find(42);
    REQUIRE(entry1 != instance.blocks().right.end());
    REQUIRE(entry1->second.block() == block1);

    auto const& entry2 = instance.blocks().right.find(43);
    REQUIRE(entry2 != instance.blocks().right.end());
    REQUIRE(entry2->second.block() == block2);
}

// remove

TEST_CASE("block pool  remove  empty  unchanged", "[block pool tests]") {
    block_pool instance(0);
    auto const block1 = make_block(1, 42);
    instance.add(block1);
    REQUIRE(instance.size() == 1u);

    instance.remove(std::make_shared<const block_const_ptr_list>());
    REQUIRE(instance.size() == 1u);
}

TEST_CASE("block pool  remove  all distinct  empty", "[block pool tests]") {
    block_pool_fixture instance(0);
    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 43);
    auto const block3 = make_block(2, 44);
    instance.add(block1);
    instance.add(block2);
    REQUIRE(instance.size() == 2u);

    block_const_ptr_list path{ block1, block2 };
    instance.remove(std::make_shared<const block_const_ptr_list>(std::move(path)));
    REQUIRE(instance.size() == 0u);
}

TEST_CASE("block pool  remove  all connected  empty", "[block pool tests]") {
    block_pool instance(0);
    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 43, block1);
    auto const block3 = make_block(3, 44, block2);
    instance.add(block1);
    instance.add(block2);
    REQUIRE(instance.size() == 2u);

    block_const_ptr_list path{ block1, block2, block3 };
    instance.remove(std::make_shared<const block_const_ptr_list>(std::move(path)));
    REQUIRE(instance.size() == 0u);
}

TEST_CASE("block pool  remove  subtree  reorganized", "[block pool tests]") {
    block_pool_fixture instance(0);
    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 43, block1);
    auto const block3 = make_block(3, 44, block2);
    auto const block4 = make_block(4, 45, block3);
    auto const block5 = make_block(5, 46, block4);

    // sub-branch of block2
    auto const block6 = make_block(6, 44, block2);
    auto const block7 = make_block(7, 45, block2);

    instance.add(block1);
    instance.add(block2);
    instance.add(block3);
    instance.add(block4);
    instance.add(block5);
    instance.add(block6);
    REQUIRE(instance.size() == 6u);

    block_const_ptr_list path{ block1, block2, block6, block7 };
    instance.remove(std::make_shared<const block_const_ptr_list>(std::move(path)));
    REQUIRE(instance.size() == 3u);

    // Entry3 is the new root block (non-zero height).
    auto const entry3 = instance.blocks().right.find(44);
    REQUIRE(entry3 != instance.blocks().right.end());
    REQUIRE(entry3->second.block() == block3);

    // Remaining entries are children (zero height).
    auto const children = instance.blocks().right.find(0);
    REQUIRE(children != instance.blocks().right.end());
}

// prune

TEST_CASE("block pool  prune  empty zero zero  empty", "[block pool tests]") {
    block_pool_fixture instance(0);
    instance.prune(0);
    REQUIRE(instance.size() == 0u);
}

TEST_CASE("block pool  prune  all current  unchanged", "[block pool tests]") {
    block_pool_fixture instance(10);
    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 43);
    auto const block3 = make_block(3, 44);
    auto const block4 = make_block(4, 45);
    auto const block5 = make_block(5, 46);

    instance.add(block1);
    instance.add(block2);
    instance.add(block3);
    instance.add(block4);
    instance.add(block5);
    BOOST_REQUIRE_EQUAL(instance.size(), 5u);

    // Any height less than 42 (52 - 10) should be pruned.
    instance.prune(52);
    BOOST_REQUIRE_EQUAL(instance.size(), 5u);
}

BOOST_AUTO_TEST_CASE(block_pool__prune__one_expired__one_deleted) {
    block_pool_fixture instance(10);
    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 43);
    auto const block3 = make_block(3, 44);
    auto const block4 = make_block(4, 45);
    auto const block5 = make_block(5, 46);

    instance.add(block1);
    instance.add(block2);
    instance.add(block3);
    instance.add(block4);
    instance.add(block5);
    BOOST_REQUIRE_EQUAL(instance.size(), 5u);

    // Any height less than 43 (53 - 10) should be pruned.
    instance.prune(53);
    BOOST_REQUIRE_EQUAL(instance.size(), 4u);
}

BOOST_AUTO_TEST_CASE(block_pool__prune__whole_branch_expired__whole_branch_deleted) {
    block_pool_fixture instance(10);

    // branch1
    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 43, block1);

    // branch2
    auto const block3 = make_block(3, 44);
    auto const block4 = make_block(4, 45, block3);
    auto const block5 = make_block(5, 46, block4);

    instance.add(block1);
    instance.add(block2);
    instance.add(block3);
    instance.add(block4);
    instance.add(block5);
    BOOST_REQUIRE_EQUAL(instance.size(), 5u);

    // Any height less than 44 (54 - 10) should be pruned.
    instance.prune(54);
    BOOST_REQUIRE_EQUAL(instance.size(), 3u);
}

BOOST_AUTO_TEST_CASE(block_pool__prune__partial_branch_expired__partial_branch_deleted) {
    block_pool_fixture instance(10);

    // branch1
    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 43, block1);

    // branch2
    auto const block3 = make_block(3, 44);
    auto const block4 = make_block(4, 45, block3);
    auto const block5 = make_block(5, 46, block4);

    // sub-branch of branch2
    auto const block6 = make_block(6, 45, block3);
    auto const block7 = make_block(7, 46, block6);
    auto const block8 = make_block(8, 47, block7);

    // sub-branch of branch2
    auto const block9 = make_block(9, 45, block3);
    auto const block10 = make_block(10, 46, block9);

    // sub-branch of sub-branch of branch2
    auto const block11 = make_block(11, 46, block9);
    auto const block12 = make_block(12, 47, block10);

    instance.add(block1);
    instance.add(block2);
    instance.add(block3);
    instance.add(block4);
    instance.add(block5);
    instance.add(block6);
    instance.add(block7);
    instance.add(block8);
    instance.add(block9);
    instance.add(block10);
    instance.add(block11);
    instance.add(block12);
    BOOST_REQUIRE_EQUAL(instance.size(), 12u);

    // Any height less than 46 (56 - 10) should be pruned, others replanted.
    instance.prune(56);
    BOOST_REQUIRE_EQUAL(instance.size(), 6u);

    // There are four blocks at height 46, make sure at least one exists.
    auto const entry = instance.blocks().right.find(46);
    BOOST_REQUIRE(entry != instance.blocks().right.end());

    // There are two blocks at 47 but neither is a root (not replanted).
    auto const entry8 = instance.blocks().right.find(47);
    BOOST_REQUIRE(entry8 == instance.blocks().right.end());
}

// filter

BOOST_AUTO_TEST_CASE(block_pool__filter__empty__empty) {
    block_pool_fixture instance(0);
    auto const message = std::make_shared<domain::message::get_data>();
    instance.filter(message);
    BOOST_REQUIRE(message->inventories().empty());
}

BOOST_AUTO_TEST_CASE(block_pool__filter__empty_filter__unchanged) {
    block_pool_fixture instance(0);
    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 42);
    instance.add(block1);
    instance.add(block2);
    auto const message = std::make_shared<domain::message::get_data>();
    instance.filter(message);
    BOOST_REQUIRE(message->inventories().empty());
}

BOOST_AUTO_TEST_CASE(block_pool__filter__matched_blocks__non_blocks_and_mismatches_remain) {
    block_pool_fixture instance(0);
    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 43);
    auto const block3 = make_block(3, 44);
    instance.add(block1);
    instance.add(block2);
    const domain::message::inventory_vector expected1{ domain::message::inventory::type_id::error, block1->hash() };
    const domain::message::inventory_vector expected2{ domain::message::inventory::type_id::transaction, block3->hash() };
    const domain::message::inventory_vector expected3{ domain::message::inventory::type_id::block, block3->hash() };
    domain::message::get_data data
    {
        expected1,
        { domain::message::inventory::type_id::block, block1->hash() },
        expected2,
        { domain::message::inventory::type_id::block, block2->hash() },
        { domain::message::inventory::type_id::block, block2->hash() },
        expected3
    };
    auto const message = std::make_shared<domain::message::get_data>(std::move(data));
    instance.filter(message);
    BOOST_REQUIRE_EQUAL(message->inventories().size(), 3u);
    BOOST_REQUIRE(message->inventories()[0] == expected1);
    BOOST_REQUIRE(message->inventories()[1] == expected2);
    BOOST_REQUIRE(message->inventories()[2] == expected3);
}

// exists

BOOST_AUTO_TEST_CASE(block_pool__exists__empty__false) {
    block_pool_fixture instance(0);
    auto const block1 = make_block(1, 42);
    BOOST_REQUIRE(!instance.exists(block1));
}

BOOST_AUTO_TEST_CASE(block_pool__exists__not_empty_mismatch__false) {
    block_pool_fixture instance(0);
    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 43, block1);
    instance.add(block1);
    BOOST_REQUIRE(!instance.exists(block2));
}

BOOST_AUTO_TEST_CASE(block_pool__exists__match__true) {
    block_pool_fixture instance(0);
    auto const block1 = make_block(1, 42);
    instance.add(block1);
    BOOST_REQUIRE(instance.exists(block1));
}

// parent

BOOST_AUTO_TEST_CASE(block_pool__parent__empty__false) {
    block_pool_fixture instance(0);
    auto const block1 = make_block(1, 42);
    BOOST_REQUIRE(!instance.parent(block1));
}

BOOST_AUTO_TEST_CASE(block_pool__parent__nonempty_mismatch___false) {
    block_pool_fixture instance(0);
    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 43);
    instance.add(block1);
    instance.add(block2);
    BOOST_REQUIRE(!instance.parent(block2));
}

BOOST_AUTO_TEST_CASE(block_pool__parent__match___true) {
    block_pool_fixture instance(0);
    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 43, block1);
    instance.add(block1);
    instance.add(block2);
    BOOST_REQUIRE(instance.parent(block2));
}

// get_path

BOOST_AUTO_TEST_CASE(block_pool__get_path__empty__self) {
    block_pool instance(0);
    auto const block1 = make_block(1, 42);
    auto const path = instance.get_path(block1);
    BOOST_REQUIRE_EQUAL(path->size(), 1u);
    BOOST_REQUIRE(path->blocks()->front() == block1);
}

BOOST_AUTO_TEST_CASE(block_pool__get_path__exists__empty) {
    block_pool instance(0);
    auto const block1 = make_block(1, 42);
    instance.add(block1);
    auto const path = instance.get_path(block1);
    BOOST_REQUIRE_EQUAL(path->size(), 0u);
}

BOOST_AUTO_TEST_CASE(block_pool__get_path__disconnected__self) {
    block_pool_fixture instance(0);
    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 43);
    auto const block3 = make_block(3, 44);

    instance.add(block1);
    instance.add(block2);
    BOOST_REQUIRE_EQUAL(instance.size(), 2u);

    auto const path = instance.get_path(block3);
    BOOST_REQUIRE_EQUAL(path->size(), 1u);
    BOOST_REQUIRE(path->blocks()->front() == block3);
}

BOOST_AUTO_TEST_CASE(block_pool__get_path__connected_one_path__expected_path) {
    block_pool_fixture instance(0);
    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 43, block1);
    auto const block3 = make_block(3, 44, block2);
    auto const block4 = make_block(4, 45, block3);
    auto const block5 = make_block(5, 46, block4);

    instance.add(block1);
    instance.add(block2);
    instance.add(block3);
    instance.add(block4);
    BOOST_REQUIRE_EQUAL(instance.size(), 4u);

    auto const path = instance.get_path(block5);
    BOOST_REQUIRE_EQUAL(path->size(), 5u);
    BOOST_REQUIRE((*path->blocks())[0] == block1);
    BOOST_REQUIRE((*path->blocks())[1] == block2);
    BOOST_REQUIRE((*path->blocks())[2] == block3);
    BOOST_REQUIRE((*path->blocks())[3] == block4);
    BOOST_REQUIRE((*path->blocks())[4] == block5);
}

BOOST_AUTO_TEST_CASE(block_pool__get_path__connected_multiple_paths__expected_path) {
    block_pool_fixture instance(0);

    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 43, block1);
    auto const block3 = make_block(3, 44, block2);
    auto const block4 = make_block(4, 45, block3);
    auto const block5 = make_block(5, 46, block4);

    auto const block11 = make_block(11, 420);
    auto const block12 = make_block(12, 421, block11);
    auto const block13 = make_block(13, 422, block12);
    auto const block14 = make_block(14, 423, block13);
    auto const block15 = make_block(15, 424, block14);

    instance.add(block1);
    instance.add(block2);
    instance.add(block3);
    instance.add(block4);
    BOOST_REQUIRE_EQUAL(instance.size(), 4u);

    instance.add(block11);
    instance.add(block12);
    instance.add(block13);
    instance.add(block14);
    BOOST_REQUIRE_EQUAL(instance.size(), 8u);

    auto const path1 = instance.get_path(block5);
    BOOST_REQUIRE_EQUAL(path1->size(), 5u);
    BOOST_REQUIRE((*path1->blocks())[0] == block1);
    BOOST_REQUIRE((*path1->blocks())[1] == block2);
    BOOST_REQUIRE((*path1->blocks())[2] == block3);
    BOOST_REQUIRE((*path1->blocks())[3] == block4);
    BOOST_REQUIRE((*path1->blocks())[4] == block5);

    auto const path2 = instance.get_path(block15);
    BOOST_REQUIRE_EQUAL(path2->size(), 5u);
    BOOST_REQUIRE((*path2->blocks())[0] == block11);
    BOOST_REQUIRE((*path2->blocks())[1] == block12);
    BOOST_REQUIRE((*path2->blocks())[2] == block13);
    BOOST_REQUIRE((*path2->blocks())[3] == block14);
    BOOST_REQUIRE((*path2->blocks())[4] == block15);
}

BOOST_AUTO_TEST_CASE(block_pool__get_path__connected_multiple_sub_branches__expected_path) {
    block_pool_fixture instance(0);

    // root branch
    auto const block1 = make_block(1, 42);
    auto const block2 = make_block(2, 43, block1);
    auto const block3 = make_block(3, 44, block2);
    auto const block4 = make_block(4, 45, block3);
    auto const block5 = make_block(5, 46, block4);

    // sub-branch of block1
    auto const block11 = make_block(11, 43, block1);
    auto const block12 = make_block(12, 44, block11);

    // sub-branch of block4
    auto const block21 = make_block(21, 46, block4);
    auto const block22 = make_block(22, 47, block21);
    auto const block23 = make_block(23, 48, block22);

    instance.add(block1);
    instance.add(block2);
    instance.add(block3);
    instance.add(block4);
    instance.add(block11);
    instance.add(block21);
    instance.add(block22);
    BOOST_REQUIRE_EQUAL(instance.size(), 7u);

    auto const path1 = instance.get_path(block5);
    BOOST_REQUIRE_EQUAL(path1->size(), 5u);
    BOOST_REQUIRE((*path1->blocks())[0] == block1);
    BOOST_REQUIRE((*path1->blocks())[1] == block2);
    BOOST_REQUIRE((*path1->blocks())[2] == block3);
    BOOST_REQUIRE((*path1->blocks())[3] == block4);
    BOOST_REQUIRE((*path1->blocks())[4] == block5);

    auto const path2 = instance.get_path(block12);
    BOOST_REQUIRE_EQUAL(path2->size(), 3u);
    BOOST_REQUIRE((*path2->blocks())[0] == block1);
    BOOST_REQUIRE((*path2->blocks())[1] == block11);
    BOOST_REQUIRE((*path2->blocks())[2] == block12);

    auto const path3 = instance.get_path(block23);
    BOOST_REQUIRE_EQUAL(path3->size(), 7u);
    BOOST_REQUIRE((*path3->blocks())[0] == block1);
    BOOST_REQUIRE((*path3->blocks())[1] == block2);
    BOOST_REQUIRE((*path3->blocks())[2] == block3);
    BOOST_REQUIRE((*path3->blocks())[3] == block4);
    BOOST_REQUIRE((*path3->blocks())[4] == block21);
    BOOST_REQUIRE((*path3->blocks())[5] == block22);
    BOOST_REQUIRE((*path3->blocks())[6] == block23);
}

BOOST_AUTO_TEST_SUITE_END()
