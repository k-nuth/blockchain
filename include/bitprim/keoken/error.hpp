/**
 * Copyright (c) 2016-2018 Bitprim Inc.
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

#ifndef BITPRIM_BLOCKCHAIN_KEOKEN_ERROR_HPP_
#define BITPRIM_BLOCKCHAIN_KEOKEN_ERROR_HPP_

namespace bitprim {
namespace keoken {
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
} // namespace keoken
} // namespace bitprim

#endif //BITPRIM_BLOCKCHAIN_KEOKEN_ERROR_HPP_
