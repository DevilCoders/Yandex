#include "server.h"

#include <kernel/common_server/library/unistat/signals.h>

#include <library/cpp/logger/global/global.h>

THttpServerQueues::THttpServerQueues(THttpServer::ICallBack* cb)
    : MainQueue(MakeSimpleShared<TCountedMtpQueue>(MakeHolder<THttpServerMtpQueue>(cb)))
    , FailQueue(MakeSimpleShared<TCountedMtpQueue>())
{
}

TSearchServerBase::TSearchServerBase(const THttpServerOptions& options)
    : THttpServerQueues(this)
    , THttpServer(this, MainQueue, FailQueue, options)
    , Running(false)
{
}

bool TSearchServerBase::ClientConnected()
{
    return true;
}

void TSearchServerBase::Run()
{
    if (!Running) {
        VERIFY_WITH_LOG(Start(), "Error running server on port %d : %s", Options().Port, GetError());
    }
}

void TSearchServerBase::Stop()
{
    if (Running) {
        NOTICE_LOG << "Closing server on port " << Options().Port << " START" << Endl;
        THttpServer::Stop();
        NOTICE_LOG << "Closing server on port " << Options().Port << " FINISHED" << Endl;
    }
}

namespace {
    TNamedSignalSimple HttpServerRequestFail("http-fails");
    TNamedSignalSimple HttpServerRequestException("http-exception");
    TNamedSignalSimple HttpServerRequestMaxConnect("http-max-connect");
}

void TSearchServerBase::OnFailRequestEx(const TFailLogData& d) {
    ERROR_LOG << "Dropped request (failstate " << d.failstate << "): " << d.url << Endl;
    HttpServerRequestFail.Signal(1);
}

void TSearchServerBase::OnException() {
    HttpServerRequestException.Signal(1);
}

void TSearchServerBase::OnMaxConn() {
    HttpServerRequestMaxConnect.Signal(1);
}
