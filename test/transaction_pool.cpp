/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
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
#include <boost/test/unit_test.hpp>
// #include <bitcoin/consensus.hpp>
#include <bitcoin/blockchain.hpp>

using namespace bc;
using namespace bc::blockchain;

#ifdef WITH_BLOCKCHAIN_REPLIER
#include <process.hpp>

struct fixture
{
#ifdef WITH_CONSENSUS_REPLIER
    ::process consensus;
#endif
    ::process replier;

    fixture()
#ifdef WITH_CONSENSUS_REPLIER
      : consensus(::process::exec(WITH_CONSENSUS_REPLIER)),
        replier(::process::exec(WITH_BLOCKCHAIN_REPLIER))
#else
      : replier(::process::exec(WITH_BLOCKCHAIN_REPLIER))
#endif
    {
#ifdef WITH_CONSENSUS_REPLIER
        libbitcoin::consensus::requester.connect({ "tcp://localhost:5501" });
#endif
    }

    ~fixture()
    {
        replier.terminate();
        replier.join();

        libbitcoin::consensus::requester.disconnect();
        consensus.terminate();
        consensus.join();
    }
};
#else
struct fixture {};
#endif

BOOST_FIXTURE_TEST_SUITE(transaction_pool_tests, ::fixture)

BOOST_AUTO_TEST_CASE(transaction_pool__construct__foo__bar) {
    // TODO
}

BOOST_AUTO_TEST_SUITE_END()
