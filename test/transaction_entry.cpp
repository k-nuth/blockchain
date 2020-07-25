// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <memory>

#include <kth/blockchain.hpp>

using namespace kth;
using namespace kd::chain;
using namespace kth::blockchain;
using namespace kd::machine;

BOOST_AUTO_TEST_SUITE(transaction_entry_tests)

static 
auto const default_tx_hash = hash_literal("f702453dd03b0f055e5437d76128141803984fb10acb85fc3b2184fae2f3fa78");

static 
chain_state::data data() {
    chain_state::data value;
    value.height = 1;
    value.bits = { 0, { 0 } };
    value.version = { 1, { 0 } };
    value.timestamp = { 0, 0, { 0 } };
    return value;
}

static 
transaction_const_ptr make_tx() {
    auto const tx = std::make_shared<const domain::message::transaction>();
    tx->validation.state = std::make_shared<chain_state>(
#ifdef KTH_CURRENCY_BCH
        chain_state {data(), 0, {}, kth::phonon_t(0), kth::axion_t(0)});
#else
        chain_state {data(), 0, {}});
#endif //KTH_CURRENCY_BCH
        
    return tx;
}

static 
transaction_entry::ptr make_instance() {
    return std::make_shared<transaction_entry>(transaction_entry(make_tx()));
}

// TODO: add populated tx and test property values.

// construct1/tx

BOOST_AUTO_TEST_CASE(transaction_entry__construct1__default_tx__expected_values) {
    const transaction_entry instance(make_tx());
    BOOST_REQUIRE(instance.is_anchor());
    BOOST_REQUIRE_EQUAL(instance.fees(), 0);
    BOOST_REQUIRE_EQUAL(instance.forks(), 0);
    BOOST_REQUIRE_EQUAL(instance.sigops(), 0);
    BOOST_REQUIRE_EQUAL(instance.size(), 10u);
    BOOST_REQUIRE(instance.hash() == default_tx_hash);
    BOOST_REQUIRE(!instance.is_marked());
    BOOST_REQUIRE(instance.parents().empty());
    BOOST_REQUIRE(instance.children().empty());
}

// construct2/hash

BOOST_AUTO_TEST_CASE(transaction_entry__construct1__default_block_hash__expected_values) {
    const transaction_entry instance(make_tx()->hash());
    BOOST_REQUIRE(instance.is_anchor());
    BOOST_REQUIRE_EQUAL(instance.fees(), 0);
    BOOST_REQUIRE_EQUAL(instance.forks(), 0);
    BOOST_REQUIRE_EQUAL(instance.sigops(), 0);
    BOOST_REQUIRE_EQUAL(instance.size(), 0);
    BOOST_REQUIRE(instance.hash() == default_tx_hash);
    BOOST_REQUIRE(!instance.is_marked());
    BOOST_REQUIRE(instance.parents().empty());
    BOOST_REQUIRE(instance.children().empty());
}

// is_anchor

BOOST_AUTO_TEST_CASE(transaction_entry__is_anchor__parents__false) {
    transaction_entry instance(make_tx());
    auto const parent = make_instance();
    instance.add_parent(parent);
    BOOST_REQUIRE(!instance.is_anchor());
}

BOOST_AUTO_TEST_CASE(transaction_entry__is_anchor__children__true) {
    transaction_entry instance(make_tx());
    auto const child = make_instance();
    instance.add_child(child);
    BOOST_REQUIRE(instance.is_anchor());
}

// mark

BOOST_AUTO_TEST_CASE(transaction_entry__mark__true__expected) {
    transaction_entry instance(make_tx());
    instance.mark(true);
    BOOST_REQUIRE(instance.is_marked());
}

BOOST_AUTO_TEST_CASE(transaction_entry__mark__true_false__expected) {
    transaction_entry instance(make_tx());
    instance.mark(true);
    instance.mark(false);
    BOOST_REQUIRE(!instance.is_marked());
}

// is_marked

BOOST_AUTO_TEST_CASE(transaction_entry__mark__default__false) {
    const transaction_entry instance(make_tx());
    BOOST_REQUIRE(!instance.is_marked());
}

BOOST_AUTO_TEST_CASE(transaction_entry__is_marked__true__true) {
    transaction_entry instance(make_tx());
    instance.mark(true);
    BOOST_REQUIRE(instance.is_marked());
}

// add_parent

BOOST_AUTO_TEST_CASE(transaction_entry__add_parent__one__expected_parents) {
    transaction_entry instance(make_tx());
    auto const parent = make_instance();
    instance.add_parent(parent);
    BOOST_REQUIRE_EQUAL(instance.parents().size(), 1u);
    BOOST_REQUIRE_EQUAL(instance.parents().front(), parent);
}

// add_child

BOOST_AUTO_TEST_CASE(transaction_entry__add_child__one__expected_children) {
    transaction_entry instance(make_tx());
    auto const child = make_instance();
    instance.add_child(child);
    BOOST_REQUIRE_EQUAL(instance.children().size(), 1u);
    BOOST_REQUIRE_EQUAL(instance.children().front(), child);
}

// remove_child

BOOST_AUTO_TEST_CASE(transaction_entry__remove_child__not_found__empty) {
    transaction_entry instance(make_tx());
    auto const child = make_instance();
    instance.remove_child(child);
    BOOST_REQUIRE(instance.children().empty());
}

BOOST_AUTO_TEST_CASE(transaction_entry__remove_child__only_found__empty) {
    transaction_entry instance(make_tx());
    auto const child = make_instance();
    instance.add_child(child);
    BOOST_REQUIRE_EQUAL(instance.children().size(), 1u);
    instance.remove_child(child);
    BOOST_REQUIRE(instance.children().empty());
}

BOOST_AUTO_TEST_CASE(transaction_entry__remove_child__one_of_two__expected_one_remains) {
    transaction_entry instance(make_tx());
    auto const child1 = make_instance();
    auto const child2 = make_instance();
    instance.add_child(child1);
    instance.add_child(child2);
    BOOST_REQUIRE_EQUAL(instance.children().size(), 2u);
    instance.remove_child(child1);
    BOOST_REQUIRE_EQUAL(instance.children().size(), 1u);
    BOOST_REQUIRE_EQUAL(instance.children().front(), child2);
}

BOOST_AUTO_TEST_SUITE_END()
