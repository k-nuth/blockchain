
#ifndef LIBBITCOIN_BLOCKCHAIN_PARSER_HPP
#define LIBBITCOIN_BLOCKCHAIN_PARSER_HPP

#include <ostream>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/blockchain/define.hpp>
#include <bitcoin/blockchain/configuration.hpp>

namespace libbitcoin {
namespace blockchain {

/// Parse configurable values from environment variables, settings file, and
/// command line positional and non-positional options.
class BCB_API parser
  : public config::parser
{
public:
    parser(const config::settings& context);
    parser(const configuration& defaults);

    /// Parse all configuration into member settings.
    virtual bool parse(int argc, const char* argv[], std::ostream& error);

    /// Load command line options (named).
    virtual options_metadata load_options();

    /// Load command line arguments (positional).
    virtual arguments_metadata load_arguments();

    /// Load configuration file settings.
    virtual options_metadata load_settings();

    /// Load environment variable settings.
    virtual options_metadata load_environment();

    /// The populated configuration settings values.
    configuration configured;
};

} // namespace blockchain
} // namespace libbitcoin

#endif
