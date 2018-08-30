/**
 * Copyright (c) 2018 Bitprim developers (see AUTHORS)
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

#include "doctest.h"

#include <bitprim/keoken/memory_state.hpp>

using namespace bitprim::keoken;
using libbitcoin::hash_digest;
using libbitcoin::hash_literal;
using libbitcoin::wallet::payment_address;

TEST_CASE("[state_asset_id_exists_empty] ") {

    // state state_(0);
    memory_state state_;
    state_.set_initial_asset_id(0);
    REQUIRE( ! state_.asset_id_exists(0));
    REQUIRE( ! state_.asset_id_exists(1));
}

TEST_CASE("[state_asset_id_exists_not_empty] ") {

    // state state_(0);
    memory_state state_;
    state_.set_initial_asset_id(0);

    std::string name = "Test";
    amount_t amount = 1559;
    size_t height = 456;
    payment_address addr("moNQd8TVGogcLsmPzNN2QdFwDfcAZFfUCr");
    const hash_digest txid = hash_literal("8b4b9487199ed6668cf6135f29f832c215ab8d32a32c323923594e7475dece25");

    state_.create_asset(name, amount, addr, height, txid);
    
    REQUIRE(state_.asset_id_exists(0));
}

TEST_CASE("[state_get_assets_empty] ") {

    // state state_(1);
    memory_state state_;
    state_.set_initial_asset_id(1);

    auto const& ret = state_.get_assets();
    REQUIRE(ret.size() == 0);
}

TEST_CASE("[state_get_assets_empty_after_reset] ") {

    // state state_(1);
    memory_state state_;
    state_.set_initial_asset_id(1);

    // ----------------------------------------
    std::string name = "Test";
    amount_t amount = 1559;
    size_t height = 456;
    payment_address addr("moNQd8TVGogcLsmPzNN2QdFwDfcAZFfUCr");
    const hash_digest txid = hash_literal("8b4b9487199ed6668cf6135f29f832c215ab8d32a32c323923594e7475dece25");
    state_.create_asset(name, amount, addr, height, txid);
    REQUIRE(state_.get_assets().size() == 1);
    // ----------------------------------------

    state_.reset();

    REQUIRE(state_.get_assets().size() == 0);
}

TEST_CASE("[state_get_assets_empty_after_rollback] ") {

    // state state_(1);
    memory_state state_;
    state_.set_initial_asset_id(1);

    // ----------------------------------------
    std::string name = "Test";
    amount_t amount = 1559;
    size_t height = 456;
    payment_address addr("moNQd8TVGogcLsmPzNN2QdFwDfcAZFfUCr");
    const hash_digest txid = hash_literal("8b4b9487199ed6668cf6135f29f832c215ab8d32a32c323923594e7475dece25");
    state_.create_asset(name, amount, addr, height, txid);
    REQUIRE(state_.get_assets().size() == 1);
    // ----------------------------------------

    state_.rollback_to(height - 1);

    REQUIRE(state_.get_assets().size() == 0);
}


TEST_CASE("[state_get_assets_not_empty] ") {

    // state state_(0);
    memory_state state_;
    state_.set_initial_asset_id(0);


    std::string name = "Test";
    amount_t amount = 1559;
    size_t height = 456;
    payment_address addr("moNQd8TVGogcLsmPzNN2QdFwDfcAZFfUCr");
    const hash_digest txid = hash_literal("8b4b9487199ed6668cf6135f29f832c215ab8d32a32c323923594e7475dece25");

    state_.create_asset(name, amount, addr, height, txid);

    auto const& ret = state_.get_assets();
    REQUIRE(ret.size() == 1);

    auto const new_asset = ret[0];

    REQUIRE(new_asset.asset_id == 0);
    REQUIRE(new_asset.asset_name == name);
    REQUIRE(new_asset.amount == amount);
    REQUIRE(new_asset.asset_creator == addr);
}

TEST_CASE("[state_create_asset] ") {

    // state state_(0);
    memory_state state_;
    state_.set_initial_asset_id(0);

    std::string name = "Test";
    amount_t amount = 1559;
    size_t height = 456;
    payment_address addr("moNQd8TVGogcLsmPzNN2QdFwDfcAZFfUCr");
    const hash_digest txid = hash_literal("8b4b9487199ed6668cf6135f29f832c215ab8d32a32c323923594e7475dece25");

    state_.create_asset(name, amount, addr, height, txid);
    
    auto const& ret = state_.get_assets();

    auto const new_asset = ret[0];

    REQUIRE(new_asset.asset_id == 0);
    REQUIRE(new_asset.asset_name == name);
    REQUIRE(new_asset.amount == amount);
    REQUIRE(new_asset.asset_creator == addr);

    auto const& balance = state_.get_balance(0,addr);

     REQUIRE(balance == 1559);
}

TEST_CASE("[state_create_balance_entry] ") {

    asset_id_t asset_id = 0;
    memory_state state_;
    state_.set_initial_asset_id(asset_id);


    std::string name = "Test";
    amount_t amount = 1559;
    size_t height = 456;
    payment_address source("moNQd8TVGogcLsmPzNN2QdFwDfcAZFfUCr");
    payment_address destination("1CK6KHY6MHgYvmRQ4PAafKYDrg1ejbH1cE");
    const hash_digest txid = hash_literal("8b4b9487199ed6668cf6135f29f832c215ab8d32a32c323923594e7475dece25");

    state_.create_asset(name, amount, source, height, txid);
    
    amount_t amount_to_transfer = 5;
    state_.create_balance_entry(asset_id, amount_to_transfer, source, destination, height, txid );

    REQUIRE(state_.get_balance(asset_id, source) == amount - amount_to_transfer);
    REQUIRE(state_.get_balance(asset_id, destination) == amount_to_transfer);
}

TEST_CASE("[state_create_balance_entry_after_reset] ") {

    asset_id_t asset_id = 0;
    memory_state state_;
    state_.set_initial_asset_id(asset_id);


    std::string name = "Test";
    amount_t amount = 1559;
    size_t height = 456;
    payment_address source("moNQd8TVGogcLsmPzNN2QdFwDfcAZFfUCr");
    payment_address destination("1CK6KHY6MHgYvmRQ4PAafKYDrg1ejbH1cE");
    const hash_digest txid = hash_literal("8b4b9487199ed6668cf6135f29f832c215ab8d32a32c323923594e7475dece25");

    state_.create_asset(name, amount, source, height, txid);
    
    amount_t amount_to_transfer = 5;
    state_.create_balance_entry(asset_id, amount_to_transfer, source, destination, height + 1, txid );

    REQUIRE(state_.get_balance(asset_id, source) == amount - amount_to_transfer);
    REQUIRE(state_.get_balance(asset_id, destination) == amount_to_transfer);

    state_.reset();

    REQUIRE(state_.get_balance(asset_id, source) == 0);
    REQUIRE(state_.get_balance(asset_id, destination) == 0);

}

TEST_CASE("[state_create_balance_entry_after_rollback] ") {

    asset_id_t asset_id = 0;
    memory_state state_;
    state_.set_initial_asset_id(asset_id);


    std::string name = "Test";
    amount_t amount = 1559;
    size_t height = 456;
    payment_address source("moNQd8TVGogcLsmPzNN2QdFwDfcAZFfUCr");
    payment_address destination("1CK6KHY6MHgYvmRQ4PAafKYDrg1ejbH1cE");
    const hash_digest txid = hash_literal("8b4b9487199ed6668cf6135f29f832c215ab8d32a32c323923594e7475dece25");

    state_.create_asset(name, amount, source, height, txid);
    
    amount_t amount_to_transfer = 5;
    state_.create_balance_entry(asset_id, amount_to_transfer, source, destination, height + 1, txid );

    REQUIRE(state_.get_balance(asset_id, source) == amount - amount_to_transfer);
    REQUIRE(state_.get_balance(asset_id, destination) == amount_to_transfer);

    state_.rollback_to(height);

    REQUIRE(state_.get_balance(asset_id, source) == amount);
    REQUIRE(state_.get_balance(asset_id, destination) == 0);

}

TEST_CASE("[state_get_assets_by_address] ") {

    // state state_(0);
    memory_state state_;
    state_.set_initial_asset_id(0);

    std::string name = "Test";
    amount_t amount = 1559;
    size_t height = 456;
    payment_address source("moNQd8TVGogcLsmPzNN2QdFwDfcAZFfUCr");
    payment_address destination("1CK6KHY6MHgYvmRQ4PAafKYDrg1ejbH1cE");
    const hash_digest txid = hash_literal("8b4b9487199ed6668cf6135f29f832c215ab8d32a32c323923594e7475dece25");

    state_.create_asset(name, amount, source, height, txid);
    
    amount_t amount_to_transfer = 5;
    state_.create_balance_entry(0, amount_to_transfer, source, destination, height, txid );

    auto const& list_source = state_.get_assets_by_address(source);
    auto const& list_destination= state_.get_assets_by_address(destination);

    REQUIRE(list_source.size() == 1);
    REQUIRE(list_destination.size() == 1);

    auto const asset_1 = list_source[0];

    REQUIRE(asset_1.asset_id == 0);
    REQUIRE(asset_1.asset_name == name);
    REQUIRE(asset_1.amount == amount - amount_to_transfer);
    REQUIRE(asset_1.asset_creator == source);

    auto const asset_2 = list_destination[0];

    REQUIRE(asset_2.asset_id == 0);
    REQUIRE(asset_2.asset_name == name);
    REQUIRE(asset_2.amount == amount_to_transfer);
    REQUIRE(asset_2.asset_creator == source);
}


TEST_CASE("[state_get_all_asset_addresses] ") {

    // state state_(0);
    memory_state state_;
    state_.set_initial_asset_id(0);

    std::string name = "Test";
    amount_t amount = 1559;
    size_t height = 456;
    payment_address source("moNQd8TVGogcLsmPzNN2QdFwDfcAZFfUCr");
    payment_address destination("1CK6KHY6MHgYvmRQ4PAafKYDrg1ejbH1cE");
    const hash_digest txid = hash_literal("8b4b9487199ed6668cf6135f29f832c215ab8d32a32c323923594e7475dece25");

    state_.create_asset(name, amount, source, height, txid);
    
    amount_t amount_to_transfer = 5;
    state_.create_balance_entry(0, amount_to_transfer, source, destination, height, txid );

    auto const& list_source = state_.get_all_asset_addresses();
    
    REQUIRE(list_source.size() == 2);

    auto it = std::find_if(list_source.begin(),list_source.end(), [&source](get_all_asset_addresses_data const& x) {
        return x.amount_owner == source;
    } );


    auto const asset_1 = *it;

    REQUIRE(asset_1.asset_id == 0);
    REQUIRE(asset_1.asset_name == name);
    REQUIRE(asset_1.amount == amount - amount_to_transfer);
    REQUIRE(asset_1.asset_creator == source);
    REQUIRE(asset_1.amount_owner == source);

    it = std::find_if(list_source.begin(),list_source.end(), [&destination](get_all_asset_addresses_data const& x) {
        return x.amount_owner == destination;
    } );

    auto const asset_2 = *it;

    REQUIRE(asset_2.asset_id == 0);
    REQUIRE(asset_2.asset_name == name);
    REQUIRE(asset_2.amount == amount_to_transfer);
    REQUIRE(asset_2.asset_creator == source);
    REQUIRE(asset_2.amount_owner == destination);
}


