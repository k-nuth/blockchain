// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef KTH_BLOCKCHAIN_KEOKEN_ERROR_HPP_
#define KTH_BLOCKCHAIN_KEOKEN_ERROR_HPP_

namespace kth::keoken {
namespace error {

// The numeric values of these codes may change without notice.
enum error_code_t {
    success = 0,

    not_keoken_tx = 1,
    invalid_version_number = 2,
    not_recognized_version_number = 3,
    invalid_type = 4,
    not_recognized_type = 5,
    invalid_asset_amount = 6,
    invalid_asset_creator = 7,
    asset_id_does_not_exist = 8,

    invalid_create_asset_message = 256,
    invalid_send_tokens_message,

    invalid_source_address,
    invalid_target_address,
    insufficient_money,
};

} // namespace error
} // namespace kth::keoken

#endif //KTH_BLOCKCHAIN_KEOKEN_ERROR_HPP_
