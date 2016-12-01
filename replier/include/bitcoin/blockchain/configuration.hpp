
#ifndef LIBBITCOIN_BLOCKCHAIN_CONFIGURATION_HPP
#define LIBBITCOIN_BLOCKCHAIN_CONFIGURATION_HPP

#include <boost/filesystem.hpp>
#include <bitcoin/blockchain/define.hpp>
#include <bitcoin/blockchain/settings.hpp>
#include <bitcoin/consensus/settings.hpp>
#include <bitcoin/database/settings.hpp>

namespace libbitcoin {
namespace blockchain {

// Not localizable.
#define BB_HELP_VARIABLE "help"
#define BB_SETTINGS_VARIABLE "settings"
#define BB_VERSION_VARIABLE "version"

// This must be lower case but the env var part can be any case.
#define BB_CONFIG_VARIABLE "config"

// This must match the case of the env var.
#define BB_ENVIRONMENT_VARIABLE_PREFIX "BB_"

/// Full node configuration, thread safe.
class BCB_API configuration
{
public:
    configuration(config::settings context);
    configuration(const configuration& other);

    /// Options.
    bool help;
    bool settings;
    bool version;

    /// Options and environment vars.
    boost::filesystem::path file;

    /// Settings.
    consensus::settings consensus;
    blockchain::settings chain;
    database::settings database;
};

} // namespace blockchain
} // namespace libbitcoin

#endif
