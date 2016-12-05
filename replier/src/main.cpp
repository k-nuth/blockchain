
#include "blockchain.hpp"

#include <functional>
#include <memory>
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

namespace libbitcoin {
namespace blockchain {

static zmq::context context;
replier replier_(context);

static int main(parser& metadata)
{
    threadpool thread_pool;
    blockchain_ = boost::in_place(
        std::ref(thread_pool), metadata.configured.chain, metadata.configured.database);

    auto ec = replier_.bind(metadata.configured.chain.replier);
    assert(!ec);

    while (true)
    {
        protocol::blockchain::request request;
        ec = replier_.receive(request);
        assert(!ec);

        zmq::message reply = dispatch(request);
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
