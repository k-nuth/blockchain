/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin-blockchain.
 *
 * libbitcoin-blockchain is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/blockchain.hpp>

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string.h>
#include <bitcoin/blockchain/define.hpp>
#include <bitcoin/blockchain/version.hpp>
#include <bitcoin/protocol/blockchain.pb.h>
#include <bitcoin/protocol/converter.hpp>
#include <bitcoin/protocol/requester.hpp>
#include <bitcoin/protocol/zmq/context.hpp>

using namespace libbitcoin::protocol;

namespace libbitcoin {
namespace blockchain {

static zmq::context context;

block_chain::block_chain(threadpool& pool,
    const blockchain::settings& chain_settings,
    const database::settings& database_settings)
  : requester_(context, chain_settings.replier),
    settings_(chain_settings)
{
}

// Close does not call stop because there is no way to detect thread join.
block_chain::~block_chain()
{
    close();
}

//# Properties.
// ----------------------------------------------------------------------------

const settings& block_chain::chain_settings() const
{
    return settings_;
}

//# Startup and shutdown.
// ----------------------------------------------------------------------------

// Start is required and the blockchain is restartable.
bool block_chain::start()
{
    protocol::blockchain::request request;
    auto* start = request.mutable_start();
    (void)start;

    protocol::blockchain::start_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    return reply.result();
}

// Start the orphan pool and the transaction pool.
bool block_chain::start_pools()
{
    protocol::blockchain::request request;
    auto* start_pools = request.mutable_start_pools();
    (void)start_pools;

    protocol::blockchain::start_pools_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    return reply.result();
}

// Stop is not required, speeds work shutdown with multiple threads.
bool block_chain::stop()
{
    protocol::blockchain::request request;
    auto* stop = request.mutable_stop();
    (void)stop;

    protocol::blockchain::start_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    return reply.result();
}

// Database threads must be joined before close is called (or destruct).
bool block_chain::close()
{
    protocol::blockchain::request request;
    auto* close = request.mutable_close();
    (void)close;

    protocol::blockchain::start_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    return reply.result();
}

//# Readers.
// ----------------------------------------------------------------------------

bool block_chain::get_gaps(database::block_database::heights& out_gaps) const
{
    protocol::blockchain::request request;
    auto* get_gaps = request.mutable_get_gaps();
    (void)get_gaps;

    protocol::blockchain::get_gaps_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    for (auto const& entry : reply.out_gaps())
    {
        out_gaps.push_back(entry);
    }
    return true;
}

bool block_chain::get_block_exists(const hash_digest& block_hash) const
{
    protocol::blockchain::request request;
    auto* get_block_exists = request.mutable_get_block_exists();
    (void)get_block_exists;

    protocol::blockchain::get_block_exists_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    return true;
}

bool block_chain::get_fork_difficulty(hash_number& out_difficulty,
    size_t height) const
{
    protocol::blockchain::request request;
    auto* get_fork_difficulty = request.mutable_get_fork_difficulty();
    get_fork_difficulty->set_height(height);

    protocol::blockchain::get_fork_difficulty_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    hash_digest hash;
    converter{}.from_protocol(&reply.out_difficulty(), hash);
    out_difficulty = hash_number(hash);
    return true;
}

bool block_chain::get_header(chain::header& out_header, size_t height) const
{
    protocol::blockchain::request request;
    auto* get_header = request.mutable_get_header();
    get_header->set_height(height);

    protocol::blockchain::get_header_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    converter{}.from_protocol(&reply.out_header(), out_header);
    return true;
}

bool block_chain::get_height(size_t& out_height, const hash_digest& block_hash) const
{
    protocol::blockchain::request request;
    auto* get_height = request.mutable_get_height();
    converter{}.to_protocol(block_hash, *get_height->mutable_block_hash());

    protocol::blockchain::get_height_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    out_height = reply.out_height();
    return true;
}

bool block_chain::get_bits(uint32_t& out_bits, const size_t& height) const
{
    protocol::blockchain::request request;
    auto* get_bits = request.mutable_get_bits();
    get_bits->set_height(height);

    protocol::blockchain::get_bits_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    out_bits = reply.out_bits();
    return true;
}

bool block_chain::get_timestamp(uint32_t& out_timestamp, const size_t& height) const
{
    protocol::blockchain::request request;
    auto* get_timestamp = request.mutable_get_timestamp();
    get_timestamp->set_height(height);

    protocol::blockchain::get_timestamp_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    out_timestamp = reply.out_timestamp();
    return true;
}

bool block_chain::get_version(uint32_t& out_version, const size_t& height) const
{
    protocol::blockchain::request request;
    auto* get_version = request.mutable_get_version();
    get_version->set_height(height);

    protocol::blockchain::get_version_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    out_version = reply.out_version();
    return true;
}

bool block_chain::get_last_height(size_t& out_height) const
{
    protocol::blockchain::request request;
    auto* get_last_height = request.mutable_get_last_height();
    (void)get_last_height;

    protocol::blockchain::get_last_height_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    out_height = reply.out_height();
    return true;
}

bool block_chain::get_output(chain::output& out_output, size_t& out_height,
    size_t& out_position, const chain::output_point& outpoint,
    size_t fork_height) const
{
    protocol::blockchain::request request;
    auto* get_output = request.mutable_get_output();
    converter{}.to_protocol(outpoint, *get_output->mutable_outpoint());
    get_output->set_fork_height(fork_height);

    protocol::blockchain::get_output_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    converter{}.from_protocol(&reply.out_output(), out_output);
    out_height = reply.out_height();
    out_position = reply.out_position();
    return true;
}

bool block_chain::get_is_unspent_transaction(const hash_digest& hash,
    size_t fork_height) const
{
    protocol::blockchain::request request;
    auto* get_is_unspent_transaction = request.mutable_get_is_unspent_transaction();
    converter{}.to_protocol(hash, *get_is_unspent_transaction->mutable_transaction_hash());
    get_is_unspent_transaction->set_fork_height(fork_height);

    protocol::blockchain::get_is_unspent_transaction_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    return true;
}

transaction_ptr block_chain::get_transaction(size_t& out_block_height,
    const hash_digest& hash) const
{
    protocol::blockchain::request request;
    auto* get_transaction = request.mutable_get_transaction();
    converter{}.to_protocol(hash, *get_transaction->mutable_transaction_hash());

    protocol::blockchain::get_transaction_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    transaction_ptr tx;
    if (reply.has_result())
    {
        tx = std::make_shared<message::transaction_message>();
        converter{}.from_protocol(&reply.result(), *tx);
        out_block_height = reply.out_block_height();
    }
    return tx;
}

// Writers.
// ----------------------------------------------------------------------------

bool block_chain::begin_writes()
{
    protocol::blockchain::request request;
    auto* begin_writes = request.mutable_begin_writes();
    (void)begin_writes;

    protocol::blockchain::begin_writes_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    return true;
}

bool block_chain::end_writes()
{
    protocol::blockchain::request request;
    auto* end_writes = request.mutable_end_writes();
    (void)end_writes;

    protocol::blockchain::end_writes_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    return true;
}

bool block_chain::insert(block_const_ptr block, size_t height, bool flush)
{
    protocol::blockchain::request request;
    auto* insert = request.mutable_insert();
    converter{}.to_protocol(*block, *insert->mutable_block());
    insert->set_height(height);
    insert->set_flush(flush);

    protocol::blockchain::insert_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    return true;
}

bool block_chain::push(const block_const_ptr_list& blocks, size_t height, bool flush)
{
    protocol::blockchain::request request;
    auto* push = request.mutable_push();
    for (auto const& entry : blocks)
    {
        converter{}.to_protocol(*entry, *push->add_blocks());
    }
    push->set_height(height);
    push->set_flush(flush);

    protocol::blockchain::push_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    return true;
}

bool block_chain::pop(block_const_ptr_list& out_blocks, const hash_digest& fork_hash,
    bool flush)
{
    protocol::blockchain::request request;
    auto* pop = request.mutable_pop();
    converter{}.to_protocol(fork_hash, *pop->mutable_fork_hash());
    pop->set_flush(flush);

    protocol::blockchain::pop_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    for (auto const& entry : reply.out_blocks())
    {
        block_ptr block = std::make_shared<message::block_message>();
        converter{}.from_protocol(&entry, *block);
        out_blocks.push_back(block);
    }
    return true;
}

bool block_chain::swap(block_const_ptr_list& out_blocks,
    const block_const_ptr_list& in_blocks, size_t fork_height,
    const hash_digest& fork_hash, bool flush)
{
    protocol::blockchain::request request;
    auto* swap = request.mutable_swap();
    for (auto const& entry : in_blocks)
    {
        converter{}.to_protocol(*entry, *swap->add_in_blocks());
    }
    swap->set_fork_height(fork_height);
    converter{}.to_protocol(fork_hash, *swap->mutable_fork_hash());
    swap->set_flush(flush);

    protocol::blockchain::swap_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);

    if (!reply.result())
        return false;

    for (auto const& entry : reply.out_blocks())
    {
        block_ptr block = std::make_shared<message::block_message>();
        converter{}.from_protocol(&entry, *block);
        out_blocks.push_back(block);
    }
    return true;
}

// Queries.
// ----------------------------------------------------------------------------
// Thread safe.

void block_chain::fetch_block(size_t height,
    block_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_block = request.mutable_fetch_block();
    fetch_block->set_height(height);
    fetch_block->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_block_handler>(
            std::move(handler),
            [] (block_fetch_handler handler,
                protocol::blockchain::fetch_block_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                block_ptr block = std::make_shared<message::block_message>();
                converter{}.from_protocol(&reply.block(), *block);
                const size_t height = reply.height();

                handler(error, block, height);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_block(const hash_digest& hash,
    block_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_block = request.mutable_fetch_block();
    converter{}.to_protocol(hash, *fetch_block->mutable_hash());
    fetch_block->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_block_handler>(
            std::move(handler),
            [] (block_fetch_handler handler,
                protocol::blockchain::fetch_block_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                block_ptr block = std::make_shared<message::block_message>();
                converter{}.from_protocol(&reply.block(), *block);
                const size_t height = reply.height();

                handler(error, block, height);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_block_header(size_t height,
    block_header_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_block_header = request.mutable_fetch_block_header();
    fetch_block_header->set_height(height);
    fetch_block_header->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_block_header_handler>(
            std::move(handler),
            [] (block_header_fetch_handler handler,
                protocol::blockchain::fetch_block_header_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                header_ptr header = std::make_shared<message::header_message>();
                converter{}.from_protocol(&reply.header(), *header);
                const size_t height = reply.height();

                handler(error, header, height);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_block_header(const hash_digest& hash,
    block_header_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_block_header = request.mutable_fetch_block_header();
    converter{}.to_protocol(hash, *fetch_block_header->mutable_hash());
    fetch_block_header->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_block_header_handler>(
            std::move(handler),
            [] (block_header_fetch_handler handler,
                protocol::blockchain::fetch_block_header_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                header_ptr header = std::make_shared<message::header_message>();
                converter{}.from_protocol(&reply.header(), *header);
                const size_t height = reply.height();

                handler(error, header, height);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_merkle_block(size_t height,
    transaction_hashes_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_merkle_block= request.mutable_fetch_merkle_block();
    fetch_merkle_block->set_height(height);
    fetch_merkle_block->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_merkle_block_handler>(
            std::move(handler),
            [] (transaction_hashes_fetch_handler handler,
                protocol::blockchain::fetch_merkle_block_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                message::merkle_block::ptr merkle_block = std::make_shared<message::merkle_block>();
                const auto& block = reply.block();
                converter{}.from_protocol(&block.header(), merkle_block->header());
                merkle_block->hashes().reserve(block.hashes_size());
                for (auto const& entry : block.hashes())
                {
                    hash_digest hash;
                    converter{}.from_protocol(&entry, hash);

                    merkle_block->hashes().push_back(std::move(hash));
                }
                const auto& flags = block.flags();
                merkle_block->flags().resize(flags.size());
                std::copy(flags.begin(), flags.end(), merkle_block->flags().begin());
                const size_t height = reply.height();

                handler(error, merkle_block, height);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_merkle_block(const hash_digest& hash,
    transaction_hashes_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_merkle_block= request.mutable_fetch_merkle_block();
    converter{}.to_protocol(hash, *fetch_merkle_block->mutable_hash());
    fetch_merkle_block->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_merkle_block_handler>(
            std::move(handler),
            [] (transaction_hashes_fetch_handler handler,
                protocol::blockchain::fetch_merkle_block_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                message::merkle_block::ptr merkle_block = std::make_shared<message::merkle_block>();
                const auto& block = reply.block();
                converter{}.from_protocol(&block.header(), merkle_block->header());
                merkle_block->hashes().reserve(block.hashes_size());
                for (auto const& entry : block.hashes())
                {
                    hash_digest hash;
                    converter{}.from_protocol(&entry, hash);

                    merkle_block->hashes().push_back(std::move(hash));
                }
                const auto& flags = block.flags();
                merkle_block->flags().resize(flags.size());
                std::copy(flags.begin(), flags.end(), merkle_block->flags().begin());
                const size_t height = reply.height();

                handler(error, merkle_block, height);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_block_height(const hash_digest& hash,
    block_height_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_block_height = request.mutable_fetch_block_height();
    converter{}.to_protocol(hash, *fetch_block_height->mutable_hash());
    fetch_block_height->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_block_height_handler>(
            std::move(handler),
            [] (block_height_fetch_handler handler,
                protocol::blockchain::fetch_block_height_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                const size_t height = reply.height();

                handler(error, height);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_last_height(last_height_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_last_height = request.mutable_fetch_last_height();
    fetch_last_height->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_last_height_handler>(
            std::move(handler),
            [] (last_height_fetch_handler handler,
                protocol::blockchain::fetch_last_height_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                const size_t height = reply.height();

                handler(error, height);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_transaction(const hash_digest& hash,
    transaction_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_transaction = request.mutable_fetch_transaction();
    converter{}.to_protocol(hash, *fetch_transaction->mutable_hash());
    fetch_transaction->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_transaction_handler>(
            std::move(handler),
            [] (transaction_fetch_handler handler,
                protocol::blockchain::fetch_transaction_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                chain::transaction tx;
                converter{}.from_protocol(&reply.transaction(), tx);
                transaction_ptr tx_ptr = std::make_shared<message::transaction_message>(std::move(tx));
                const size_t height = reply.height();

                handler(error, tx_ptr, height);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_transaction_position(const hash_digest& hash,
    transaction_index_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_transaction_position = request.mutable_fetch_transaction_position();
    converter{}.to_protocol(hash, *fetch_transaction_position->mutable_hash());
    fetch_transaction_position->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_transaction_position_handler>(
            std::move(handler),
            [] (transaction_index_fetch_handler handler,
                protocol::blockchain::fetch_transaction_position_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                const uint64_t position = reply.position();
                const size_t height = reply.height();

                handler(error, position, height);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_output(const chain::output_point& outpoint,
    output_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_output = request.mutable_fetch_output();
    converter{}.to_protocol(outpoint, *fetch_output->mutable_outpoint());
    fetch_output->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_output_handler>(
            std::move(handler),
            [] (output_fetch_handler handler,
                protocol::blockchain::fetch_output_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                chain::output output;
                converter{}.from_protocol(&reply.output(), output);

                handler(error, output);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_spend(const chain::output_point& outpoint,
    spend_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_spend = request.mutable_fetch_spend();
    converter{}.to_protocol(outpoint, *fetch_spend->mutable_outpoint());
    fetch_spend->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_spend_handler>(
            std::move(handler),
            [] (spend_fetch_handler handler,
                protocol::blockchain::fetch_spend_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                chain::input_point point;
                converter{}.from_protocol(&reply.point(), point);

                handler(error, point);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_history(const wallet::payment_address& address,
    uint64_t limit, uint64_t from_height,
    history_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_history = request.mutable_fetch_history();
    auto* fetch_address = fetch_history->mutable_address();
    fetch_address->set_valid(bool(address));
    fetch_address->set_version(address.version());
    converter{}.to_protocol(address.hash(), *fetch_address->mutable_hash());
    fetch_history->set_limit(limit);
    fetch_history->set_from_height(from_height);
    fetch_history->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_history_handler>(
            std::move(handler),
            [] (history_fetch_handler handler,
                protocol::blockchain::fetch_history_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                chain::history_compact::list history;
                history.reserve(reply.history_size());
                for (auto const& entry : reply.history())
                {
                    chain::history_compact history_compact;
                    history_compact.kind = static_cast<chain::point_kind>(entry.kind());
                    converter{}.from_protocol(&entry.point(), history_compact.point);
                    history_compact.height = entry.height();
                    history_compact.value = entry.value();

                    history.push_back(std::move(history_compact));
                }

                handler(error, history);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_stealth(const binary& filter, uint64_t from_height,
    stealth_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_stealth = request.mutable_fetch_stealth();
    fetch_stealth->set_filter_size(filter.size());
    fetch_stealth->set_filter_blocks(filter.blocks().data(), filter.blocks().size());
    fetch_stealth->set_from_height(from_height);
    fetch_stealth->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_stealth_handler>(
            std::move(handler),
            [] (stealth_fetch_handler handler,
                protocol::blockchain::fetch_stealth_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                chain::stealth_compact::list stealth;
                stealth.reserve(reply.stealth_size());
                for (auto const& entry : reply.stealth())
                {
                    chain::stealth_compact stealth_compact;
                    converter{}.from_protocol(&entry.ephemeral_public_key_hash(), stealth_compact.ephemeral_public_key_hash);
                    converter{}.from_protocol(&entry.public_key_hash(), stealth_compact.public_key_hash);
                    converter{}.from_protocol(&entry.transaction_hash(), stealth_compact.transaction_hash);

                    stealth.push_back(std::move(stealth_compact));
                }

                handler(error, stealth);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_block_locator(const chain::block::indexes& heights,
    block_locator_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_block_locator = request.mutable_fetch_block_locator();
    for (auto const& entry : heights)
    {
        fetch_block_locator->add_heights(entry);
    }
    fetch_block_locator->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_block_locator_handler>(
            std::move(handler),
            [] (block_locator_fetch_handler handler,
                protocol::blockchain::fetch_block_locator_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                get_blocks_ptr locator =  std::make_shared<message::get_blocks>();
                locator->start_hashes().reserve(reply.locator().start_hashes_size());
                for (auto const& entry : reply.locator().start_hashes())
                {
                    hash_digest hash;
                    converter{}.from_protocol(&entry, hash);

                    locator->start_hashes().push_back(std::move(hash));
                }
                converter{}.from_protocol(&reply.locator().stop_hash(), locator->stop_hash());

                handler(error, locator);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_locator_block_hashes(get_blocks_const_ptr locator,
    const hash_digest& threshold, size_t limit,
    inventory_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_locator_block_hashes = request.mutable_fetch_locator_block_hashes();
    auto* get_blocks = fetch_locator_block_hashes->mutable_locator();
    for (auto const& entry : locator->start_hashes())
    {
        converter{}.to_protocol(entry, *get_blocks->add_start_hashes());
    }
    converter{}.to_protocol(locator->stop_hash(), *get_blocks->mutable_stop_hash());
    converter{}.to_protocol(threshold, *fetch_locator_block_hashes->mutable_threshold());
    fetch_locator_block_hashes->set_limit(limit);
    fetch_locator_block_hashes->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_locator_block_hashes_handler>(
            std::move(handler),
            [] (inventory_fetch_handler handler,
                protocol::blockchain::fetch_locator_block_hashes_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                inventory_ptr inventory = std::make_shared<message::inventory>();
                inventory->inventories().reserve(reply.inventories_size());
                for (auto const& entry : reply.inventories())
                {
                    auto type = static_cast<message::inventory_vector::type_id>(entry.type());
                    hash_digest hash;
                    converter{}.from_protocol(&entry.hash(), hash);

                    inventory->inventories().emplace_back(type, std::move(hash));
                }

                handler(error, inventory);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_locator_block_headers(get_headers_const_ptr locator,
    const hash_digest& threshold, size_t limit,
    locator_block_headers_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_locator_block_headers = request.mutable_fetch_locator_block_headers();
    auto* get_blocks = fetch_locator_block_headers->mutable_locator();
    for (auto const& entry : locator->start_hashes())
    {
        converter{}.to_protocol(entry, *get_blocks->add_start_hashes());
    }
    converter{}.to_protocol(locator->stop_hash(), *get_blocks->mutable_stop_hash());
    converter{}.to_protocol(threshold, *fetch_locator_block_headers->mutable_threshold());
    fetch_locator_block_headers->set_limit(limit);
    fetch_locator_block_headers->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_locator_block_headers_handler>(
            std::move(handler),
            [] (locator_block_headers_fetch_handler handler,
                protocol::blockchain::fetch_locator_block_headers_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                headers_ptr headers = std::make_shared<message::headers>();
                headers->elements().reserve(reply.headers_size());
                for (auto const& entry : reply.headers())
                {
                    chain::header header;
                    converter{}.from_protocol(&entry, header);

                    headers->elements().push_back(std::move(header));
                }

                handler(error, headers);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::fetch_floaters(size_t limit,
    inventory_fetch_handler handler) const
{
    protocol::blockchain::request request;
    auto* fetch_floaters = request.mutable_fetch_floaters();
    fetch_floaters->set_limit(limit);
    fetch_floaters->set_handler(
        requester_.make_handler<protocol::blockchain::fetch_floaters_handler>(
            std::move(handler),
            [] (inventory_fetch_handler handler,
                protocol::blockchain::fetch_floaters_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                inventory_ptr hashes = std::make_shared<message::inventory>();
                hashes->inventories().reserve(reply.inventories_size());
                for (auto const& entry : reply.inventories())
                {
                    auto type = static_cast<message::inventory_vector::type_id>(entry.type());
                    hash_digest hash;
                    converter{}.from_protocol(&entry.hash(), hash);

                    hashes->inventories().emplace_back(type, std::move(hash));
                }

                handler(error, hashes);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

// Filters.
//-----------------------------------------------------------------------------

void block_chain::filter_blocks(get_data_ptr message,
    result_handler handler) const
{
    protocol::blockchain::request request;
    auto* filter_blocks = request.mutable_filter_blocks();
    for (auto const& inventory : message->inventories())
    {
        auto* message = filter_blocks->add_message();
        message->set_type(static_cast<int>(inventory.type()));
        converter{}.to_protocol(inventory.hash(), *message->mutable_hash());
    }
    filter_blocks->set_handler(
        requester_.make_handler<protocol::blockchain::filter_blocks_handler>(
            std::move(handler),
            [] (result_handler handler,
                protocol::blockchain::filter_blocks_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());

                handler(error);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::filter_transactions(get_data_ptr message,
    result_handler handler) const
{
    protocol::blockchain::request request;
    auto* filter_transactions = request.mutable_filter_transactions();
    for (auto const& inventory : message->inventories())
    {
        auto* message = filter_transactions->add_message();
        message->set_type(static_cast<int>(inventory.type()));
        converter{}.to_protocol(inventory.hash(), *message->mutable_hash());
    }
    filter_transactions->set_handler(
        requester_.make_handler<protocol::blockchain::filter_transactions_handler>(
            std::move(handler),
            [] (result_handler handler,
                protocol::blockchain::filter_transactions_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());

                handler(error);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::filter_orphans(get_data_ptr message,
    result_handler handler) const
{
    protocol::blockchain::request request;
    auto* filter_orphans = request.mutable_filter_orphans();
    for (auto const& inventory : message->inventories())
    {
        auto* message = filter_orphans->add_message();
        message->set_type(static_cast<int>(inventory.type()));
        converter{}.to_protocol(inventory.hash(), *message->mutable_hash());
    }
    filter_orphans->set_handler(
        requester_.make_handler<protocol::blockchain::filter_orphans_handler>(
            std::move(handler),
            [] (result_handler handler,
                protocol::blockchain::filter_orphans_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());

                handler(error);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::filter_floaters(get_data_ptr message,
    result_handler handler) const
{
    protocol::blockchain::request request;
    auto* filter_floaters = request.mutable_filter_floaters();
    for (auto const& inventory : message->inventories())
    {
        auto* message = filter_floaters->add_message();
        message->set_type(static_cast<int>(inventory.type()));
        converter{}.to_protocol(inventory.hash(), *message->mutable_hash());
    }
    filter_floaters->set_handler(
        requester_.make_handler<protocol::blockchain::filter_floaters_handler>(
            std::move(handler),
            [] (result_handler handler,
                protocol::blockchain::filter_floaters_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());

                handler(error);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

//# Subscribers.
// ----------------------------------------------------------------------------

void block_chain::subscribe_reorganize(reorganize_handler handler)
{
    protocol::blockchain::request request;
    auto* subscribe_reorganize = request.mutable_subscribe_reorganize();
    subscribe_reorganize->set_handler(
        requester_.make_subscription<protocol::blockchain::subscribe_reorganize_handler>(
            std::move(handler),
            [] (reorganize_handler handler,
                protocol::blockchain::subscribe_reorganize_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                const uint64_t fork_point = reply.fork_point();
                block_const_ptr_list new_blocks;
                new_blocks.reserve(reply.new_blocks_size());
                for (auto const& entry : reply.new_blocks())
                {
                    chain::block actual;
                    converter{}.from_protocol(&entry.actual(), actual);
                    message::block_message::ptr block =
                        std::make_shared<message::block_message>(std::move(actual));
                    block->set_originator(entry.originator());

                    new_blocks.push_back(std::move(block));
                }
                block_const_ptr_list replaced_blocks;
                replaced_blocks.reserve(reply.replaced_blocks_size());
                for (auto const& entry : reply.replaced_blocks())
                {
                    chain::block actual;
                    converter{}.from_protocol(&entry.actual(), actual);
                    message::block_message::ptr block =
                        std::make_shared<message::block_message>(std::move(actual));
                    block->set_originator(entry.originator());

                    replaced_blocks.push_back(std::move(block));
                }

                handler(error, fork_point, new_blocks, replaced_blocks);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

void block_chain::subscribe_transaction(transaction_handler handler)
{
    protocol::blockchain::request request;
    auto* subscribe_transaction = request.mutable_subscribe_transaction();
    subscribe_transaction->set_handler(
        requester_.make_subscription<protocol::blockchain::subscribe_transaction_handler>(
            std::move(handler),
            [] (transaction_handler handler,
                protocol::blockchain::subscribe_transaction_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                chain::point::indexes indexes;
                indexes.reserve(reply.indexes_size());
                for (auto const& entry : reply.indexes())
                {
                    indexes.push_back(entry);
                }
                chain::transaction tx;
                converter{}.from_protocol(&reply.transaction(), tx);
                transaction_ptr tx_ptr = std::make_shared<message::transaction_message>(std::move(tx));

                handler(error, indexes, tx_ptr);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

//# Organizers (pools).
// ----------------------------------------------------------------------------

/// Store a block in the orphan pool, triggers may trigger reorganization.
void block_chain::organize(block_const_ptr block, result_handler handler)
{
    protocol::blockchain::request request;
    auto* organize_block = request.mutable_organize_block();
    converter{}.to_protocol(*block, *organize_block->mutable_block());
    organize_block->set_handler(
        requester_.make_handler<protocol::blockchain::organize_block_handler>(
            std::move(handler),
            [] (result_handler handler,
                protocol::blockchain::organize_block_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());

                handler(error);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

/// Store a transaction to the pool.
void block_chain::organize(transaction_const_ptr transaction,
    transaction_store_handler handler)
{
    protocol::blockchain::request request;
    auto* organize_transaction = request.mutable_organize_transaction();
    converter{}.to_protocol(*transaction, *organize_transaction->mutable_transaction());
    organize_transaction->set_handler(
        requester_.make_handler<protocol::blockchain::organize_transaction_handler>(
            std::move(handler),
            [] (transaction_store_handler handler,
                protocol::blockchain::organize_transaction_handler const& reply)
            {
                const code error = error::error_code_t(reply.error());
                chain::point::indexes indexes;
                indexes.reserve(reply.indexes_size());
                for (auto const& entry : reply.indexes())
                {
                    indexes.push_back(entry);
                }

                handler(error, indexes);
            }));

    protocol::void_reply reply;
    auto ec = requester_.send(request, reply);
    BITCOIN_ASSERT(!ec);
}

} // namespace blockchain
} // namespace libbitcoin

