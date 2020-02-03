// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
