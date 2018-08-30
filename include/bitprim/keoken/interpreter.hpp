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

#include <bitprim/integer_sequence.hpp>
#include <bitprim/keoken/dispatcher.hpp>
#include <bitprim/keoken/error.hpp>
#include <bitprim/keoken/message/create_asset.hpp>
#include <bitprim/keoken/message/send_tokens.hpp>
#include <bitprim/keoken/transaction_extractor.hpp>
#include <bitprim/keoken/transaction_processors/v0/transactions.hpp>
#include <bitprim/tuple_element.hpp>

// #define Tuple typename

namespace bitprim {
namespace keoken {

enum class version_t {
    zero = 0
};

template <typename State, typename Fastchain>
class interpreter {
public:
    interpreter(State& st, Fastchain& fast_chain)
        : state_(st)
        , fast_chain_(fast_chain)
    {}

    // non-copyable and non-movable class
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

        using dispatch_t = v0::dispatcher<transaction_processors::v0::transactions>;
        return dispatch_t{}(static_cast<message_type_t>(type), state_, fast_chain_, block_height, tx, source);
    }

    State& state_;
    Fastchain& fast_chain_;
};

} // namespace keoken
} // namespace bitprim

#endif //BITPRIM_BLOCKCHAIN_KEOKEN_INTERPRETER_HPP_
