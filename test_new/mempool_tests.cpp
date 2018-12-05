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

#include <bitprim/mining/mempool.hpp>

#include <bitcoin/bitcoin/chain/transaction.hpp>

TEST_CASE("[mempool] ") {
    libbitcoin::mining::mempool mp;

    libbitcoin::chain::transaction tx;
    mp.add(tx);

    REQUIRE(true);
    // REQUIRE( ! state_.asset_id_exists(0));
    // REQUIRE( ! state_.asset_id_exists(1));
}

