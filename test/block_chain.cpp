// Copyright (c) 2016-2021 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test_helpers.hpp>

#include <future>
#include <memory>
#include <string>
#include <kth/blockchain.hpp>

using namespace kth;
using namespace kth::blockchain;
using namespace boost::system;
using namespace std::filesystem;

#define MAINNET_BLOCK1 \
"010000006fe28c0ab6f1b372c1a6a246ae63f74f931e8365e15a089c68d6190000000000982" \
"051fd1e4ba744bbbe680e1fee14677ba1a3c3540bf7b1cdb606e857233e0e61bc6649ffff00" \
"1d01e3629901010000000100000000000000000000000000000000000000000000000000000" \
"00000000000ffffffff0704ffff001d0104ffffffff0100f2052a0100000043410496b538e8" \
"53519c726a2c91e61ec11600ae1390813a627c66fb8be7947be63c52da7589379515d4e0a60" \
"4f8141781e62294721166bf621e73a82cbf2342c858eeac00000000"

#define MAINNET_BLOCK2 \
"010000004860eb18bf1b1620e37e9490fc8a427514416fd75159ab86688e9a8300000000d5f" \
"dcc541e25de1c7a5addedf24858b8bb665c9f36ef744ee42c316022c90f9bb0bc6649ffff00" \
"1d08d2bd6101010000000100000000000000000000000000000000000000000000000000000" \
"00000000000ffffffff0704ffff001d010bffffffff0100f2052a010000004341047211a824" \
"f55b505228e4c3d5194c1fcfaa15a456abdf37f9b9d97a4040afc073dee6c89064984f03385" \
"237d92167c13e236446b417ab79a0fcae412ae3316b77ac00000000"

#define MAINNET_BLOCK3 \
"01000000bddd99ccfda39da1b108ce1a5d70038d0a967bacb68b6b63065f626a0000000044f" \
"672226090d85db9a9f2fbfe5f0f9609b387af7be5b7fbb7a1767c831c9e995dbe6649ffff00" \
"1d05e0ed6d01010000000100000000000000000000000000000000000000000000000000000" \
"00000000000ffffffff0704ffff001d010effffffff0100f2052a0100000043410494b9d3e7" \
"6c5b1629ecf97fff95d7a4bbdac87cc26099ada28066c6ff1eb9191223cd897194a08d0c272" \
"6c5747f1db49e8cf90e75dc3e3550ae9b30086f3cd5aaac00000000"

#define TEST_SET_NAME \
    "p2p_tests"

#define TEST_NAME \
    Catch::getResultCapture().getCurrentTestName()

#define START_BLOCKCHAIN(name, flush)                               \
    threadpool pool;                                                \
    database::settings database_settings;                           \
    database_settings.flush_writes = flush;                         \
    database_settings.directory = TEST_NAME;                        \
    REQUIRE(create_database(database_settings));                    \
    blockchain::settings blockchain_settings;                       \
    block_chain name(pool, blockchain_settings, database_settings); \
    REQUIRE(name.start())

#define NEW_BLOCK(height) \
    std::make_shared<const domain::message::block>(read_block(MAINNET_BLOCK##height))

static const uint64_t genesis_mainnet_work = 0x0000000100010001;

static 
void print_headers(std::string const& test) {
    auto const header = "=========== " + test + " ==========";
    LOG_INFO(TEST_SET_NAME, header);
}

bool create_database(database::settings& out_database) {
    print_headers(out_database.directory.string());

    // Blockchain doesn't care about other indexes.
    out_database.index_start_height = max_uint32;

    // Table optimization parameters, reduced for speed and more collision.
    out_database.file_growth_rate = 42;
#ifdef KTH_DB_LEGACY
    out_database.block_table_buckets = 42;
    out_database.transaction_table_buckets = 42;
#endif // KTH_DB_LEGACY
#ifdef KTH_DB_SPEND
    out_database.spend_table_buckets = 42;
#endif // KTH_DB_SPEND
#ifdef KTH_DB_HISTORY
    out_database.history_table_buckets = 42;
#endif // KTH_DB_HISTORY
#ifdef KTH_DB_TRANSACTION_UNCONFIRMED
    out_database.transaction_unconfirmed_table_buckets = 42;
#endif // KTH_DB_TRANSACTION_UNCONFIRMED

    std::error_code ec;
    remove_all(out_database.directory, ec);
    database::data_base database(out_database);
    return create_directories(out_database.directory, ec) && database.create(domain::chain::block::genesis_mainnet());
}

domain::chain::block read_block(const std::string hex) {
    data_chunk data;
    REQUIRE(decode_base16(data, hex));
    domain::chain::block result;
    REQUIRE(kd::entity_from_data(result, data));
    return result;
}

// Start Boost Suite: fast chain tests

#ifdef KTH_DB_LEGACY
TEST_CASE("block chain  insert  flushed  expected", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, true);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(instance.get_block_exists(block1->hash()));
    REQUIRE(instance.get_block_exists(domain::chain::block::genesis_mainnet().hash()));
}

TEST_CASE("block chain  insert  unflushed  expected block", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(instance.get_block_exists(block1->hash()));
    REQUIRE(instance.get_block_exists(domain::chain::block::genesis_mainnet().hash()));
}

TEST_CASE("block chain  get gaps  none  none", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(instance.insert(block1, 1));

    database::block_database::heights heights;
    REQUIRE(instance.get_gaps(heights));
    REQUIRE(heights.empty());
}

TEST_CASE("block chain  get gaps  one  one", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block2 = NEW_BLOCK(2);
    REQUIRE(instance.insert(block2, 2));

    database::block_database::heights heights;
    REQUIRE(instance.get_gaps(heights));
    REQUIRE(heights.size() == 1u);
    REQUIRE(heights[0] == 1u);
}

TEST_CASE("block chain  get block hash  not found  false", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    hash_digest hash;
    REQUIRE( ! instance.get_block_hash(hash, 1));
}

TEST_CASE("block chain  get block hash  found  true", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(instance.insert(block1, 1));

    hash_digest hash;
    REQUIRE(instance.get_block_hash(hash, 1));
    REQUIRE(hash == block1->hash());
}

TEST_CASE("block chain  get branch work  height above top  true", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    uint256_t work;
    uint256_t maximum(max_uint64);

    // This is allowed and just returns zero (standard new single block).
    REQUIRE(instance.get_branch_work(work, maximum, 1));
    REQUIRE(work == 0);
}

TEST_CASE("block chain  get branch work  maximum zero  true", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    uint256_t work;
    uint256_t maximum(0);

    // This should exit early due to hitting the maximum before the genesis block.
    REQUIRE(instance.get_branch_work(work, maximum, 0));
    REQUIRE(work == 0);
}

TEST_CASE("block chain  get branch work  maximum one  true", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(instance.insert(block1, 1));
    uint256_t work;
    uint256_t maximum(genesis_mainnet_work);

    // This should exit early due to hitting the maximum on the genesis block.
    REQUIRE(instance.get_branch_work(work, maximum, 0));
    REQUIRE(work == genesis_mainnet_work);
}

TEST_CASE("block chain  get branch work  unbounded  true", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    auto const block2 = NEW_BLOCK(2);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(instance.insert(block2, 2));

    uint256_t work;
    uint256_t maximum(max_uint64);

    // This should not exit early.
    REQUIRE(instance.get_branch_work(work, maximum, 0));
    REQUIRE(work == 0x0000000300030003);
}

TEST_CASE("block chain  get header  not found  false", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    domain::chain::header header;
    REQUIRE( ! instance.get_header(header, 1));
}

TEST_CASE("block chain  get header  found  true", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(instance.insert(block1, 1));

    domain::chain::header header;
    REQUIRE(instance.get_header(header, 1));
    REQUIRE(header == block1->header());
}

TEST_CASE("block chain  get height  not found  false", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    size_t height;
    REQUIRE( ! instance.get_height(height, null_hash));
}

TEST_CASE("block chain  get height  found  true", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(instance.insert(block1, 1));

    size_t height;
    REQUIRE(instance.get_height(height, block1->hash()));
    REQUIRE(height == 1u);
}

TEST_CASE("block chain  get bits  not found  false", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    uint32_t bits;
    REQUIRE( ! instance.get_bits(bits, 1));
}

TEST_CASE("block chain  get bits  found  true", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(instance.insert(block1, 1));

    uint32_t bits;
    REQUIRE(instance.get_bits(bits, 1));
    REQUIRE(bits == block1->header().bits());
}

TEST_CASE("block chain  get timestamp  not found  false", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    uint32_t timestamp;
    REQUIRE( ! instance.get_timestamp(timestamp, 1));
}

TEST_CASE("block chain  get timestamp  found  true", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(instance.insert(block1, 1));

    uint32_t timestamp;
    REQUIRE(instance.get_timestamp(timestamp, 1));
    REQUIRE(timestamp == block1->header().timestamp());
}

TEST_CASE("block chain  get version  not found  false", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    uint32_t version;
    REQUIRE( ! instance.get_version(version, 1));
}

TEST_CASE("block chain  get version  found  true", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(instance.insert(block1, 1));

    uint32_t version;
    REQUIRE(instance.get_version(version, 1));
    REQUIRE(version == block1->header().version());
}

TEST_CASE("block chain  get last height  no gaps  last block", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    auto const block2 = NEW_BLOCK(2);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(instance.insert(block2, 2));

    size_t height;
    REQUIRE(instance.get_last_height(height));
    REQUIRE(height == 2u);
}

TEST_CASE("block chain  get last height  gap  last block", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block2 = NEW_BLOCK(2);
    REQUIRE(instance.insert(block2, 2));

    size_t height;
    REQUIRE(instance.get_last_height(height));
    REQUIRE(height == 2u);
}

TEST_CASE("block chain  get output  not found  false", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    domain::chain::output output;
    size_t height;
    uint32_t median_time_past;
    bool coinbase;
    const domain::chain::output_point outpoint{ null_hash, 42 };
    size_t branch_height = 0;
    REQUIRE( ! instance.get_output(output, height, median_time_past, coinbase, outpoint, branch_height, true));
}

TEST_CASE("block chain  get output  found  expected", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    auto const block2 = NEW_BLOCK(2);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(instance.insert(block2, 2));

    domain::chain::output output;
    size_t height;
    uint32_t median_time_past;
    bool coinbase;
    const domain::chain::output_point outpoint{ block2->transactions()[0].hash(), 0 };
    auto const expected_value = initial_block_subsidy_satoshi();
    auto const expected_script = block2->transactions()[0].outputs()[0].script().to_string(0);
    REQUIRE(instance.get_output(output, height, median_time_past, coinbase, outpoint, 2, true));
    REQUIRE(coinbase);
    REQUIRE(height == 2u);
    REQUIRE(output.value() == expected_value);
    REQUIRE(output.script().to_string(0) == expected_script);
}

TEST_CASE("block chain  get output  above fork  false", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    auto const block2 = NEW_BLOCK(2);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(instance.insert(block2, 2));

    domain::chain::output output;
    size_t height;
    uint32_t median_time_past;
    bool coinbase;
    const domain::chain::output_point outpoint{ block2->transactions()[0].hash(), 0 };
    REQUIRE( ! instance.get_output(output, height, median_time_past, coinbase, outpoint, 1, true));
}

TEST_CASE("block chain  get is unspent transaction  unspent at fork  true", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    auto const block2 = NEW_BLOCK(2);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(instance.insert(block2, 2));

    auto const hash = block2->transactions()[0].hash();
    REQUIRE(instance.get_is_unspent_transaction(hash, 2, true));
}

TEST_CASE("block chain  get is unspent transaction  unspent above fork  false", "[fast chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    auto const block2 = NEW_BLOCK(2);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(instance.insert(block2, 2));

    auto const hash = block2->transactions()[0].hash();
    REQUIRE( ! instance.get_is_unspent_transaction(hash, 1, true));
}

TEST_CASE("block chain  get is unspent transaction  spent below fork  false", "[fast chain tests]") {
    // TODO: generate spent tx test vector.
}
#endif // KTH_DB_LEGACY

////TEST_CASE("block chain  get transaction  exists  true", "[fast chain tests]")
////{
////    START_BLOCKCHAIN(instance, false);
////
////    auto const block1 = NEW_BLOCK(1);
////    auto const block2 = NEW_BLOCK(2);
////    REQUIRE(instance.insert(block1, 1));
////    REQUIRE(instance.insert(block2, 2));
////
////    size_t height;
////    auto const hash = block1->transactions()[0].hash();
////    REQUIRE(instance.get_transaction(height, hash, false));
////    REQUIRE(height == 1u);
////}

////TEST_CASE("block chain  get transaction  not exists and gapped  false", "[fast chain tests]")
////{
////    START_BLOCKCHAIN(instance, false);
////
////    auto const block1 = NEW_BLOCK(1);
////    auto const block2 = NEW_BLOCK(2);
////    REQUIRE(instance.insert(block2, 2));
////
////    size_t height;
////    auto const hash = block1->transactions()[0].hash();
////    REQUIRE( ! instance.get_transaction(height, hash, false));
////}

// End Boost Suite

//-----------------------------------------------------------------------------

// Start Boost Suite: safe chain tests


#ifdef KTH_DB_LEGACY

// fetch_block

static 
int fetch_block_by_height_result(block_chain& instance, block_const_ptr block, size_t height) {
    std::promise<code> promise;
    auto const handler = [=, &promise](code ec, block_const_ptr result_block, size_t result_height) {
        if (ec) {
            promise.set_value(ec);
            return;
        }

        auto const match = result_height == height && *result_block == *block;
        promise.set_value(match ? error::success : error::operation_failed_24);
    };
    instance.fetch_block(height, true, handler);
    return promise.get_future().get().value();
}

TEST_CASE("block chain  fetch block1  unstarted  error service stopped", "[safe chain tests]") {
    threadpool pool;
    database::settings database_settings;
    database_settings.directory = TEST_NAME;
    REQUIRE(create_database(database_settings));

    blockchain::settings blockchain_settings;
    block_chain instance(pool, blockchain_settings, database_settings);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(fetch_block_by_height_result(instance == block1, 1), error::service_stopped);
}

TEST_CASE("block chain  fetch block1  exists  success", "[safe chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(fetch_block_by_height_result(instance == block1, 1), error::success);
}

TEST_CASE("block chain  fetch block1  not exists  error not found", "[safe chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(fetch_block_by_height_result(instance == block1, 1), error::not_found);
}

static 
int fetch_block_by_hash_result(block_chain& instance, block_const_ptr block, size_t height) {
    std::promise<code> promise;
    auto const handler = [=, &promise](code ec, block_const_ptr result_block, size_t result_height) {
        if (ec) {
            promise.set_value(ec);
            return;
        }

        auto const match = result_height == height && *result_block == *block;
        promise.set_value(match ? error::success : error::operation_failed_25);
    };
    instance.fetch_block(block->hash(), true, handler);
    return promise.get_future().get().value();
}

TEST_CASE("block chain  fetch block2  exists  success", "[safe chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(fetch_block_by_hash_result(instance == block1, 1), error::success);
}

TEST_CASE("block chain  fetch block2  not exists  error not found", "[safe chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(fetch_block_by_hash_result(instance == block1, 1), error::not_found);
}

// fetch_block_header

static 
int fetch_block_header_by_height_result(block_chain& instance, block_const_ptr block, size_t height) {
    std::promise<code> promise;
    auto const handler = [=, &promise](code ec, header_ptr result_header, size_t result_height) {
        if (ec) {
            promise.set_value(ec);
            return;
        }

        auto const match = result_height == height &&
            *result_header == block->header();
        promise.set_value(match ? error::success : error::operation_failed_26);
    };
    instance.fetch_block_header(height, handler);
    return promise.get_future().get().value();
}

TEST_CASE("block chain  fetch block header1  exists  success", "[safe chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(fetch_block_header_by_height_result(instance == block1, 1), error::success);
}

TEST_CASE("block chain  fetch block header1  not exists  error not found", "[safe chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(fetch_block_header_by_height_result(instance == block1, 1), error::not_found);
}

static 
int fetch_block_header_by_hash_result(block_chain& instance, block_const_ptr block, size_t height) {
    std::promise<code> promise;
    auto const handler = [=, &promise](code ec, header_ptr result_header, size_t result_height) {
        if (ec) {
            promise.set_value(ec);
            return;
        }

        auto const match = result_height == height &&
            *result_header == block->header();
        promise.set_value(match ? error::success : error::operation_failed_27);
    };
    instance.fetch_block_header(block->hash(), handler);
    return promise.get_future().get().value();
}

TEST_CASE("block chain  fetch block header2  exists  success", "[safe chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(fetch_block_header_by_hash_result(instance == block1, 1), error::success);
}

TEST_CASE("block chain  fetch block header2  not exists  error not found", "[safe chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(fetch_block_header_by_hash_result(instance == block1, 1), error::not_found);
}

// fetch_merkle_block

static 
int fetch_merkle_block_by_height_result(block_chain& instance,
    block_const_ptr block, size_t height) {
    std::promise<code> promise;
    auto const handler = [=, &promise](code ec, merkle_block_ptr result_merkle, size_t result_height) {
        if (ec) {
            promise.set_value(ec);
            return;
        }

        auto const match = result_height == height &&
            *result_merkle == domain::message::merkle_block(*block);
        promise.set_value(match ? error::success : error::operation_failed_28);
    };
    instance.fetch_merkle_block(height, handler);
    return promise.get_future().get().value();
}

TEST_CASE("block chain  fetch merkle block1  exists  success", "[safe chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(fetch_merkle_block_by_height_result(instance == block1, 1), error::success);
}

TEST_CASE("block chain  fetch merkle block1  not exists  error not found", "[safe chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(fetch_merkle_block_by_height_result(instance == block1, 1), error::not_found);
}

static 
int fetch_merkle_block_by_hash_result(block_chain& instance,
    block_const_ptr block, size_t height) {
    std::promise<code> promise;
    auto const handler = [=, &promise](code ec, merkle_block_ptr result_merkle, size_t result_height) {
        if (ec) {
            promise.set_value(ec);
            return;
        }

        auto const match = result_height == height &&
            *result_merkle == domain::message::merkle_block(*block);
        promise.set_value(match ? error::success : error::operation_failed_29);
    };
    instance.fetch_merkle_block(block->hash(), handler);
    return promise.get_future().get().value();
}

TEST_CASE("block chain  fetch merkle block2  exists  success", "[safe chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(fetch_merkle_block_by_hash_result(instance == block1, 1), error::success);
}

TEST_CASE("block chain  fetch merkle block2  not exists  error not found", "[safe chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    REQUIRE(fetch_merkle_block_by_hash_result(instance == block1, 1), error::not_found);
}


// TODO: fetch_block_height
// TODO: fetch_last_height
// TODO: fetch_transaction
// TODO: fetch_transaction_position
// TODO: fetch_output
// TODO: fetch_spend
// TODO: fetch_history
// TODO: fetch_stealth
// TODO: fetch_block_locator
// TODO: fetch_locator_block_hashes

// fetch_locator_block_headers

static 
int fetch_locator_block_headers(block_chain& instance, get_headers_const_ptr locator, hash_digest const& threshold, size_t limit) {
    std::promise<code> promise;
    auto const handler = [=, &promise](code ec, headers_ptr result_headers) {
        if (ec) {
            promise.set_value(ec);
            return;
        }

        // TODO: incorporate other expectations.
        auto const sequential = result_headers->is_sequential();

        promise.set_value(sequential ? error::success : error::operation_failed_30);
    };
    instance.fetch_locator_block_headers(locator, threshold, limit, handler);
    return promise.get_future().get().value();
}

TEST_CASE("block chain  fetch locator block headers  empty  sequential", "[safe chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    auto const block2 = NEW_BLOCK(2);
    auto const block3 = NEW_BLOCK(3);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(instance.insert(block2, 2));
    REQUIRE(instance.insert(block3, 3));

    auto const locator = std::make_shared<const domain::message::get_headers>();
    REQUIRE(fetch_locator_block_headers(instance == locator, null_hash, 0), error::success);
}

TEST_CASE("block chain  fetch locator block headers  full  sequential", "[safe chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    auto const block2 = NEW_BLOCK(2);
    auto const block3 = NEW_BLOCK(3);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(instance.insert(block2, 2));
    REQUIRE(instance.insert(block3, 3));

    size_t const limit = 3;
    auto const threshold = null_hash;
    auto const locator = std::make_shared<const domain::message::get_headers>();
    REQUIRE(fetch_locator_block_headers(instance == locator, null_hash, 3), error::success);
}

TEST_CASE("block chain  fetch locator block headers  limited  sequential", "[safe chain tests]") {
    START_BLOCKCHAIN(instance, false);

    auto const block1 = NEW_BLOCK(1);
    auto const block2 = NEW_BLOCK(2);
    auto const block3 = NEW_BLOCK(3);
    REQUIRE(instance.insert(block1, 1));
    REQUIRE(instance.insert(block2, 2));
    REQUIRE(instance.insert(block3, 3));

    size_t const limit = 3;
    auto const threshold = null_hash;
    auto const locator = std::make_shared<const domain::message::get_headers>();
    REQUIRE(fetch_locator_block_headers(instance == locator, null_hash, 2), error::success);
}
#endif // KTH_DB_LEGACY

// TODO: fetch_template
// TODO: fetch_mempool
// TODO: filter_blocks
// TODO: filter_transactions
// TODO: subscribe_blockchain
// TODO: subscribe_transaction
// TODO: unsubscribe
// TODO: organize_block
// TODO: organize_transaction
// TODO: chain_settings
// TODO: stopped
// TODO: to_hashes

// End Boost Suite
