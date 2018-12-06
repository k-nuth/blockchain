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

static chain_state::data get_data() {
    chain_state::data value;
    value.height = 1;
    value.bits = { 0, { 0 } };
    value.version = { 1, { 0 } };
    value.timestamp = { 0, 0, { 0 } };
    return value;
}

transaction get_tx(std::string const& hex) {
    data_chunk data;
    decode_base16(data, hex);
    auto tx = transaction::factory_from_data(data);

    tx.validation.state = std::make_shared<chain_state>(
#ifdef BITPRIM_CURRENCY_BCH
        chain_state{ get_data(), {}, 0, 0, 0 });
#else
        chain_state{ get_data(), {}, 0 });
#endif //BITPRIM_CURRENCY_BCH


    return tx;
}

TEST_CASE("[mempool] add one transaction") {
    // Transaction included in Block #80000:
    // https://blockdozer.com/tx/5a4ebf66822b0b2d56bd9dc64ece0bc38ee7844a23ff1d7320a88c5fdb2ad3e2
    auto tx = get_tx("0100000001a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f5000000004948304502206e21798a42fae0e854281abd38bacd1aeed3ee3738d9e1446618c4571d1090db022100e2ac980643b0b82c0e88ffdfec6b64e3e6ba35e7ba5fdd7d5d6cc8d25c6b241501ffffffff0100f2052a010000001976a914404371705fa9bd789a2fcd52d2c580b65d35549d88ac00000000");
    tx.inputs()[0].previous_output().validation.cache = output{17,  script{}};

    mempool mp;
    REQUIRE(mp.add(tx) == result_code::success);
    REQUIRE(mp.all_transactions() == 1);
    REQUIRE(mp.candidate_transactions() == 1);
    REQUIRE(mp.candidate_bytes() == tx.to_data(BITPRIM_WITNESS_DEFAULT).size());
}

TEST_CASE("[mempool] duplicated transactions") {
    // Transaction included in Block #80000:
    // https://blockdozer.com/tx/5a4ebf66822b0b2d56bd9dc64ece0bc38ee7844a23ff1d7320a88c5fdb2ad3e2
    auto tx = get_tx("0100000001a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f5000000004948304502206e21798a42fae0e854281abd38bacd1aeed3ee3738d9e1446618c4571d1090db022100e2ac980643b0b82c0e88ffdfec6b64e3e6ba35e7ba5fdd7d5d6cc8d25c6b241501ffffffff0100f2052a010000001976a914404371705fa9bd789a2fcd52d2c580b65d35549d88ac00000000");
    tx.inputs()[0].previous_output().validation.cache = output{17,  script{}};

    mempool mp;
    REQUIRE(mp.add(tx) == result_code::success);
    REQUIRE(mp.all_transactions() == 1);
    REQUIRE(mp.candidate_transactions() == 1);
    REQUIRE(mp.candidate_bytes() == tx.to_data(BITPRIM_WITNESS_DEFAULT).size());

    auto res = mp.add(tx);
    REQUIRE(res != result_code::success);
    REQUIRE(res == result_code::duplicated_transaction);
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
    spender.inputs()[2].previous_output().validation.cache = output{17, script{}};
    spender.inputs()[3].previous_output().validation.cache = output{17, script{}};

    mempool mp;
    REQUIRE(mp.add(tx) == result_code::success);
    REQUIRE(mp.all_transactions() == 1);
    REQUIRE(mp.candidate_transactions() == 1);
    REQUIRE(mp.candidate_bytes() == tx.to_data(BITPRIM_WITNESS_DEFAULT).size());

    REQUIRE(mp.add(spender) == result_code::success);
    REQUIRE(mp.all_transactions() == 2);
    REQUIRE(mp.candidate_transactions() == 2);
    REQUIRE(mp.candidate_bytes() == tx.to_data(BITPRIM_WITNESS_DEFAULT).size() 
                                  + spender.to_data(BITPRIM_WITNESS_DEFAULT).size());
}

TEST_CASE("[mempool] double spend") {
    // Transaction included in Block #80000:
    // https://blockdozer.com/tx/5a4ebf66822b0b2d56bd9dc64ece0bc38ee7844a23ff1d7320a88c5fdb2ad3e2
    auto tx = get_tx("0100000001a6b97044d03da79c005b20ea9c0e1a6d9dc12d9f7b91a5911c9030a439eed8f5000000004948304502206e21798a42fae0e854281abd38bacd1aeed3ee3738d9e1446618c4571d1090db022100e2ac980643b0b82c0e88ffdfec6b64e3e6ba35e7ba5fdd7d5d6cc8d25c6b241501ffffffff0100f2052a010000001976a914404371705fa9bd789a2fcd52d2c580b65d35549d88ac00000000");

    // Transaction included in Block #80311:
    // https://blockdozer.com/tx/0c04096b6500773010eb042da00b9b2224afc63fe42f3379bfb1fecd4f528c5f/
    auto spender = get_tx("0100000004928050ac066c66aa8e84d1c8f93467a858d9ddadeec3628052427c5c4286e747000000008c49304602210080ce7a5203367ac53c28ed5aa8f940bfe280adfc4325840a097f0d779de87d06022100865c6574721a79fd9423d27aa6c5e14efaada9ed17f825697902701121915da1014104c5449df304c97850e294fc9c878db9ebb3039eeace22cf0a47eeee3da0a650623c997729dfba7b0ffd6b3cf076a209776b65ff604bfa4ca8ee83cbdd15348cceffffffffe2d32adb5f8ca820731dff234a84e78ec30bce4ec69dbd562d0b2b8266bf4e5a000000008c493046022100c0e77d0b559d2c3c18af307509faefe5d646714755024be062d82a0eeaf6258e022100ed3ebfe3fd60f087870f69c96c557b1fd6ca7007c373990de4a84df30442f889014104d4fb35c2cdb822644f1057e9bd07e3d3b0a36702662327ef4eb799eb219856d0fd884fce43082b73424a3293837c5f94a478f7bc4ec4da82bfb7e0b43fb218ccffffffff38363a089fc60d3e22e20b302b817283e55f121c8186c5157ada710bc730e98c000000008a4730440220554913ac5c016c36f9c155c69bc82ca0516c40593c0ad9256930afafd182efe602207a8c52f7ba926b151fef8b4dcb2223982cfc1b785e9893eb6c24bc7b8d599caf0141048d8c9eec2bd11a6e307a0c2d0b2731438b9cab19edfc6f85a9e559fd2b62b3c65ce250d26c324fcc7faf909c3c4956f58c5963539dd6f765b5e6100d4c3d8c0fffffffff473cf152cfa4100efff3b7dfa220b48a772b0e612ca4f06fa9c0a6c9d5f9644e010000008b483045022100afb4404ff9f694466455d97df15606c784392cb34f506c414c9398385a52873b0220178deba3a523b8a6111875c473827c62f09803295c2327d86897cc8087f134aa014104c44fb05a3a999dcdd8e02418f8f86d7624ee13103c799f27310d8e0a8f3d06f97688536b7b9a0b5a252176288251e3670190caa5a074e96165841f9f83f404c8ffffffff0100205fa0120000001976a9140e15309b21d1e5769104588d4e72dda7834728e388ac00000000");

    //Modified the first byte
    auto spender_fake = get_tx("0200000004928050ac066c66aa8e84d1c8f93467a858d9ddadeec3628052427c5c4286e747000000008c49304602210080ce7a5203367ac53c28ed5aa8f940bfe280adfc4325840a097f0d779de87d06022100865c6574721a79fd9423d27aa6c5e14efaada9ed17f825697902701121915da1014104c5449df304c97850e294fc9c878db9ebb3039eeace22cf0a47eeee3da0a650623c997729dfba7b0ffd6b3cf076a209776b65ff604bfa4ca8ee83cbdd15348cceffffffffe2d32adb5f8ca820731dff234a84e78ec30bce4ec69dbd562d0b2b8266bf4e5a000000008c493046022100c0e77d0b559d2c3c18af307509faefe5d646714755024be062d82a0eeaf6258e022100ed3ebfe3fd60f087870f69c96c557b1fd6ca7007c373990de4a84df30442f889014104d4fb35c2cdb822644f1057e9bd07e3d3b0a36702662327ef4eb799eb219856d0fd884fce43082b73424a3293837c5f94a478f7bc4ec4da82bfb7e0b43fb218ccffffffff38363a089fc60d3e22e20b302b817283e55f121c8186c5157ada710bc730e98c000000008a4730440220554913ac5c016c36f9c155c69bc82ca0516c40593c0ad9256930afafd182efe602207a8c52f7ba926b151fef8b4dcb2223982cfc1b785e9893eb6c24bc7b8d599caf0141048d8c9eec2bd11a6e307a0c2d0b2731438b9cab19edfc6f85a9e559fd2b62b3c65ce250d26c324fcc7faf909c3c4956f58c5963539dd6f765b5e6100d4c3d8c0fffffffff473cf152cfa4100efff3b7dfa220b48a772b0e612ca4f06fa9c0a6c9d5f9644e010000008b483045022100afb4404ff9f694466455d97df15606c784392cb34f506c414c9398385a52873b0220178deba3a523b8a6111875c473827c62f09803295c2327d86897cc8087f134aa014104c44fb05a3a999dcdd8e02418f8f86d7624ee13103c799f27310d8e0a8f3d06f97688536b7b9a0b5a252176288251e3670190caa5a074e96165841f9f83f404c8ffffffff0100205fa0120000001976a9140e15309b21d1e5769104588d4e72dda7834728e388ac00000000");

    tx.inputs()[0].previous_output().validation.cache = output{17,  script{}};
    spender.inputs()[0].previous_output().validation.cache = output{17, script{}};
    spender.inputs()[2].previous_output().validation.cache = output{17, script{}};
    spender.inputs()[3].previous_output().validation.cache = output{17, script{}};

    spender_fake.inputs()[0].previous_output().validation.cache = output{17, script{}};
    spender_fake.inputs()[2].previous_output().validation.cache = output{17, script{}};
    spender_fake.inputs()[3].previous_output().validation.cache = output{17, script{}};

    mempool mp;
    REQUIRE(mp.add(tx) == result_code::success);
    REQUIRE(mp.all_transactions() == 1);
    REQUIRE(mp.candidate_transactions() == 1);
    REQUIRE(mp.candidate_bytes() == tx.to_data(BITPRIM_WITNESS_DEFAULT).size());

    REQUIRE(mp.add(spender) == result_code::success);
    REQUIRE(mp.all_transactions() == 2);
    REQUIRE(mp.candidate_transactions() == 2);
    REQUIRE(mp.candidate_bytes() == tx.to_data(BITPRIM_WITNESS_DEFAULT).size() 
                                  + spender.to_data(BITPRIM_WITNESS_DEFAULT).size());


    // auto internal_utxo_set_ = mp.internal_utxo_set_;
    // auto all_transactions_ = mp.all_transactions_;
    // auto hash_index_ = mp.hash_index_;
    // auto candidate_transactions_ = mp.candidate_transactions_;

    REQUIRE(mp.add(spender_fake) == result_code::double_spend);
    REQUIRE(mp.all_transactions() == 2);
    REQUIRE(mp.candidate_transactions() == 2);
    REQUIRE(mp.candidate_bytes() == tx.to_data(BITPRIM_WITNESS_DEFAULT).size() 
                                  + spender.to_data(BITPRIM_WITNESS_DEFAULT).size());

    // REQUIRE(internal_utxo_set_ == mp.internal_utxo_set_);
    // // REQUIRE(all_transactions_ == mp.all_transactions_);
    // REQUIRE(hash_index_ == mp.hash_index_);
    // REQUIRE(candidate_transactions_ == mp.candidate_transactions_);

}
