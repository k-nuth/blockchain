// Copyright (c) 2016-2020 Knuth Project developers.
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef KTH_BLOCKCHAIN_KEOKEN_INTERPRETER_HPP_
#define KTH_BLOCKCHAIN_KEOKEN_INTERPRETER_HPP_

#include <kth/keoken/dispatcher.hpp>
#include <kth/keoken/error.hpp>
#include <kth/keoken/message/create_asset.hpp>
#include <kth/keoken/message/send_tokens.hpp>
#include <kth/keoken/transaction_extractor.hpp>
#include <kth/keoken/transaction_processors/v0/transactions.hpp>

// #define Tuple typename

namespace kth {
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

#ifdef KTH_USE_DOMAIN
    template <Reader R, KTH_IS_READER(R)>
    error::error_code_t version_dispatcher(size_t block_height, bc::chain::transaction const& tx, R& source) {
#else
    error::error_code_t version_dispatcher(size_t block_height, bc::chain::transaction const& tx, bc::reader& source) {
#endif // KTH_USE_DOMAIN
        auto version = source.read_2_bytes_big_endian();
        if ( ! source) return error::invalid_version_number;

        switch (static_cast<version_t>(version)) {
            case version_t::zero:
                return version_0_type_dispatcher(block_height, tx, source);
        }
        return error::not_recognized_version_number;
    }


#ifdef KTH_USE_DOMAIN
    template <Reader R, KTH_IS_READER(R)>
    error::error_code_t version_0_type_dispatcher(size_t block_height, bc::chain::transaction const& tx, R& source) {
#else
    error::error_code_t version_0_type_dispatcher(size_t block_height, bc::chain::transaction const& tx, bc::reader& source) {
#endif // KTH_USE_DOMAIN

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
} // namespace kth

#endif //KTH_BLOCKCHAIN_KEOKEN_INTERPRETER_HPP_
