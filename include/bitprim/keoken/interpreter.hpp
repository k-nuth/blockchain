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

// #include <bitcoin/bitcoin/chain/output.hpp>
// #include <bitcoin/bitcoin/chain/transaction.hpp>
// #include <bitcoin/bitcoin/utility/container_source.hpp>
// #include <bitcoin/bitcoin/utility/istream_reader.hpp>
// #include <bitcoin/bitcoin/utility/reader.hpp>

#include <bitprim/keoken/error.hpp>
// #include <bitprim/keoken/message/base.hpp>
#include <bitprim/keoken/message/create_asset.hpp>
#include <bitprim/keoken/message/send_tokens.hpp>
#include <bitprim/keoken/transaction_extractor.hpp>

#include <bitprim/keoken/transaction_processors/v0/create_asset.hpp>
#include <bitprim/keoken/transaction_processors/v0/send_tokens.hpp>

namespace bitprim {
namespace keoken {

enum class version_t {
    zero = 0
};

enum class message_type_t {
    create_asset = 0,
    send_tokens = 1
};

template <typename State, typename Fastchain>
class interpreter {
public:
    // explicit
    interpreter(State& st, Fastchain& fast_chain)
        : state_(st)
        , fast_chain_(fast_chain)
    {}

    // non-copyable class
    interpreter(interpreter const&) = delete;
    interpreter& operator=(interpreter const&) = delete;

    error::error_code_t process(size_t block_height, bc::chain::transaction const& tx) {
        using bc::istream_reader;
        using bc::data_source;

        auto data = first_keoken_output(tx);
        if ( ! data.empty()) {
            data_source ds(data);
            istream_reader source(ds);

            return version_dispatcher(block_height, tx, source);
        }
        return error::not_keoken_tx;
    }

private:
    error::error_code_t version_dispatcher(size_t block_height, bc::chain::transaction const& tx, bc::reader& source) {
        auto version = source.read_2_bytes_big_endian();
        if ( ! source) return error::invalid_version_number;

        switch (static_cast<version_t>(version)) {
            case version_t::zero:
                return version_0_type_dispatcher(block_height, tx, source);
        }
        return error::not_recognized_version_number;
    }

    error::error_code_t version_0_type_dispatcher(size_t block_height, bc::chain::transaction const& tx, bc::reader& source) {
        using namespace transaction_processors::v0;

        auto type = source.read_2_bytes_big_endian();
        if ( ! source) return error::invalid_type;

        switch (static_cast<message_type_t>(type)) {
            case message_type_t::create_asset:
                return create_asset(state_, fast_chain_, block_height, tx, source);
            case message_type_t::send_tokens:
                return send_tokens(state_, fast_chain_, block_height, tx, source);
        }
        return error::not_recognized_type;
    }

    State& state_;
    Fastchain& fast_chain_;
};

} // namespace keoken
} // namespace bitprim

#endif //BITPRIM_BLOCKCHAIN_KEOKEN_INTERPRETER_HPP_
