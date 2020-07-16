// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test_helpers.hpp>

#include <memory>
#include <kth/blockchain.hpp>

using namespace kth;
using namespace kd::message;
using namespace kth::blockchain;

// Start Boost Suite: branch tests

#define DECLARE_BLOCK(name, number) \
    auto const name##number = std::make_shared<block>(); \
    name##number->header().set_bits(number);

// Access to protected members.
class branch_fixture
  : public branch
{
public:
    size_t index_of(size_t height) const
    {
        return branch::index_of(height);
    }

    size_t height_at(size_t index) const
    {
        return branch::height_at(index);
    }
};

// hash

TEST_CASE("branch  hash  default  null hash", "[branch tests]") {
    branch instance;
    REQUIRE(instance.hash() == null_hash);
}

TEST_CASE("branch  hash  one block  only previous block hash", "[branch tests]") {
    DECLARE_BLOCK(block, 0);
    DECLARE_BLOCK(block, 1);

    auto const expected = block0->hash();
    block1->header().set_previous_block_hash(expected);

    branch instance;
    BOOST_REQUIRE(instance.push_front(block1));
    BOOST_REQUIRE(instance.hash() == expected);
}

BOOST_AUTO_TEST_CASE(branch__hash__two_blocks__first_previous_block_hash) {
    branch instance;
    DECLARE_BLOCK(top, 42);
    DECLARE_BLOCK(block, 0);
    DECLARE_BLOCK(block, 1);

    // Link the blocks.
    auto const expected = top42->hash();
    block0->header().set_previous_block_hash(expected);
    block1->header().set_previous_block_hash(block0->hash());

    BOOST_REQUIRE(instance.push_front(block1));
    BOOST_REQUIRE(instance.push_front(block0));
    BOOST_REQUIRE(instance.hash() == expected);
}

// height/set_height

BOOST_AUTO_TEST_CASE(branch__height__default__zero) {
    branch instance;
    BOOST_REQUIRE_EQUAL(instance.height(), 0);
}

BOOST_AUTO_TEST_CASE(branch__set_height__round_trip__unchanged) {
    static const size_t expected = 42;
    branch instance;
    instance.set_height(expected);
    BOOST_REQUIRE_EQUAL(instance.height(), expected);
}

// index_of

BOOST_AUTO_TEST_CASE(branch__index_of__one__zero) {
    branch_fixture instance;
    instance.set_height(0);
    BOOST_REQUIRE_EQUAL(instance.index_of(1), 0u);
}

BOOST_AUTO_TEST_CASE(branch__index_of__two__one) {
    branch_fixture instance;
    instance.set_height(0);
    BOOST_REQUIRE_EQUAL(instance.index_of(2), 1u);
}

BOOST_AUTO_TEST_CASE(branch__index_of__value__expected) {
    branch_fixture instance;
    instance.set_height(42);
    BOOST_REQUIRE_EQUAL(instance.index_of(53), 10u);
}

// height_at

BOOST_AUTO_TEST_CASE(branch__height_at__zero__one) {
    branch_fixture instance;
    instance.set_height(0);
    BOOST_REQUIRE_EQUAL(instance.height_at(0), 1u);
}

BOOST_AUTO_TEST_CASE(branch__height_at__one__two) {
    branch_fixture instance;
    instance.set_height(0);
    BOOST_REQUIRE_EQUAL(instance.height_at(1), 2u);
}

BOOST_AUTO_TEST_CASE(branch__height_at__value__expected) {
    branch_fixture instance;
    instance.set_height(42);
    BOOST_REQUIRE_EQUAL(instance.height_at(10), 53u);
}

// size

BOOST_AUTO_TEST_CASE(branch__size__empty__zero) {
    branch instance;
    BOOST_REQUIRE_EQUAL(instance.size(), 0);
}

// empty

BOOST_AUTO_TEST_CASE(branch__empty__default__true) {
    branch instance;
    BOOST_REQUIRE(instance.empty());
}

BOOST_AUTO_TEST_CASE(branch__empty__push_one__false) {
    branch instance;
    DECLARE_BLOCK(block, 0);
    BOOST_REQUIRE(instance.push_front(block0));
    BOOST_REQUIRE(!instance.empty());
}

// blocks

BOOST_AUTO_TEST_CASE(branch__blocks__default__empty) {
    branch instance;
    BOOST_REQUIRE(instance.blocks()->empty());
}

BOOST_AUTO_TEST_CASE(branch__blocks__one__empty) {
    branch instance;
    DECLARE_BLOCK(block, 0);
    BOOST_REQUIRE(instance.push_front(block0));
    BOOST_REQUIRE(!instance.empty());
    BOOST_REQUIRE_EQUAL(instance.blocks()->size(), 1u);
}

// push_front

BOOST_AUTO_TEST_CASE(branch__push_front__one__success) {
    branch_fixture instance;
    DECLARE_BLOCK(block, 0);
    BOOST_REQUIRE(instance.push_front(block0));
    BOOST_REQUIRE(!instance.empty());
    BOOST_REQUIRE_EQUAL(instance.size(), 1u);
    BOOST_REQUIRE((*instance.blocks())[0] == block0);
}

BOOST_AUTO_TEST_CASE(branch__push_front__two_linked__success) {
    branch_fixture instance;
    DECLARE_BLOCK(block, 0);
    DECLARE_BLOCK(block, 1);

    // Link the blocks.
    block1->header().set_previous_block_hash(block0->hash());

    BOOST_REQUIRE(instance.push_front(block1));
    BOOST_REQUIRE(instance.push_front(block0));
    BOOST_REQUIRE_EQUAL(instance.size(), 2u);
    BOOST_REQUIRE((*instance.blocks())[0] == block0);
    BOOST_REQUIRE((*instance.blocks())[1] == block1);
}

BOOST_AUTO_TEST_CASE(branch__push_front__two_unlinked__link_failure) {
    branch_fixture instance;
    DECLARE_BLOCK(block, 0);
    DECLARE_BLOCK(block, 1);

    // Ensure the blocks are not linked.
    block1->header().set_previous_block_hash(null_hash);

    BOOST_REQUIRE(instance.push_front(block1));
    BOOST_REQUIRE(!instance.push_front(block0));
    BOOST_REQUIRE_EQUAL(instance.size(), 1u);
    BOOST_REQUIRE((*instance.blocks())[0] == block1);
}

// top

BOOST_AUTO_TEST_CASE(branch__top__default__nullptr) {
    branch instance;
    BOOST_REQUIRE(!instance.top());
}

BOOST_AUTO_TEST_CASE(branch__top__two_blocks__expected) {
    branch_fixture instance;
    DECLARE_BLOCK(block, 0);
    DECLARE_BLOCK(block, 1);

    // Link the blocks.
    block1->header().set_previous_block_hash(block0->hash());

    BOOST_REQUIRE(instance.push_front(block1));
    BOOST_REQUIRE(instance.push_front(block0));
    BOOST_REQUIRE_EQUAL(instance.size(), 2u);
    BOOST_REQUIRE(instance.top() == block1);
}

// top_height

BOOST_AUTO_TEST_CASE(branch__top_height__default__0) {
    branch instance;
    BOOST_REQUIRE_EQUAL(instance.top_height(), 0u);
}

BOOST_AUTO_TEST_CASE(branch__top_height__two_blocks__expected) {
    branch_fixture instance;
    DECLARE_BLOCK(block, 0);
    DECLARE_BLOCK(block, 1);

    static const size_t expected = 42;
    instance.set_height(expected - 2);

    // Link the blocks.
    block1->header().set_previous_block_hash(block0->hash());

    BOOST_REQUIRE(instance.push_front(block1));
    BOOST_REQUIRE(instance.push_front(block0));
    BOOST_REQUIRE_EQUAL(instance.size(), 2u);
    BOOST_REQUIRE_EQUAL(instance.top_height(), expected);
}

// work

BOOST_AUTO_TEST_CASE(branch__work__default__zero) {
    branch instance;
    BOOST_REQUIRE(instance.work() == 0);
}

BOOST_AUTO_TEST_CASE(branch__work__two_blocks__expected) {
    branch instance;
    DECLARE_BLOCK(block, 0);
    DECLARE_BLOCK(block, 1);

    // Link the blocks.
    block1->header().set_previous_block_hash(block0->hash());

    BOOST_REQUIRE(instance.push_front(block1));
    BOOST_REQUIRE(instance.push_front(block0));
    BOOST_REQUIRE_EQUAL(instance.size(), 2u);

    ///////////////////////////////////////////////////////////////////////////
    // TODO: devise value tests.
    ///////////////////////////////////////////////////////////////////////////
    BOOST_REQUIRE(instance.work() == 0);
}

BOOST_AUTO_TEST_SUITE_END()
