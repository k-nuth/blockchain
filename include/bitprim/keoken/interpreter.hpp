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

#ifndef BITPRIM_BLOCKCHAIN_KEOKEN_INTERPRETER_HPP_
#define BITPRIM_BLOCKCHAIN_KEOKEN_INTERPRETER_HPP_

#include <bitcoin/blockchain/interface/fast_chain.hpp>

#include <bitcoin/bitcoin/chain/transaction.hpp>
#include <bitcoin/bitcoin/utility/reader.hpp>

#include <bitprim/keoken/error.hpp>
#include <bitprim/keoken/message/base.hpp>
#include <bitprim/keoken/message/create_asset.hpp>
#include <bitprim/keoken/state.hpp>

namespace bitprim {
namespace keoken {

enum class version_t {
    zero = 0
};

enum class message_type_t {
    create_asset = 0,
    send_tokens = 1
};


class interpreter {
public:
    explicit
    interpreter(libbitcoin::blockchain::fast_chain& fast_chain, state& st);

    // non-copyable class
    interpreter(interpreter const&) = delete;
    interpreter& operator=(interpreter const&) = delete;

    error::error_code_t process(size_t block_height, libbitcoin::chain::transaction const& tx);

private:
    libbitcoin::wallet::payment_address get_first_input_addr(libbitcoin::chain::transaction const& tx) const;
    std::pair<libbitcoin::wallet::payment_address, libbitcoin::wallet::payment_address> get_send_tokens_addrs(libbitcoin::chain::transaction const& tx) const;
    
    error::error_code_t version_dispatcher(size_t block_height, libbitcoin::chain::transaction const& tx, libbitcoin::reader& source);
    error::error_code_t version_0_type_dispatcher(size_t block_height, libbitcoin::chain::transaction const& tx, libbitcoin::reader& source);
    error::error_code_t process_create_asset_version_0(size_t block_height, libbitcoin::chain::transaction const& tx, libbitcoin::reader& source);
    error::error_code_t process_send_tokens_version_0(size_t block_height, libbitcoin::chain::transaction const& tx, libbitcoin::reader& source);

    state& state_;
    libbitcoin::blockchain::fast_chain& fast_chain_;
};

} // namespace keoken
} // namespace bitprim

#endif //BITPRIM_BLOCKCHAIN_KEOKEN_INTERPRETER_HPP_
