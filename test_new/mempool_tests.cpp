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
#include <bitcoin/blockchain.hpp>

using namespace libbitcoin;
using namespace libbitcoin::chain;
using namespace libbitcoin::mining;

hash_digest hash_one   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
hash_digest hash_two   {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2};
hash_digest hash_three {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3};

static chain_state::data get_data() {
    chain_state::data value;
    value.height = 1;
    value.bits = { 0, { 0 } };
    value.version = { 1, { 0 } };
    value.timestamp = { 0, 0, { 0 } };
    return value;
}

void add_state(transaction& tx) {
    tx.validation.state = std::make_shared<chain_state>(
#ifdef BITPRIM_CURRENCY_BCH
        chain_state{ get_data(), {}, 0, 0, 0 });
#else
        chain_state{ get_data(), {}, 0 });
#endif //BITPRIM_CURRENCY_BCH
}

transaction get_tx(std::string const& hex) {
    data_chunk data;
    decode_base16(data, hex);
    auto tx = transaction::factory_from_data(data);
    add_state(tx);
    return tx;
}

transaction get_tx_from_mempool(mining::mempool const& mp, std::unordered_map<chain::point, chain::output> const& internal_utxo, std::string const& hex) {
    data_chunk data;
    decode_base16(data, hex);
    auto tx = transaction::factory_from_data(data);
    add_state(tx);

    for (auto& i : tx.inputs()) {
        auto o = mp.get_utxo(i.previous_output());
        if (o.is_valid()) {
            i.previous_output().validation.from_mempool = true;
            i.previous_output().validation.cache = o;
        } else {
            // std::cout << "Not Found: " encode_base16(tx.to_data(true, BITPRIM_WITNESS_DEFAULT)) << std::endl;

            i.previous_output().validation.from_mempool = false;

            auto it = internal_utxo.find(i.previous_output());
            if (it != internal_utxo.end()) {
                std::cout << "Found in Internal UTXO: " << encode_hash(i.previous_output().hash()) << ":" << i.previous_output().index() << std::endl;
                i.previous_output().validation.cache = it->second;
            } else {
                std::cout << "Not Found: " << encode_hash(i.previous_output().hash()) << ":" << i.previous_output().index() << std::endl;
            }
        }
    }
    
    return tx;
}


TEST_CASE("[mempool] add one transaction") {
    // Transaction included in Block #80000:
    // https://blockdozer.com/tx/5a4ebf66822b0b2d56bd9dc64ece0bc38ee7844a23ff1d7320a88c5fdb2ad3e2
    auto tx = get_tx("0100000001a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f5000000004948304502206e21798a42fae0e854281abd38bacd1aeed3ee3738d9e1446618c4571d1090db022100e2ac980643b0b82c0e88ffdfec6b64e3e6ba35e7ba5fdd7d5d6cc8d25c6b241501ffffffff0100f2052a010000001976a914404371705fa9bd789a2fcd52d2c580b65d35549d88ac00000000");
    tx.inputs()[0].previous_output().validation.cache = output{17,  script{}};

    mempool mp;
    REQUIRE(mp.add(tx) == error::success);
    REQUIRE(mp.all_transactions() == 1);
    REQUIRE(mp.candidate_transactions() == 1);
    REQUIRE(mp.candidate_bytes() == tx.to_data(true, BITPRIM_WITNESS_DEFAULT).size());
}

TEST_CASE("[mempool] duplicated transactions") {
    // Transaction included in Block #80000:
    // https://blockdozer.com/tx/5a4ebf66822b0b2d56bd9dc64ece0bc38ee7844a23ff1d7320a88c5fdb2ad3e2
    auto tx = get_tx("0100000001a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f5000000004948304502206e21798a42fae0e854281abd38bacd1aeed3ee3738d9e1446618c4571d1090db022100e2ac980643b0b82c0e88ffdfec6b64e3e6ba35e7ba5fdd7d5d6cc8d25c6b241501ffffffff0100f2052a010000001976a914404371705fa9bd789a2fcd52d2c580b65d35549d88ac00000000");
    tx.inputs()[0].previous_output().validation.cache = output{17,  script{}};

    mempool mp;
    REQUIRE(mp.add(tx) == error::success);
    REQUIRE(mp.all_transactions() == 1);
    REQUIRE(mp.candidate_transactions() == 1);
    REQUIRE(mp.candidate_bytes() == tx.to_data(true, BITPRIM_WITNESS_DEFAULT).size());

    auto res = mp.add(tx);
    REQUIRE(res != error::success);
    REQUIRE(res == error::duplicate_transaction);
}

TEST_CASE("[mempool] chained transactions") {
    // Transaction included in Block #80000:
    // https://blockdozer.com/tx/5a4ebf66822b0b2d56bd9dc64ece0bc38ee7844a23ff1d7320a88c5fdb2ad3e2
    auto tx = get_tx("0100000001a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f5000000004948304502206e21798a42fae0e854281abd38bacd1aeed3ee3738d9e1446618c4571d1090db022100e2ac980643b0b82c0e88ffdfec6b64e3e6ba35e7ba5fdd7d5d6cc8d25c6b241501ffffffff0100f2052a010000001976a914404371705fa9bd789a2fcd52d2c580b65d35549d88ac00000000");

    // Transaction included in Block #80311:
    // https://blockdozer.com/tx/0c04096b6500773010eb042da00b9b2224afc63fe42f3379bfb1fecd4f528c5f/
    auto spender = get_tx("0100000004928050ac066c66aa8e84d1c8f93467a858d9ddadeec3628052427c5c4286e747000000008c49304602210080ce7a5203367ac53c28ed5aa8f940bfe280adfc4325840a097f0d779de87d06022100865c6574721a79fd9423d27aa6c5e14efaada9ed17f825697902701121915da1014104c5449df304c97850e294fc9c878db9ebb3039eeace22cf0a47eeee3da0a650623c997729dfba7b0ffd6b3cf076a209776b65ff604bfa4ca8ee83cbdd15348cceffffffffe2d32adb5f8ca820731dff234a84e78ec30bce4ec69dbd562d0b2b8266bf4e5a000000008c493046022100c0e77d0b559d2c3c18af307509faefe5d646714755024be062d82a0eeaf6258e022100ed3ebfe3fd60f087870f69c96c557b1fd6ca7007c373990de4a84df30442f889014104d4fb35c2cdb822644f1057e9bd07e3d3b0a36702662327ef4eb799eb219856d0fd884fce43082b73424a3293837c5f94a478f7bc4ec4da82bfb7e0b43fb218ccffffffff38363a089fc60d3e22e20b302b817283e55f121c8186c5157ada710bc730e98c000000008a4730440220554913ac5c016c36f9c155c69bc82ca0516c40593c0ad9256930afafd182efe602207a8c52f7ba926b151fef8b4dcb2223982cfc1b785e9893eb6c24bc7b8d599caf0141048d8c9eec2bd11a6e307a0c2d0b2731438b9cab19edfc6f85a9e559fd2b62b3c65ce250d26c324fcc7faf909c3c4956f58c5963539dd6f765b5e6100d4c3d8c0fffffffff473cf152cfa4100efff3b7dfa220b48a772b0e612ca4f06fa9c0a6c9d5f9644e010000008b483045022100afb4404ff9f694466455d97df15606c784392cb34f506c414c9398385a52873b0220178deba3a523b8a6111875c473827c62f09803295c2327d86897cc8087f134aa014104c44fb05a3a999dcdd8e02418f8f86d7624ee13103c799f27310d8e0a8f3d06f97688536b7b9a0b5a252176288251e3670190caa5a074e96165841f9f83f404c8ffffffff0100205fa0120000001976a9140e15309b21d1e5769104588d4e72dda7834728e388ac00000000");

    tx.inputs()[0].previous_output().validation.cache = output{17,  script{}};

    spender.inputs()[0].previous_output().validation.cache = output{17, script{}};
    spender.inputs()[1].previous_output().validation.cache = output{17, script{}};
    spender.inputs()[1].previous_output().validation.from_mempool = true;
    spender.inputs()[2].previous_output().validation.cache = output{17, script{}};
    spender.inputs()[3].previous_output().validation.cache = output{17, script{}};

    mempool mp;
    REQUIRE(mp.add(tx) == error::success);
    REQUIRE(mp.all_transactions() == 1);
    REQUIRE(mp.candidate_transactions() == 1);
    REQUIRE(mp.candidate_bytes() == tx.to_data(true, BITPRIM_WITNESS_DEFAULT).size());

    REQUIRE(mp.add(spender) == error::success);
    REQUIRE(mp.all_transactions() == 2);
    REQUIRE(mp.candidate_transactions() == 2);
    REQUIRE(mp.candidate_bytes() == tx.to_data(true, BITPRIM_WITNESS_DEFAULT).size() 
                                  + spender.to_data(true, BITPRIM_WITNESS_DEFAULT).size());
}

// TEST_CASE("[mempool] double spend mempool") {
//     // Transaction included in Block #80000:
//     // https://blockdozer.com/tx/5a4ebf66822b0b2d56bd9dc64ece0bc38ee7844a23ff1d7320a88c5fdb2ad3e2
//     auto tx = get_tx("0100000001a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f5000000004948304502206e21798a42fae0e854281abd38bacd1aeed3ee3738d9e1446618c4571d1090db022100e2ac980643b0b82c0e88ffdfec6b64e3e6ba35e7ba5fdd7d5d6cc8d25c6b241501ffffffff0100f2052a010000001976a914404371705fa9bd789a2fcd52d2c580b65d35549d88ac00000000");

//     // Transaction included in Block #80311:
//     // https://blockdozer.com/tx/0c04096b6500773010eb042da00b9b2224afc63fe42f3379bfb1fecd4f528c5f/
//     auto spender = get_tx("0100000004928050ac066c66aa8e84d1c8f93467a858d9ddadeec3628052427c5c4286e747000000008c49304602210080ce7a5203367ac53c28ed5aa8f940bfe280adfc4325840a097f0d779de87d06022100865c6574721a79fd9423d27aa6c5e14efaada9ed17f825697902701121915da1014104c5449df304c97850e294fc9c878db9ebb3039eeace22cf0a47eeee3da0a650623c997729dfba7b0ffd6b3cf076a209776b65ff604bfa4ca8ee83cbdd15348cceffffffffe2d32adb5f8ca820731dff234a84e78ec30bce4ec69dbd562d0b2b8266bf4e5a000000008c493046022100c0e77d0b559d2c3c18af307509faefe5d646714755024be062d82a0eeaf6258e022100ed3ebfe3fd60f087870f69c96c557b1fd6ca7007c373990de4a84df30442f889014104d4fb35c2cdb822644f1057e9bd07e3d3b0a36702662327ef4eb799eb219856d0fd884fce43082b73424a3293837c5f94a478f7bc4ec4da82bfb7e0b43fb218ccffffffff38363a089fc60d3e22e20b302b817283e55f121c8186c5157ada710bc730e98c000000008a4730440220554913ac5c016c36f9c155c69bc82ca0516c40593c0ad9256930afafd182efe602207a8c52f7ba926b151fef8b4dcb2223982cfc1b785e9893eb6c24bc7b8d599caf0141048d8c9eec2bd11a6e307a0c2d0b2731438b9cab19edfc6f85a9e559fd2b62b3c65ce250d26c324fcc7faf909c3c4956f58c5963539dd6f765b5e6100d4c3d8c0fffffffff473cf152cfa4100efff3b7dfa220b48a772b0e612ca4f06fa9c0a6c9d5f9644e010000008b483045022100afb4404ff9f694466455d97df15606c784392cb34f506c414c9398385a52873b0220178deba3a523b8a6111875c473827c62f09803295c2327d86897cc8087f134aa014104c44fb05a3a999dcdd8e02418f8f86d7624ee13103c799f27310d8e0a8f3d06f97688536b7b9a0b5a252176288251e3670190caa5a074e96165841f9f83f404c8ffffffff0100205fa0120000001976a9140e15309b21d1e5769104588d4e72dda7834728e388ac00000000");

//     //Modified the first byte
//     auto spender_fake = get_tx("0200000004928050ac066c66aa8e84d1c8f93467a858d9ddadeec3628052427c5c4286e747000000008c49304602210080ce7a5203367ac53c28ed5aa8f940bfe280adfc4325840a097f0d779de87d06022100865c6574721a79fd9423d27aa6c5e14efaada9ed17f825697902701121915da1014104c5449df304c97850e294fc9c878db9ebb3039eeace22cf0a47eeee3da0a650623c997729dfba7b0ffd6b3cf076a209776b65ff604bfa4ca8ee83cbdd15348cceffffffffe2d32adb5f8ca820731dff234a84e78ec30bce4ec69dbd562d0b2b8266bf4e5a000000008c493046022100c0e77d0b559d2c3c18af307509faefe5d646714755024be062d82a0eeaf6258e022100ed3ebfe3fd60f087870f69c96c557b1fd6ca7007c373990de4a84df30442f889014104d4fb35c2cdb822644f1057e9bd07e3d3b0a36702662327ef4eb799eb219856d0fd884fce43082b73424a3293837c5f94a478f7bc4ec4da82bfb7e0b43fb218ccffffffff38363a089fc60d3e22e20b302b817283e55f121c8186c5157ada710bc730e98c000000008a4730440220554913ac5c016c36f9c155c69bc82ca0516c40593c0ad9256930afafd182efe602207a8c52f7ba926b151fef8b4dcb2223982cfc1b785e9893eb6c24bc7b8d599caf0141048d8c9eec2bd11a6e307a0c2d0b2731438b9cab19edfc6f85a9e559fd2b62b3c65ce250d26c324fcc7faf909c3c4956f58c5963539dd6f765b5e6100d4c3d8c0fffffffff473cf152cfa4100efff3b7dfa220b48a772b0e612ca4f06fa9c0a6c9d5f9644e010000008b483045022100afb4404ff9f694466455d97df15606c784392cb34f506c414c9398385a52873b0220178deba3a523b8a6111875c473827c62f09803295c2327d86897cc8087f134aa014104c44fb05a3a999dcdd8e02418f8f86d7624ee13103c799f27310d8e0a8f3d06f97688536b7b9a0b5a252176288251e3670190caa5a074e96165841f9f83f404c8ffffffff0100205fa0120000001976a9140e15309b21d1e5769104588d4e72dda7834728e388ac00000000");

//     tx.inputs()[0].previous_output().validation.cache = output{17,  script{}};
//     spender.inputs()[0].previous_output().validation.cache = output{17, script{}};
//     spender.inputs()[1].previous_output().validation.cache = output{17, script{}};
//     spender.inputs()[1].previous_output().validation.from_mempool = true;
//     spender.inputs()[2].previous_output().validation.cache = output{17, script{}};
//     spender.inputs()[3].previous_output().validation.cache = output{17, script{}};


//     spender_fake.inputs()[0].previous_output().validation.cache = output{17, script{}};
//     spender_fake.inputs()[1].previous_output().validation.cache = output{17, script{}};
//     spender_fake.inputs()[1].previous_output().validation.from_mempool = true;
//     spender_fake.inputs()[2].previous_output().validation.cache = output{17, script{}};
//     spender_fake.inputs()[3].previous_output().validation.cache = output{17, script{}};

//     mempool mp;
//     REQUIRE(mp.add(tx) == error::success);
//     REQUIRE(mp.all_transactions() == 1);
//     REQUIRE(mp.candidate_transactions() == 1);
//     REQUIRE(mp.candidate_bytes() == tx.to_data(true, BITPRIM_WITNESS_DEFAULT).size());

//     REQUIRE(mp.add(spender) == error::success);
//     REQUIRE(mp.all_transactions() == 2);
//     REQUIRE(mp.candidate_transactions() == 2);
//     REQUIRE(mp.candidate_bytes() == tx.to_data(true, BITPRIM_WITNESS_DEFAULT).size() 
//                                   + spender.to_data(true, BITPRIM_WITNESS_DEFAULT).size());


//     // auto internal_utxo_set_ = mp.internal_utxo_set_;
//     // auto all_transactions_ = mp.all_transactions_;
//     // auto hash_index_ = mp.hash_index_;
//     // auto candidate_transactions_ = mp.candidate_transactions_;

//     REQUIRE(mp.add(spender_fake) == error::double_spend_mempool);
//     REQUIRE(mp.all_transactions() == 2);
//     REQUIRE(mp.candidate_transactions() == 2);
//     REQUIRE(mp.candidate_bytes() == tx.to_data(true, BITPRIM_WITNESS_DEFAULT).size() 
//                                   + spender.to_data(true, BITPRIM_WITNESS_DEFAULT).size());

//     // REQUIRE(internal_utxo_set_ == mp.internal_utxo_set_);
//     // // REQUIRE(all_transactions_ == mp.all_transactions_);
//     // REQUIRE(hash_index_ == mp.hash_index_);
//     // REQUIRE(candidate_transactions_ == mp.candidate_transactions_);

// }

TEST_CASE("[mempool] replace TX Candidate for a better one") {

    transaction coinbase0 {1, 1, {input{output_point{null_hash, point::null_index}, script{}, 1}}, {output{17, script{}}}};
    add_state(coinbase0);
    REQUIRE(coinbase0.is_valid());
    REQUIRE(coinbase0.is_coinbase());

    transaction coinbase1 {1, 1, {input{output_point{null_hash, point::null_index}, script{}, 1}}, {output{34, script{}}}};
    add_state(coinbase1);
    REQUIRE(coinbase1.is_valid());
    REQUIRE(coinbase1.is_coinbase());

    transaction spender0 {1, 1, {input{output_point{coinbase0.hash(), 0}, script{}, 1}}, {output{10, script{}}}};
    add_state(spender0);
    spender0.inputs()[0].previous_output().validation.cache = coinbase0.outputs()[0];
    spender0.inputs()[0].previous_output().validation.from_mempool = false;

    REQUIRE(spender0.is_valid());
    REQUIRE(spender0.fees() == 7);

    transaction spender1 {1, 1, {input{output_point{coinbase1.hash(), 0}, script{}, 1}}, {output{10, script{}}}};
    add_state(spender1);
    spender1.inputs()[0].previous_output().validation.cache = coinbase1.outputs()[0];
    spender1.inputs()[0].previous_output().validation.from_mempool = false;


    REQUIRE(spender1.is_valid());
    REQUIRE(spender1.fees() == 24);

    mempool mp(spender0.serialized_size());

    REQUIRE(mp.add(spender0) == error::success);
    REQUIRE(mp.contains(spender0.hash()));
    REQUIRE(mp.is_candidate(spender0));

    REQUIRE(mp.add(spender1) == error::success);
    REQUIRE(mp.contains(spender1.hash()));
    REQUIRE(mp.is_candidate(spender1));
    REQUIRE(mp.contains(spender0.hash()));
    REQUIRE( ! mp.is_candidate(spender0));
}

TEST_CASE("[mempool] Try to insert a Low Benefit Transaction") {

    transaction coinbase0 {1, 1, {input{output_point{null_hash, point::null_index}, script{}, 1}}, {output{17, script{}}}};
    add_state(coinbase0);
    REQUIRE(coinbase0.is_valid());
    REQUIRE(coinbase0.is_coinbase());

    transaction coinbase1 {1, 1, {input{output_point{null_hash, point::null_index}, script{}, 1}}, {output{34, script{}}}};
    add_state(coinbase1);
    REQUIRE(coinbase1.is_valid());
    REQUIRE(coinbase1.is_coinbase());

    transaction spender0 {1, 1, {input{output_point{coinbase0.hash(), 0}, script{}, 1}}, {output{10, script{}}}};
    add_state(spender0);
    spender0.inputs()[0].previous_output().validation.cache = coinbase0.outputs()[0];
    spender0.inputs()[0].previous_output().validation.from_mempool = false;

    REQUIRE(spender0.is_valid());
    REQUIRE(spender0.fees() == 7);

    transaction spender1 {1, 1, {input{output_point{coinbase1.hash(), 0}, script{}, 1}}, {output{33, script{}}}};
    add_state(spender1);
    spender1.inputs()[0].previous_output().validation.cache = coinbase1.outputs()[0];
    spender1.inputs()[0].previous_output().validation.from_mempool = false;

    REQUIRE(spender1.is_valid());
    REQUIRE(spender1.fees() == 1);

    mempool mp(spender0.serialized_size());

    auto res = mp.add(spender0);

    REQUIRE(res == error::success);
    REQUIRE(mp.contains(spender0.hash()));
    REQUIRE(mp.is_candidate(spender0));

    REQUIRE(mp.add(spender1) == error::low_benefit_transaction);
    REQUIRE(mp.contains(spender1.hash()));
    REQUIRE( ! mp.is_candidate(spender1));
    REQUIRE(mp.contains(spender0.hash()));
    REQUIRE(mp.is_candidate(spender0));
}

/*
    A ... A
    B ... B
    C ... C
    D ... D
*/
TEST_CASE("[mempool] Dependencies 0") {

    transaction coinbase0 {1, 1, {input{output_point{null_hash, point::null_index}, script{}, 1}}, {output{50, script{}}}};
    add_state(coinbase0);
    REQUIRE(coinbase0.is_valid());
    REQUIRE(coinbase0.is_coinbase());

    transaction a {1, 1, {input{output_point{coinbase0.hash(), 0}, script{}, 1}}, {output{40, script{}}}};
    add_state(a);
    a.inputs()[0].previous_output().validation.cache = coinbase0.outputs()[0];
    a.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(a.is_valid());
    REQUIRE(a.fees() == 10);

    transaction b {1, 1, {input{output_point{a.hash(), 0}, script{}, 1}}, {output{31, script{}}}};
    add_state(b);
    b.inputs()[0].previous_output().validation.cache = a.outputs()[0];
    b.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(b.is_valid());
    REQUIRE(b.fees() == 9);

    transaction c {1, 1, {input{output_point{b.hash(), 0}, script{}, 1}}, {output{23, script{}}}};
    add_state(c);
    c.inputs()[0].previous_output().validation.cache = b.outputs()[0];
    c.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(c.is_valid());
    REQUIRE(c.fees() == 8);

    transaction d {1, 1, {input{output_point{c.hash(), 0}, script{}, 1}}, {output{16, script{}}}};
    add_state(d);
    d.inputs()[0].previous_output().validation.cache = c.outputs()[0];
    d.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(d.is_valid());
    REQUIRE(d.fees() == 7);

    mempool mp;
    REQUIRE(mp.add(a) == error::success);
    REQUIRE(mp.add(b) == error::success);
    REQUIRE(mp.add(c) == error::success);
    REQUIRE(mp.add(d) == error::success);

    REQUIRE(mp.all_transactions() == 4);
    REQUIRE(mp.candidate_transactions() == 4);
    REQUIRE(mp.candidate_bytes() == 4 * 60);
    REQUIRE(mp.candidate_fees() == a.fees() + b.fees() + c.fees() + d.fees());

    REQUIRE(mp.candidate_rank(a) == 0);
    REQUIRE(mp.candidate_rank(b) == 1);
    REQUIRE(mp.candidate_rank(c) == 2);
    REQUIRE(mp.candidate_rank(d) == 3);
}

/*
    A ... D
    B ... C
    C ... B
    D ... A
*/
TEST_CASE("[mempool] Dependencies 1") {

    transaction coinbase0 {1, 1, {input{output_point{null_hash, point::null_index}, script{}, 1}}, {output{50, script{}}}};
    add_state(coinbase0);
    REQUIRE(coinbase0.is_valid());
    REQUIRE(coinbase0.is_coinbase());

    transaction a {1, 1, {input{output_point{coinbase0.hash(), 0}, script{}, 1}}, {output{43, script{}}}};
    add_state(a);
    a.inputs()[0].previous_output().validation.cache = coinbase0.outputs()[0];
    a.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(a.is_valid());
    REQUIRE(a.fees() == 7);

    transaction b {1, 1, {input{output_point{a.hash(), 0}, script{}, 1}}, {output{35, script{}}}};
    add_state(b);
    b.inputs()[0].previous_output().validation.cache = a.outputs()[0];
    b.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(b.is_valid());
    REQUIRE(b.fees() == 8);

    transaction c {1, 1, {input{output_point{b.hash(), 0}, script{}, 1}}, {output{26, script{}}}};
    add_state(c);
    c.inputs()[0].previous_output().validation.cache = b.outputs()[0];
    c.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(c.is_valid());
    REQUIRE(c.fees() == 9);

    transaction d {1, 1, {input{output_point{c.hash(), 0}, script{}, 1}}, {output{16, script{}}}};
    add_state(d);
    d.inputs()[0].previous_output().validation.cache = c.outputs()[0];
    d.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(d.is_valid());
    REQUIRE(d.fees() == 10);

    mempool mp;
    REQUIRE(mp.add(a) == error::success);
    REQUIRE(mp.add(b) == error::success);
    REQUIRE(mp.add(c) == error::success);
    REQUIRE(mp.add(d) == error::success);

    REQUIRE(mp.all_transactions() == 4);
    REQUIRE(mp.candidate_transactions() == 4);
    REQUIRE(mp.candidate_bytes() == 4 * 60);
    REQUIRE(mp.candidate_fees() == a.fees() + b.fees() + c.fees() + d.fees());

    REQUIRE(mp.candidate_rank(a) == 3);
    REQUIRE(mp.candidate_rank(b) == 2);
    REQUIRE(mp.candidate_rank(c) == 1);
    REQUIRE(mp.candidate_rank(d) == 0);
}

/*
    A ... D
    B ... C
    C ... B
    D ... A

    E (better than the "pack")
*/
TEST_CASE("[mempool] Dependencies 2") {

    transaction coinbase0 {1, 1, {input{output_point{null_hash, point::null_index}, script{}, 1}}, {output{50, script{}}}};
    add_state(coinbase0);
    REQUIRE(coinbase0.is_valid());
    REQUIRE(coinbase0.is_coinbase());

    transaction a {1, 1, {input{output_point{coinbase0.hash(), 0}, script{}, 1}}, {output{43, script{}}}};
    add_state(a);
    a.inputs()[0].previous_output().validation.cache = coinbase0.outputs()[0];
    a.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(a.is_valid());
    REQUIRE(a.fees() == 7);

    transaction b {1, 1, {input{output_point{a.hash(), 0}, script{}, 1}}, {output{35, script{}}}};
    add_state(b);
    b.inputs()[0].previous_output().validation.cache = a.outputs()[0];
    b.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(b.is_valid());
    REQUIRE(b.fees() == 8);

    transaction c {1, 1, {input{output_point{b.hash(), 0}, script{}, 1}}, {output{26, script{}}}};
    add_state(c);
    c.inputs()[0].previous_output().validation.cache = b.outputs()[0];
    c.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(c.is_valid());
    REQUIRE(c.fees() == 9);

    transaction d {1, 1, {input{output_point{c.hash(), 0}, script{}, 1}}, {output{16, script{}}}};
    add_state(d);
    d.inputs()[0].previous_output().validation.cache = c.outputs()[0];
    d.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(d.is_valid());
    REQUIRE(d.fees() == 10);

    mempool mp(4 * 60);
    REQUIRE(mp.add(a) == error::success);
    REQUIRE(mp.add(b) == error::success);
    REQUIRE(mp.add(c) == error::success);
    REQUIRE(mp.add(d) == error::success);

    REQUIRE(mp.all_transactions() == 4);
    REQUIRE(mp.candidate_transactions() == 4);
    REQUIRE(mp.candidate_bytes() == 4 * 60);
    REQUIRE(mp.candidate_fees() == a.fees() + b.fees() + c.fees() + d.fees());

    REQUIRE(mp.candidate_rank(a) == 3);
    REQUIRE(mp.candidate_rank(b) == 2);
    REQUIRE(mp.candidate_rank(c) == 1);
    REQUIRE(mp.candidate_rank(d) == 0);


    transaction e {1, 1, {input{output_point{null_hash, 0}, script{}, 1}}, {output{15, script{}}}};
    add_state(e);
    e.inputs()[0].previous_output().validation.cache = output{26, script{}};
    e.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(e.is_valid());
    REQUIRE(e.fees() == 11);

    REQUIRE(mp.add(e) == error::success);

    REQUIRE(mp.all_transactions() == 5);
    REQUIRE(mp.candidate_transactions() == 1);
    REQUIRE(mp.candidate_bytes() == 1 * 60);
    REQUIRE(mp.candidate_fees() == e.fees());

    REQUIRE(mp.candidate_rank(a) == null_index);
    REQUIRE(mp.candidate_rank(b) == null_index);
    REQUIRE(mp.candidate_rank(c) == null_index);
    REQUIRE(mp.candidate_rank(d) == null_index);
    REQUIRE(mp.candidate_rank(e) == 0);
}


/*
???
*/
TEST_CASE("[mempool] Dependencies 3") {

    transaction coinbase0 {1, 1, {input{output_point{null_hash, point::null_index}, script{}, 1}}, {output{50, script{}}}};
    add_state(coinbase0);
    REQUIRE(coinbase0.is_valid());
    REQUIRE(coinbase0.is_coinbase());

    transaction a {1, 1, {input{output_point{coinbase0.hash(), 0}, script{}, 1}}, {output{43, script{}}}};
    add_state(a);
    a.inputs()[0].previous_output().validation.cache = coinbase0.outputs()[0];
    a.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(a.is_valid());
    REQUIRE(a.fees() == 7);

    transaction b {1, 1, {input{output_point{a.hash(), 0}, script{}, 1}}, {output{35, script{}}}};
    add_state(b);
    b.inputs()[0].previous_output().validation.cache = a.outputs()[0];
    b.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(b.is_valid());
    REQUIRE(b.fees() == 8);

    mempool mp(4 * 60);
    REQUIRE(mp.add(a) == error::success);
    REQUIRE(mp.add(b) == error::success);

    REQUIRE(mp.all_transactions() == 2);
    REQUIRE(mp.candidate_transactions() == 2);
    REQUIRE(mp.candidate_bytes() == 2 * 60);
    REQUIRE(mp.candidate_fees() == a.fees() + b.fees());

    REQUIRE(mp.candidate_rank(a) == 1);
    REQUIRE(mp.candidate_rank(b) == 0);

    transaction z {1, 1, {input{output_point{null_hash, 0}, script{}, 1}}, {output{19, script{}}}};
    add_state(z);
    z.inputs()[0].previous_output().validation.cache = output{26, script{}};
    z.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(z.is_valid());
    REQUIRE(z.fees() == 7);
    
    REQUIRE(mp.add(z) == error::success);
    REQUIRE(mp.all_transactions() == 3);
    REQUIRE(mp.candidate_transactions() == 3);
    REQUIRE(mp.candidate_bytes() == 3 * 60);
    REQUIRE(mp.candidate_fees() == a.fees() + b.fees() + z.fees());

    REQUIRE(mp.candidate_rank(a) == 1);
    REQUIRE(mp.candidate_rank(b) == 0);
    REQUIRE(mp.candidate_rank(z) == 2);

    transaction c {1, 1, {input{output_point{b.hash(), 0}, script{}, 1}}, {output{34, script{}}}};
    add_state(c);
    c.inputs()[0].previous_output().validation.cache = b.outputs()[0];
    c.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(c.is_valid());
    REQUIRE(c.fees() == 1);

    REQUIRE(mp.add(c) == error::success);
    REQUIRE(mp.all_transactions() == 4);
    REQUIRE(mp.candidate_transactions() == 4);
    REQUIRE(mp.candidate_bytes() == 4 * 60);
    REQUIRE(mp.candidate_fees() ==  a.fees() + b.fees() + z.fees() + c.fees());

    REQUIRE(mp.candidate_rank(a) == 1);
    REQUIRE(mp.candidate_rank(b) == 2);
    REQUIRE(mp.candidate_rank(z) == 0);
    REQUIRE(mp.candidate_rank(c) == 3);
}

/*
???
*/
TEST_CASE("[mempool] Dependencies 4") {

    transaction x {1, 1, {input{output_point{null_hash, 0}, script{}, 1}}, {output{48, script{}}}};
    add_state(x);
    x.inputs()[0].previous_output().validation.cache = output{50, script{}};
    x.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(x.is_valid());
    REQUIRE(x.fees() == 2);

    transaction y {1, 1, {input{output_point{hash_one, 0}, script{}, 1}}, {output{49, script{}}}};
    add_state(y);
    y.inputs()[0].previous_output().validation.cache = output{50, script{}};
    y.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(y.is_valid());
    REQUIRE(y.fees() == 1);

    transaction z {1, 1, {input{output_point{hash_two, 0}, script{}, 1}}, {output{47, script{}}}};
    add_state(z);
    z.inputs()[0].previous_output().validation.cache = output{50, script{}};
    z.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(z.is_valid());
    REQUIRE(z.fees() == 3);


    mempool mp(3 * 60);
    REQUIRE(mp.add(x) == error::success);
    REQUIRE(mp.add(y) == error::success);
    REQUIRE(mp.add(z) == error::success);

    REQUIRE(mp.all_transactions() == 3);
    REQUIRE(mp.candidate_transactions() == 3);
    REQUIRE(mp.candidate_bytes() == 3 * 60);
    REQUIRE(mp.candidate_fees() == x.fees() + y.fees() + z.fees());

    REQUIRE(mp.candidate_rank(x) == 1);
    REQUIRE(mp.candidate_rank(y) == 2);
    REQUIRE(mp.candidate_rank(z) == 0);



    transaction a {1, 1, {input{output_point{hash_three, 0}, script{}, 1}}, {output{50, script{}}}};
    add_state(a);
    a.inputs()[0].previous_output().validation.cache = output{50, script{}};
    a.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(a.is_valid());
    REQUIRE(a.fees() == 0);

    transaction b {1, 1, {input{output_point{a.hash(), 0}, script{}, 1}}, {output{50, script{}}}};
    add_state(b);
    b.inputs()[0].previous_output().validation.cache = a.outputs()[0];
    b.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(b.is_valid());
    REQUIRE(b.fees() == 0);

    REQUIRE(mp.add(a) == error::low_benefit_transaction);
    REQUIRE(mp.add(b) == error::low_benefit_transaction);

    REQUIRE(mp.all_transactions() == 5);
    REQUIRE(mp.candidate_transactions() == 3);
    REQUIRE(mp.candidate_bytes() == 3 * 60);
    REQUIRE(mp.candidate_fees() == x.fees() + y.fees() + z.fees());

    REQUIRE(mp.candidate_rank(x) == 1);
    REQUIRE(mp.candidate_rank(y) == 2);
    REQUIRE(mp.candidate_rank(z) == 0);
    REQUIRE(mp.candidate_rank(a) == null_index);
    REQUIRE(mp.candidate_rank(b) == null_index);


    transaction c {1, 1, {input{output_point{b.hash(), 0}, script{}, 1}}, {output{40, script{}}}};
    add_state(c);
    c.inputs()[0].previous_output().validation.cache = b.outputs()[0];
    c.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(c.is_valid());
    REQUIRE(c.fees() == 10);

    //TODO(fernando): check this case...
    auto res = mp.add(c);
    REQUIRE(res == error::success);

    REQUIRE(mp.all_transactions() == 6);
    REQUIRE(mp.candidate_transactions() == 3);
    REQUIRE(mp.candidate_bytes() == 3 * 60);
    REQUIRE(mp.candidate_fees() == a.fees() + b.fees() + c.fees());

    REQUIRE(mp.candidate_rank(x) == null_index);
    REQUIRE(mp.candidate_rank(y) == null_index);
    REQUIRE(mp.candidate_rank(z) == null_index);
    REQUIRE(mp.candidate_rank(a) == 2);
    REQUIRE(mp.candidate_rank(b) == 1);
    REQUIRE(mp.candidate_rank(c) == 0);
}


/*
???
*/
TEST_CASE("[mempool] Dependencies 4b") {

    transaction x {1, 1, {input{output_point{null_hash, 0}, script{}, 1}}, {output{48, script{}}}};
    add_state(x);
    x.inputs()[0].previous_output().validation.cache = output{50, script{}};
    x.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(x.is_valid());
    REQUIRE(x.fees() == 2);

    transaction y {1, 1, {input{output_point{x.hash(), 0}, script{}, 1}}, {output{47, script{}}}};
    add_state(y);
    y.inputs()[0].previous_output().validation.cache = x.outputs()[0];
    y.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(y.is_valid());
    REQUIRE(y.fees() == 1);

    transaction z {1, 1, {input{output_point{y.hash(), 0}, script{}, 1}}, {output{44, script{}}}};
    add_state(z);
    z.inputs()[0].previous_output().validation.cache = y.outputs()[0];
    z.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(z.is_valid());
    REQUIRE(z.fees() == 3);


    mempool mp(3 * 60);
    REQUIRE(mp.add(x) == error::success);
    REQUIRE(mp.add(y) == error::success);
    REQUIRE(mp.add(z) == error::success);

    REQUIRE(mp.all_transactions() == 3);
    REQUIRE(mp.candidate_transactions() == 3);
    REQUIRE(mp.candidate_bytes() == 3 * 60);
    REQUIRE(mp.candidate_fees() == x.fees() + y.fees() + z.fees());

    REQUIRE(mp.candidate_rank(x) == 2);
    REQUIRE(mp.candidate_rank(y) == 1);
    REQUIRE(mp.candidate_rank(z) == 0);



    transaction a {1, 1, {input{output_point{hash_one, 0}, script{}, 1}}, {output{50, script{}}}};
    add_state(a);
    a.inputs()[0].previous_output().validation.cache = output{50, script{}};
    a.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(a.is_valid());
    REQUIRE(a.fees() == 0);

    transaction b {1, 1, {input{output_point{a.hash(), 0}, script{}, 1}}, {output{50, script{}}}};
    add_state(b);
    b.inputs()[0].previous_output().validation.cache = a.outputs()[0];
    b.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(b.is_valid());
    REQUIRE(b.fees() == 0);

    REQUIRE(mp.add(a) == error::low_benefit_transaction);
    REQUIRE(mp.add(b) == error::low_benefit_transaction);

    REQUIRE(mp.all_transactions() == 5);
    REQUIRE(mp.candidate_transactions() == 3);
    REQUIRE(mp.candidate_bytes() == 3 * 60);
    REQUIRE(mp.candidate_fees() == x.fees() + y.fees() + z.fees());

    REQUIRE(mp.candidate_rank(x) == 2);
    REQUIRE(mp.candidate_rank(y) == 1);
    REQUIRE(mp.candidate_rank(z) == 0);
    REQUIRE(mp.candidate_rank(a) == null_index);
    REQUIRE(mp.candidate_rank(b) == null_index);


    transaction c {1, 1, {input{output_point{b.hash(), 0}, script{}, 1}}, {output{40, script{}}}};
    add_state(c);
    c.inputs()[0].previous_output().validation.cache = b.outputs()[0];
    c.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(c.is_valid());
    REQUIRE(c.fees() == 10);

    //TODO(fernando): check this case...
    auto res = mp.add(c);
    REQUIRE(res == error::success);

    REQUIRE(mp.all_transactions() == 6);
    REQUIRE(mp.candidate_transactions() == 3);
    REQUIRE(mp.candidate_bytes() == 3 * 60);
    REQUIRE(mp.candidate_fees() == a.fees() + b.fees() + c.fees());

    REQUIRE(mp.candidate_rank(x) == null_index);
    REQUIRE(mp.candidate_rank(y) == null_index);
    REQUIRE(mp.candidate_rank(z) == null_index);
    REQUIRE(mp.candidate_rank(a) == 2);
    REQUIRE(mp.candidate_rank(b) == 1);
    REQUIRE(mp.candidate_rank(c) == 0);
}

/*
    A, C
    B
*/
TEST_CASE("[mempool] Dependencies 5") {

    transaction a {1, 1, {input{output_point{null_hash, 0}, script{}, 1}}, {output{40, script{}}}};
    add_state(a);
    a.inputs()[0].previous_output().validation.cache = output{50, script{}};
    a.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(a.is_valid());
    REQUIRE(a.fees() == 10);

    transaction b {1, 1, {input{output_point{hash_one, 0}, script{}, 1}}, {output{41, script{}}}};
    add_state(b);
    b.inputs()[0].previous_output().validation.cache = output{50, script{}};
    b.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(b.is_valid());
    REQUIRE(b.fees() == 9);

    transaction c {1, 1, {input{output_point{a.hash(), 0}, script{}, 1}}, {output{39, script{}}}};
    add_state(c);
    c.inputs()[0].previous_output().validation.cache = a.outputs()[0];
    c.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(c.is_valid());
    REQUIRE(c.fees() == 1);

    mempool mp(2 * 60);
    REQUIRE(mp.add(a) == error::success);
    REQUIRE(mp.add(b) == error::success);

    REQUIRE(mp.all_transactions() == 2);
    REQUIRE(mp.candidate_transactions() == 2);
    REQUIRE(mp.candidate_bytes() == 2 * 60);
    REQUIRE(mp.candidate_fees() == a.fees() + b.fees());
    REQUIRE(mp.candidate_rank(a) == 0);
    REQUIRE(mp.candidate_rank(b) == 1);

    auto res = mp.add(c);
    REQUIRE(res == error::low_benefit_transaction);
    REQUIRE(mp.all_transactions() == 3);
    REQUIRE(mp.candidate_transactions() == 2);
    REQUIRE(mp.candidate_bytes() == 2 * 60);
    REQUIRE(mp.candidate_fees() == a.fees() + b.fees());
    REQUIRE(mp.candidate_rank(a) == 0);
    REQUIRE(mp.candidate_rank(b) == 1);
    REQUIRE(mp.candidate_rank(c) == null_index);
}

/*
    A, C
    B
*/
TEST_CASE("[mempool] Dependencies 6") {

    transaction a {1, 1, {input{output_point{null_hash, 0}, script{}, 1}}, {output{40, script{}}}};
    add_state(a);
    a.inputs()[0].previous_output().validation.cache = output{50, script{}};
    a.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(a.is_valid());
    REQUIRE(a.fees() == 10);

    transaction b {1, 1, {input{output_point{hash_one, 0}, script{}, 1}}, {output{41, script{}}}};
    add_state(b);
    b.inputs()[0].previous_output().validation.cache = output{50, script{}};
    b.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(b.is_valid());
    REQUIRE(b.fees() == 9);

    transaction c {1, 1, {input{output_point{a.hash(), 0}, script{}, 1}}, {output{39, script{}}}};
    add_state(c);
    c.inputs()[0].previous_output().validation.cache = a.outputs()[0];
    c.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(c.is_valid());
    REQUIRE(c.fees() == 1);

    mempool mp(3 * 60);
    REQUIRE(mp.add(a) == error::success);
    REQUIRE(mp.add(b) == error::success);

    REQUIRE(mp.all_transactions() == 2);
    REQUIRE(mp.candidate_transactions() == 2);
    REQUIRE(mp.candidate_bytes() == 2 * 60);
    REQUIRE(mp.candidate_fees() == a.fees() + b.fees());
    REQUIRE(mp.candidate_rank(a) == 0);
    REQUIRE(mp.candidate_rank(b) == 1);

    auto res = mp.add(c);
    REQUIRE(res == error::success);
    REQUIRE(mp.all_transactions() == 3);
    REQUIRE(mp.candidate_transactions() == 3);
    REQUIRE(mp.candidate_bytes() == 3 * 60);
    REQUIRE(mp.candidate_fees() == a.fees() + b.fees() + c.fees());
    REQUIRE(mp.candidate_rank(a) == 1);
    REQUIRE(mp.candidate_rank(b) == 0);
    REQUIRE(mp.candidate_rank(c) == 2);
}

TEST_CASE("[mempool] Double Spend Mempool") {

    // hash_digest aaa {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    transaction a {1, 1, {input{output_point{null_hash, 0}, script{}, 1}}, {output{40, script{}}}};
    add_state(a);
    a.inputs()[0].previous_output().validation.cache = output{50, script{}};
    a.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(a.is_valid());
    REQUIRE(a.fees() == 10);

    transaction b {1, 1, {input{output_point{a.hash(), 0}, script{}, 1}}, {output{31, script{}}}};
    add_state(b);
    b.inputs()[0].previous_output().validation.cache = a.outputs()[0];
    b.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(b.is_valid());
    REQUIRE(b.fees() == 9);

    transaction c {1, 1, {input{output_point{a.hash(), 0}, script{}, 1}}, {output{30, script{}}}};
    add_state(c);
    c.inputs()[0].previous_output().validation.cache = a.outputs()[0];
    c.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(c.is_valid());
    REQUIRE(c.fees() == 10);

    mempool mp;
    REQUIRE(mp.add(a) == error::success);
    REQUIRE(mp.add(b) == error::success);
    REQUIRE(mp.add(c) == error::double_spend_mempool);

    REQUIRE(mp.all_transactions() == 2);
    REQUIRE(mp.candidate_transactions() == 2);
}


TEST_CASE("[mempool] Double Spend Blockchain") {

    // hash_digest aaa {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    transaction a {1, 1, {input{output_point{hash_one, 0}, script{}, 1}}, {output{40, script{}}}};
    add_state(a);
    a.inputs()[0].previous_output().validation.cache = output{50, script{}};
    a.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(a.is_valid());
    REQUIRE(a.fees() == 10);

    transaction b {1, 1, {input{output_point{hash_one, 0}, script{}, 1}}, {output{41, script{}}}};
    add_state(b);
    b.inputs()[0].previous_output().validation.cache = output{50, script{}};
    b.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(b.is_valid());
    REQUIRE(b.fees() == 9);

    mempool mp;
    REQUIRE(mp.add(a) == error::success);
    REQUIRE(mp.add(b) == error::double_spend_blockchain);


}












/*

    A
B       C

    A
B       C
    D


A       B
    C


A       B
    C
D       E


A       B
    C
D       E
            F


    A       B
D       C
    
    
    E


*/



TEST_CASE("[mempool] Remove Transactions 0") {
    transaction x {1, 1, {input{output_point{null_hash, 0}, script{}, 1}}, {output{48, script{}}}};
    add_state(x);
    x.inputs()[0].previous_output().validation.cache = output{50, script{}};
    x.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(x.is_valid());
    REQUIRE(x.fees() == 2);

    transaction y {1, 1, {input{output_point{hash_one, 0}, script{}, 1}}, {output{49, script{}}}};
    add_state(y);
    y.inputs()[0].previous_output().validation.cache = output{50, script{}};
    y.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(y.is_valid());
    REQUIRE(y.fees() == 1);

    transaction z {1, 1, {input{output_point{hash_two, 0}, script{}, 1}}, {output{47, script{}}}};
    add_state(z);
    z.inputs()[0].previous_output().validation.cache = output{50, script{}};
    z.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(z.is_valid());
    REQUIRE(z.fees() == 3);


    mempool mp(3 * 60);
    REQUIRE(mp.add(x) == error::success);
    REQUIRE(mp.add(y) == error::success);
    REQUIRE(mp.add(z) == error::success);

    REQUIRE(mp.all_transactions() == 3);
    REQUIRE(mp.candidate_transactions() == 3);
    REQUIRE(mp.candidate_bytes() == 3 * 60);
    REQUIRE(mp.candidate_fees() == x.fees() + y.fees() + z.fees());

    REQUIRE(mp.candidate_rank(x) == 1);
    REQUIRE(mp.candidate_rank(y) == 2);
    REQUIRE(mp.candidate_rank(z) == 0);


    std::vector<transaction> remove {x, y, z};
    mp.remove(remove.begin(), remove.end(), 0);

    REQUIRE(mp.all_transactions() == 0);
    REQUIRE(mp.candidate_transactions() == 0);
    REQUIRE(mp.candidate_bytes() == 0);
    REQUIRE(mp.candidate_fees() == 0);
    REQUIRE(mp.candidate_sigops() == 0);
}


TEST_CASE("[mempool] Remove Transactions 1") {
    transaction x {1, 1, {input{output_point{null_hash, 0}, script{}, 1}}, {output{48, script{}}}};
    add_state(x);
    x.inputs()[0].previous_output().validation.cache = output{50, script{}};
    x.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(x.is_valid());
    REQUIRE(x.fees() == 2);

    transaction y {1, 1, {input{output_point{hash_one, 0}, script{}, 1}}, {output{49, script{}}}};
    add_state(y);
    y.inputs()[0].previous_output().validation.cache = output{50, script{}};
    y.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(y.is_valid());
    REQUIRE(y.fees() == 1);

    transaction z {1, 1, {input{output_point{hash_two, 0}, script{}, 1}}, {output{47, script{}}}};
    add_state(z);
    z.inputs()[0].previous_output().validation.cache = output{50, script{}};
    z.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(z.is_valid());
    REQUIRE(z.fees() == 3);


    mempool mp(3 * 60);
    REQUIRE(mp.add(x) == error::success);
    REQUIRE(mp.add(y) == error::success);
    REQUIRE(mp.add(z) == error::success);

    REQUIRE(mp.all_transactions() == 3);
    REQUIRE(mp.candidate_transactions() == 3);
    REQUIRE(mp.candidate_bytes() == 3 * 60);
    REQUIRE(mp.candidate_fees() == x.fees() + y.fees() + z.fees());

    REQUIRE(mp.candidate_rank(x) == 1);
    REQUIRE(mp.candidate_rank(y) == 2);
    REQUIRE(mp.candidate_rank(z) == 0);

    std::vector<transaction> remove {z, x, y};
    mp.remove(remove.begin(), remove.end(), 0);

    REQUIRE(mp.all_transactions() == 0);
    REQUIRE(mp.candidate_transactions() == 0);
    REQUIRE(mp.candidate_bytes() == 0);
    REQUIRE(mp.candidate_fees() == 0);
    REQUIRE(mp.candidate_sigops() == 0);
}

TEST_CASE("[mempool] Remove Transactions 2") {
    transaction x {1, 1, {input{output_point{null_hash, 0}, script{}, 1}}, {output{48, script{}}}};
    add_state(x);
    x.inputs()[0].previous_output().validation.cache = output{50, script{}};
    x.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(x.is_valid());
    REQUIRE(x.fees() == 2);

    transaction y {1, 1, {input{output_point{hash_one, 0}, script{}, 1}}, {output{49, script{}}}};
    add_state(y);
    y.inputs()[0].previous_output().validation.cache = output{50, script{}};
    y.inputs()[0].previous_output().validation.from_mempool = false;
    REQUIRE(y.is_valid());
    REQUIRE(y.fees() == 1);

    transaction z {1, 1, {input{output_point{y.hash(), 0}, script{}, 1}}, {output{47, script{}}}};
    add_state(z);
    z.inputs()[0].previous_output().validation.cache = y.outputs()[0];
    z.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(z.is_valid());
    REQUIRE(z.fees() == 2);

    mempool mp;
    REQUIRE(mp.add(x) == error::success);
    REQUIRE(mp.add(y) == error::success);
    REQUIRE(mp.add(z) == error::success);

    REQUIRE(mp.all_transactions() == 3);
    REQUIRE(mp.candidate_transactions() == 3);
    REQUIRE(mp.candidate_bytes() == 3 * 60);
    REQUIRE(mp.candidate_fees() == x.fees() + y.fees() + z.fees());

    REQUIRE(mp.candidate_rank(x) == 0);
    REQUIRE(mp.candidate_rank(y) == 2);
    REQUIRE(mp.candidate_rank(z) == 1);


    transaction w {1, 1, {input{output_point{y.hash(), 0}, script{}, 1}}, {output{46, script{}}}};
    add_state(w);
    w.inputs()[0].previous_output().validation.cache = y.outputs()[0];
    w.inputs()[0].previous_output().validation.from_mempool = true;
    REQUIRE(w.is_valid());
    REQUIRE(w.fees() == 3);


    std::vector<transaction> remove {y, w};
    mp.remove(remove.begin(), remove.end(), 0);

    REQUIRE(mp.all_transactions() == 1);
    REQUIRE(mp.candidate_transactions() == 1);
    REQUIRE(mp.candidate_bytes() == 60);
    REQUIRE(mp.candidate_fees() == x.fees());
    REQUIRE(mp.candidate_sigops() == x.signature_operations());
}

    
TEST_CASE("[mempool] testnet case 0") {
    mempool mp(20000);
    
    std::cout << "error::low_benefit_transaction: " << error::low_benefit_transaction << std::endl;

    std::vector<std::string> olds {
        "01000000015d2121df162cb4a7ab369e9e32a0d60ec9815e42eaf6aae122903e802af59590000000006a4730440220553035d3bcffc7134b01e32ee78ebcbd7d645e1791cd55d74b4934e213e3506f022049d8e56dc5cba61067beb99e363f49faf3fb770ef78597ee96afeb96777caab0412102ec71b0b9ac278d1d4c1deab2b18487f3ec5af3aa9ed2b5fec34d2daf6232ed9cfeffffff0200000000000000002c6a09696e7465726c696e6b2018d0dc97df1bb11bccd6d10500d569351bd272bdae46a8ecf514e9a7ce5a4d082bda2600000000001976a9148fecabe27a217c9c4ad30f02eba0fcdf43f24c1588ac00000000",
        "0100000001b303ac1f851827a39898a4cde748631840bbc08f4fdddb9125b24974a348c6b8010000006a473044022063e7ab00eaa27e31ab999c47164f3dd72ed9b95c63cb4868ced2e2f26bf0260e0220245192fa6c6db43a356a6da09a5b089d6d97ff5fd985beeda59643b804e5dc034121031368605b0bb15d897cbf82a24b7e1c157dcf169fe53d88da2057d719742995f6ffffffff0200000000000000002c6a09696e7465726c696e6b20e8dba1aeb5854ca70c750b2ac217ef8ff0398bdae537006785842f60bb4515c42829a103000000001976a914f1cb50d97855b854913c457f64ca32278693ed0d88ac00000000",
        "01000000016c3d7ebeb230048f553b2b11f8c391a8378f12b876c84f8d9ab9baf451e39abb010000006b483045022100d0ade41834d108a3da8322b2d793b7f1073a72b2c1cb93016049c37ed2ccdd8f02205819b43d091876b3fff9f9af473bcd7e3a99cf6aaef3cd1377fd842ee851ae9a412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acc79c8700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000",
        "0100000001808e7962dcd1078caa575a7c92ac78a0743876512458e9bdef2c155620a530ee010000006b483045022100f72d76e45a8dc8a5f597ae51442fcba609f7c42b09d81abb5c12be623dd5fe32022039171f7f172d109ca7c9c0b9abf186358e3cf3caaffef16dc243b249bcd50d09412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac809d8700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000",
        "0100000001d7339ce4c845da96c36cc962e822ca057e598b8d38c0310c5a7d1dfd8bca7fbe010000006a47304402203e442074ecdb4c42c50219d936e9086b9b7e2581cc51967ca59783cede04d08d02204a1c8312a248f1be9fd3e3208f4dd8e0af1449e2744ace8fda4738b8f96d6671412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac9b828800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000",
        "0100000001c35da233dd893ffdca0eb492fd55afe92bf20469b670434b423833d89f8a7a7f010000006b483045022100e8f1d98ad285bed1a4e64c4706e8ad3ec28b18f4d5b7c987a736713644f125c702202c0630d7fcb9e77a00471936722d2b6cd195157bc54bdc58243ec374ef033ed44121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac80bd8800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000",
        "01000000010a389fc1d3c006501f29f464299c6112502044a625ce65c4737154d8a430d68e010000006b4830450221008a661f5014f331c7b934ad01c39fc65d445ad8d11a6d4d08e2fd59838730f37402203b046a9d07a48a2529c29d2cdf65e46c28aae1f93613cd9921b1ceaf0079c33a4121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac1da08700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000",
        "0100000001ffc4e3c82c253141de188eb53d19c74c4ff46744da49aa67aed184dff5072dbb010000006a47304402204b4db0e444c9162316372f457cfcf7eade098954770401879d0b6937ffbf7c7b022034a084edb6c25489d4e3f7aa795d62a73126d4ad115a46b6d870f072bed296b641210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac4e838800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000",
        "0100000001dd9f57f7716b8a1a6eb4ac6f19d91c13f785bd3ffb701ad9d8fcb72b50bcc3e9010000006b483045022100eb0388546a11f1b37da8785f5098c22ea4101cf5913f3ebe44ad6fb21f48eaae022031d7ea3b099d78e60a9ee0e6f5c6e22c4fda79335b9d81a04091e6ca87adb36741210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac49998700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000",
        "0100000001e8fce13750e59778d6bd47fdc599236851b8fc7e680ab6ce3ed06d29faec7035010000006a47304402203bee48e4dc2d2faf57ec885232efcddc27a840542164c5262b3bb46d864b15b6022025bdad9a557cb9509df862e6b5e112cfdc6237b430e95c1d206f711b2a5ab97e4121038bbe3c760c28cf2fed54abe683a4a88ab860a4f26e209d679dfba33e281bb4c4ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6a1b8900000000001976a9147827fe05a57f60eba10d31c21810273c4e88859a88ac00000000",
        "0100000001ba984772df8136fa25026bfff3b07d35d259f50c451ea078e5a92d06aa035521010000006a4730440220675dc0222a585bbb11dc3aa72b794dd06c424c95c7010b4033c4898716d042db0220709a8efef19816748178b4ce9476fbbac5e09fc73c44294ef6a031669d2ed13e4121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac54808800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"
    };

    std::unordered_map<chain::point, chain::output> internal_utxo;

    for (auto const& oldstr : olds) {
        auto tx = get_tx(oldstr);

        uint32_t i = 0;
        for (auto const& out : tx.outputs()) {
            internal_utxo.emplace(chain::point{tx.hash(), i}, out);
            ++i;
        }
    }

    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001bdb1ba353b91f33c91c9860ed82acea5e853e3f61394501e5e48a6cb524f2bd4010000006b48304502210081149fdd50398db9f8494eaff4111200286b2f2f93e45c9bd155a04a1629cbc602207a39adf344da455d739acbf9e1befde1cd931fa9a0558ff65f3f5df6f5a23f1c412102e0cfe082d408b020cc9b8c297f22ad94d1a390a10f62dafdc01ea97dfdbe4387feffffff02dcc62600000000001976a91446e1437885b22d939ef736344b798a9cbf433c0888ac00000000000000002c6a09696e7465726c696e6b2084cda8971d0b07bdfb0da84915b33d07bb8265481e4774d0e666f93dbdfb643c00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001b6a220148f412dae574e51e92f072aa90f4baee88f1ad285a736b2f63cf3460b010000006b483045022100ac88e4708716af3e1b19a7bec8fef9b6bc7d7829974997eb1b266dfb3923c689022039d39f76857aea5a8b466fa5a37c94a09a328af7d92a24ae342915b7ab0de94c412102f6b775b62f032e462df94d6d15f69db114bb7feb2748c6c2886a3d4d925703c3ffffffff0200000000000000002c6a09696e7465726c696e6b209a67ec902a1b500d939bf2a0dfb79adf2db3e6d10c6c565671b38806a993fc72f015a103000000001976a914a588cb7b781a0b67c02e87129b8ccb56fde3469c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000014b21ee302772fbc21f8e8259027d18eeea59e72dcc74b7a324eaa45eec31b9fa010000006a47304402204dbd3629acc05b5ba2747ff54c839af0423552f2582d3d025f63b87756098aee02206707a7cce3945e91c372b9778488bce772f5e16fa6cb426dbe8f78d165aa58db412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6c938700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000145421c50d6de761c1f2883031efbca82d45bb61047ffc58cfad5cb90b01e43bf010000006b48304502210082f333b5716010fcbd58846b8b46ad94fe823dc1305d91c2df2a7d10fd70cf8c02201c3a07e6426e99dd7a64c202fbd519a58246390aa6242e2478403a0ac7d378c0412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac2d948700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001a857e56d293466fa88f386fb3cd6d8df852c3d30194156cda267f2f8061ebf25010000006a473044022016cc935298f95fab304e2db085eb59e12d7eb289b67b65e70cdf4976e616b3ab02205adb5cc44e69b4b7cf001c9d65649c7a63c61d7f4b58d022cfdfb909f28a56ef412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac4a798800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000145d05cf52335339f2a0b88b37e7647e3efceecaa614fca267fb4b0add6b42d8f010000006b483045022100d1f6361837e434606b4661c3492b679f9642937d3c0fc513a891e5f333a539f702204be6f6eeca3a944e9ab20a61612a84ea989f7a3ec9a4afb5e4eacab1459953804121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac36b48800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001042285e129734551ae32b1950291ab04dfff158d4f751e7011c427af285173f5010000006b483045022100d302b1f0a7cc41b48e4e4d8adec47fbaca4736d0f59b48df98cf15397f7e32d0022078bccb24b4266b69178dbba497cf02a96d0b28491c83ca9e543689bcc29f52344121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acd6968700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000106fd6ad50034509df5399562d8d108ea448553093c721ca9aaf477b98a589e61010000006a47304402206ff11e73e3be12c23667a161ee56f5e7ddb0f861cc404438cae1cbbef40b135902201793b598f3779b5cc367ae78f46549396afa2c1afd8ed19ca580ed0aaef9b97841210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac377a8800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001fb560d9e0583859a94234f651ab65b1345ff945c2b5dddca21fb004800ac3140010000006b483045022100ed3aa767f3f0063fcaf49a3982c6c9070e272e908b8853d3f5c6d01bfa26824f0220616f4c034a10166fad877019f820f7fb80b04ef1ed17496559bd149b3f14a5ae41210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac43908700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001c22a0ade4102c1825cc3649a6b12a44ddeb8cb699c60b0c179b6c4042037608b010000006a47304402205e1b22bd6a3ca7ac71c84b50d961cfa2b4889a8e23289a531bac9e7fabfa502b022001de4f30e75398fb540652f086d4a5b8635dfe244d92f07b1cc10276b8b64d654121038bbe3c760c28cf2fed54abe683a4a88ab860a4f26e209d679dfba33e281bb4c4ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac69128900000000001976a9147827fe05a57f60eba10d31c21810273c4e88859a88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000016436cbc1072170e7672e2d93d91e15405b66576dc40f296ece6d8ab92bee1a05010000006a4730440220728b26996a12d436b78b942a2bf9c532056bd8fb93c42672e8ea7fd8ac2780e702200b17bd2e1361441e8385f3d37ed9606cbf8b36dfe4a8736904f7823a88dd7e874121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac57778800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001bce54d1806212a7cebcaadbba13e482f9fd3cd298d48e296b13b0d596fc5afa9010000006a47304402206e8faffd8c1e9cdc2b960bd61a26bf10cb3f773abac749b6f670cc3cdb5776f102204d805ff292583b27d683cf4598b137e835aae687e19f17a8d95f107fa14ad0564121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acd9aa8800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000011192925461debe816afe95a562aa1e6c94491d949af45f7d0a8e87711a20bd23010000006a47304402205b2088b43fc165f660072a3b3f31669df67b38c80dbd85b8bbb790adfd9a1baf0220647dcfc72b91451bb92eb67f6d666f5b303178ccb16a840e091886e8fbb8276a412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac0f8a8700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000015ae3137691151eb02138aafd166ffd5b5a3b3d74907e06c874b41eeebce09fae010000006a4730440220426146162387c20de0aa8e0975e018ffe735fb38e8f274425041356d704454de02207a17c6f993bbf73725ba75e16b176051a1d7a8af7ed470ef64ed464d8afed22f41210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acda708800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000164b34a9cb7b1caa540d830a6a04e8cb1cebdbd18a16832ce6dd80abf31b013e1010000006b483045022100e882bda032a31690e36368e022e890d627db34df9495a1ea9a2d925cccd888ca02206bc3624329f77bc4126b0212dc711ef0cdefcf2906bfb4e8353b72d9daa8a545412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acd78a8700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000017d1619061e8dfd13cb0098e2be8d7a1786667b9467400e7984e74b7d18141f29010000006b483045022100ffb05a1502e2774973cf285fc056ae962a558241642383086a6e7a293ff13cce022079de13d52999f6b7e14f3bb7274a1a8dc5c63c43c10ff1b6c79260622d9f2f5d41210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac02878700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001d8ec0ba27b16d5d4dc5af2dd5304346504b42b9f58b02bc5ae4a1dd0df258dc0010000006b48304502210084d302b9576ba166e45acd1568adeabe6450ba4e4e14935c8f876884f28e5013022012fdf27020cdc8e5efba0ddce1cb6831608e8718af74d7a8c6f77f28f3ec644d4121038bbe3c760c28cf2fed54abe683a4a88ab860a4f26e209d679dfba33e281bb4c4ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac3f098900000000001976a9147827fe05a57f60eba10d31c21810273c4e88859a88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001c8c00de420737a054d1604ef11a0f7639437335c3f9007d4d97317cb29a3826c010000006a47304402203417d3030e78504c2696411c3df9738dc2b601cce2e3c7e8aca7091d5462b3f202201e956e2458d8cdfa7812ddb6b36fcddf92d98178e1836b93b47a2433c56b89984121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acc88d8700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001c7d2b9e672848287161f3c42d0bd64a51c88540e4b90264b6607479f3fdc0c9f010000006a47304402200290ab868ad305d8f131d594ee95bc1818058fabc891a5cc4f4a3b6c89e491ea02206778b666e75241fe44a6a06d8cd7d9451bffaf8597752c892414932e00d778294121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac4b6e8800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001c7e2cb11c418c4b87f27cffadf96fe48b241a52e01291859782f52c4d533d869010000006a473044022015be5217d23564de1ae8c0670e6b4c714cbb38f433e0310008e63fbd4b0f77df022071b4b27d63a50a1d7767200b2b8f556b8414ab9b22cc5c00c37138f553e3a5de412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac43708800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001af650160f466f16217f2aafa5c462f7a352d8ca382067d8da2fdad821315e15d010000006b483045022100df15bfd84002f2ce909188edc7bf3c0e979f81333fe1794499f81730773d341c0220017209cf19f37aec67547a208cdea938ba991809efe7c4992570f5bd7a77f82b412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acee668800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000178dc8033893bcc5231a4b94c8b425eaa9ec85266a14d528379c7a0cb31ce8347010000006a473044022049ddc7a7a2ace3395709e8f2b30ffa889d0856e09fcb39fe694722b64cffc5ce022050b59c14da59487b487b5144caa357bf93c43391a90b52806bdc59158291c7db4121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac8b848700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000019e883010152dbf368ba64ed66d9c40e7659228ffaf17d15e94b684cb54e35fb6010000006a473044022012a65f5468460644acdfc7d3f97a6be3a6194cbd9073e5b99ccf6f8106f6f69102207f7fd7b2894feb31f4c45fe01a24c24d4c7277d3347753c6ac7374011fdfc3cc4121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac9ea18800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000184b4a02010dfdd3f28651a8275696c4d81ed579402ba2f025ff41f4722659920010000006b483045022100efdf6b89caba928e949b4a6df8b2320d917e64a5efb61b3680ea0d31c0884ef4022002e23ae0c49b2e5f4162d9ecec69dfb0e82b960912ff63a183567b003e10b3e24121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac14658800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000107033e6cf00a723ceae6a6d0ad61187301b48731ff983faa627e83a6802899ae010000006a47304402205ff7db7dc3a1d31870724dcaa8774e4f27ef45bf03e026993510a1c5bb0eb7a302204849dd3e56c642174f65b72642523df3e4e8c1fd835c920f93bbe6049b321435412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acdc808700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001a828dca6063004575be809b5d187410290fa5286b70a9624a1c34b44e7e44b17010000006b483045022100d88d1d4bb0d35ae26b8b029108061fe8d4a69f0862f8cddd2b46393fb1d66a2702204e70b2492cfa9bfe29917f0c6f5809e3f4da2016b74003b6647a4b98abd6ae2641210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acdd7d8700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001ff6684a58ce9166b16687b302fe6ae46e556c65d263f53419d915bf04bc09480010000006a473044022006422eb639759cec3da906a99f19d5005e879fefdbec114b6fcd15a133a9417202202d0f03131ecd5d8c83c2fba494e73fe2b3cfb3ada2242a4a42765c335deb8b7a4121038bbe3c760c28cf2fed54abe683a4a88ab860a4f26e209d679dfba33e281bb4c4ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac1c008900000000001976a9147827fe05a57f60eba10d31c21810273c4e88859a88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001e6430b0ece8e32a7f9a2a85c009ae96e0b2fc95f78582f4d3e14baf43800864f010000006a47304402206382a98b180b4165b9c2d96ccc1d1b95709b569bd778f2169cd9a8e2560d58900220265b456f2e1aea56cd24f4dd6f6664eaad683ad898c95d723cd6e2a621bb8b2541210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acbc678800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000012917150fc6cfc8423e8353ec00b8f22f9da23c15457a47b3443f4d4bb9f022d9010000006a47304402204f80a8640851606e8cbd19daf105902babbc62706c6e061cb7cb7f0180eefe440220741e4219635169dcad6b49f61ad40870e3b622630a7a892b92af88233fef107c412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acc0818700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000129abdafcfe3d40081f82b3ff422b6f4b2e64e9d8bab84d321ff93ce37c9ce409010000006a47304402207ddb47b030dc0f9805123fa84c4763c58b7f7128671b79083184b51354c79c7d02206567d4644dad3df5712ce7136f337e1ab99d45df4ee7cd61ba3c7de88e2333ab412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac935d8800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000015a7adc70f7e31cbf2209c44a1f0a11ceeeb9205b3a0734f27ac3a14db1a067fc010000006b483045022100af071131a3d9da47eb684cecb0dd181f20d155c8668e10e7c6712987401705760220032ef518ec3b5de1e35c249402a7332a6eef002c1abd2aef47bebe446df716bc41210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac625e8800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000198429ee724acaecf8cb3d0d951dda5c2848f95cdee7f8b5020c99a5c1df9fafb010000006b483045022100b6a0a3d4fc7a2528514cdafb3960a340d3310bcea519a5e4b4dc88ff4c9c72bd022060a1cb3966009a8b5ad4b559182aafc9e71b815b3968c1600283762607ddc0ee412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac7f788700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001f947be0cd0a625b605fecc1f742f98768ba253310445555683ee8f2d5941d3a1010000006a47304402205c0c195bcf745694d59f2dd5f051151dc6eda91c5c523a3da15560bbfa81688502207c75032e4168e1a2bfa09913040d11bf815b9ed610de5389a2da57809d30cbf041210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acad748700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001dcc1eefe1a5a3525ddd06e739b60976c3d9aaacae88a0d844e966d45b9cf1729010000006b483045022100a86a7dfe41f4704292305f38aa23cef1bb583f0c39112b663ea326dfad0bec44022068c83c87524e4c4e90479ea3bf3b0e958500e2bf618ac8586928b2e92b27fa4c4121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acfe5b8800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000106a20e6875f354615133ba6e23255e509c9c6e328d954d32eef4c228cf2fd52a010000006a47304402205c1ba7fb9602dcb1ae05e1be19213dbbcfc2116d18b21e2dd250ac4b798943ee022013fd0c11591297de111d897d413199636ad0abb11b8802244318b89bf0f421f0412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88accd778700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000014253946a0dc23d648c6c983d8ef63b05c2a3f5027531d50c4930ad6e5b486b4e010000006b48304502210084fd5e8a52d4a24f00d4ac762ab887032e4779614de272636fd61f57a1071a2802206d3d8e38ef9aaedb106f41a0e4a003a0758d90499f9ee88634b4b62d1f7b89b14121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac847b8700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000019fca040eb07458ea9d43e58911fd1168c50e9ee1fa9f5dc2f17bf1d032bf206f010000006b48304502210094cc6e1002ca3b866e63640e952fe3b274490a545f9ca46077522bc05c11fc6e02202b5bb7214625545128d84a71c69ee893cd9eea5678f37ab7ef8181b415e18ebb4121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac9e988800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001d7773908843e54801e33116a39a5effe93959ee0ccd3f4df084ab9f1a4bbb7f8010000006b483045022100884d48b3316410ee36ddc68e03972e9f0f49f02884df6d91a393911438b6ecbb02205f899fbdf9dcf54396d82cf819abed7f10b04cceedd24242c5fd5de0b919e8304121038bbe3c760c28cf2fed54abe683a4a88ab860a4f26e209d679dfba33e281bb4c4ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac20f78800000000001976a9147827fe05a57f60eba10d31c21810273c4e88859a88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001f090d1d4a0c757f77590082ade02ae697f19e8280c8291c16c489e9ac186a5c4010000006a47304402204c5939535b991581dbae6ed6fdc04f741953c02c7225ddb281a95a33ee3c160b02207b129c77cec4fe4b66426abca2515fd94408d1b8f01549c10e6387c9b9e54de54121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acac528800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000010ba567d0c5322b211c7f8822c32b9c9f60c5976c3753dc73f96105b4d2aee448010000006b483045022100c49c21cde35474b9b87fd15ac511d2c2388cb4d74a7f100102f7d2f7dda892c6022014f764ed8a8314936aa22324178504b6a50986b1573f7128b948da0f4edd671c412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac4e6f8700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000011966c6203bfdc580eee31a72a70cc2520d436672d54fb8a7cd3cb8ec408a1828010000006a473044022061659b3ed3cfeff54bab45ba30afddd1a2b9c8d8822581d377179a4ef473a55f022009a8c3b59f865c35e4886a422649c68e7059914349e684741d7e43964250e70d41210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac7f6b8700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000114f6cf76560e548ab7117a8bbec4b89b9e1adf9be115ad0e99702be4080ee5b5010000006b483045022100844e25fdfc3de20d29b0d5ca240b1065cf2fe4ca4bacb391cd92b68df8bf532d02201aa55d685c82f9282887707bc4de995ce3abf044abec25fd9a1d8630ea1bdd0f41210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac36558800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000190fec6fddd2d86b0a1d435f9036e49ce4ee676781882528daa6ec1be478b277c010000006b483045022100963e26f05d44d802b89d45d78b83786e0d02604b5de59d9f6acff747b778c3a1022078f14922b4612bb135b001a4cdd5af057ea997d3a469f8307af217613716a6384121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac7b8f8800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000018d80b834b4680c940a8552836e9750e0a3fd4d7804ed1011b1bc3b34756e5a48010000006b483045022100ec633b6bbaf756084912c7a80aef72c9aeb1f1513e879edc09bf13da4c03380402205a54bdd9320e5a80eaaa42231d7f6e9893486874d8a26d893325dbd5113b8284412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acab6e8700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000018f4c689aee9851a7c058464d5eba85acba56c9361266bea01e4452768387a782010000006b48304502210097d7c2df520b8b6a963b7f368105ddf5ba993d809316fcfe5d20faab9813734102207bd1192331506bf6c5ab1d68dd8bbb109af8196d2c2da2a454b68b7e494dc26c4121038bbe3c760c28cf2fed54abe683a4a88ab860a4f26e209d679dfba33e281bb4c4ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac08ee8800000000001976a9147827fe05a57f60eba10d31c21810273c4e88859a88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001c9995dcc151ef7ea24e95ec6b909ec9061cc6e972f69567ef9bb0635a4d7fe0e010000006b4830450221009672c00560c040afa2d8ac0f362dd366cb62394769ac6d6a5744ef724fd42454022047f7420b2decdad5da92b4e46578c86f8a1dc10638348e8e1078f8bad8805ef54121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6d728700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000128c4ea82377f31f672fcf2a2e17f527eb8f5211e8fdbc38592ab182e23bee302010000006b483045022100851f80830e8fb50a52fc301d9b887f585560e48c08f991c7bbe574b5762d31af02202f4f2416d56dcd4e81c6fa36e7964995b9d2d51ab38c08f229254bbf5632a595412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac85548800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001fab7ca52b20ac3eaf7ea80d6bd99b05c3e517af269afeaec359a95639c33648a010000006b48304502210099164daaa87c962e5a020e4ecc92edad364012eea28e491168b1a3e1006f730602207bfd2f1ac5194c5b4a9818e415bbbe4ac61565fd985037f433ea535287cac5564121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac2d868800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000013209b604b961edd6a0c8dbdeb86137bfa964b124bf4a74da9c92eae675a2cae4010000006b483045022100cf7594e6a47c01ca75e6fa8452f8ce87eeeda1d7279103a2acc31724d2071fdf0220154fbe3f3e53c47590066a2cf26026bd94f1f4e6a15c61f1b4d249c89982d7c8412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac3b4b8800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000106eb59234bf4810bf24e20901ebba8d538b45e1e12c4c62a51d753a8d05b2d25010000006a473044022014c40dd41704cb58684b4a53b8859398bdceacb3b11720570cfda2bac2d0010d02203c047cf12d938a218ea554cb135e372b33d580a2419ef758ab74baa9163cba204121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac2b698700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001a26b203605a801319ffde29ffe36af9608a20964a5d9d72cbd870182ea58b9d4010000006b483045022100f638891931008793af719f81e7867015435aa905d4f58a13729a1f0d5a027d380220378c4c7772808f2288aa193a2356e8810f08ee7182f5258b5675b176be0c39b341210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac0d4c8800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001a9acbea0d609a9e264ee4bfd65ee1b14f0997211e2e85cad436d0bc5dafb6918010000006b4830450221009adc20e7aca3f94abe42cbbe79cdfdba67615706186895997fa50838544d091502203ac7e5d9d0748e4e63decc7c2c69f59f147355429ff19d4f1e32d3245a8b5379412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac29668700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000016d628b7c95a6bc1ec4c16aa29929f1ab11db9bd6bd0a877e235e27e8d2e4def3010000006b483045022100f05fc6acabdd57954251c713f1b6008706697ff39e3f24321656f215402d1914022024377b0b2bc96b2228f77817e6af3c525102a337987afbce0353fde097be268541210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac77628700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001174adccd8ee3264cf77ff5b5941c700247b7fcb49047fc51159ba762e4e02389010000006b483045022100a80e134de21076096cfabdef3d01dd69634337413e7644178bf51410ac5d0ac202202eff3cf197ed4f78e38b2d2af09ee6c8658857f23bb3433a950ab62e754a3fd04121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88aca8498800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000190b9ca4c3f62e0cda6f55ef1005779eca591a70972fdf8699609230d4c04d4c4010000006b483045022100df652ce9fa31b00213d1a5f22da633c5b23fa2fc7e8aff8bc386b0cadfeaebd3022004b0e1cb1d779cef107c74886f752de84c63f9c326729b44c3167a0899983efa412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acad658700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000124de049f062778975b7e6bb7e6a5b55adb8abbd38e2457c4ec03e0b045659f02010000006a4730440220280c7c9ddc570ac1296bff407a98de60237d60039c96981fe9f70c3cf586cee302204077547813bdb7f17c62fdddb5b5cc60d00f3761a1202b892c1cd59feb0aebeb4121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acdc7c8800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000015a2c19a46ad01a1f0203aa59c5c7889b39e28409fe9da1751a9f9b8a879bcf35010000006a47304402200fe08d76bc40b0d021ed9f979197db91dbb9625f866ca89abc0a5ccc1d35bf5c02207a51ce8b8a6370a73c40d947c502c8dc29a766b9539522a337ff2b561a8baf894121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac59408800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000013927c262699a6bcbebec1598ba2214a2d36118cdc637c557cd6ca24adb48c87d010000006b483045022100d19b86714834ac49ee9728b51a0fdfbdf4d3309118be8de229092c1772ff5494022051633473bb3dbfc6d452c76d7889d360ed532de4b9650c41ec96dbc29a5b43b141210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88accd428800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001dfc984a2a7999d7dfd9f02433bbd943f69bf9a67f6ba7e62bd0bd2563c556e61010000006a47304402205df8a30022306121417e8ce8055b2dd9523451e65646e971b54e8562e7808699022061bcd358dcd90ccdd942e71cca4fd7d98da3b19a0df0b639856040732c36c5ce412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6f5c8700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000124382ad7090aa9a4e4c473e78809ff97d92d49d053a18c3034725998684ff66f010000006a473044022069616ccae530c0f7785decf172b287f36831464b8ced42c96a990cd3b074bf6f0220169c2853f64ee4aa1959765e3920ba31a4bff405567b20856080ee290ea359944121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acf05f8700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000127ebdf990948ab13f7076fc6ec3f60d437dceec7885258d55ff51dcfc94231cf010000006b483045022100ce4820463c2226f529e43d1cff88149a581065f4078067076d708439bd11ce7f022011db9d57c8b8640f9a7ecf0ac3cb0ede47ccf1e106e25f1cc9158081a5f62d0841210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac4c598700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001639d8079dde424c27c4b7b819551faff06254d402eabe59e5bb90e3571e4da04010000006a473044022009b41495f5d27fb23702d6e66549ed8ddeb2a9eb1d12a1be3a2069d2ae67a6f10220061911384b907585eb2b2153b63ba37b852f7836be5651de05ca908bc213bd54412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac2e428800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000018c7a4eabcea6eabc554ed97762639c103aba67ae3d11a52e20735ef1d0028c5e010000006b483045022100db1ca9355f08fc8d3cf9b8fa41a9f87e7f0112d0c2c278dce0116afdf396235c0220103d27aaab4bf5854ea276b6ab33ab6c6ec29ed11b2bc405a5afe0dd52d89410412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac205d8700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000179d763f8a3c193f74d4a19dd7b3c801f607bc5b8c140977a73779d46ba8b45db010000006a47304402202061399c2363ec9a9a51610a18689d5134d04463969cb0d355bf9ff3b1d6317302205cf5670dc0c5e5efb0d50c77fd55128e37581462de80318155b5e9eb3ee1bd334121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acff368800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000016633318a75f6a8282cffc3df5f237062e9b514929249d3d46bb65287f53f58d3010000006b4830450221008aa092e55dfee4448dce691d5be92be9e5eeca125e123434874cf62aae9d1fe80220709f152a98536b773361600588d1226064712371ba0b95d5aef8593c8a99c53e41210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac03508700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001a6f81e3d7e42b1e8419e1cb9580f8d580f6309d8cc2156cf1f0d0ba2cdda2beb010000006a47304402200b467151cb0ecd985866b485a42e5d9beb4f37f57876ec66974d3c958e5ce8a00220094309f831c14059eef5b65a751ede763e0e533d6b8a39bd70cd5089da4dd7344121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acad738800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001ea4f394de2bcd78b85aee1f70135f05b36f3e93cb783baf9c26092fc5d269e06010000006a473044022002f9a063314bf9c0866c5fbd39df991e3effe426d59649ca7e02e9accfe4db87022064e06e69daec1f4ea0328662535dc94b309edd55e614c42391315714aeaa58f4412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac03398800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001730e71adc7b397f5b8a8c413ecf7004d5432c60eb32128cde5a2c62fa384fc9e010000006b483045022100fffc2fb1ed0c3768869ac718e53a36e08dd2031e14529c45d2845065534ad2a502207a74a85c1343de8c4659fe40be87ddef4dbdaffe927d462153803edf307ae5644121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88accc568700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001fe676d36302c96f6cf34d3b217dfd62ecca380c9316033dfb665a0a06ce34c1d010000006b483045022100c82bf3ed1644547762ad5e5c5584737d3492241226ad5d69336b73c814b09f7d022005bac191b1549df2577a241d3cc4e4cc1beb18e87468009d4c335cb33f431b64412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac04548700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001123e243eb3d060399d847a3b65b1c3b99c4acfb0accb9e970412a2b42bcceed4010000006a4730440220492ae330bc0d96f6a598f12741fb9bfc36337f2cc7b315583d8b05d489ae9a8302205503c8e6073f4a182ee45473082ec66e7f6c28fad89da10b6103e15bd831ee20412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac5f538700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001a81a64efcb113f92497bfb27f43ef11321715821934338b3a8dd498b0656b9b5010000006a47304402206ce2de106c63230090e816dbaa3184bf655db1bf008490194838d5889a2756b502205289f3ac25db323fc300ebb66a31551da71ba2347d7830707d5a53193476625d41210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88accd398800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000018ab732cd67a4a3c468465da0ba4e49f1f64d7f93b919539ecc187874ba591779010000006a47304402206b12c068b5492d8cfd76093996103ceabeefa06daad4e26affcb16aecffa135702204917074e398673568123db453a548402ab147d21c7d6071a0aa652467bd4fe314121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6d4d8700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000119c0090466f4962f445f515c51c8ac00b5bd7a54ce8408c6557ef4d14cc8264d010000006a47304402204715b10cd9b71e4bd857d308f9b2ee2b86be9fcb555692cd62c8968fcfbd78a50220610c74a3397f5f50b8464c67af7d3f87847194c9295e3753ef9b54465c760b02412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac024a8700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000138bc711b58d5eb79b41805004712d7f13dd39bc4afd98f239439269b2434b7f9010000006b483045022100dcf7a55846edabd3ff2f5a6e179c3547202dabdc06646c50d0d256ac2eefa026022011af2c1be76c6d342be15812697e824000ad94299fc76cd7f4087b0224bfc48e4121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acaf2d8800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001f33cb6b20f48aee5ca3de13f79649c2006ec6759411f4e608d105fcf4de36095010000006b483045022100df51c31530322707c7df517bf15c484f98be1d41befd8705a6b58d32bbdf69be022001db73ccc586e1dc5261375a6bc123a5c29d787fdf5780102849287ba0e99af14121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac756a8800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001736467afd4f43734eb441775bea9475a9d0f9e07c69b8ed1cf57a59d9f6888a4010000006b4830450221009c3f3a675459bbfb91f25a98ef0f7ff37b19cf1da405041111d0570b7fcbcd34022047a4ba2f7ea40c5aea5b76058d80ea8c4a0f591370299c36e5d5cb55a8547bf141210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acdd468700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001ddc16a1ab42b924b6a8c4e96ed6642f018a2e57ffd3856a2632a35da1455b756010000006b483045022100a14fbd467fdf129bde121b3e1db09dc5a0e9bc5475217a43608d9185791bbd72022046a0b4a1a4a9b2eed1bc756e1916483777890dddbb2cc8482a6ad7f2174d76b6412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ace72f8800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000190cb748ad4c79226eb4c383879e6083ea5aca58b9fc6bf3c9f74bb2c6780018b010000006b483045022100fb4073924321287dab481740bcf54f684038ad3a7162cc36049da7e7553e8e5502201627a05be06d37ba33d19b2e62fee85e84186e6eb1db83da50b55211ca4fbea341210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acc7308800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001119b251e105188d8ec25f4e430b16664735b6bfbaa0e762c204f54ade7a35fbb010000006b483045022100e60bec797f275ce88b699c92cffe70ec8f0698aff33cd927314c1f1eca74393302201361ac6bbe051ec437bba00cc8b5913aee4450085f7175d82c79e7f8aecd4a0d412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac004b8700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000019eaa61487b7bcced6a14f41b86e0406bd9d2aa524570764542fce48e19406e60010000006a47304402204006ad26b099c232cefa4747843746002e6fcbe96b1e1d34554a047d5fdf442602204bd612a361309595d5b5c8e1ae857f2346df3c3458b5b9a861186ca6ddb617f6412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac98268800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001d33bca546a8dbeb7f83cfe6fa9faffc05120707bbf4e2c918232e093aae3d9d0010000006b483045022100c445fb74a32a1a9cba5e2f384ef51d186e5297c58e2a0aa192b75a9b6f48a67b02205d5ef3a934159934e702d00536d8f1de00aaea3fddd029ae8207436f49afc0d24121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6f248800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000015079f8b799e0ae9aa61755c1c101ff0f730e7f8efa20573cb5dc4b50a71a3f90010000006b4830450221008e92ec1a8689a0d6864a09dd6520b5581ac7240ea9587818b92db9f578f43f78022030d7b52b0683d18f08644542a6edda0369165ea13d47340fb2ba70437b8d6bc241210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac87278800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001ade5335915fd688cacc904c27e679c321efe35d7b39c7d3534ec0e449a51dab5010000006b483045022100df9124ba65065452fa0cd178c285cf8dd9879a2604eb6a7dec22a8c7dcc8458e02205cbfdfa1642ea6edbf7e2742be5d84bee69355be5e3981faacf69df86b23580f41210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac9e3d8700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000015492ca8b642bf30c920d70d0e920df5166f3bdd22541c844fc087e289c624787010000006b483045022100f2da1759f7eda9e0132265df2a67aeac94101592ade0a7f2ef4cd3bd9fc5804702205eb7d29ee822104446c008eed7cf78f9b4bf419428a25744f835b3cb54ce42654121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac32448700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001d988afdfb1bec8345a24bfc72379dab42b20a763f8f288e76434a5df7b1b5cc8010000006b48304502210083224c4794b83bab8a2bf1956e6354c1b15096ef4d170a19bdbe4d98c15348fc02202b1b68f07cedf02e272f60707b1938f2be6ea76ed5de69b491639be92b67758c412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ace4408700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000019b61b1537e089b0ad6a6af54531760efc97483018937adfe8c40a252132f74c8010000006a47304402203aa898ec79faad5c9815b2df9977b6bdd5a05bedf864fe27211e51a923f9df02022061fb34fd349ac367e7d6ea814898aa12e8a1148b3523d5f52ea0a80173986de0412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acf0418700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000013e94e96279734014c83ea35bb80da359a67409143afc8967fc8f88d2fed098d9010000006b483045022100cc2636895c944c06c8737ca82a0e5d07dc03c62312f9bd0a0860b7a2244999a8022041004c5ef773bdf73b2826fdc570b22c1f49927d943d1a2b7b6727a4fac371ab4121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac79618800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001d5671f53cec0f9cdf803cda75c5266021bbdb6dcb0f39e8bd62d701235fc16c5010000006b483045022100d9c1d5da361c22d1bc0209706a0dfd59e7ba7a630602362d806d17a37b3ba01402203da284f049d2e11be6580492ea545ef139f975af6860d0549aa5ba96b6deeae04121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac101b8800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001e9ba7506a151371fd7e8c3c03f41261e79d830c24fd1cd4d8ab072ddcb4f8b28010000006a47304402200c1e595cc06733c74abf42fa12e9cf07dd1550f9f526f8d7ff8b2fc3a89cd4f502207eecdb9a364a8d1062359c429af5e7e2f3f8d633e10110524ba37d742d35b6cf41210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac521e8800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000015b478f8b66078ec7f24d009f980d9196d0daeba290e4ad6dd66312b189047b59010000006b483045022100c12a477a370926a9a34414de895e8dfcfc2c969747c70b83431679870cc9163002206cae664a7cdfb3cf0b31eecf131dff43178df214614faeb4e5fcc83b4952d22e41210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6a348700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001a34efd6da333b8e1fa4513b8e75755d984becd2402e4504902bff87753b74ef5010000006b483045022100c5f023edb81a540d5db49648697092770cc0b99f7206c91f1b0ee2b204f7689c0220173511c318ec1789ef5d29fdf7e2f20aaa914cf7c873058f0669b94256b5f417412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac661d8800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000019518fd1f86d33db3b7e17cc58b267439082029b8a057242fefc8214a3e87fd47010000006b483045022100abdd19f3124028588a319632bc84ee3ab62c29dc32a39679018e25dfec776d3f02205cac56206a3ab98fd77dc3305c1c10468f19caded4f738bc26c9854eb9c061e7412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88accd388700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001dace75b91109d4170f5939a050fcfb9c33a28033950966553026ddda33f599eb010000006b483045022100cdffe9b165334e3c7d708345a06ce257374e6637a697d3ceff58731f1147e4030220734cc1be372db9a85459dd9b39bcf7004cb0ad1504ce64ef0225187fca9251514121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac1d3b8700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000011d9497f51ee2a0b269902d5f87a6704253b968a5fab42d3cbc212a024a4babd9010000006b483045022100a468122eb4ddff2a7706131b96033c8fddb4b69a2ca1b86ab52dcabbe926aca0022053a8c85c6da9fa0be4e8921b29779d1dfd0e6dd3039a5bb8e32cea3376b619a9412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ace2378700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001a3cf1f6fb7cbb598be3c974982cc38af03dfbb87a38c36c28f0ec6a6e4ec46b9010000006a4730440220781b8944db8484d6731302381f4514b8e939d55df17af832fa96fb8c85d58a9c022002c325932fbcadebd7ce23a45c4b9a7f89836e0e55005a9047501e325a74b6ff4121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac7a588800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000199662302e7daa2c24ab70881699c8085eddf9a8c801f1c57e3f2ba44bdd12c84010000006b483045022100c6eccdb89d651e55e01354fac223ae8916fda1814c9abe4e9d9645251d4cfd01022011dd2e886703f47b9eb43d46f5def9d5636f8cb91bcac5bd4b8dced2f0afc2154121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac324f8800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000101c1148cc88421b18a9f5087d528bc0f10ff8bd8e0076c68fb8637422ade94ee010000006a47304402203cc083782d287e0a0f9d2799e824c2af247756fcaea9d432985bd970577a8efa02201a4b25689f6e939e3198cb88bd940bb880903172584a2264f202de94f03bbb27412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acbb2e8700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001142abfaa54c48a1502c67a2f4f82d5d0495f6217df141dd4c4029a97325e4601010000006b483045022100d51a3663c38904529dee03c43c55ca6d96096da92c3319ea72a78dac95d96bb0022033bb4a9cd114ff2e249cca095c67a87323c23c9f2b89baa0cf6e4139c7c1c1a0412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acaa2f8700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000015d44df8bad65f70f7f16792572a74b438d5266d1d1cb61d00a5416e4127cf316010000006b483045022100ec7a4ea0c3f0b7df609f772c157877dc5611e49ec30d004a12c44a95a117099502205764744d4e869a771388138799d40cf28efa0994f2b8317f46801186e681f26a412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac46148800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001eba7c383d98487abcf7b56234507af9a303fb91934a826e531c8421321ac11cc010000006b4830450221009237eebb8b74c5a73c52c5a191167c899f8d7ce4e8bbf60f5aa6564de77429c502201f2ef4e7a149b7e4858f962c76524bbb4841943be3aa493e3070b844b75b79ec4121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acf4118800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001002391ce1d1b03694ae0d45a80c16e931bd81b0bbfef0c1ca0537d115a5e5d98010000006a473044022072aab2bdf6c3357d7818c2c8a8782e3855369989e84e0284c7a9f13826c9e98d022060b9c2c42e2c0cc47298bc53bed188b457f5770a1bb3874c7bfa3a79df70b6214121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac04328700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000016cb4e6678704e0d47746490d5a0bee88423e0ccfb2389671dda4f0c15886ae79010000006b483045022100b808228aedb5b90f29877370e917b03ace8d636f10851e495bd6c4a4f277b0cc0220554701460ffa713c577f9998a3806a9497bb96d9407ae40be691a4c4a4f5321341210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac38158800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001d0fe2195e00e9b4b29f2a3ca5e10351eae4e028aa4b6843706f9583fa7f189d0010000006a47304402206495de82c2cc06d3e0cd2cb49ef173a75b4d44ee368709cf839715ae1d12a317022033386807d26ad3fbd52b6644386fd23557373bbd35f9afb9a1584f06a04023d541210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac582b8700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000019a4bba476ce57911f7b1c62a43f8cab456a53c19f12c2b42c82f63eee3d7ff1b010000006a473044022041a7c53ba84995b3a9aea37b8cb1445e0bd449a5e2bebce954a355e1bcd7da6602204596b3a80f2a5edb91e304e388e60ea9281f93400131d6fbdbfca45226dfd2ae41210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac15228700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000010226f373c93798c7977a1f94bd54ac0133183dc777f5b07b2fb731c754893a5a010000006b48304502210088fb432b96553a3a9a2944a47f52e400aaaef02782df071cae874c1ae503fd73022005a90ae2c20c51dec3252030142fd14857c13f0722c7a1f0298048d00f9a77ad412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac73268700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001ab9ad5f58febc9375b7a71757bb62f21556c4e94aaccfc5680976b847d17d348010000006b483045022100873bcbc8a815063e8d77764efab8a2d6b5406f082a0317513fcabb86557f4a580220749cbea361c69f3d22bfc34118b2926554ff566f4b0b16935cd63c050322ac33412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac240b8800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000017427d5a5ed45a60859e47e0b465a43f5a38f962a1cdcbd008a9c15dde2e24230010000006b483045022100847d5fb8d6be443068c6e0ae26da986e794e475a0465411dd70eb631c66f650502206366a7938a3da687cbe1f73b38aef87c9a4f89bd3f9835fbd79d8f555c24f38e4121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ace7288700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001b2713b2c0edcb6363c01bfdf421385c8155b767f40237165b0f4a9376777db13010000006b483045022100f3b0fc1b6850ed39f19548241719a0356cb3ca32f27db73a8d81298a74956f1c02206a892936481a1445dfe9a3fc7b92b586f0140be75df9fcc9f57eae0f6bd5db2141210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac1e0c8800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001e77c3289499b2c42963ef6fb382e38d234fae5520225516a7889d4abde84c77a010000006b483045022100fe0e413c1680c6d191d58b16335c6ffeaf2bd211325e9764074c998ca5fdc155022001ead81ddd073e6a4e548e8dc9f90d6d97b97ebb495cef9e06df680ef78f6c0b4121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ace6088800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000183f60f70e04b00f51d6e66f6d408c3ff8df1d892ecc00c3136e3dd80b2093de2010000006b483045022100a26ca4418eb7cf3c2e0c7e3efdb4d5ef4dea5b9a8faef841e83705511a13999402204937063e217fa3716b68e8d1eaffc07596a8b4ad3d6cbedddb5ee3f4cd88617a412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acb2258700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000019d183a23a6fe6feea4ca24f2fd18b30e05d891e96d2f7d595b2f4cab7096ecd8010000006a473044022057d8947dcbad01b8dab5ad7a9e18dd90a4a7c6e72354f712be9949b59e4a62110220407c284ef13db66afd3e11bce9374a07586a7c4ef5795ca539ebe8afd705d93c4121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac2d468800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000018e54bcb18bad17dcdc51ead0fae430cd6e719bf6b370096a89b3bebd14c78ebb010000006b483045022100ed624f6033e15a73bb39e50f2226acd49b217fe179e007f276c1e369110844a50220609a3910af0d3bca3df9541d977b4c97a768613cd7346d2931b81c1236c99c094121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac8dff8700000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001ec6ede79b6b10263b01096db5de06ce4c5dfeb601657722a6b5bd881b9e999b0010000006b483045022100e579ffaab77951eeebf998fc7499b4d37522723c4d7ee846acc83e4fc40b06d002201c8840a4444fcf7a94ce793b26f6d3d3cc29a4aa448b8e9ce4adabf34f1e9da841210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acc5028800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000012d43d863e2f4bb364043c355721f30bb990b588ee2b50e14fcebba2724d1f529010000006a47304402206122fdfc781821bea5258e03a9f84dd5287f7a3b67bd0b3ab91b5a30611ef83e02203268c7d070f84ddb85640ca917121b8daf3f3a5e065b8169ce4dee6b4507fb3c412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acda018800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001ce1157ea4be605f23c1878217e514f55c2f5151a06cd69b21d225cb3d8876daa010000006a473044022075323738718e82c06b8768acd13568b89d9275cafa7372174412fdb38f33f6b0022013faa4a9cab7c35e13d3426962e3b1429abaf7641fe7c7e3efc8023317651996412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6e1c8700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000126fbb8833c1c506571a9a13a2ca135a205a2aef59f52090b13e3f31d1f6a0088010000006a47304402206b6517bb5e087f8a42ce0adf103b6c02dddb1c5194bcd06d448bc9379429b3a902204acfd8f088cb31533bc6b84a1622253cffadba1f88f2756d02963953a8d311a64121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acf43c8800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001574c635ed152283dfcac73b99f7edfffc983c7b273aa75db18ea2892599112e4010000006b483045022100a3e77c01b1765b8364b205ab3ece7d7065e65ae6da66afdeda518e31ced70643022074df9cfd978663b3b7bf9f0109de438c1291747ee3aca6d0a18580bd22a3a6f6412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac641d8700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001e539d6b3d92e40ed62dfb52428dd1fb90817df8f56421abe399992e315ff60bd010000006a473044022043f9ba165817ebae7208e1d136cc0b9c1bcc0bef02295fbb73010288a94f239402201c09f4afbb1e0d2cf0864c9bd927d3ecb73c861c0771384a0219c8b5820e8c4041210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac08198700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000115e0ff3494a5bd47a127ebbcaa3615980a24aab07f4f63a439d537ec9e9aa13e010000006b483045022100d7faa295febe5bcfed831227e0178003277f22b75c9aca96a766cb88bf46529202201ab6e96dc5f8196b2ce8befa4ded19f5ff1e88b87ab2ce0e4c7fa82ec62a82f04121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88aceb1f8700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001be39eb48943d87af5b5c5fe9ffdbbc48eb3f630ff8446efdd12dd95464a949cd010000006b483045022100885f3a7e24c6b8be8aaacdffcaa21493957773c8d40044e3fa61be8d5bf5b20b02200af7c4ecbf01e536f4223cf7cd4dbde0a2d1d49bacb5c7c3599ca43873f67c894121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88aca2338800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000013e39dfb4df9c8c81c361f76feaabe5d7b8fe8584b1073a749efd3a9579ed52a6010000006b483045022100f5207bdc1f1f0a0ceae6f41cc524d670068d13b8a686ca7021515dad0f1c08cf022076c5a599ca913033c899f410132c20b094b8969a6f6be2754289225a391e4136412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac30138700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001c2346b3ff17f7ff9f2ab43b80521900d926a58ab38632b1490e7d4872776381b010000006b483045022100afd048a48df6063ff492cafa2a63403bc5a23e90632adb824d8977a5b6fcb1a6022043ab81ce959a99c35ecfe855d8a245fbbef213ccaa94b973b77b81924fca440c41210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac8ff98700000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001cd13e5de7c3d4901c38cf00d27b97f65002a5dc225e6dcccf081be25dbf90524010000006a4730440220049b504b4334e955691982d94e3c7623f4256cc455997096f0b7c29ec36e3f63022047ee27cd1f3b566332664079b161deda0dbf12bbc1195a3ac86ea0b6c0b39b16412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac3b148700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000143943a1eb0a99c9426747523aeb08905bd599d2460936b69b90b651d55712797010000006b483045022100a2b0186d5bd601a0cde6b8548a20332c67d6792af5ac3406664c1c4ffca13b00022029e54aab38f4f18bbe9d2c68d5140052f36b3e09a8e94a520cdf06c7f5763a704121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88accd168700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001764f293329edbdda6df5425103024082151a58854f9789364a111c0849ceda6e010000006b483045022100a473ca247d72c9eb901feeab340b70c199afd1012bd17de11c95af821de86acb022039500349119f8e9998f2417e0c2cf804e16e198d502034798658aeebf1601c364121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac76f68700000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001253d3afe56554fd428d269783b3978c026da652b62e7488a957fbec102f9b23a010000006b4830450221008d677334e86b7af053ce28d4f22859c0162ad5fcfe575fca0a00d093ef6d44b802205f8bc123a31c8b77ba21ad8ba0f2893e752c951c4db4da544b53353618817f0241210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acf40f8700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001765c316dba7e334820c42d37b4c77d48c52c5c71e091589195b0dbf7d11c8f17010000006b483045022100a91dca83f1954ec9257fc4486ccfaa84b570ce3cf35adc233d8918b792f27efd022005a5d464db5e5a9056543e2940928ef2595dcce57344e6750b849009e9d8e9c0412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acc8f88700000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000184baa3386a9d5b0567f7222aba8418854b23bb06b37e03f02012e3d6324c6821010000006b483045022100bf252d7a94482b700e17b7157bd854b0e4e496dbdca851e1319e2d47c94ee476022032addf8297c7e325587a2cbf561c899ea19b5a6e68369f25a851ec874c18b44b41210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac96068700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000")) << std::endl;
    std::cout << mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000015e4561ad09b3f1a9268a6cb27a69cd1f62c86299934e99af9373854ab31e4f91010000006b483045022100e3f220641e649afa2d7f34814419772032309ca2e3b2b399cbda440e3ebb96da02200a701f31015cfbe0a15982636f565b79ea9530a9b68cabca43d61c77f6a8bd30412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acdf0a8700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")) << std::endl;


    auto segfault = get_tx_from_mempool(mp, internal_utxo, "01000000016653bb0c030b5a361c197a1d97c565cd0cbbef648a8599a835aa2f66738552f3010000006a47304402200eca9a84f0ccadb3a130ae2f0d5c95ba9ed112a0c7dbe494fe300d56f3c84eaf022021a2c4e6695737d06e860b7714df337d238f34060f08dfb71a1f736916917c204121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac482a8800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000");
    auto res = mp.add(segfault);
    std::cout << res << std::endl;
}



TEST_CASE("[mempool] testnet case 1") {
    mempool mp(20000);
    
    std::cout << "error::low_benefit_transaction: " << error::low_benefit_transaction << std::endl;

    std::vector<std::string> olds {
        "01000000015d2121df162cb4a7ab369e9e32a0d60ec9815e42eaf6aae122903e802af59590000000006a4730440220553035d3bcffc7134b01e32ee78ebcbd7d645e1791cd55d74b4934e213e3506f022049d8e56dc5cba61067beb99e363f49faf3fb770ef78597ee96afeb96777caab0412102ec71b0b9ac278d1d4c1deab2b18487f3ec5af3aa9ed2b5fec34d2daf6232ed9cfeffffff0200000000000000002c6a09696e7465726c696e6b2018d0dc97df1bb11bccd6d10500d569351bd272bdae46a8ecf514e9a7ce5a4d082bda2600000000001976a9148fecabe27a217c9c4ad30f02eba0fcdf43f24c1588ac00000000",
        "0100000001b303ac1f851827a39898a4cde748631840bbc08f4fdddb9125b24974a348c6b8010000006a473044022063e7ab00eaa27e31ab999c47164f3dd72ed9b95c63cb4868ced2e2f26bf0260e0220245192fa6c6db43a356a6da09a5b089d6d97ff5fd985beeda59643b804e5dc034121031368605b0bb15d897cbf82a24b7e1c157dcf169fe53d88da2057d719742995f6ffffffff0200000000000000002c6a09696e7465726c696e6b20e8dba1aeb5854ca70c750b2ac217ef8ff0398bdae537006785842f60bb4515c42829a103000000001976a914f1cb50d97855b854913c457f64ca32278693ed0d88ac00000000",
        "01000000016c3d7ebeb230048f553b2b11f8c391a8378f12b876c84f8d9ab9baf451e39abb010000006b483045022100d0ade41834d108a3da8322b2d793b7f1073a72b2c1cb93016049c37ed2ccdd8f02205819b43d091876b3fff9f9af473bcd7e3a99cf6aaef3cd1377fd842ee851ae9a412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acc79c8700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000",
        "0100000001808e7962dcd1078caa575a7c92ac78a0743876512458e9bdef2c155620a530ee010000006b483045022100f72d76e45a8dc8a5f597ae51442fcba609f7c42b09d81abb5c12be623dd5fe32022039171f7f172d109ca7c9c0b9abf186358e3cf3caaffef16dc243b249bcd50d09412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac809d8700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000",
        "0100000001d7339ce4c845da96c36cc962e822ca057e598b8d38c0310c5a7d1dfd8bca7fbe010000006a47304402203e442074ecdb4c42c50219d936e9086b9b7e2581cc51967ca59783cede04d08d02204a1c8312a248f1be9fd3e3208f4dd8e0af1449e2744ace8fda4738b8f96d6671412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac9b828800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000",
        "0100000001c35da233dd893ffdca0eb492fd55afe92bf20469b670434b423833d89f8a7a7f010000006b483045022100e8f1d98ad285bed1a4e64c4706e8ad3ec28b18f4d5b7c987a736713644f125c702202c0630d7fcb9e77a00471936722d2b6cd195157bc54bdc58243ec374ef033ed44121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac80bd8800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000",
        "01000000010a389fc1d3c006501f29f464299c6112502044a625ce65c4737154d8a430d68e010000006b4830450221008a661f5014f331c7b934ad01c39fc65d445ad8d11a6d4d08e2fd59838730f37402203b046a9d07a48a2529c29d2cdf65e46c28aae1f93613cd9921b1ceaf0079c33a4121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac1da08700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000",
        "0100000001ffc4e3c82c253141de188eb53d19c74c4ff46744da49aa67aed184dff5072dbb010000006a47304402204b4db0e444c9162316372f457cfcf7eade098954770401879d0b6937ffbf7c7b022034a084edb6c25489d4e3f7aa795d62a73126d4ad115a46b6d870f072bed296b641210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac4e838800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000",
        "0100000001dd9f57f7716b8a1a6eb4ac6f19d91c13f785bd3ffb701ad9d8fcb72b50bcc3e9010000006b483045022100eb0388546a11f1b37da8785f5098c22ea4101cf5913f3ebe44ad6fb21f48eaae022031d7ea3b099d78e60a9ee0e6f5c6e22c4fda79335b9d81a04091e6ca87adb36741210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac49998700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000",
        "0100000001e8fce13750e59778d6bd47fdc599236851b8fc7e680ab6ce3ed06d29faec7035010000006a47304402203bee48e4dc2d2faf57ec885232efcddc27a840542164c5262b3bb46d864b15b6022025bdad9a557cb9509df862e6b5e112cfdc6237b430e95c1d206f711b2a5ab97e4121038bbe3c760c28cf2fed54abe683a4a88ab860a4f26e209d679dfba33e281bb4c4ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6a1b8900000000001976a9147827fe05a57f60eba10d31c21810273c4e88859a88ac00000000",
        "0100000001ba984772df8136fa25026bfff3b07d35d259f50c451ea078e5a92d06aa035521010000006a4730440220675dc0222a585bbb11dc3aa72b794dd06c424c95c7010b4033c4898716d042db0220709a8efef19816748178b4ce9476fbbac5e09fc73c44294ef6a031669d2ed13e4121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac54808800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"
    };

    std::unordered_map<chain::point, chain::output> internal_utxo;

    for (auto const& oldstr : olds) {
        auto tx = get_tx(oldstr);

        uint32_t i = 0;
        for (auto const& out : tx.outputs()) {
            internal_utxo.emplace(chain::point{tx.hash(), i}, out);
            ++i;
        }
    }

    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001bdb1ba353b91f33c91c9860ed82acea5e853e3f61394501e5e48a6cb524f2bd4010000006b48304502210081149fdd50398db9f8494eaff4111200286b2f2f93e45c9bd155a04a1629cbc602207a39adf344da455d739acbf9e1befde1cd931fa9a0558ff65f3f5df6f5a23f1c412102e0cfe082d408b020cc9b8c297f22ad94d1a390a10f62dafdc01ea97dfdbe4387feffffff02dcc62600000000001976a91446e1437885b22d939ef736344b798a9cbf433c0888ac00000000000000002c6a09696e7465726c696e6b2084cda8971d0b07bdfb0da84915b33d07bb8265481e4774d0e666f93dbdfb643c00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001b6a220148f412dae574e51e92f072aa90f4baee88f1ad285a736b2f63cf3460b010000006b483045022100ac88e4708716af3e1b19a7bec8fef9b6bc7d7829974997eb1b266dfb3923c689022039d39f76857aea5a8b466fa5a37c94a09a328af7d92a24ae342915b7ab0de94c412102f6b775b62f032e462df94d6d15f69db114bb7feb2748c6c2886a3d4d925703c3ffffffff0200000000000000002c6a09696e7465726c696e6b209a67ec902a1b500d939bf2a0dfb79adf2db3e6d10c6c565671b38806a993fc72f015a103000000001976a914a588cb7b781a0b67c02e87129b8ccb56fde3469c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000014b21ee302772fbc21f8e8259027d18eeea59e72dcc74b7a324eaa45eec31b9fa010000006a47304402204dbd3629acc05b5ba2747ff54c839af0423552f2582d3d025f63b87756098aee02206707a7cce3945e91c372b9778488bce772f5e16fa6cb426dbe8f78d165aa58db412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6c938700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000145421c50d6de761c1f2883031efbca82d45bb61047ffc58cfad5cb90b01e43bf010000006b48304502210082f333b5716010fcbd58846b8b46ad94fe823dc1305d91c2df2a7d10fd70cf8c02201c3a07e6426e99dd7a64c202fbd519a58246390aa6242e2478403a0ac7d378c0412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac2d948700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001a857e56d293466fa88f386fb3cd6d8df852c3d30194156cda267f2f8061ebf25010000006a473044022016cc935298f95fab304e2db085eb59e12d7eb289b67b65e70cdf4976e616b3ab02205adb5cc44e69b4b7cf001c9d65649c7a63c61d7f4b58d022cfdfb909f28a56ef412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac4a798800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000145d05cf52335339f2a0b88b37e7647e3efceecaa614fca267fb4b0add6b42d8f010000006b483045022100d1f6361837e434606b4661c3492b679f9642937d3c0fc513a891e5f333a539f702204be6f6eeca3a944e9ab20a61612a84ea989f7a3ec9a4afb5e4eacab1459953804121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac36b48800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001042285e129734551ae32b1950291ab04dfff158d4f751e7011c427af285173f5010000006b483045022100d302b1f0a7cc41b48e4e4d8adec47fbaca4736d0f59b48df98cf15397f7e32d0022078bccb24b4266b69178dbba497cf02a96d0b28491c83ca9e543689bcc29f52344121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acd6968700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000106fd6ad50034509df5399562d8d108ea448553093c721ca9aaf477b98a589e61010000006a47304402206ff11e73e3be12c23667a161ee56f5e7ddb0f861cc404438cae1cbbef40b135902201793b598f3779b5cc367ae78f46549396afa2c1afd8ed19ca580ed0aaef9b97841210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac377a8800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001fb560d9e0583859a94234f651ab65b1345ff945c2b5dddca21fb004800ac3140010000006b483045022100ed3aa767f3f0063fcaf49a3982c6c9070e272e908b8853d3f5c6d01bfa26824f0220616f4c034a10166fad877019f820f7fb80b04ef1ed17496559bd149b3f14a5ae41210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac43908700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001c22a0ade4102c1825cc3649a6b12a44ddeb8cb699c60b0c179b6c4042037608b010000006a47304402205e1b22bd6a3ca7ac71c84b50d961cfa2b4889a8e23289a531bac9e7fabfa502b022001de4f30e75398fb540652f086d4a5b8635dfe244d92f07b1cc10276b8b64d654121038bbe3c760c28cf2fed54abe683a4a88ab860a4f26e209d679dfba33e281bb4c4ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac69128900000000001976a9147827fe05a57f60eba10d31c21810273c4e88859a88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000016436cbc1072170e7672e2d93d91e15405b66576dc40f296ece6d8ab92bee1a05010000006a4730440220728b26996a12d436b78b942a2bf9c532056bd8fb93c42672e8ea7fd8ac2780e702200b17bd2e1361441e8385f3d37ed9606cbf8b36dfe4a8736904f7823a88dd7e874121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac57778800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001bce54d1806212a7cebcaadbba13e482f9fd3cd298d48e296b13b0d596fc5afa9010000006a47304402206e8faffd8c1e9cdc2b960bd61a26bf10cb3f773abac749b6f670cc3cdb5776f102204d805ff292583b27d683cf4598b137e835aae687e19f17a8d95f107fa14ad0564121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acd9aa8800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000011192925461debe816afe95a562aa1e6c94491d949af45f7d0a8e87711a20bd23010000006a47304402205b2088b43fc165f660072a3b3f31669df67b38c80dbd85b8bbb790adfd9a1baf0220647dcfc72b91451bb92eb67f6d666f5b303178ccb16a840e091886e8fbb8276a412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac0f8a8700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000015ae3137691151eb02138aafd166ffd5b5a3b3d74907e06c874b41eeebce09fae010000006a4730440220426146162387c20de0aa8e0975e018ffe735fb38e8f274425041356d704454de02207a17c6f993bbf73725ba75e16b176051a1d7a8af7ed470ef64ed464d8afed22f41210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acda708800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000164b34a9cb7b1caa540d830a6a04e8cb1cebdbd18a16832ce6dd80abf31b013e1010000006b483045022100e882bda032a31690e36368e022e890d627db34df9495a1ea9a2d925cccd888ca02206bc3624329f77bc4126b0212dc711ef0cdefcf2906bfb4e8353b72d9daa8a545412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acd78a8700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000017d1619061e8dfd13cb0098e2be8d7a1786667b9467400e7984e74b7d18141f29010000006b483045022100ffb05a1502e2774973cf285fc056ae962a558241642383086a6e7a293ff13cce022079de13d52999f6b7e14f3bb7274a1a8dc5c63c43c10ff1b6c79260622d9f2f5d41210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac02878700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001d8ec0ba27b16d5d4dc5af2dd5304346504b42b9f58b02bc5ae4a1dd0df258dc0010000006b48304502210084d302b9576ba166e45acd1568adeabe6450ba4e4e14935c8f876884f28e5013022012fdf27020cdc8e5efba0ddce1cb6831608e8718af74d7a8c6f77f28f3ec644d4121038bbe3c760c28cf2fed54abe683a4a88ab860a4f26e209d679dfba33e281bb4c4ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac3f098900000000001976a9147827fe05a57f60eba10d31c21810273c4e88859a88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001c8c00de420737a054d1604ef11a0f7639437335c3f9007d4d97317cb29a3826c010000006a47304402203417d3030e78504c2696411c3df9738dc2b601cce2e3c7e8aca7091d5462b3f202201e956e2458d8cdfa7812ddb6b36fcddf92d98178e1836b93b47a2433c56b89984121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acc88d8700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001c7d2b9e672848287161f3c42d0bd64a51c88540e4b90264b6607479f3fdc0c9f010000006a47304402200290ab868ad305d8f131d594ee95bc1818058fabc891a5cc4f4a3b6c89e491ea02206778b666e75241fe44a6a06d8cd7d9451bffaf8597752c892414932e00d778294121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac4b6e8800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001c7e2cb11c418c4b87f27cffadf96fe48b241a52e01291859782f52c4d533d869010000006a473044022015be5217d23564de1ae8c0670e6b4c714cbb38f433e0310008e63fbd4b0f77df022071b4b27d63a50a1d7767200b2b8f556b8414ab9b22cc5c00c37138f553e3a5de412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac43708800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001af650160f466f16217f2aafa5c462f7a352d8ca382067d8da2fdad821315e15d010000006b483045022100df15bfd84002f2ce909188edc7bf3c0e979f81333fe1794499f81730773d341c0220017209cf19f37aec67547a208cdea938ba991809efe7c4992570f5bd7a77f82b412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acee668800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000178dc8033893bcc5231a4b94c8b425eaa9ec85266a14d528379c7a0cb31ce8347010000006a473044022049ddc7a7a2ace3395709e8f2b30ffa889d0856e09fcb39fe694722b64cffc5ce022050b59c14da59487b487b5144caa357bf93c43391a90b52806bdc59158291c7db4121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac8b848700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000019e883010152dbf368ba64ed66d9c40e7659228ffaf17d15e94b684cb54e35fb6010000006a473044022012a65f5468460644acdfc7d3f97a6be3a6194cbd9073e5b99ccf6f8106f6f69102207f7fd7b2894feb31f4c45fe01a24c24d4c7277d3347753c6ac7374011fdfc3cc4121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac9ea18800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000184b4a02010dfdd3f28651a8275696c4d81ed579402ba2f025ff41f4722659920010000006b483045022100efdf6b89caba928e949b4a6df8b2320d917e64a5efb61b3680ea0d31c0884ef4022002e23ae0c49b2e5f4162d9ecec69dfb0e82b960912ff63a183567b003e10b3e24121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac14658800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000107033e6cf00a723ceae6a6d0ad61187301b48731ff983faa627e83a6802899ae010000006a47304402205ff7db7dc3a1d31870724dcaa8774e4f27ef45bf03e026993510a1c5bb0eb7a302204849dd3e56c642174f65b72642523df3e4e8c1fd835c920f93bbe6049b321435412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acdc808700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001a828dca6063004575be809b5d187410290fa5286b70a9624a1c34b44e7e44b17010000006b483045022100d88d1d4bb0d35ae26b8b029108061fe8d4a69f0862f8cddd2b46393fb1d66a2702204e70b2492cfa9bfe29917f0c6f5809e3f4da2016b74003b6647a4b98abd6ae2641210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acdd7d8700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001ff6684a58ce9166b16687b302fe6ae46e556c65d263f53419d915bf04bc09480010000006a473044022006422eb639759cec3da906a99f19d5005e879fefdbec114b6fcd15a133a9417202202d0f03131ecd5d8c83c2fba494e73fe2b3cfb3ada2242a4a42765c335deb8b7a4121038bbe3c760c28cf2fed54abe683a4a88ab860a4f26e209d679dfba33e281bb4c4ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac1c008900000000001976a9147827fe05a57f60eba10d31c21810273c4e88859a88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001e6430b0ece8e32a7f9a2a85c009ae96e0b2fc95f78582f4d3e14baf43800864f010000006a47304402206382a98b180b4165b9c2d96ccc1d1b95709b569bd778f2169cd9a8e2560d58900220265b456f2e1aea56cd24f4dd6f6664eaad683ad898c95d723cd6e2a621bb8b2541210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acbc678800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000012917150fc6cfc8423e8353ec00b8f22f9da23c15457a47b3443f4d4bb9f022d9010000006a47304402204f80a8640851606e8cbd19daf105902babbc62706c6e061cb7cb7f0180eefe440220741e4219635169dcad6b49f61ad40870e3b622630a7a892b92af88233fef107c412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acc0818700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000129abdafcfe3d40081f82b3ff422b6f4b2e64e9d8bab84d321ff93ce37c9ce409010000006a47304402207ddb47b030dc0f9805123fa84c4763c58b7f7128671b79083184b51354c79c7d02206567d4644dad3df5712ce7136f337e1ab99d45df4ee7cd61ba3c7de88e2333ab412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac935d8800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000015a7adc70f7e31cbf2209c44a1f0a11ceeeb9205b3a0734f27ac3a14db1a067fc010000006b483045022100af071131a3d9da47eb684cecb0dd181f20d155c8668e10e7c6712987401705760220032ef518ec3b5de1e35c249402a7332a6eef002c1abd2aef47bebe446df716bc41210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac625e8800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000198429ee724acaecf8cb3d0d951dda5c2848f95cdee7f8b5020c99a5c1df9fafb010000006b483045022100b6a0a3d4fc7a2528514cdafb3960a340d3310bcea519a5e4b4dc88ff4c9c72bd022060a1cb3966009a8b5ad4b559182aafc9e71b815b3968c1600283762607ddc0ee412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac7f788700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001f947be0cd0a625b605fecc1f742f98768ba253310445555683ee8f2d5941d3a1010000006a47304402205c0c195bcf745694d59f2dd5f051151dc6eda91c5c523a3da15560bbfa81688502207c75032e4168e1a2bfa09913040d11bf815b9ed610de5389a2da57809d30cbf041210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acad748700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001dcc1eefe1a5a3525ddd06e739b60976c3d9aaacae88a0d844e966d45b9cf1729010000006b483045022100a86a7dfe41f4704292305f38aa23cef1bb583f0c39112b663ea326dfad0bec44022068c83c87524e4c4e90479ea3bf3b0e958500e2bf618ac8586928b2e92b27fa4c4121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acfe5b8800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000106a20e6875f354615133ba6e23255e509c9c6e328d954d32eef4c228cf2fd52a010000006a47304402205c1ba7fb9602dcb1ae05e1be19213dbbcfc2116d18b21e2dd250ac4b798943ee022013fd0c11591297de111d897d413199636ad0abb11b8802244318b89bf0f421f0412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88accd778700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000014253946a0dc23d648c6c983d8ef63b05c2a3f5027531d50c4930ad6e5b486b4e010000006b48304502210084fd5e8a52d4a24f00d4ac762ab887032e4779614de272636fd61f57a1071a2802206d3d8e38ef9aaedb106f41a0e4a003a0758d90499f9ee88634b4b62d1f7b89b14121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac847b8700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000019fca040eb07458ea9d43e58911fd1168c50e9ee1fa9f5dc2f17bf1d032bf206f010000006b48304502210094cc6e1002ca3b866e63640e952fe3b274490a545f9ca46077522bc05c11fc6e02202b5bb7214625545128d84a71c69ee893cd9eea5678f37ab7ef8181b415e18ebb4121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac9e988800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001d7773908843e54801e33116a39a5effe93959ee0ccd3f4df084ab9f1a4bbb7f8010000006b483045022100884d48b3316410ee36ddc68e03972e9f0f49f02884df6d91a393911438b6ecbb02205f899fbdf9dcf54396d82cf819abed7f10b04cceedd24242c5fd5de0b919e8304121038bbe3c760c28cf2fed54abe683a4a88ab860a4f26e209d679dfba33e281bb4c4ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac20f78800000000001976a9147827fe05a57f60eba10d31c21810273c4e88859a88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001f090d1d4a0c757f77590082ade02ae697f19e8280c8291c16c489e9ac186a5c4010000006a47304402204c5939535b991581dbae6ed6fdc04f741953c02c7225ddb281a95a33ee3c160b02207b129c77cec4fe4b66426abca2515fd94408d1b8f01549c10e6387c9b9e54de54121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acac528800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000010ba567d0c5322b211c7f8822c32b9c9f60c5976c3753dc73f96105b4d2aee448010000006b483045022100c49c21cde35474b9b87fd15ac511d2c2388cb4d74a7f100102f7d2f7dda892c6022014f764ed8a8314936aa22324178504b6a50986b1573f7128b948da0f4edd671c412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac4e6f8700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000011966c6203bfdc580eee31a72a70cc2520d436672d54fb8a7cd3cb8ec408a1828010000006a473044022061659b3ed3cfeff54bab45ba30afddd1a2b9c8d8822581d377179a4ef473a55f022009a8c3b59f865c35e4886a422649c68e7059914349e684741d7e43964250e70d41210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac7f6b8700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000114f6cf76560e548ab7117a8bbec4b89b9e1adf9be115ad0e99702be4080ee5b5010000006b483045022100844e25fdfc3de20d29b0d5ca240b1065cf2fe4ca4bacb391cd92b68df8bf532d02201aa55d685c82f9282887707bc4de995ce3abf044abec25fd9a1d8630ea1bdd0f41210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac36558800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000190fec6fddd2d86b0a1d435f9036e49ce4ee676781882528daa6ec1be478b277c010000006b483045022100963e26f05d44d802b89d45d78b83786e0d02604b5de59d9f6acff747b778c3a1022078f14922b4612bb135b001a4cdd5af057ea997d3a469f8307af217613716a6384121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac7b8f8800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000018d80b834b4680c940a8552836e9750e0a3fd4d7804ed1011b1bc3b34756e5a48010000006b483045022100ec633b6bbaf756084912c7a80aef72c9aeb1f1513e879edc09bf13da4c03380402205a54bdd9320e5a80eaaa42231d7f6e9893486874d8a26d893325dbd5113b8284412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acab6e8700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000018f4c689aee9851a7c058464d5eba85acba56c9361266bea01e4452768387a782010000006b48304502210097d7c2df520b8b6a963b7f368105ddf5ba993d809316fcfe5d20faab9813734102207bd1192331506bf6c5ab1d68dd8bbb109af8196d2c2da2a454b68b7e494dc26c4121038bbe3c760c28cf2fed54abe683a4a88ab860a4f26e209d679dfba33e281bb4c4ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac08ee8800000000001976a9147827fe05a57f60eba10d31c21810273c4e88859a88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001c9995dcc151ef7ea24e95ec6b909ec9061cc6e972f69567ef9bb0635a4d7fe0e010000006b4830450221009672c00560c040afa2d8ac0f362dd366cb62394769ac6d6a5744ef724fd42454022047f7420b2decdad5da92b4e46578c86f8a1dc10638348e8e1078f8bad8805ef54121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6d728700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000128c4ea82377f31f672fcf2a2e17f527eb8f5211e8fdbc38592ab182e23bee302010000006b483045022100851f80830e8fb50a52fc301d9b887f585560e48c08f991c7bbe574b5762d31af02202f4f2416d56dcd4e81c6fa36e7964995b9d2d51ab38c08f229254bbf5632a595412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac85548800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001fab7ca52b20ac3eaf7ea80d6bd99b05c3e517af269afeaec359a95639c33648a010000006b48304502210099164daaa87c962e5a020e4ecc92edad364012eea28e491168b1a3e1006f730602207bfd2f1ac5194c5b4a9818e415bbbe4ac61565fd985037f433ea535287cac5564121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac2d868800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000013209b604b961edd6a0c8dbdeb86137bfa964b124bf4a74da9c92eae675a2cae4010000006b483045022100cf7594e6a47c01ca75e6fa8452f8ce87eeeda1d7279103a2acc31724d2071fdf0220154fbe3f3e53c47590066a2cf26026bd94f1f4e6a15c61f1b4d249c89982d7c8412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac3b4b8800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000106eb59234bf4810bf24e20901ebba8d538b45e1e12c4c62a51d753a8d05b2d25010000006a473044022014c40dd41704cb58684b4a53b8859398bdceacb3b11720570cfda2bac2d0010d02203c047cf12d938a218ea554cb135e372b33d580a2419ef758ab74baa9163cba204121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac2b698700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001a26b203605a801319ffde29ffe36af9608a20964a5d9d72cbd870182ea58b9d4010000006b483045022100f638891931008793af719f81e7867015435aa905d4f58a13729a1f0d5a027d380220378c4c7772808f2288aa193a2356e8810f08ee7182f5258b5675b176be0c39b341210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac0d4c8800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001a9acbea0d609a9e264ee4bfd65ee1b14f0997211e2e85cad436d0bc5dafb6918010000006b4830450221009adc20e7aca3f94abe42cbbe79cdfdba67615706186895997fa50838544d091502203ac7e5d9d0748e4e63decc7c2c69f59f147355429ff19d4f1e32d3245a8b5379412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac29668700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000016d628b7c95a6bc1ec4c16aa29929f1ab11db9bd6bd0a877e235e27e8d2e4def3010000006b483045022100f05fc6acabdd57954251c713f1b6008706697ff39e3f24321656f215402d1914022024377b0b2bc96b2228f77817e6af3c525102a337987afbce0353fde097be268541210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac77628700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001174adccd8ee3264cf77ff5b5941c700247b7fcb49047fc51159ba762e4e02389010000006b483045022100a80e134de21076096cfabdef3d01dd69634337413e7644178bf51410ac5d0ac202202eff3cf197ed4f78e38b2d2af09ee6c8658857f23bb3433a950ab62e754a3fd04121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88aca8498800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000190b9ca4c3f62e0cda6f55ef1005779eca591a70972fdf8699609230d4c04d4c4010000006b483045022100df652ce9fa31b00213d1a5f22da633c5b23fa2fc7e8aff8bc386b0cadfeaebd3022004b0e1cb1d779cef107c74886f752de84c63f9c326729b44c3167a0899983efa412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acad658700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000124de049f062778975b7e6bb7e6a5b55adb8abbd38e2457c4ec03e0b045659f02010000006a4730440220280c7c9ddc570ac1296bff407a98de60237d60039c96981fe9f70c3cf586cee302204077547813bdb7f17c62fdddb5b5cc60d00f3761a1202b892c1cd59feb0aebeb4121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acdc7c8800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000015a2c19a46ad01a1f0203aa59c5c7889b39e28409fe9da1751a9f9b8a879bcf35010000006a47304402200fe08d76bc40b0d021ed9f979197db91dbb9625f866ca89abc0a5ccc1d35bf5c02207a51ce8b8a6370a73c40d947c502c8dc29a766b9539522a337ff2b561a8baf894121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac59408800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000013927c262699a6bcbebec1598ba2214a2d36118cdc637c557cd6ca24adb48c87d010000006b483045022100d19b86714834ac49ee9728b51a0fdfbdf4d3309118be8de229092c1772ff5494022051633473bb3dbfc6d452c76d7889d360ed532de4b9650c41ec96dbc29a5b43b141210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88accd428800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001dfc984a2a7999d7dfd9f02433bbd943f69bf9a67f6ba7e62bd0bd2563c556e61010000006a47304402205df8a30022306121417e8ce8055b2dd9523451e65646e971b54e8562e7808699022061bcd358dcd90ccdd942e71cca4fd7d98da3b19a0df0b639856040732c36c5ce412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6f5c8700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000124382ad7090aa9a4e4c473e78809ff97d92d49d053a18c3034725998684ff66f010000006a473044022069616ccae530c0f7785decf172b287f36831464b8ced42c96a990cd3b074bf6f0220169c2853f64ee4aa1959765e3920ba31a4bff405567b20856080ee290ea359944121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acf05f8700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000127ebdf990948ab13f7076fc6ec3f60d437dceec7885258d55ff51dcfc94231cf010000006b483045022100ce4820463c2226f529e43d1cff88149a581065f4078067076d708439bd11ce7f022011db9d57c8b8640f9a7ecf0ac3cb0ede47ccf1e106e25f1cc9158081a5f62d0841210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac4c598700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001639d8079dde424c27c4b7b819551faff06254d402eabe59e5bb90e3571e4da04010000006a473044022009b41495f5d27fb23702d6e66549ed8ddeb2a9eb1d12a1be3a2069d2ae67a6f10220061911384b907585eb2b2153b63ba37b852f7836be5651de05ca908bc213bd54412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac2e428800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000018c7a4eabcea6eabc554ed97762639c103aba67ae3d11a52e20735ef1d0028c5e010000006b483045022100db1ca9355f08fc8d3cf9b8fa41a9f87e7f0112d0c2c278dce0116afdf396235c0220103d27aaab4bf5854ea276b6ab33ab6c6ec29ed11b2bc405a5afe0dd52d89410412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac205d8700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000179d763f8a3c193f74d4a19dd7b3c801f607bc5b8c140977a73779d46ba8b45db010000006a47304402202061399c2363ec9a9a51610a18689d5134d04463969cb0d355bf9ff3b1d6317302205cf5670dc0c5e5efb0d50c77fd55128e37581462de80318155b5e9eb3ee1bd334121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acff368800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000016633318a75f6a8282cffc3df5f237062e9b514929249d3d46bb65287f53f58d3010000006b4830450221008aa092e55dfee4448dce691d5be92be9e5eeca125e123434874cf62aae9d1fe80220709f152a98536b773361600588d1226064712371ba0b95d5aef8593c8a99c53e41210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac03508700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001a6f81e3d7e42b1e8419e1cb9580f8d580f6309d8cc2156cf1f0d0ba2cdda2beb010000006a47304402200b467151cb0ecd985866b485a42e5d9beb4f37f57876ec66974d3c958e5ce8a00220094309f831c14059eef5b65a751ede763e0e533d6b8a39bd70cd5089da4dd7344121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acad738800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001ea4f394de2bcd78b85aee1f70135f05b36f3e93cb783baf9c26092fc5d269e06010000006a473044022002f9a063314bf9c0866c5fbd39df991e3effe426d59649ca7e02e9accfe4db87022064e06e69daec1f4ea0328662535dc94b309edd55e614c42391315714aeaa58f4412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac03398800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001730e71adc7b397f5b8a8c413ecf7004d5432c60eb32128cde5a2c62fa384fc9e010000006b483045022100fffc2fb1ed0c3768869ac718e53a36e08dd2031e14529c45d2845065534ad2a502207a74a85c1343de8c4659fe40be87ddef4dbdaffe927d462153803edf307ae5644121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88accc568700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001fe676d36302c96f6cf34d3b217dfd62ecca380c9316033dfb665a0a06ce34c1d010000006b483045022100c82bf3ed1644547762ad5e5c5584737d3492241226ad5d69336b73c814b09f7d022005bac191b1549df2577a241d3cc4e4cc1beb18e87468009d4c335cb33f431b64412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac04548700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001123e243eb3d060399d847a3b65b1c3b99c4acfb0accb9e970412a2b42bcceed4010000006a4730440220492ae330bc0d96f6a598f12741fb9bfc36337f2cc7b315583d8b05d489ae9a8302205503c8e6073f4a182ee45473082ec66e7f6c28fad89da10b6103e15bd831ee20412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac5f538700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001a81a64efcb113f92497bfb27f43ef11321715821934338b3a8dd498b0656b9b5010000006a47304402206ce2de106c63230090e816dbaa3184bf655db1bf008490194838d5889a2756b502205289f3ac25db323fc300ebb66a31551da71ba2347d7830707d5a53193476625d41210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88accd398800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000018ab732cd67a4a3c468465da0ba4e49f1f64d7f93b919539ecc187874ba591779010000006a47304402206b12c068b5492d8cfd76093996103ceabeefa06daad4e26affcb16aecffa135702204917074e398673568123db453a548402ab147d21c7d6071a0aa652467bd4fe314121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6d4d8700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000119c0090466f4962f445f515c51c8ac00b5bd7a54ce8408c6557ef4d14cc8264d010000006a47304402204715b10cd9b71e4bd857d308f9b2ee2b86be9fcb555692cd62c8968fcfbd78a50220610c74a3397f5f50b8464c67af7d3f87847194c9295e3753ef9b54465c760b02412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac024a8700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000138bc711b58d5eb79b41805004712d7f13dd39bc4afd98f239439269b2434b7f9010000006b483045022100dcf7a55846edabd3ff2f5a6e179c3547202dabdc06646c50d0d256ac2eefa026022011af2c1be76c6d342be15812697e824000ad94299fc76cd7f4087b0224bfc48e4121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acaf2d8800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001f33cb6b20f48aee5ca3de13f79649c2006ec6759411f4e608d105fcf4de36095010000006b483045022100df51c31530322707c7df517bf15c484f98be1d41befd8705a6b58d32bbdf69be022001db73ccc586e1dc5261375a6bc123a5c29d787fdf5780102849287ba0e99af14121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac756a8800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001736467afd4f43734eb441775bea9475a9d0f9e07c69b8ed1cf57a59d9f6888a4010000006b4830450221009c3f3a675459bbfb91f25a98ef0f7ff37b19cf1da405041111d0570b7fcbcd34022047a4ba2f7ea40c5aea5b76058d80ea8c4a0f591370299c36e5d5cb55a8547bf141210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acdd468700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001ddc16a1ab42b924b6a8c4e96ed6642f018a2e57ffd3856a2632a35da1455b756010000006b483045022100a14fbd467fdf129bde121b3e1db09dc5a0e9bc5475217a43608d9185791bbd72022046a0b4a1a4a9b2eed1bc756e1916483777890dddbb2cc8482a6ad7f2174d76b6412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ace72f8800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000190cb748ad4c79226eb4c383879e6083ea5aca58b9fc6bf3c9f74bb2c6780018b010000006b483045022100fb4073924321287dab481740bcf54f684038ad3a7162cc36049da7e7553e8e5502201627a05be06d37ba33d19b2e62fee85e84186e6eb1db83da50b55211ca4fbea341210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acc7308800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001119b251e105188d8ec25f4e430b16664735b6bfbaa0e762c204f54ade7a35fbb010000006b483045022100e60bec797f275ce88b699c92cffe70ec8f0698aff33cd927314c1f1eca74393302201361ac6bbe051ec437bba00cc8b5913aee4450085f7175d82c79e7f8aecd4a0d412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac004b8700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000019eaa61487b7bcced6a14f41b86e0406bd9d2aa524570764542fce48e19406e60010000006a47304402204006ad26b099c232cefa4747843746002e6fcbe96b1e1d34554a047d5fdf442602204bd612a361309595d5b5c8e1ae857f2346df3c3458b5b9a861186ca6ddb617f6412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac98268800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001d33bca546a8dbeb7f83cfe6fa9faffc05120707bbf4e2c918232e093aae3d9d0010000006b483045022100c445fb74a32a1a9cba5e2f384ef51d186e5297c58e2a0aa192b75a9b6f48a67b02205d5ef3a934159934e702d00536d8f1de00aaea3fddd029ae8207436f49afc0d24121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6f248800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000015079f8b799e0ae9aa61755c1c101ff0f730e7f8efa20573cb5dc4b50a71a3f90010000006b4830450221008e92ec1a8689a0d6864a09dd6520b5581ac7240ea9587818b92db9f578f43f78022030d7b52b0683d18f08644542a6edda0369165ea13d47340fb2ba70437b8d6bc241210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac87278800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001ade5335915fd688cacc904c27e679c321efe35d7b39c7d3534ec0e449a51dab5010000006b483045022100df9124ba65065452fa0cd178c285cf8dd9879a2604eb6a7dec22a8c7dcc8458e02205cbfdfa1642ea6edbf7e2742be5d84bee69355be5e3981faacf69df86b23580f41210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac9e3d8700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000015492ca8b642bf30c920d70d0e920df5166f3bdd22541c844fc087e289c624787010000006b483045022100f2da1759f7eda9e0132265df2a67aeac94101592ade0a7f2ef4cd3bd9fc5804702205eb7d29ee822104446c008eed7cf78f9b4bf419428a25744f835b3cb54ce42654121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac32448700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001d988afdfb1bec8345a24bfc72379dab42b20a763f8f288e76434a5df7b1b5cc8010000006b48304502210083224c4794b83bab8a2bf1956e6354c1b15096ef4d170a19bdbe4d98c15348fc02202b1b68f07cedf02e272f60707b1938f2be6ea76ed5de69b491639be92b67758c412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ace4408700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000019b61b1537e089b0ad6a6af54531760efc97483018937adfe8c40a252132f74c8010000006a47304402203aa898ec79faad5c9815b2df9977b6bdd5a05bedf864fe27211e51a923f9df02022061fb34fd349ac367e7d6ea814898aa12e8a1148b3523d5f52ea0a80173986de0412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acf0418700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000013e94e96279734014c83ea35bb80da359a67409143afc8967fc8f88d2fed098d9010000006b483045022100cc2636895c944c06c8737ca82a0e5d07dc03c62312f9bd0a0860b7a2244999a8022041004c5ef773bdf73b2826fdc570b22c1f49927d943d1a2b7b6727a4fac371ab4121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac79618800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001d5671f53cec0f9cdf803cda75c5266021bbdb6dcb0f39e8bd62d701235fc16c5010000006b483045022100d9c1d5da361c22d1bc0209706a0dfd59e7ba7a630602362d806d17a37b3ba01402203da284f049d2e11be6580492ea545ef139f975af6860d0549aa5ba96b6deeae04121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac101b8800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001e9ba7506a151371fd7e8c3c03f41261e79d830c24fd1cd4d8ab072ddcb4f8b28010000006a47304402200c1e595cc06733c74abf42fa12e9cf07dd1550f9f526f8d7ff8b2fc3a89cd4f502207eecdb9a364a8d1062359c429af5e7e2f3f8d633e10110524ba37d742d35b6cf41210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac521e8800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000015b478f8b66078ec7f24d009f980d9196d0daeba290e4ad6dd66312b189047b59010000006b483045022100c12a477a370926a9a34414de895e8dfcfc2c969747c70b83431679870cc9163002206cae664a7cdfb3cf0b31eecf131dff43178df214614faeb4e5fcc83b4952d22e41210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6a348700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001a34efd6da333b8e1fa4513b8e75755d984becd2402e4504902bff87753b74ef5010000006b483045022100c5f023edb81a540d5db49648697092770cc0b99f7206c91f1b0ee2b204f7689c0220173511c318ec1789ef5d29fdf7e2f20aaa914cf7c873058f0669b94256b5f417412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac661d8800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000019518fd1f86d33db3b7e17cc58b267439082029b8a057242fefc8214a3e87fd47010000006b483045022100abdd19f3124028588a319632bc84ee3ab62c29dc32a39679018e25dfec776d3f02205cac56206a3ab98fd77dc3305c1c10468f19caded4f738bc26c9854eb9c061e7412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88accd388700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001dace75b91109d4170f5939a050fcfb9c33a28033950966553026ddda33f599eb010000006b483045022100cdffe9b165334e3c7d708345a06ce257374e6637a697d3ceff58731f1147e4030220734cc1be372db9a85459dd9b39bcf7004cb0ad1504ce64ef0225187fca9251514121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac1d3b8700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000011d9497f51ee2a0b269902d5f87a6704253b968a5fab42d3cbc212a024a4babd9010000006b483045022100a468122eb4ddff2a7706131b96033c8fddb4b69a2ca1b86ab52dcabbe926aca0022053a8c85c6da9fa0be4e8921b29779d1dfd0e6dd3039a5bb8e32cea3376b619a9412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ace2378700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001a3cf1f6fb7cbb598be3c974982cc38af03dfbb87a38c36c28f0ec6a6e4ec46b9010000006a4730440220781b8944db8484d6731302381f4514b8e939d55df17af832fa96fb8c85d58a9c022002c325932fbcadebd7ce23a45c4b9a7f89836e0e55005a9047501e325a74b6ff4121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac7a588800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000199662302e7daa2c24ab70881699c8085eddf9a8c801f1c57e3f2ba44bdd12c84010000006b483045022100c6eccdb89d651e55e01354fac223ae8916fda1814c9abe4e9d9645251d4cfd01022011dd2e886703f47b9eb43d46f5def9d5636f8cb91bcac5bd4b8dced2f0afc2154121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac324f8800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000101c1148cc88421b18a9f5087d528bc0f10ff8bd8e0076c68fb8637422ade94ee010000006a47304402203cc083782d287e0a0f9d2799e824c2af247756fcaea9d432985bd970577a8efa02201a4b25689f6e939e3198cb88bd940bb880903172584a2264f202de94f03bbb27412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acbb2e8700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001142abfaa54c48a1502c67a2f4f82d5d0495f6217df141dd4c4029a97325e4601010000006b483045022100d51a3663c38904529dee03c43c55ca6d96096da92c3319ea72a78dac95d96bb0022033bb4a9cd114ff2e249cca095c67a87323c23c9f2b89baa0cf6e4139c7c1c1a0412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acaa2f8700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000015d44df8bad65f70f7f16792572a74b438d5266d1d1cb61d00a5416e4127cf316010000006b483045022100ec7a4ea0c3f0b7df609f772c157877dc5611e49ec30d004a12c44a95a117099502205764744d4e869a771388138799d40cf28efa0994f2b8317f46801186e681f26a412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac46148800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001eba7c383d98487abcf7b56234507af9a303fb91934a826e531c8421321ac11cc010000006b4830450221009237eebb8b74c5a73c52c5a191167c899f8d7ce4e8bbf60f5aa6564de77429c502201f2ef4e7a149b7e4858f962c76524bbb4841943be3aa493e3070b844b75b79ec4121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acf4118800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001002391ce1d1b03694ae0d45a80c16e931bd81b0bbfef0c1ca0537d115a5e5d98010000006a473044022072aab2bdf6c3357d7818c2c8a8782e3855369989e84e0284c7a9f13826c9e98d022060b9c2c42e2c0cc47298bc53bed188b457f5770a1bb3874c7bfa3a79df70b6214121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac04328700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000016cb4e6678704e0d47746490d5a0bee88423e0ccfb2389671dda4f0c15886ae79010000006b483045022100b808228aedb5b90f29877370e917b03ace8d636f10851e495bd6c4a4f277b0cc0220554701460ffa713c577f9998a3806a9497bb96d9407ae40be691a4c4a4f5321341210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac38158800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001d0fe2195e00e9b4b29f2a3ca5e10351eae4e028aa4b6843706f9583fa7f189d0010000006a47304402206495de82c2cc06d3e0cd2cb49ef173a75b4d44ee368709cf839715ae1d12a317022033386807d26ad3fbd52b6644386fd23557373bbd35f9afb9a1584f06a04023d541210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac582b8700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000019a4bba476ce57911f7b1c62a43f8cab456a53c19f12c2b42c82f63eee3d7ff1b010000006a473044022041a7c53ba84995b3a9aea37b8cb1445e0bd449a5e2bebce954a355e1bcd7da6602204596b3a80f2a5edb91e304e388e60ea9281f93400131d6fbdbfca45226dfd2ae41210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac15228700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000010226f373c93798c7977a1f94bd54ac0133183dc777f5b07b2fb731c754893a5a010000006b48304502210088fb432b96553a3a9a2944a47f52e400aaaef02782df071cae874c1ae503fd73022005a90ae2c20c51dec3252030142fd14857c13f0722c7a1f0298048d00f9a77ad412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac73268700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001ab9ad5f58febc9375b7a71757bb62f21556c4e94aaccfc5680976b847d17d348010000006b483045022100873bcbc8a815063e8d77764efab8a2d6b5406f082a0317513fcabb86557f4a580220749cbea361c69f3d22bfc34118b2926554ff566f4b0b16935cd63c050322ac33412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac240b8800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000017427d5a5ed45a60859e47e0b465a43f5a38f962a1cdcbd008a9c15dde2e24230010000006b483045022100847d5fb8d6be443068c6e0ae26da986e794e475a0465411dd70eb631c66f650502206366a7938a3da687cbe1f73b38aef87c9a4f89bd3f9835fbd79d8f555c24f38e4121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ace7288700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001b2713b2c0edcb6363c01bfdf421385c8155b767f40237165b0f4a9376777db13010000006b483045022100f3b0fc1b6850ed39f19548241719a0356cb3ca32f27db73a8d81298a74956f1c02206a892936481a1445dfe9a3fc7b92b586f0140be75df9fcc9f57eae0f6bd5db2141210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac1e0c8800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001e77c3289499b2c42963ef6fb382e38d234fae5520225516a7889d4abde84c77a010000006b483045022100fe0e413c1680c6d191d58b16335c6ffeaf2bd211325e9764074c998ca5fdc155022001ead81ddd073e6a4e548e8dc9f90d6d97b97ebb495cef9e06df680ef78f6c0b4121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ace6088800000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000183f60f70e04b00f51d6e66f6d408c3ff8df1d892ecc00c3136e3dd80b2093de2010000006b483045022100a26ca4418eb7cf3c2e0c7e3efdb4d5ef4dea5b9a8faef841e83705511a13999402204937063e217fa3716b68e8d1eaffc07596a8b4ad3d6cbedddb5ee3f4cd88617a412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acb2258700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000019d183a23a6fe6feea4ca24f2fd18b30e05d891e96d2f7d595b2f4cab7096ecd8010000006a473044022057d8947dcbad01b8dab5ad7a9e18dd90a4a7c6e72354f712be9949b59e4a62110220407c284ef13db66afd3e11bce9374a07586a7c4ef5795ca539ebe8afd705d93c4121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac2d468800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000018e54bcb18bad17dcdc51ead0fae430cd6e719bf6b370096a89b3bebd14c78ebb010000006b483045022100ed624f6033e15a73bb39e50f2226acd49b217fe179e007f276c1e369110844a50220609a3910af0d3bca3df9541d977b4c97a768613cd7346d2931b81c1236c99c094121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac8dff8700000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001ec6ede79b6b10263b01096db5de06ce4c5dfeb601657722a6b5bd881b9e999b0010000006b483045022100e579ffaab77951eeebf998fc7499b4d37522723c4d7ee846acc83e4fc40b06d002201c8840a4444fcf7a94ce793b26f6d3d3cc29a4aa448b8e9ce4adabf34f1e9da841210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acc5028800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000012d43d863e2f4bb364043c355721f30bb990b588ee2b50e14fcebba2724d1f529010000006a47304402206122fdfc781821bea5258e03a9f84dd5287f7a3b67bd0b3ab91b5a30611ef83e02203268c7d070f84ddb85640ca917121b8daf3f3a5e065b8169ce4dee6b4507fb3c412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acda018800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001ce1157ea4be605f23c1878217e514f55c2f5151a06cd69b21d225cb3d8876daa010000006a473044022075323738718e82c06b8768acd13568b89d9275cafa7372174412fdb38f33f6b0022013faa4a9cab7c35e13d3426962e3b1429abaf7641fe7c7e3efc8023317651996412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6e1c8700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000126fbb8833c1c506571a9a13a2ca135a205a2aef59f52090b13e3f31d1f6a0088010000006a47304402206b6517bb5e087f8a42ce0adf103b6c02dddb1c5194bcd06d448bc9379429b3a902204acfd8f088cb31533bc6b84a1622253cffadba1f88f2756d02963953a8d311a64121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acf43c8800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001574c635ed152283dfcac73b99f7edfffc983c7b273aa75db18ea2892599112e4010000006b483045022100a3e77c01b1765b8364b205ab3ece7d7065e65ae6da66afdeda518e31ced70643022074df9cfd978663b3b7bf9f0109de438c1291747ee3aca6d0a18580bd22a3a6f6412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac641d8700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001e539d6b3d92e40ed62dfb52428dd1fb90817df8f56421abe399992e315ff60bd010000006a473044022043f9ba165817ebae7208e1d136cc0b9c1bcc0bef02295fbb73010288a94f239402201c09f4afbb1e0d2cf0864c9bd927d3ecb73c861c0771384a0219c8b5820e8c4041210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac08198700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000115e0ff3494a5bd47a127ebbcaa3615980a24aab07f4f63a439d537ec9e9aa13e010000006b483045022100d7faa295febe5bcfed831227e0178003277f22b75c9aca96a766cb88bf46529202201ab6e96dc5f8196b2ce8befa4ded19f5ff1e88b87ab2ce0e4c7fa82ec62a82f04121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88aceb1f8700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001be39eb48943d87af5b5c5fe9ffdbbc48eb3f630ff8446efdd12dd95464a949cd010000006b483045022100885f3a7e24c6b8be8aaacdffcaa21493957773c8d40044e3fa61be8d5bf5b20b02200af7c4ecbf01e536f4223cf7cd4dbde0a2d1d49bacb5c7c3599ca43873f67c894121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88aca2338800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000013e39dfb4df9c8c81c361f76feaabe5d7b8fe8584b1073a749efd3a9579ed52a6010000006b483045022100f5207bdc1f1f0a0ceae6f41cc524d670068d13b8a686ca7021515dad0f1c08cf022076c5a599ca913033c899f410132c20b094b8969a6f6be2754289225a391e4136412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac30138700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001c2346b3ff17f7ff9f2ab43b80521900d926a58ab38632b1490e7d4872776381b010000006b483045022100afd048a48df6063ff492cafa2a63403bc5a23e90632adb824d8977a5b6fcb1a6022043ab81ce959a99c35ecfe855d8a245fbbef213ccaa94b973b77b81924fca440c41210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac8ff98700000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001cd13e5de7c3d4901c38cf00d27b97f65002a5dc225e6dcccf081be25dbf90524010000006a4730440220049b504b4334e955691982d94e3c7623f4256cc455997096f0b7c29ec36e3f63022047ee27cd1f3b566332664079b161deda0dbf12bbc1195a3ac86ea0b6c0b39b16412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac3b148700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000143943a1eb0a99c9426747523aeb08905bd599d2460936b69b90b651d55712797010000006b483045022100a2b0186d5bd601a0cde6b8548a20332c67d6792af5ac3406664c1c4ffca13b00022029e54aab38f4f18bbe9d2c68d5140052f36b3e09a8e94a520cdf06c7f5763a704121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88accd168700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001764f293329edbdda6df5425103024082151a58854f9789364a111c0849ceda6e010000006b483045022100a473ca247d72c9eb901feeab340b70c199afd1012bd17de11c95af821de86acb022039500349119f8e9998f2417e0c2cf804e16e198d502034798658aeebf1601c364121033592498a12775a8a46dc580cf418256beab4a6a21c223d06b46de2ef02098a99ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac76f68700000000001976a91417da70492df2f93234367d8146908abc85cc19fd88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001253d3afe56554fd428d269783b3978c026da652b62e7488a957fbec102f9b23a010000006b4830450221008d677334e86b7af053ce28d4f22859c0162ad5fcfe575fca0a00d093ef6d44b802205f8bc123a31c8b77ba21ad8ba0f2893e752c951c4db4da544b53353618817f0241210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acf40f8700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "0100000001765c316dba7e334820c42d37b4c77d48c52c5c71e091589195b0dbf7d11c8f17010000006b483045022100a91dca83f1954ec9257fc4486ccfaa84b570ce3cf35adc233d8918b792f27efd022005a5d464db5e5a9056543e2940928ef2595dcce57344e6750b849009e9d8e9c0412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acc8f88700000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "010000000184baa3386a9d5b0567f7222aba8418854b23bb06b37e03f02012e3d6324c6821010000006b483045022100bf252d7a94482b700e17b7157bd854b0e4e496dbdca851e1319e2d47c94ee476022032addf8297c7e325587a2cbf561c899ea19b5a6e68369f25a851ec874c18b44b41210289d92b4239c112e97db3aac9590681968bd00415e974dba04bc81cca96b8c7ecffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac96068700000000001976a914b8e0a1c9040f8636a453ba077d7fa5f2318c7d0c88ac00000000"));
    mp.add(get_tx_from_mempool(mp, internal_utxo, "01000000015e4561ad09b3f1a9268a6cb27a69cd1f62c86299934e99af9373854ab31e4f91010000006b483045022100e3f220641e649afa2d7f34814419772032309ca2e3b2b399cbda440e3ebb96da02200a701f31015cfbe0a15982636f565b79ea9530a9b68cabca43d61c77f6a8bd30412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acdf0a8700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000"));

    auto segfault = get_tx_from_mempool(mp, internal_utxo, "01000000016653bb0c030b5a361c197a1d97c565cd0cbbef648a8599a835aa2f66738552f3010000006a47304402200eca9a84f0ccadb3a130ae2f0d5c95ba9ed112a0c7dbe494fe300d56f3c84eaf022021a2c4e6695737d06e860b7714df337d238f34060f08dfb71a1f736916917c204121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac482a8800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000");
    REQUIRE(mp.add(segfault) == error::success);


    chain::transaction::list txs {
          get_tx("0100000001bdb1ba353b91f33c91c9860ed82acea5e853e3f61394501e5e48a6cb524f2bd4010000006b48304502210081149fdd50398db9f8494eaff4111200286b2f2f93e45c9bd155a04a1629cbc602207a39adf344da455d739acbf9e1befde1cd931fa9a0558ff65f3f5df6f5a23f1c412102e0cfe082d408b020cc9b8c297f22ad94d1a390a10f62dafdc01ea97dfdbe4387feffffff02dcc62600000000001976a91446e1437885b22d939ef736344b798a9cbf433c0888ac00000000000000002c6a09696e7465726c696e6b2084cda8971d0b07bdfb0da84915b33d07bb8265481e4774d0e666f93dbdfb643c00000000")
        , get_tx("0100000001b6a220148f412dae574e51e92f072aa90f4baee88f1ad285a736b2f63cf3460b010000006b483045022100ac88e4708716af3e1b19a7bec8fef9b6bc7d7829974997eb1b266dfb3923c689022039d39f76857aea5a8b466fa5a37c94a09a328af7d92a24ae342915b7ab0de94c412102f6b775b62f032e462df94d6d15f69db114bb7feb2748c6c2886a3d4d925703c3ffffffff0200000000000000002c6a09696e7465726c696e6b209a67ec902a1b500d939bf2a0dfb79adf2db3e6d10c6c565671b38806a993fc72f015a103000000001976a914a588cb7b781a0b67c02e87129b8ccb56fde3469c88ac00000000")
        , get_tx("01000000014b21ee302772fbc21f8e8259027d18eeea59e72dcc74b7a324eaa45eec31b9fa010000006a47304402204dbd3629acc05b5ba2747ff54c839af0423552f2582d3d025f63b87756098aee02206707a7cce3945e91c372b9778488bce772f5e16fa6cb426dbe8f78d165aa58db412103438ab580b4db848f5dcdc84ef989afd8da761177c7eb3e412b0374e3631afc6bffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac6c938700000000001976a914dd4a414c40b3ea4bcab0a1374de0bcd03d999c9888ac00000000")
        , get_tx("010000000145421c50d6de761c1f2883031efbca82d45bb61047ffc58cfad5cb90b01e43bf010000006b48304502210082f333b5716010fcbd58846b8b46ad94fe823dc1305d91c2df2a7d10fd70cf8c02201c3a07e6426e99dd7a64c202fbd519a58246390aa6242e2478403a0ac7d378c0412102c202701835c99b3e977acd51afc478325cc981bb8456be585307a8382cd7f2c7ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac2d948700000000001976a91486101da37d5ed4800ca902fdad8aaf99ec7df25088ac00000000")
        , get_tx("0100000001a857e56d293466fa88f386fb3cd6d8df852c3d30194156cda267f2f8061ebf25010000006a473044022016cc935298f95fab304e2db085eb59e12d7eb289b67b65e70cdf4976e616b3ab02205adb5cc44e69b4b7cf001c9d65649c7a63c61d7f4b58d022cfdfb909f28a56ef412103d969a70b208d2a864f169a5d722958895eacd163ede4be3de256f7fedfc1d88fffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac4a798800000000001976a914254378665eb61dac80318751ebdb6f0f44981ca688ac00000000")
        , get_tx("010000000145d05cf52335339f2a0b88b37e7647e3efceecaa614fca267fb4b0add6b42d8f010000006b483045022100d1f6361837e434606b4661c3492b679f9642937d3c0fc513a891e5f333a539f702204be6f6eeca3a944e9ab20a61612a84ea989f7a3ec9a4afb5e4eacab1459953804121026f77aac396bd82dde783509ccf188c5140a1b3e69809bbe65309fab98e97d95affffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac36b48800000000001976a914e048131a271885ad572722ff444e6b133e123e3088ac00000000")
        , get_tx("0100000001042285e129734551ae32b1950291ab04dfff158d4f751e7011c427af285173f5010000006b483045022100d302b1f0a7cc41b48e4e4d8adec47fbaca4736d0f59b48df98cf15397f7e32d0022078bccb24b4266b69178dbba497cf02a96d0b28491c83ca9e543689bcc29f52344121039d83bf4eed55226d2830a86f07f36cb06518f9e16c0258c92f38d87f802cf754ffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88acd6968700000000001976a914cd317d07b2f636a1515e2014f95b6563f6cf950988ac00000000")
        , get_tx("010000000106fd6ad50034509df5399562d8d108ea448553093c721ca9aaf477b98a589e61010000006a47304402206ff11e73e3be12c23667a161ee56f5e7ddb0f861cc404438cae1cbbef40b135902201793b598f3779b5cc367ae78f46549396afa2c1afd8ed19ca580ed0aaef9b97841210374a762d0a9e678aef54ba5d7a98a0f24c252008d31e6e5087c5557009acf786dffffffff02d0070000000000001976a91432b57f34861bcbe33a701be9ac3a50288fbc0a3d88ac377a8800000000001976a91427422cd8315f1701f206c261fb46609cea1b648588ac00000000")
    };

    chain::block blk{chain::header{}, txs};
    auto res = mp.remove(blk.transactions().begin(), blk.transactions().end());

    std::cout << res << std::endl;
}


