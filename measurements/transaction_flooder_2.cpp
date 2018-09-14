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

/*

root@ovh-insight-test-1
    screen -x -S gea
        rsync -avpP /mnt/db_backup/bch-mainnet .



----------------------------------------------------------



Rancher/Docker:
    ssh root@ovh-rancher-bitprim-4.dev.bitprim.org

    docker ps |grep  r-bitprim-node-bch-mainnet
        5666df191bd7        registry.dev.bitprim.org/bitprim:0.10.2           "/.r/r /bitprim/bin/…"   29 hours ago         Up 28 seconds                             r-bitprim-node-bch-mainnet-bitprim-node-1-1dabba1c
    docker exec -ti 5666df191bd7 bash
    stty rows 50 cols 150

Para restaurar la DB:
    cp --reflink /data/restapi-fullnode-database-bch-mainnet/_data/database-snapshot/* /bitprim/database/bch-mainnet
    cp --reflink /data/restapi-fullnode-database-bch-mainnet/_data/database-snapshot/* .  
    cp /data/restapi-fullnode-database-bch-mainnet/_data/database-snapshot/* .  

    Server Curasao
        cp /home/ubuntu/db_backup/bch-mainnet/* .

pip install conan --user
export PATH=$PATH:/root/.local/bin/
pip install cpuid --user

conan remote add bitprim https://api.bintray.com/conan/bitprim/bitprim
conan remove "*"


git clone  https://github.com/bitprim/bitprim-blockchain.git
cd bitprim-blockchain/
git checkout feature_measurements
git submodule init
git submodule update

# git checkout 83801e26caacb376eac5a9f8f6926c253862b086

git pull
conan remove bitprim-blockchain -f
conan remove bitprim-node-cint -f
conan remove bitprim-node -f
conan create . bitprim-blockchain/0.14.0@bitprim/testing -o measurements=True -s build_type=Release
conan install bitprim-node-cint/0.14.0@bitprim/testing -o shared=True -o "*:measurements=True" -s build_type=Release

cd bin

scp root@ovh-insight-test-1.dev.bitprim.org:/bitprim/fer/blocks_and_txs_from_545619_raw.txt .
scp root@ovh-insight-test-1.dev.bitprim.org:/bitprim/log/restapi-bch-mainnet-debug.log  .








rm conanfile.txt
vi conanfile.txt
#################################
[requires]
bitprim-node-cint/0.14.0@bitprim/testing

[options]
bitprim-node-cint:shared=True
bitprim-blockchain:measurements=True
# bitprim-node-cint:microarchitecture=x86_64

[imports]
include/bitprim, *.h -> ./include/bitprim
include/bitprim, *.hpp -> ./include/bitprim
lib, *.so -> ./lib
#################################



rm transaction_flooder_blk_txs.cpp
vi transaction_flooder_blk_txs.cpp

rm -rf include lib *.o transaction_flooder_only_blks transaction_flooder_blk_txs  conaninfo.txt conanbuildinfo.txt conan_imports_manifest.txt deploy_manifest.txt archive  bn
conan install . -s build_type=Release
g++ -O3 -march=native -std=c++11 -Iinclude -c transaction_flooder_blk_txs.cpp
g++ -O3 -march=native -Llib -o transaction_flooder_only_blks transaction_flooder_blk_txs.o -lbitprim-node-cint -lpthread
rm *.o
g++ -DWITH_TXS -O3 -march=native -std=c++11 -Iinclude -c transaction_flooder_blk_txs.cpp
g++ -DWITH_TXS -O3 -march=native -Llib -o transaction_flooder_blk_txs transaction_flooder_blk_txs.o -lbitprim-node-cint -lpthread



# g++ -g -std=c++11 -Iinclude -c transaction_flooder_blk_txs.cpp
# g++ -g -Llib -o transaction_flooder_blk_txs transaction_flooder_blk_txs.o -lbitprim-node-cint -lpthread

export LD_LIBRARY_PATH="$PWD/lib:$LD_LIBRARY_PATH"


./transaction_flooder_blk_txs
./transaction_flooder_blk_txs > log5.txt
./transaction_flooder_blk_txs  2>log.txt

# /bitprim/database/bch-mainnet


scp root@ovh-rancher-bitprim-4.dev.bitprim.org:/bitprim/bin/log.txt .
scp root@ovh-rancher-bitprim-4.dev.bitprim.org:/bitprim/log/restapi-bch-mainnet-debug.log .


scp ubuntu@190.123.23.7:/home/ubuntu/fer2/bin/log8.txt .





------------------------------------
log-anal.py > log2.txt


------------------------------------
Todo en la misma máquina, con File System BTRFS?
log.txt         Primera prueba, cp --reflink
log2.txt        cp SIN reflink
log3.txt        cp SIN reflink, SIN flush_writes
log4.txt        IDEM log3.txt,, agregando log de inputs y block size
log5.txt        IDEM log4.txt, pero enviando TXs antes de los bloques
log6.txt        IDEM log5.txt, network.threads = 20
log7.txt        IDEM log6.txt, chain.cores = 20


*/


#include <csignal>
#include <cstdio>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <bitprim/nodecint/chain/chain.h>
#include <bitprim/nodecint/chain/history_compact.h>
#include <bitprim/nodecint/chain/history_compact_list.h>
#include <bitprim/nodecint/chain/input.h>
#include <bitprim/nodecint/chain/input_list.h>
#include <bitprim/nodecint/chain/output.h>
#include <bitprim/nodecint/chain/output_list.h>

#include <bitprim/nodecint/chain/script.h>
#include <bitprim/nodecint/chain/block.h>
#include <bitprim/nodecint/chain/transaction.h>
#include <bitprim/nodecint/executor_c.h>
#include <bitprim/nodecint/wallet/payment_address.h>
#include <bitprim/nodecint/wallet/wallet.h>
#include <bitprim/nodecint/wallet/word_list.h>

// #include "ctpl.h"

// ----------------------------------------------------------------------------------------------------------------------------------------

inline
int char2int(char input) {
    if (input >= '0' && input <= '9') {
        return input - '0';
    }
    if (input >= 'A' && input <= 'F') {
        return input - 'A' + 10;
    }
    if (input >= 'a' && input <= 'f') {
        return input - 'a' + 10;
    }
    throw std::invalid_argument("Invalid input string");
}

inline
void hex2bin(const char* src, uint8_t* target) {
    while ((*src != 0) && (src[1] != 0)) {
        *(target++) = char2int(*src) * 16 + char2int(src[1]);
        src += 2;
    }
}

executor_t exec_global;
bool stopped = false;

void handle_stop(int /*signal*/) {
    // std::cout << "handle_stop()\n";
    executor_stop(exec_global);
}

std::atomic<size_t> transactions_valid(0);
std::atomic<size_t> transactions_invalid(0);
std::atomic<size_t> blocks_valid(0);
std::atomic<size_t> blocks_invalid(0);


// typedef void (*result_handler_t)(chain_t, void*, error_code_t);
void organize_transaction_handler(chain_t chain, void* ctx, error_code_t err) {
    if (err != bitprim_ec_success) {
        ++transactions_invalid;
        // std::cout << "organize_transaction_handler - err: " << err << "\n";
    } else {
        ++transactions_valid;
    }
}

void organize_block_handler(chain_t chain, void* ctx, error_code_t err) {
    if (err != bitprim_ec_success) {
        ++blocks_invalid;
        // std::cout << "organize_block_handler - err: " << err << "\n";
    } else {
        ++blocks_valid;
    }
}

void test_full(executor_t exec, std::string const& file_path) {
    chain_t chain = executor_get_chain(exec);

    size_t tx_count = 0;
    size_t tx_count_last_time = 0;
    size_t block_count = 0;
    // transactions_valid = 0;
    // transactions_invalid = 0;
    blocks_valid = 0;
    blocks_invalid = 0;

    std::cout << "Start reading the Transactions/Blocks file...\n";

    std::ifstream file(file_path);
    std::string str; 
    while (std::getline(file, str)) {

        if (executor_stopped(exec) != 0) {
            std::cout << "Node stoped ...\n";
            return;
        }

        // std::cout << "str: " << str << std::endl;

        if (str.find(std::string("-T-")) == 0) {
#ifdef WITH_TXS
            str = str.substr(3);
            ++tx_count;
            ++tx_count_last_time;
            std::vector<uint8_t> tx_bytes(str.size() / 2);
            hex2bin(str.c_str(), tx_bytes.data());
            std::reverse(tx_bytes.begin(), tx_bytes.end());

            auto tx = chain_transaction_factory_from_data(31402, tx_bytes.data(), tx_bytes.size());
            auto is_valid = chain_transaction_is_valid(tx);
            // if (is_valid == 0) {
            //     std::cout << "Invalid Transaction. Transaction data: " << str << std::endl;
            // }
            chain_organize_transaction(chain, nullptr, tx, organize_transaction_handler);
#endif // WITH_TXS

        } else if (str.find(std::string("-B-")) == 0) {


            // std::cout << "***** BLOCK DETECTED, PRESS ENTER TO VALIDATE IT\n";
            // std::cin.get();
            // std::cout << "***** BLOCK VALIDATION STARTED\n";

            // if (tx_count_last_time > 1000) {
            //     std::this_thread::sleep_for(std::chrono::seconds(tx_count_last_time / 1000));
            // }

            str = str.substr(3);
            ++block_count;
            std::vector<uint8_t> block_bytes(str.size() / 2);
            hex2bin(str.c_str(), block_bytes.data());
            std::reverse(block_bytes.begin(), block_bytes.end());

            auto block = chain_block_factory_from_data(31402, block_bytes.data(), block_bytes.size());
            auto is_valid = chain_block_is_valid(block);
            if (is_valid == 0) {
                std::cout << "Invalid Block. Block data: " << str << std::endl;
            }

            uint64_t txquant = chain_block_transaction_count(block);
            uint64_t inputsquant = chain_block_total_inputs(block, 1);
            // uint64_t blocksize = chain_block_serialized_size(block);

            // std::cout << "***************** txquant\t" << txquant << '\t' << std::flush;
            std::cout << "***************** txquant\t" << txquant << '\t' << "inputsquant\t" << inputsquant << '\t' << "blocksize\t" << block_bytes.size() << '\t' << std::flush;

            chain_organize_block(chain, nullptr, block, organize_block_handler);
            
            // // std::this_thread::sleep_for(std::chrono::seconds(10));
            // if (tx_count_last_time > 1000) {
            //     std::this_thread::sleep_for(std::chrono::seconds(tx_count_last_time / 1000));
            // }
            tx_count_last_time = 0;

        } else {
            std::cout << "*********** INVALID PREFIX!!!! ***************************" << std::endl;
        }

        // if (tx_count % 100 == 0) {
        //     std::cout << ".";
        // }

        // if (tx_count % 5000 == 0) {
        //     // std::cout << ".";
        //     std::cout << std::endl;
        //     std::cout << "---------------------------------------------------------------------------" << std::endl;
        //     std::cout << "transactions total:   " << tx_count << std::endl;
        //     std::cout << "transactions valid:   " << transactions_valid << std::endl;
        //     std::cout << "transactions invalid: " << transactions_invalid << std::endl;
        //     std::cout << "blocks total:         " << block_count<< std::endl;
        //     std::cout << "blocks valid:         " << blocks_valid << std::endl;
        //     std::cout << "blocks invalid:       " << blocks_invalid << std::endl;
        //     std::cout << "---------------------------------------------------------------------------" << std::endl;
        // }
    }

#ifdef WITH_TXS
    std::cout << "read_transactions_from_file - tx count:        " << tx_count << std::endl;
#endif // WITH_TXS            

    std::cout << "read_transactions_from_file - block count:     " << block_count << std::endl;
    std::cout << "Finished reading the Transactions file, ready for validation process\nWarming up...\n";
    std::cout << "Transaction Organization process finished. Press Ctrl-C to end the program.\n";

    while (executor_stopped(exec) == 0) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    std::cout << "---------------------------------------------------------------------------" << std::endl;
#ifdef WITH_TXS
    std::cout << "transactions total:   " << tx_count << std::endl;
    std::cout << "transactions valid:   " << transactions_valid << std::endl;
    std::cout << "transactions invalid: " << transactions_invalid << std::endl;
#endif // WITH_TXS            
    std::cout << "blocks total:         " << block_count<< std::endl;
    std::cout << "blocks valid:         " << blocks_valid << std::endl;
    std::cout << "blocks invalid:       " << blocks_invalid << std::endl;
    std::cout << "---------------------------------------------------------------------------" << std::endl;
}

int main(int /*argc*/, char* /*argv*/[]) {
    std::signal(SIGINT, handle_stop);
    std::signal(SIGTERM, handle_stop);

    std::string const tx_file = "/bitprim/fer/blocks_and_txs_from_545619_raw.txt";
    std::cout << "***** Transactions File: '" << tx_file << "'\n";

    std::cout.precision(17); 
    std::cout << "Initializing the node...\n";

    exec_global = executor_construct("/bitprim/conf/node-mainnet-2.cfg", stdout, stderr);
    // exec_global = executor_construct("/bitprim/conf/bitprim-node.cfg", stdout, stderr);

	int res2 = executor_run_wait(exec_global);
    if (res2 != 0) {
        printf("Error running node\n");
        executor_destruct(exec_global);
        return -1;
    }

    // std::cout << "***** NODE STARTED, PRE-CHECK\n";
    // test_full_pre(exec_global, tx_file);
    // std::cout << "***** NODE STARTED, PRE-CHECK FINISHED\n";
    // return 0;


    std::cout << "***** NODE STARTED, PRESS ENTER TO CONTINUE\n";
    std::cin.get();
    std::cout << "***** ENTER PRESSED\n";


    // -------------------------------------------------------
    // Use one or the other tests
    // -------------------------------------------------------

    test_full(exec_global, tx_file);
    // -------------------------------------------------------

    executor_destruct(exec_global);

    return 0;
}
