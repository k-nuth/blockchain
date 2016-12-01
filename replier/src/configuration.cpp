
#include <bitcoin/blockchain/configuration.hpp>

#include <cstddef>

namespace libbitcoin {
namespace blockchain {

// Construct with defaults derived from given context.
configuration::configuration(config::settings context)
  : help(false),
    settings(false),
    version(false),
    consensus(context),
    chain(context),
    database(context)
{
}

// Copy constructor.
configuration::configuration(const configuration& other)
  : help(other.help),
    settings(other.settings),
    version(other.version),
    file(other.file),
    consensus(other.consensus),
    chain(other.chain),
    database(other.database)
{
}

} // namespace blockchain
} // namespace libbitcoin
