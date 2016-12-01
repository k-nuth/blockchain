
#include <bitcoin/blockchain/parser.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <bitcoin/blockchain/settings.hpp>
#include <bitcoin/consensus/settings.hpp>

BC_DECLARE_CONFIG_DEFAULT_PATH("libbitcoin" / "bb.cfg")

// TODO: localize descriptions.

namespace libbitcoin {
namespace blockchain {

using namespace boost::filesystem;
using namespace boost::program_options;
using namespace bc::config;

// Initialize configuration by copying the given instance.
parser::parser(const configuration& defaults)
  : configured(defaults)
{
}

// Initialize configuration using defaults of the given context.
parser::parser(const config::settings& context)
  : configured(context)
{
    // Default endpoint for consensus replier.
    configured.consensus.replier = { "tcp://*:5501" };

    // Default endpoint for blockchain replier.
    configured.chain.replier = { "tcp://*:5502" };
}

options_metadata parser::load_options()
{
    options_metadata description("options");
    description.add_options()
    (
        BB_CONFIG_VARIABLE ",c",
        value<path>(&configured.file),
        "Specify path to a configuration settings file."
    )
    (
        BB_HELP_VARIABLE ",h",
        value<bool>(&configured.help)->
            default_value(false)->zero_tokens(),
        "Display command line options."
    )
    (
        BB_SETTINGS_VARIABLE ",s",
        value<bool>(&configured.settings)->
            default_value(false)->zero_tokens(),
        "Display all configuration settings."
    )
    (
        BB_VERSION_VARIABLE ",v",
        value<bool>(&configured.version)->
            default_value(false)->zero_tokens(),
        "Display version information."
    );

    return description;
}

arguments_metadata parser::load_arguments()
{
    arguments_metadata description;
    return description
        .add(BB_CONFIG_VARIABLE, 1);
}

options_metadata parser::load_environment()
{
    options_metadata description("environment");
    description.add_options()
    (
        // For some reason po requires this to be a lower case name.
        // The case must match the other declarations for it to compose.
        // This composes with the cmdline options and inits to system path.
        BB_CONFIG_VARIABLE,
        value<path>(&configured.file)->composing()
            ->default_value(config_default_path()),
        "The path to the configuration settings file."
    );

    return description;
}

options_metadata parser::load_settings()
{
    options_metadata description("settings");
    description.add_options()
    /* [consensus] */
    (
        "consensus.replier",
        value<config::endpoint>(&configured.consensus.replier),
        "The endpoint for the consensus replier."
    )

    /* [database] */
    (
        "database.directory",
        value<path>(&configured.database.directory),
        "The blockchain database directory, defaults to 'blockchain'."
    )
    (
        "database.file_growth_rate",
        value<uint16_t>(&configured.database.file_growth_rate),
        "Full database files increase by this percentage, defaults to 50."
    )
    (
        "database.block_table_buckets",
        value<uint32_t>(&configured.database.block_table_buckets),
        "Block hash table size, defaults to 650000."
    )
    (
        "database.transaction_table_buckets",
        value<uint32_t>(&configured.database.transaction_table_buckets),
        "Transaction hash table size, defaults to 110000000."
    )
    (
        "database.spend_table_buckets",
        value<uint32_t>(&configured.database.block_table_buckets),
        "Spend hash table size, defaults to 250000000."
    )
    (
        "database.history_table_buckets",
        value<uint32_t>(&configured.database.history_table_buckets),
        "History hash table size, defaults to 107000000."
    )

    /* [blockchain] */
    (
        "blockchain.threads",
        value<uint32_t>(&configured.chain.threads),
        "The number of threads dedicated to block validation, defaults to 8."
    )
    (
        "blockchain.priority",
        value<bool>(&configured.chain.priority),
        "The number of threads used for block validation, defaults to 8."
    )
    (
        "blockchain.use_libconsensus",
        value<bool>(&configured.chain.use_libconsensus),
        "Use libconsensus for script validation if integrated, defaults to false."
    )
    (
        "blockchain.use_testnet_rules",
        value<bool>(&configured.chain.use_testnet_rules),
        "Use testnet rules for determination of work required, defaults to false."
    )
    (
        "blockchain.flush_reorganizations",
        value<bool>(&configured.chain.flush_reorganizations),
        "Flush each reorganization to disk, defaults to false."
    )
    (
        "blockchain.transaction_pool_consistency",
        value<bool>(&configured.chain.transaction_pool_consistency),
        "Enforce consistency between the pool and the blockchain, defaults to false."
    )
    (
        "blockchain.transaction_pool_capacity",
        value<uint32_t>(&configured.chain.transaction_pool_capacity),
        "The maximum number of transactions in the pool, defaults to 2000."
    )
    (
        "blockchain.block_pool_capacity",
        value<uint32_t>(&configured.chain.block_pool_capacity),
        "The maximum number of orphan blocks in the pool, defaults to 50."
    )
    (
        "blockchain.checkpoint",
        value<config::checkpoint::list>(&configured.chain.checkpoints),
        "A hash:height checkpoint, multiple entries allowed."
    );

    return description;
}

bool parser::parse(int argc, const char* argv[], std::ostream& error)
{
    try
    {
        auto file = false;
        variables_map variables;
        load_command_variables(variables, argc, argv);
        load_environment_variables(variables, BB_ENVIRONMENT_VARIABLE_PREFIX);

        // Don't load the rest if any of these options are specified.
        if (!get_option(variables, BB_VERSION_VARIABLE) &&
            !get_option(variables, BB_SETTINGS_VARIABLE) &&
            !get_option(variables, BB_HELP_VARIABLE))
        {
            // Returns true if the settings were loaded from a file.
            file = load_configuration_variables(variables, BB_CONFIG_VARIABLE);
        }

        // Update bound variables in metadata.settings.
        notify(variables);

        // Clear the config file path if it wasn't used.
        if (!file)
            configured.file.clear();
    }
    catch (const boost::program_options::error& e)
    {
        // This is obtained from boost, which circumvents our localization.
        error << format_invalid_parameter(e.what()) << std::endl;
        return false;
    }

    return true;
}

} // namespace blockchain
} // namespace libbitcoin
