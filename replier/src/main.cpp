
#include "blockchain.hpp"

#include <memory>
#include <bitcoin/blockchain.hpp>
#include <bitcoin/blockchain/configuration.hpp>
#include <bitcoin/blockchain/define.hpp>
#include <bitcoin/blockchain/parser.hpp>
#include <bitcoin/blockchain/settings.hpp>
#include <bitcoin/protocol/blockchain.pb.h>
#include <bitcoin/protocol/zmq/context.hpp>
#include <bitcoin/protocol/zmq/message.hpp>
#include <bitcoin/protocol/zmq/socket.hpp>

using namespace libbitcoin::protocol;

namespace libbitcoin {
namespace blockchain {

static int main(parser& metadata)
{
    threadpool thread_pool;
    blockchain_.reset(new block_chain(
        thread_pool, metadata.configured.chain, metadata.configured.database));
    //transaction_pool_.reset(new transaction_pool(
    //    thread_pool, *blockchain_, metadata.configured.chain));

    zmq::context context;
    zmq::socket socket(context, zmq::socket::role::replier);
    auto ec = socket.bind(metadata.configured.chain.replier);
    assert(!ec);

    while (true)
    {
        zmq::message message;
        ec = socket.receive(message);
        assert(!ec);

        protocol::blockchain::request request;
        if (!message.dequeue(request))
            break;

        zmq::message reply = dispatch(request);
        ec = socket.send(reply);
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
