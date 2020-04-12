// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>

// #include <boost/format.hpp>
#include <boost/filesystem.hpp>

#include <fmt/core.h>

#include <kth/blockchain.hpp>
#include <kth/database.hpp>


// #define BS_INITCHAIN_DIR_NEW "Failed to create directory %1% with error, '%2%'.\n"
// #define BS_INITCHAIN_DIR_EXISTS "Failed because the directory %1% already exists.\n"
#define BS_INITCHAIN_DIR_NEW "Failed to create directory {} with error, '{}'.\n"
#define BS_INITCHAIN_DIR_EXISTS "Failed because the directory {} already exists.\n"
#define BS_INITCHAIN_FAIL "Failed to initialize blockchain files.\n"

using namespace bc;
using namespace bc::blockchain;
using namespace bc::chain;
using namespace bc::database;
using namespace boost::filesystem;
using namespace boost::system;
// using boost::format;

// Create a new mainnet blockchain database.
int main(int argc, char** argv) {
    std::string prefix("mainnet");

    if (argc > 1) {
        prefix = argv[1];
    }

    if (argc > 2 && std::string("--clean") == argv[2]) {
        boost::filesystem::remove_all(prefix);
    }

    error_code code;
    if ( ! create_directories(prefix, code)) {
        if (code.value() == 0) {
            std::cerr << fmt::format(BS_INITCHAIN_DIR_EXISTS, prefix);
        } else {
            std::cerr << fmt::format(BS_INITCHAIN_DIR_NEW, prefix, code.message());
        }
        return -1;
    }

    database::settings settings(config::settings::mainnet);

    if ( ! data_base(settings).create(block::genesis_mainnet())) {
        std::cerr << BS_INITCHAIN_FAIL;
        return -1;
    }

    return 0;
}
