
#include "blockchain.hpp"

#include <functional>
#include <memory>
#include <boost/core/null_deleter.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <bitcoin/blockchain.hpp>
#include <bitcoin/blockchain/configuration.hpp>
#include <bitcoin/blockchain/define.hpp>
#include <bitcoin/blockchain/parser.hpp>
#include <bitcoin/blockchain/settings.hpp>
#include <bitcoin/protocol/blockchain.pb.h>
#include <bitcoin/protocol/replier.hpp>
#include <bitcoin/protocol/zmq/context.hpp>
#include <bitcoin/protocol/zmq/message.hpp>
#include <bitcoin/protocol/zmq/socket.hpp>

using namespace libbitcoin::protocol;

using boost::format;
using namespace boost;
using namespace boost::system;
using namespace bc::chain;
using namespace bc::config;
using namespace bc::database;
using namespace bc::network;
using namespace std::placeholders;

namespace libbitcoin {
namespace blockchain {

static libbitcoin::protocol::zmq::context context;
replier replier_(context);

#define BB_UNINITIALIZED_CHAIN \
    "The %1% directory is not initialized, run: bn --initchain"
#define BB_INITIALIZING_CHAIN \
    "Please wait while initializing %1% directory..."
#define BB_INITCHAIN_NEW \
    "Failed to create directory %1% with error, '%2%'."
#define BB_INITCHAIN_EXISTS \
    "Failed because the directory %1% already exists."
#define BB_INITCHAIN_TRY \
    "Failed to test directory %1% with error, '%2%'."
#define BB_INITCHAIN_COMPLETE \
    "Completed initialization."

#define BB_USING_CONFIG_FILE \
    "Using config file: %1%"
#define BB_USING_DEFAULT_CONFIG \
    "Using default configuration settings."
#define BB_VERSION_MESSAGE \
    "\nVersion Information:\n\n" \
    "libbitcoin-node:       %1%\n" \
    "libbitcoin-blockchain: %2%\n" \
    "libbitcoin:            %3%"
#define BB_LOG_HEADER \
    "================= startup =================="

static constexpr int directory_exists = 0;

static void initialize_output(parser& metadata)
{
    LOG_DEBUG(LOG_BLOCKCHAIN) << BB_LOG_HEADER;
    LOG_INFO(LOG_BLOCKCHAIN) << BB_LOG_HEADER;
    LOG_WARNING(LOG_BLOCKCHAIN) << BB_LOG_HEADER;
    LOG_ERROR(LOG_BLOCKCHAIN) << BB_LOG_HEADER;
    LOG_FATAL(LOG_BLOCKCHAIN) << BB_LOG_HEADER;

    const auto& file = metadata.configured.file;

    if (file.empty())
        LOG_INFO(LOG_BLOCKCHAIN) << BB_USING_DEFAULT_CONFIG;
    else
        LOG_INFO(LOG_BLOCKCHAIN) << format(BB_USING_CONFIG_FILE) % file;
}

static void do_initlog(parser& metadata, std::ostream& output, std::ostream& error)
{
    const auto& network = metadata.configured.network;

    const log::rotable_file debug_file
    {
        network.debug_file,
        network.archive_directory,
        network.rotation_size,
        network.maximum_archive_size,
        network.minimum_free_space,
        network.maximum_archive_files
    };

    const log::rotable_file error_file
    {
        network.error_file,
        network.archive_directory,
        network.rotation_size,
        network.maximum_archive_size,
        network.minimum_free_space,
        network.maximum_archive_files
    };

    log::stream console_out(&output, null_deleter());
    log::stream console_err(&error, null_deleter());

    log::initialize(debug_file, error_file, console_out, console_err);
    //handle_stop(initialize_stop);
}

static bool do_initchain(parser& metadata)
{
    initialize_output(metadata);

    error_code ec;
    const auto& directory = metadata.configured.database.directory;

    if (create_directories(directory, ec))
    {
        LOG_INFO(LOG_BLOCKCHAIN) << format(BB_INITIALIZING_CHAIN) % directory;

        // Unfortunately we are still limited to a choice of hardcoded chains.
        const auto genesis = metadata.configured.chain.use_testnet_rules ?
            chain::block::genesis_testnet() : chain::block::genesis_mainnet();

        const auto& settings = metadata.configured.database;
        const auto result = data_base(settings).create(genesis);

        LOG_INFO(LOG_BLOCKCHAIN) << BB_INITCHAIN_COMPLETE;
        return result;
    }

    if (ec.value() == directory_exists)
    {
        LOG_ERROR(LOG_BLOCKCHAIN) << format(BB_INITCHAIN_EXISTS) % directory;
        return false;
    }

    LOG_ERROR(LOG_BLOCKCHAIN) << format(BB_INITCHAIN_NEW) % directory % ec.message();
    return false;
}

static int main(parser& metadata)
{
    do_initlog(metadata, std::cout, std::cerr);

    if (metadata.configured.initchain) {
        return do_initchain(metadata);
    }

    threadpool thread_pool(1);
    blockchain_ = boost::in_place(
        std::ref(thread_pool), metadata.configured.chain, metadata.configured.database);

    auto ec = replier_.bind(metadata.configured.chain.replier);
    assert(!ec);

    while (true)
    {
        protocol::blockchain::request request;
        ec = replier_.receive(request);
        assert(!ec);

        libbitcoin::protocol::zmq::message reply = dispatch(request);
        ec = replier_.send(reply);
        assert(!ec);
    }

    return 0;
}

} // namespace blockchain
} // namespace libbitcoin

BC_USE_LIBBITCOIN_MAIN

int bc::main(int argc, char* argv[])
{
    set_utf8_stdio();
    blockchain::parser metadata(config::settings::mainnet);
    const auto& args = const_cast<const char**>(argv);

    if (!metadata.parse(argc, args, cerr))
        return console_result::failure;

    return blockchain::main(metadata);
}
