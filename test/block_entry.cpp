// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <memory>
#include <bitcoin/blockchain.hpp>

using namespace bc;
using namespace bc::blockchain;

BOOST_AUTO_TEST_SUITE(block_entry_tests)

static auto const hash42 = hash_literal("4242424242424242424242424242424242424242424242424242424242424242");
static auto const default_block_hash = hash_literal("14508459b221041eab257d2baaa7459775ba748246c8403609eb708f0e57e74b");

// construct1/block

BOOST_AUTO_TEST_CASE(block_entry__construct1__default_block__expected)
{
    auto const block = std::make_shared<const message::block>();
    block_entry instance(block);
    BOOST_REQUIRE(instance.block() == block);
    BOOST_REQUIRE(instance.hash() == default_block_hash);
}

// construct2/hash

BOOST_AUTO_TEST_CASE(block_entry__construct2__default_block_hash__round_trips)
{
    block_entry instance(default_block_hash);
    BOOST_REQUIRE(instance.hash() == default_block_hash);
}

// parent

BOOST_AUTO_TEST_CASE(block_entry__parent__hash42__expected)
{
    auto const block = std::make_shared<message::block>();
    block->header().set_previous_block_hash(hash42);
    block_entry instance(block);
    BOOST_REQUIRE(instance.parent() == hash42);
}

// children

BOOST_AUTO_TEST_CASE(block_entry__children__default__empty)
{
    block_entry instance(default_block_hash);
    BOOST_REQUIRE(instance.children().empty());
}

// add_child

BOOST_AUTO_TEST_CASE(block_entry__add_child__one__single)
{
    block_entry instance(null_hash);
    auto const child = std::make_shared<const message::block>();
    instance.add_child(child);
    BOOST_REQUIRE_EQUAL(instance.children().size(), 1u);
    BOOST_REQUIRE(instance.children()[0] == child->hash());
}

BOOST_AUTO_TEST_CASE(block_entry__add_child__two__expected_order)
{
    block_entry instance(null_hash);

    auto const child1 = std::make_shared<const message::block>();
    instance.add_child(child1);

    auto const child2 = std::make_shared<message::block>();
    child2->header().set_previous_block_hash(hash42);
    instance.add_child(child2);

    BOOST_REQUIRE_EQUAL(instance.children().size(), 2u);
    BOOST_REQUIRE(instance.children()[0] == child1->hash());
    BOOST_REQUIRE(instance.children()[1] == child2->hash());
}

// equality

BOOST_AUTO_TEST_CASE(block_entry__equality__same__true)
{
    auto const block = std::make_shared<const message::block>();
    block_entry instance1(block);
    block_entry instance2(block->hash());
    BOOST_REQUIRE(instance1 == instance2);
}

BOOST_AUTO_TEST_CASE(block_entry__equality__different__false)
{
    auto const block = std::make_shared<const message::block>();
    block_entry instance1(block);
    block_entry instance2(null_hash);
    BOOST_REQUIRE(!(instance1 == instance2));
}

BOOST_AUTO_TEST_SUITE_END()
