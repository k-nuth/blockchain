
#include "blockchain.hpp"

#include <memory>
#include <boost/optional.hpp>
#include <bitcoin/blockchain.hpp>
#include <bitcoin/blockchain/define.hpp>
#include <bitcoin/protocol/blockchain.pb.h>
#include <bitcoin/protocol/converter.hpp>
#include <bitcoin/protocol/zmq/message.hpp>

using namespace libbitcoin::protocol;

namespace libbitcoin {
namespace blockchain {

boost::optional<block_chain> blockchain_;

//# Startup and shutdown.
// ----------------------------------------------------------------------------

//! bool block_chain::start();
static protocol::blockchain::start_reply dispatch_start(
    const protocol::blockchain::start_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    const bool result = blockchain_->start();

    protocol::blockchain::start_reply reply;
    reply.set_result(result);
    return reply;
}

//! bool block_chain::start_pools();
static protocol::blockchain::start_pools_reply dispatch_start_pools(
    const protocol::blockchain::start_pools_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    const bool result = blockchain_->start_pools();

    protocol::blockchain::start_pools_reply reply;
    reply.set_result(result);
    return reply;
}

//! bool block_chain::stop();
static protocol::blockchain::stop_reply dispatch_stop(
    const protocol::blockchain::stop_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    const bool result = blockchain_->stop();

    protocol::blockchain::stop_reply reply;
    reply.set_result(result);
    return reply;
}

//! bool block_chain::close();
static protocol::blockchain::close_reply dispatch_close(
    const protocol::blockchain::close_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    const bool result = blockchain_->close();

    protocol::blockchain::close_reply reply;
    reply.set_result(result);
    return reply;
}

//# Readers.
// ----------------------------------------------------------------------------

//! bool block_chain::get_gaps(database::block_database::heights& out_gaps) const;
static protocol::blockchain::get_gaps_reply dispatch_get_gaps(
    const protocol::blockchain::get_gaps_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    database::block_database::heights out_gaps;
    const bool result = blockchain_->get_gaps(out_gaps);

    protocol::blockchain::get_gaps_reply reply;
    reply.set_result(result);
    for (auto const& entry : out_gaps)
    {
        reply.add_out_gaps(entry);
    }
    return reply;
}

//! bool block_chain::get_block_exists(const hash_digest& block_hash) const;
static protocol::blockchain::get_block_exists_reply dispatch_get_block_exists(
    const protocol::blockchain::get_block_exists_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    hash_digest block_hash;
    converter{}.from_protocol(&request.block_hash(), block_hash);

    const bool result = blockchain_->get_block_exists(block_hash);

    protocol::blockchain::get_block_exists_reply reply;
    reply.set_result(result);
    return reply;
}

//! bool block_chain::get_fork_difficulty(hash_number& out_difficulty,
//!    size_t height) const;
static protocol::blockchain::get_fork_difficulty_reply dispatch_get_fork_difficulty(
    const protocol::blockchain::get_fork_difficulty_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    hash_number out_difficulty;
    const size_t height = request.height();
    const bool result = blockchain_->get_fork_difficulty(out_difficulty, height);

    protocol::blockchain::get_fork_difficulty_reply reply;
    reply.set_result(result);
    if (result)
    {
        hash_digest hash = out_difficulty.hash();
        converter{}.to_protocol(hash, *reply.mutable_out_difficulty());
    }
    return reply;
}

//! bool block_chain::get_header(chain::header& out_header, size_t height) const;
static protocol::blockchain::get_header_reply dispatch_get_header(
    const protocol::blockchain::get_header_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    chain::header out_header;
    const size_t height = request.height();
    const bool result = blockchain_->get_header(out_header, height);

    protocol::blockchain::get_header_reply reply;
    reply.set_result(result);
    if (result)
    {
        converter{}.to_protocol(out_header, *reply.mutable_out_header());
    }
    return reply;
}

//! bool block_chain::get_height(size_t& out_height, const hash_digest& block_hash) const;
static protocol::blockchain::get_height_reply dispatch_get_height(
    const protocol::blockchain::get_height_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    size_t out_height;
    hash_digest block_hash;
    converter{}.from_protocol(&request.block_hash(), block_hash);
    const bool result = blockchain_->get_height(out_height, block_hash);

    protocol::blockchain::get_height_reply reply;
    reply.set_result(result);
    if (result)
    {
        reply.set_out_height(out_height);
    }
    return reply;
}

//! bool block_chain::get_bits(uint32_t& out_bits, const size_t& height) const;
static protocol::blockchain::get_bits_reply dispatch_get_bits(
    const protocol::blockchain::get_bits_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    uint32_t out_height;
    const size_t height = request.height();
    const bool result = blockchain_->get_bits(out_height, height);

    protocol::blockchain::get_bits_reply reply;
    reply.set_result(result);
    if (result)
    {
        reply.set_out_bits(out_height);
    }
    return reply;
}

//! bool block_chain::get_timestamp(uint32_t& out_timestamp, const size_t& height) const;
static protocol::blockchain::get_timestamp_reply dispatch_get_timestamp(
    const protocol::blockchain::get_timestamp_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    uint32_t out_height;
    const size_t height = request.height();
    const bool result = blockchain_->get_timestamp(out_height, height);

    protocol::blockchain::get_timestamp_reply reply;
    reply.set_result(result);
    if (result)
    {
        reply.set_out_timestamp(out_height);
    }
    return reply;
}

//! bool block_chain::get_version(uint32_t& out_version, const size_t& height) const;
static protocol::blockchain::get_version_reply dispatch_get_version(
    const protocol::blockchain::get_version_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    uint32_t out_height;
    const size_t height = request.height();
    const bool result = blockchain_->get_version(out_height, height);

    protocol::blockchain::get_version_reply reply;
    reply.set_result(result);
    if (result)
    {
        reply.set_out_version(out_height);
    }
    return reply;
}

//! bool block_chain::get_last_height(size_t& out_height) const;
static protocol::blockchain::get_last_height_reply dispatch_get_last_height(
    const protocol::blockchain::get_last_height_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    size_t out_height;
    const bool result = blockchain_->get_last_height(out_height);

    protocol::blockchain::get_last_height_reply reply;
    reply.set_result(result);
    if (result)
    {
        reply.set_out_height(out_height);
    }
    return reply;
}

//! bool block_chain::get_output(chain::output& out_output, size_t& out_height,
//!     size_t& out_position, const chain::output_point& outpoint,
//!     size_t fork_height) const;
static protocol::blockchain::get_output_reply dispatch_get_output(
    const protocol::blockchain::get_output_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    chain::output out_output;
    size_t out_height;
    size_t out_position;
    chain::output_point outpoint;
    converter{}.from_protocol(&request.outpoint(), outpoint);
    size_t fork_height = request.fork_height();
    const bool result = blockchain_->get_output(out_output, out_height, out_position, outpoint, fork_height);

    protocol::blockchain::get_output_reply reply;
    reply.set_result(result);
    if (result)
    {
        converter{}.to_protocol(out_output, *reply.mutable_out_output());
        reply.set_out_height(out_height);
        reply.set_out_position(out_position);
    }
    return reply;
}

//! bool block_chain::get_is_unspent_transaction(const hash_digest& hash,
//!     size_t fork_height) const;
static protocol::blockchain::get_is_unspent_transaction_reply dispatch_get_is_unspent_transaction(
    const protocol::blockchain::get_is_unspent_transaction_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    hash_digest transaction_hash;
    converter{}.from_protocol(&request.transaction_hash(), transaction_hash);
    size_t fork_height = request.fork_height();
    const bool result = blockchain_->get_is_unspent_transaction(transaction_hash, fork_height);

    protocol::blockchain::get_is_unspent_transaction_reply reply;
    reply.set_result(result);
    return reply;
}

//! transaction_ptr block_chain::get_transaction(size_t& out_block_height,
//!     const hash_digest& hash) const;
static protocol::blockchain::get_transaction_reply dispatch_get_transaction(
    const protocol::blockchain::get_transaction_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    size_t out_block_height;
    hash_digest transaction_hash;
    converter{}.from_protocol(&request.transaction_hash(), transaction_hash);
    const transaction_ptr result = blockchain_->get_transaction(out_block_height, transaction_hash);

    protocol::blockchain::get_transaction_reply reply;
    if (result)
    {
        converter{}.to_protocol(*result, *reply.mutable_result());
        reply.set_out_block_height(out_block_height);
    }
    return reply;
}

//# Writers.
// ----------------------------------------------------------------------------

//! bool block_chain::begin_writes();
static protocol::blockchain::begin_writes_reply dispatch_begin_writes(
    const protocol::blockchain::begin_writes_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    const bool result = blockchain_->begin_writes();

    protocol::blockchain::begin_writes_reply reply;
    reply.set_result(result);
    return reply;
}

//! bool block_chain::end_writes();
static protocol::blockchain::end_writes_reply dispatch_end_writes(
    const protocol::blockchain::end_writes_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    const bool result = blockchain_->end_writes();

    protocol::blockchain::end_writes_reply reply;
    reply.set_result(result);
    return reply;
}

//! bool block_chain::insert(block_const_ptr block, size_t height, bool flush);
static protocol::blockchain::insert_reply dispatch_insert(
    const protocol::blockchain::insert_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    chain::block actual;
    converter{}.from_protocol(&request.block(), actual);
    message::block_message::ptr const block =
        std::make_shared<message::block_message>(std::move(actual));
    size_t height = request.height();
    bool flush = request.flush();
    const bool result = blockchain_->insert(block, height, flush);

    protocol::blockchain::insert_reply reply;
    reply.set_result(result);
    return reply;
}

//! bool block_chain::push(const block_const_ptr_list& block, size_t height, bool flush);
static protocol::blockchain::push_reply dispatch_push(
    const protocol::blockchain::push_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    message::block_message::const_ptr_list blocks;
    for (auto const& entry : request.blocks())
    {
        chain::block actual;
        converter{}.from_protocol(&entry, actual);
        blocks.push_back(std::make_shared<message::block_message>(std::move(actual)));
    }
    size_t height = request.height();
    bool flush = request.flush();
    const bool result = blockchain_->push(blocks, height, flush);

    protocol::blockchain::push_reply reply;
    reply.set_result(result);
    return reply;
}

//! bool block_chain::pop(block_const_ptr_list& out_blocks, const hash_digest& fork_hash,
//!     bool flush);
static protocol::blockchain::pop_reply dispatch_pop(
    const protocol::blockchain::pop_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    message::block_message::const_ptr_list out_blocks;
    hash_digest fork_hash;
    converter{}.from_protocol(&request.fork_hash(), fork_hash);
    bool flush = request.flush();
    const bool result = blockchain_->pop(out_blocks, fork_hash, flush);

    protocol::blockchain::pop_reply reply;
    reply.set_result(result);
    for (auto const& entry : out_blocks)
    {
        converter{}.to_protocol(*entry, *reply.add_out_blocks());
    }
    return reply;
}

//! bool block_chain::swap(block_const_ptr_list& out_blocks,
//!     const block_const_ptr_list& in_blocks, size_t fork_height,
//!     const hash_digest& fork_hash, bool flush);
static protocol::blockchain::swap_reply dispatch_swap(
    const protocol::blockchain::swap_request& request)
{
    BITCOIN_ASSERT(blockchain_);
    
    message::block_message::const_ptr_list out_blocks;
    message::block_message::const_ptr_list in_blocks;
    for (auto const& entry : request.in_blocks())
    {
        chain::block actual;
        converter{}.from_protocol(&entry, actual);
        in_blocks.push_back(std::make_shared<message::block_message>(std::move(actual)));
    }
    size_t fork_height = request.fork_height();
    hash_digest fork_hash;
    converter{}.from_protocol(&request.fork_hash(), fork_hash);
    bool flush = request.flush();
    const bool result = blockchain_->swap(out_blocks, in_blocks, fork_height, fork_hash, flush);

    protocol::blockchain::swap_reply reply;
    reply.set_result(result);
    for (auto const& entry : out_blocks)
    {
        converter{}.to_protocol(*entry, *reply.add_out_blocks());
    }
    return reply;
}

//# Queries.
// ----------------------------------------------------------------------------

//! void block_chain::fetch_block(size_t height, block_fetch_handler handler);
//! void block_chain::fetch_block(const hash_digest& hash, block_fetch_handler handler);
static protocol::void_reply dispatch_fetch_block(
    const protocol::blockchain::fetch_block_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    auto const& handler =
        replier_.make_handler<protocol::blockchain::fetch_block_handler>(
            request.handler(),
            [] (const code& error, block_ptr block, size_t height,
                protocol::blockchain::fetch_block_handler& handler) -> void
            {
                handler.set_error(error.value());
                if (block)
                {
                    converter{}.to_protocol(*block, *handler.mutable_block());
                    handler.set_height(height);
                }
            });
    if (request.hash().empty())
    {
        size_t height = request.height();
        blockchain_->fetch_block(height, handler);
    } else {
        hash_digest hash;
        converter{}.from_protocol(&request.hash(), hash);
        blockchain_->fetch_block(hash, handler);
    }

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::fetch_block_header(size_t height,
//!     block_header_fetch_handler handler);
//! void block_chain::fetch_block_header(const hash_digest& hash,
//!     block_header_fetch_handler handler);
static protocol::void_reply dispatch_fetch_block_header(
    const protocol::blockchain::fetch_block_header_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    auto const& handler =
        replier_.make_handler<protocol::blockchain::fetch_block_header_handler>(
            request.handler(),
            [] (const code& error, header_ptr header, size_t height,
                protocol::blockchain::fetch_block_header_handler& handler) -> void
            {
                handler.set_error(error.value());
                if (header)
                {
                    converter{}.to_protocol(*header, *handler.mutable_header());
                    handler.set_height(height);
                }
            });
    if (request.hash().empty())
    {
        size_t height = request.height();
        blockchain_->fetch_block_header(height, handler);
    } else {
        hash_digest hash;
        converter{}.from_protocol(&request.hash(), hash);
        blockchain_->fetch_block_header(hash, handler);
    }

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::fetch_merkle_block(size_t height,
//!     merkle_block_fetch_handler handler);
//! void block_chain::fetch_merkle_block(const hash_digest& hash,
//!     merkle_block_fetch_handler handler);
static protocol::void_reply dispatch_fetch_merkle_block(
    const protocol::blockchain::fetch_merkle_block_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    auto const& handler =
        replier_.make_handler<protocol::blockchain::fetch_merkle_block_handler>(
            request.handler(),
            [] (const code& error, merkle_block_ptr block, size_t height,
                protocol::blockchain::fetch_merkle_block_handler& handler) -> void
            {
                handler.set_error(error.value());
                if (block)
                {
                    auto* merkle_block = handler.mutable_block();
                    converter{}.to_protocol(block->header(), *merkle_block->mutable_header());
                    for (auto const& entry : block->hashes())
                    {
                        converter{}.to_protocol(entry, *merkle_block->add_hashes());
                    }
                    merkle_block->set_flags(block->flags().data(), block->flags().size());
                }
                handler.set_height(height);
            });
    if (request.hash().empty())
    {
        size_t height = request.height();
        blockchain_->fetch_merkle_block(height, handler);
    } else {
        hash_digest hash;
        converter{}.from_protocol(&request.hash(), hash);
        blockchain_->fetch_merkle_block(hash, handler);
    }

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::fetch_block_height(const hash_digest& hash,
//!     block_height_fetch_handler handler);
static protocol::void_reply dispatch_fetch_block_height(
    const protocol::blockchain::fetch_block_height_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    hash_digest hash;
    converter{}.from_protocol(&request.hash(), hash);
    blockchain_->fetch_block_height(hash,
        replier_.make_handler<protocol::blockchain::fetch_block_height_handler>(
            request.handler(),
            [] (const code& error, size_t height,
                protocol::blockchain::fetch_block_height_handler& handler) -> void
            {
                handler.set_error(error.value());
                handler.set_height(height);
            }));

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::fetch_last_height(last_height_fetch_handler handler);
static protocol::void_reply dispatch_fetch_last_height(
    const protocol::blockchain::fetch_last_height_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    blockchain_->fetch_last_height(
        replier_.make_handler<protocol::blockchain::fetch_last_height_handler>(
            request.handler(),
            [] (const code& error, size_t height,
                protocol::blockchain::fetch_last_height_handler& handler) -> void
            {
                handler.set_error(error.value());
                handler.set_height(height);
            }));

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::fetch_transaction(const hash_digest& hash,
//!     transaction_fetch_handler handler);
static protocol::void_reply dispatch_fetch_transaction(
    const protocol::blockchain::fetch_transaction_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    hash_digest hash;
    converter{}.from_protocol(&request.hash(), hash);
    blockchain_->fetch_transaction(hash,
        replier_.make_handler<protocol::blockchain::fetch_transaction_handler>(
            request.handler(),
            [] (const code& error, transaction_ptr tx, size_t height,
                protocol::blockchain::fetch_transaction_handler& handler) -> void
            {
                handler.set_error(error.value());
                if (tx)
                {
                    converter{}.to_protocol(*tx, *handler.mutable_transaction());
                    handler.set_height(height);
                }
            }));

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::fetch_transaction_position(const hash_digest& hash,
//!     transaction_index_fetch_handler handler);
static protocol::void_reply dispatch_fetch_transaction_position(
    const protocol::blockchain::fetch_transaction_position_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    hash_digest hash;
    converter{}.from_protocol(&request.hash(), hash);
    blockchain_->fetch_transaction_position(hash,
        replier_.make_handler<protocol::blockchain::fetch_transaction_position_handler>(
            request.handler(),
            [] (const code& error, uint64_t position, uint64_t height,
                protocol::blockchain::fetch_transaction_position_handler& handler) -> void
            {
                handler.set_error(error.value());
                handler.set_position(position);
                handler.set_height(height);
            }));

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::fetch_output(const chain::output_point& outpoint,
//!     output_fetch_handler handler) const;
static protocol::void_reply dispatch_fetch_output(
    const protocol::blockchain::fetch_output_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    chain::output_point outpoint;
    converter{}.from_protocol(&request.outpoint(), outpoint);
    blockchain_->fetch_output(outpoint,
        replier_.make_handler<protocol::blockchain::fetch_output_handler>(
            request.handler(),
            [] (const code& error, chain::output const& point,
                protocol::blockchain::fetch_output_handler& handler) -> void
            {
                handler.set_error(error.value());
                converter{}.to_protocol(point, *handler.mutable_output());
            }));

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::fetch_spend(const chain::output_point& outpoint,
//!     spend_fetch_handler handler);
static protocol::void_reply dispatch_fetch_spend(
    const protocol::blockchain::fetch_spend_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    chain::output_point outpoint;
    converter{}.from_protocol(&request.outpoint(), outpoint);
    blockchain_->fetch_spend(outpoint,
        replier_.make_handler<protocol::blockchain::fetch_spend_handler>(
            request.handler(),
            [] (const code& error, chain::input_point const& point,
                protocol::blockchain::fetch_spend_handler& handler) -> void
            {
                handler.set_error(error.value());
                converter{}.to_protocol(point, *handler.mutable_point());
            }));

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::fetch_history(const wallet::payment_address& address,
//!     uint64_t limit, size_t from_height, history_fetch_handler handler);
static protocol::void_reply dispatch_fetch_history(
    const protocol::blockchain::fetch_history_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    wallet::payment_address address;
    if (request.address().valid())
    {
        const uint8_t version = request.address().version();
        short_hash hash;
        converter{}.from_protocol(&request.address().hash(), hash);
        address = { hash, version };
    }
    const uint64_t limit = request.limit();
    const size_t from_height = request.from_height();
    blockchain_->fetch_history(address, limit, from_height,
        replier_.make_handler<protocol::blockchain::fetch_history_handler>(
            request.handler(),
            [] (const code& error, chain::history_compact::list const& history,
                protocol::blockchain::fetch_history_handler& handler) -> void
            {
                handler.set_error(error.value());
                for (auto const& entry : history)
                {
                    auto* history_compact = handler.add_history();
                    history_compact->set_kind(static_cast<int>(entry.kind));
                    converter{}.to_protocol(entry.point, *history_compact->mutable_point());
                    history_compact->set_height(entry.height);
                    history_compact->set_value(entry.value);
                }
            }));

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::fetch_stealth(const binary& filter, size_t from_height,
//!     stealth_fetch_handler handler);
static protocol::void_reply dispatch_fetch_stealth(
    const protocol::blockchain::fetch_stealth_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    const uint64_t filter_size = request.filter_size();
    const auto& filter_blocks = request.filter_blocks();
    const data_slice filter_slice(
        reinterpret_cast<uint8_t const*>(filter_blocks.data()) + 0,
        reinterpret_cast<uint8_t const*>(filter_blocks.data()) + filter_blocks.size());
    binary filter(filter_size, filter_slice);
    const size_t from_height = request.from_height();
    blockchain_->fetch_stealth(filter, from_height,
        replier_.make_handler<protocol::blockchain::fetch_stealth_handler>(
            request.handler(),
            [] (const code& error, chain::stealth_compact::list const& stealth,
                protocol::blockchain::fetch_stealth_handler& handler) -> void
            {
                handler.set_error(error.value());
                for (auto const& entry : stealth)
                {
                    auto* stealth_compact = handler.add_stealth();
                    converter{}.to_protocol(entry.ephemeral_public_key_hash, *stealth_compact->mutable_ephemeral_public_key_hash());
                    converter{}.to_protocol(entry.public_key_hash, *stealth_compact->mutable_public_key_hash());
                    converter{}.to_protocol(entry.transaction_hash, *stealth_compact->mutable_transaction_hash());
                }
            }));

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::fetch_block_locator(const chain::block::indexes& heights,
//!     block_locator_fetch_handler handler) const;
static protocol::void_reply dispatch_fetch_block_locator(
    const protocol::blockchain::fetch_block_locator_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    chain::block::indexes heights;
    for (auto const& entry : request.heights())
    {
        heights.push_back(entry);
    }
    blockchain_->fetch_block_locator(heights,
        replier_.make_handler<protocol::blockchain::fetch_block_locator_handler>(
            request.handler(),
            [] (const code& error, get_blocks_ptr locator,
                protocol::blockchain::fetch_block_locator_handler& handler) -> void
            {
                handler.set_error(error.value());
                if (locator)
                {
                    for (auto const& entry : locator->start_hashes())
                    {
                        converter{}.to_protocol(entry, *handler.mutable_locator()->add_start_hashes());
                    }
                    converter{}.to_protocol(locator->stop_hash(), *handler.mutable_locator()->mutable_stop_hash());
                }
            }));

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::fetch_locator_block_hashes(const message::get_blocks& locator,
//!     const hash_digest& threshold, size_t limit,
//!     locator_block_hashes_fetch_handler handler);
static protocol::void_reply dispatch_fetch_locator_block_hashes(
    const protocol::blockchain::fetch_locator_block_hashes_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    get_blocks_ptr locator =  std::make_shared<message::get_blocks>();
    locator->start_hashes().reserve(request.locator().start_hashes_size());
    for (auto const& entry : request.locator().start_hashes())
    {
        hash_digest hash;
        converter{}.from_protocol(&entry, hash);

        locator->start_hashes().push_back(std::move(hash));
    }
    converter{}.from_protocol(&request.locator().stop_hash(), locator->stop_hash());
    hash_digest threshold;
    converter{}.from_protocol(&request.threshold(), threshold);
    const size_t limit = request.limit();
    blockchain_->fetch_locator_block_hashes(locator, threshold, limit,
        replier_.make_handler<protocol::blockchain::fetch_locator_block_hashes_handler>(
            request.handler(),
            [] (const code& error, inventory_ptr inventory,
                protocol::blockchain::fetch_locator_block_hashes_handler& handler) -> void
            {
                handler.set_error(error.value());
                if (inventory)
                {
                    for (auto const& entry : inventory->inventories())
                    {
                        auto* inventory_vector = handler.add_inventories();
                        inventory_vector->set_type(static_cast<int>(entry.type()));
                        converter{}.to_protocol(entry.hash(), *inventory_vector->mutable_hash());
                    }
                }
            }));

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::fetch_locator_block_headers(const message::get_headers& locator,
//!     const hash_digest& threshold, size_t limit,
//!     locator_block_headers_fetch_handler handler);
static protocol::void_reply dispatch_fetch_locator_block_headers(
    const protocol::blockchain::fetch_locator_block_headers_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    get_headers_ptr locator =  std::make_shared<message::get_headers>();
    locator->start_hashes().reserve(request.locator().start_hashes_size());
    for (auto const& entry : request.locator().start_hashes())
    {
        hash_digest hash;
        converter{}.from_protocol(&entry, hash);

        locator->start_hashes().push_back(std::move(hash));
    }
    converter{}.from_protocol(&request.locator().stop_hash(), locator->stop_hash());
    hash_digest threshold;
    converter{}.from_protocol(&request.threshold(), threshold);
    const size_t limit = request.limit();
    blockchain_->fetch_locator_block_headers(locator, threshold, limit,
        replier_.make_handler<protocol::blockchain::fetch_locator_block_headers_handler>(
            request.handler(),
            [] (const code& error, headers_ptr headers,
                protocol::blockchain::fetch_locator_block_headers_handler& handler) -> void
            {
                handler.set_error(error.value());
                if (headers)
                {
                    for (auto const& entry : headers->elements())
                    {
                        converter{}.to_protocol(entry, *handler.add_headers());
                    }
                }
            }));

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::fetch_floaters(size_t limit,
//!         inventory_fetch_handler handler) const;
static protocol::void_reply dispatch_fetch_floaters(
    const protocol::blockchain::fetch_floaters_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    const size_t limit = request.limit();
    blockchain_->fetch_floaters(limit,
        replier_.make_handler<protocol::blockchain::fetch_floaters_handler>(
            request.handler(),
            [] (const code& error, inventory_ptr inventory,
                protocol::blockchain::fetch_floaters_handler& handler) -> void
            {
                handler.set_error(error.value());
                if (inventory)
                {
                    for (auto const& entry : inventory->inventories())
                    {
                        auto* inventory_vector = handler.add_inventories();
                        inventory_vector->set_type(static_cast<int>(entry.type()));
                        converter{}.to_protocol(entry.hash(), *inventory_vector->mutable_hash());
                    }
                }
            }));

    protocol::void_reply reply;
    return reply;
}

//# Filters.
// ----------------------------------------------------------------------------

//! void block_chain::filter_blocks(get_data_ptr message,
//!     result_handler handler);
static protocol::void_reply dispatch_filter_blocks(
    const protocol::blockchain::filter_blocks_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    get_data_ptr message = std::make_shared<message::get_data>();
    message->inventories().reserve(request.message_size());
    for (auto const& entry : request.message())
    {
        message::inventory_vector inventory_vector;
        inventory_vector.set_type(static_cast<message::inventory_vector::type_id>(entry.type()));
        converter{}.from_protocol(&entry.hash(), inventory_vector.hash());

        message->inventories().push_back(std::move(inventory_vector));
    }
    blockchain_->filter_blocks(message,
        replier_.make_handler<protocol::blockchain::filter_blocks_handler>(
            request.handler(),
            [] (const code& error,
                protocol::blockchain::filter_blocks_handler& handler) -> void
            {
                handler.set_error(error.value());
            }));

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::filter_transactions(get_data_ptr message,
//!     result_handler handler);
static protocol::void_reply dispatch_filter_transactions(
    const protocol::blockchain::filter_transactions_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    get_data_ptr message = std::make_shared<message::get_data>();
    message->inventories().reserve(request.message_size());
    for (auto const& entry : request.message())
    {
        message::inventory_vector inventory_vector;
        inventory_vector.set_type(static_cast<message::inventory_vector::type_id>(entry.type()));
        converter{}.from_protocol(&entry.hash(), inventory_vector.hash());

        message->inventories().push_back(std::move(inventory_vector));
    }
    blockchain_->filter_transactions(message,
        replier_.make_handler<protocol::blockchain::filter_transactions_handler>(
            request.handler(),
            [] (const code& error,
                protocol::blockchain::filter_transactions_handler& handler) -> void
            {
                handler.set_error(error.value());
            }));

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::filter_orphans(get_data_ptr message,
//!     result_handler handler);
static protocol::void_reply dispatch_filter_orphans(
    const protocol::blockchain::filter_orphans_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    get_data_ptr message = std::make_shared<message::get_data>();
    message->inventories().reserve(request.message_size());
    for (auto const& entry : request.message())
    {
        message::inventory_vector inventory_vector;
        inventory_vector.set_type(static_cast<message::inventory_vector::type_id>(entry.type()));
        converter{}.from_protocol(&entry.hash(), inventory_vector.hash());

        message->inventories().push_back(std::move(inventory_vector));
    }
    blockchain_->filter_orphans(message,
        replier_.make_handler<protocol::blockchain::filter_orphans_handler>(
            request.handler(),
            [] (const code& error,
                protocol::blockchain::filter_orphans_handler& handler) -> void
            {
                handler.set_error(error.value());
            }));

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::filter_floaters(get_data_ptr message,
//!     result_handler handler);
static protocol::void_reply dispatch_filter_floaters(
    const protocol::blockchain::filter_floaters_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    get_data_ptr message = std::make_shared<message::get_data>();
    message->inventories().reserve(request.message_size());
    for (auto const& entry : request.message())
    {
        message::inventory_vector inventory_vector;
        inventory_vector.set_type(static_cast<message::inventory_vector::type_id>(entry.type()));
        converter{}.from_protocol(&entry.hash(), inventory_vector.hash());

        message->inventories().push_back(std::move(inventory_vector));
    }
    blockchain_->filter_floaters(message,
        replier_.make_handler<protocol::blockchain::filter_floaters_handler>(
            request.handler(),
            [] (const code& error,
                protocol::blockchain::filter_floaters_handler& handler) -> void
            {
                handler.set_error(error.value());
            }));

    protocol::void_reply reply;
    return reply;
}

//# Subscribers.
// ----------------------------------------------------------------------------

//! void block_chain::subscribe_reorganize(reorganize_handler handler);
static protocol::void_reply dispatch_subscribe_reorganize(
    const protocol::blockchain::subscribe_reorganize_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    blockchain_->subscribe_reorganize(
        replier_.make_subscription<protocol::blockchain::subscribe_reorganize_handler>(
            request.handler(),
            [] (const code& error, size_t fork_point,
                const block_const_ptr_list& new_blocks,
                const block_const_ptr_list& replaced_blocks,
                protocol::blockchain::subscribe_reorganize_handler& handler) -> void
            {
                handler.set_error(error.value());
                handler.set_fork_point(fork_point);
                for (auto const& entry : new_blocks)
                {
                    auto* new_block = handler.add_new_blocks();
                    converter{}.to_protocol(*entry, *new_block->mutable_actual());
                    new_block->set_originator(entry->originator());
                }
                for (auto const& entry : replaced_blocks)
                {
                    auto* replaced_block = handler.add_replaced_blocks();
                    converter{}.to_protocol(*entry, *replaced_block->mutable_actual());
                    replaced_block->set_originator(entry->originator());
                }
            }));

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::subscribe_transaction(transaction_handler handler);
static protocol::void_reply dispatch_subscribe_transaction(
    const protocol::blockchain::subscribe_transaction_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    blockchain_->subscribe_transaction(
        replier_.make_subscription<protocol::blockchain::subscribe_transaction_handler>(
            request.handler(),
            [] (const code& error, const chain::point::indexes& indexes,
                transaction_const_ptr tx,
                protocol::blockchain::subscribe_transaction_handler& handler) -> void
            {
                handler.set_error(error.value());
                for (auto const& entry : indexes)
                {
                    handler.add_indexes(entry);
                }
                converter{}.to_protocol(*tx, *handler.mutable_transaction());
            }));

    protocol::void_reply reply;
    return reply;
}

//# Organizers (pools).
// ----------------------------------------------------------------------------

//! void block_chain::organize(block_const_ptr block, result_handler handler);
static protocol::void_reply dispatch_organize_block(
    const protocol::blockchain::organize_block_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    chain::block actual;
    converter{}.from_protocol(&request.actual(), actual);
    message::block_message::ptr const block =
        std::make_shared<message::block_message>(std::move(actual));
    block->set_originator(request.originator());
    blockchain_->organize(block,
        replier_.make_handler<protocol::blockchain::organize_block_handler>(
            request.handler(),
            [] (const code& error,
                protocol::blockchain::organize_block_handler& handler) -> void
            {
                handler.set_error(error.value());
            }));

    protocol::void_reply reply;
    return reply;
}

//! void block_chain::organize(transaction_const_ptr transaction,
//!     transaction_store_handler handler);
static protocol::void_reply dispatch_organize_transaction(
    const protocol::blockchain::organize_transaction_request& request)
{
    BITCOIN_ASSERT(blockchain_);

    transaction_ptr tx =std::make_shared<message::transaction_message>();
    converter{}.from_protocol(&request.transaction(), *tx);
    blockchain_->organize(tx,
        replier_.make_handler<protocol::blockchain::organize_transaction_handler>(
            request.handler(),
            [] (const code& error, const chain::point::indexes& indexes,
                protocol::blockchain::organize_transaction_handler& handler) -> void
            {
                handler.set_error(error.value());
                for (auto const& entry : indexes)
                {
                    handler.add_indexes(entry);
                }
            }));

    protocol::void_reply reply;
    return reply;
}

// ----------------------------------------------------------------------------
zmq::message dispatch(
    const protocol::blockchain::request& request)
{
    zmq::message reply;
    switch (request.request_type_case())
    {
    // Startup and shutdown.
    case protocol::blockchain::request::kStart:
    {
        reply.enqueue_protobuf_message(
            dispatch_start(request.start()));
        break;
    }
    case protocol::blockchain::request::kStartPools:
    {
        reply.enqueue_protobuf_message(
            dispatch_start_pools(request.start_pools()));
        break;
    }
    case protocol::blockchain::request::kStop:
    {
        reply.enqueue_protobuf_message(
            dispatch_stop(request.stop()));
        break;
    }
    case protocol::blockchain::request::kClose:
    {
        reply.enqueue_protobuf_message(
            dispatch_close(request.close()));
        break;
    }

    // Readers.
    case protocol::blockchain::request::kGetGaps:
    {
        reply.enqueue_protobuf_message(
            dispatch_get_gaps(request.get_gaps()));
        break;
    }
    case protocol::blockchain::request::kGetBlockExists:
    {
        reply.enqueue_protobuf_message(
            dispatch_get_block_exists(request.get_block_exists()));
        break;
    }
    case protocol::blockchain::request::kGetForkDifficulty:
    {
        reply.enqueue_protobuf_message(
            dispatch_get_fork_difficulty(request.get_fork_difficulty()));
        break;
    }
    case protocol::blockchain::request::kGetHeader:
    {
        reply.enqueue_protobuf_message(
            dispatch_get_header(request.get_header()));
        break;
    }
    case protocol::blockchain::request::kGetHeight:
    {
        reply.enqueue_protobuf_message(
            dispatch_get_height(request.get_height()));
        break;
    }
    case protocol::blockchain::request::kGetBits:
    {
        reply.enqueue_protobuf_message(
            dispatch_get_bits(request.get_bits()));
        break;
    }
    case protocol::blockchain::request::kGetTimestamp:
    {
        reply.enqueue_protobuf_message(
            dispatch_get_timestamp(request.get_timestamp()));
        break;
    }
    case protocol::blockchain::request::kGetVersion:
    {
        reply.enqueue_protobuf_message(
            dispatch_get_version(request.get_version()));
        break;
    }
    case protocol::blockchain::request::kGetLastHeight:
    {
        reply.enqueue_protobuf_message(
            dispatch_get_last_height(request.get_last_height()));
        break;
    }
    case protocol::blockchain::request::kGetIsUnspentTransaction:
    {
        reply.enqueue_protobuf_message(
            dispatch_get_is_unspent_transaction(request.get_is_unspent_transaction()));
        break;
    }
    case protocol::blockchain::request::kGetTransaction:
    {
        reply.enqueue_protobuf_message(
            dispatch_get_transaction(request.get_transaction()));
        break;
    }

    // Writers.
    case protocol::blockchain::request::kBeginWrites:
    {
        reply.enqueue_protobuf_message(
            dispatch_begin_writes(request.begin_writes()));
        break;
    }
    case protocol::blockchain::request::kEndWrites:
    {
        reply.enqueue_protobuf_message(
            dispatch_end_writes(request.end_writes()));
        break;
    }
    case protocol::blockchain::request::kInsert:
    {
        reply.enqueue_protobuf_message(
            dispatch_insert(request.insert()));
        break;
    }
    case protocol::blockchain::request::kPush:
    {
        reply.enqueue_protobuf_message(
            dispatch_push(request.push()));
        break;
    }
    case protocol::blockchain::request::kPop:
    {
        reply.enqueue_protobuf_message(
            dispatch_pop(request.pop()));
        break;
    }
    case protocol::blockchain::request::kSwap:
    {
        reply.enqueue_protobuf_message(
            dispatch_swap(request.swap()));
        break;
    }

    // Queries.
    case protocol::blockchain::request::kFetchBlock:
    {
        reply.enqueue_protobuf_message(
            dispatch_fetch_block(request.fetch_block()));
        break;
    }
    case protocol::blockchain::request::kFetchBlockHeader:
    {
        reply.enqueue_protobuf_message(
            dispatch_fetch_block_header(request.fetch_block_header()));
        break;
    }
    case protocol::blockchain::request::kFetchMerkleBlock:
    {
        reply.enqueue_protobuf_message(
            dispatch_fetch_merkle_block(request.fetch_merkle_block()));
        break;
    }
    case protocol::blockchain::request::kFetchBlockHeight:
    {
        reply.enqueue_protobuf_message(
            dispatch_fetch_block_height(request.fetch_block_height()));
        break;
    }
    case protocol::blockchain::request::kFetchLastHeight:
    {
        reply.enqueue_protobuf_message(
            dispatch_fetch_last_height(request.fetch_last_height()));
        break;
    }
    case protocol::blockchain::request::kFetchTransaction:
    {
        reply.enqueue_protobuf_message(
            dispatch_fetch_transaction(request.fetch_transaction()));
        break;
    }
    case protocol::blockchain::request::kFetchTransactionPosition:
    {
        reply.enqueue_protobuf_message(
            dispatch_fetch_transaction_position(request.fetch_transaction_position()));
        break;
    }
    case protocol::blockchain::request::kFetchOutput:
    {
        reply.enqueue_protobuf_message(
            dispatch_fetch_output(request.fetch_output()));
        break;
    }
    case protocol::blockchain::request::kFetchSpend:
    {
        reply.enqueue_protobuf_message(
            dispatch_fetch_spend(request.fetch_spend()));
        break;
    }
    case protocol::blockchain::request::kFetchHistory:
    {
        reply.enqueue_protobuf_message(
            dispatch_fetch_history(request.fetch_history()));
        break;
    }
    case protocol::blockchain::request::kFetchStealth:
    {
        reply.enqueue_protobuf_message(
            dispatch_fetch_stealth(request.fetch_stealth()));
        break;
    }
    case protocol::blockchain::request::kFetchBlockLocator:
    {
        reply.enqueue_protobuf_message(
            dispatch_fetch_block_locator(request.fetch_block_locator()));
        break;
    }
    case protocol::blockchain::request::kFetchLocatorBlockHashes:
    {
        reply.enqueue_protobuf_message(
            dispatch_fetch_locator_block_hashes(request.fetch_locator_block_hashes()));
        break;
    }
    case protocol::blockchain::request::kFetchLocatorBlockHeaders:
    {
        reply.enqueue_protobuf_message(
            dispatch_fetch_locator_block_headers(request.fetch_locator_block_headers()));
        break;
    }
    case protocol::blockchain::request::kFetchFloaters:
    {
        reply.enqueue_protobuf_message(
            dispatch_fetch_floaters(request.fetch_floaters()));
        break;
    }

    // Filters.
    case protocol::blockchain::request::kFilterBlocks:
    {
        reply.enqueue_protobuf_message(
            dispatch_filter_blocks(request.filter_blocks()));
        break;
    }
    case protocol::blockchain::request::kFilterTransactions:
    {
        reply.enqueue_protobuf_message(
            dispatch_filter_transactions(request.filter_transactions()));
        break;
    }
    case protocol::blockchain::request::kFilterOrphans:
    {
        reply.enqueue_protobuf_message(
            dispatch_filter_orphans(request.filter_orphans()));
        break;
    }
    case protocol::blockchain::request::kFilterFloaters:
    {
        reply.enqueue_protobuf_message(
            dispatch_filter_floaters(request.filter_floaters()));
        break;
    }

    // Subscribers.
    case protocol::blockchain::request::kSubscribeReorganize:
    {
        reply.enqueue_protobuf_message(
            dispatch_subscribe_reorganize(request.subscribe_reorganize()));
        break;
    }
    case protocol::blockchain::request::kSubscribeTransaction:
    {
        reply.enqueue_protobuf_message(
            dispatch_subscribe_transaction(request.subscribe_transaction()));
        break;
    }

    // Organizers.
    case protocol::blockchain::request::kOrganizeBlock:
    {
        reply.enqueue_protobuf_message(
            dispatch_organize_block(request.organize_block()));
        break;
    }
    case protocol::blockchain::request::kOrganizeTransaction:
    {
        reply.enqueue_protobuf_message(
            dispatch_organize_transaction(request.organize_transaction()));
        break;
    }
    }
    return reply;
}

} // namespace blockchain
} // namespace libbitcoin
