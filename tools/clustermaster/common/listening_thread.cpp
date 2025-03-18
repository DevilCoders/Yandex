#include "listening_thread.h"

#include "log.h"
#include "thread_util.h"

#include <algorithm>

TListeningThread::TListeningThread(const char* host, const TIpPort port, TWorkflowFactory workflowFactory)
    : Pool()
    , Factory(std::move(workflowFactory))
    , Socket(new TInet6StreamSocket())
    , Thread([this]() { this->ThreadProc(); })
{
    TSockAddrInet6Stream sockAddr(host, port);

    if (SetSockOpt(*Socket, SOL_SOCKET, SO_REUSEADDR, 1) != 0) {
        ythrow yexception() << "failed to SetSockOpt(SO_REUSEADDR, true): " << LastSystemErrorText();
    }
    if (SetSockOpt(*Socket, IPPROTO_IPV6, IPV6_V6ONLY, 0) != 0) {
        ythrow yexception() << "failed to SetSockOpt(IPV6_V6ONLY, false): " << LastSystemErrorText();
    }

    int error = 0;
    if ((error = Socket->Bind(&sockAddr)) < 0) {
        ythrow yexception() << "failed to bind on " << sockAddr.ToString() << ": " << LastSystemErrorText(-error);
    }
    if ((error = Socket->Listen(4096)) < 0) {
        ythrow yexception() << "listen: " << LastSystemErrorText(-error);
    }

    LOG("Listening thread created");
    Pool.Start();
    Thread.Start();
}

TListeningThread::~TListeningThread() {
    Thread.Join();
    Pool.Stop();
    Socket->Close();

    LOG("Listening thread destroyed");
}

void TListeningThread::ThreadProc() {
    LOG("Listening thread started");
    SetCurrentThreadName("listening thread");

    for (;;) {
        auto newSock = MakeHolder<TInet6StreamSocket>();
        TSockAddrInet6Stream newSockAddr;

        int error = 0;
        if ((error = Socket->Accept(newSock.Get(), &newSockAddr)) == 0) {
            LOG("Connection accepted, starting workflow");
            SetNonBlock(*newSock);
            auto workflow = Factory(std::move(newSock));
            Pool.SafeAddFunc([workflow = std::move(workflow)]() { workflow->Run(); });
        } else if (-error != EINTR) {
            ythrow yexception() << "accept: " << LastSystemErrorText(-error);
        }
    }
    LOG("Listening thread finished");
}
