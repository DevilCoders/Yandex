#include "client_acceptor.h"

namespace NCloud::NBlockStore::NServer {

namespace {

////////////////////////////////////////////////////////////////////////////////

struct TClientAcceptorStub final
    : public IClientAcceptor
{
    void Accept(
        const TSocketHolder& socket,
        IBlockStorePtr sessionService) override
    {
        Y_UNUSED(socket);
        Y_UNUSED(sessionService);
    }

    void Remove(const TSocketHolder& socket) override
    {
        Y_UNUSED(socket);
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IClientAcceptorPtr CreateClientAcceptorStub()
{
    return std::make_shared<TClientAcceptorStub>();
}

}   // namespace NCloud::NBlockStore::NServer
