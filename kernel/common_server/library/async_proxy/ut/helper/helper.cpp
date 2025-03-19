#include "helper.h"


namespace NAPHelper {

    TAtomic Counter5xx = 0;
    TAtomic CounterDisconnect = 0;
}

void NAPHelper::CheckRequestsFinishes(const TDuration timeout) {
    TInstant dl = Now() + timeout;
    while (IAddrDelivery::ObjectCount() && dl > Now()) {
        NanoSleep(50);
    }
    CHECK_WITH_LOG(!IAddrDelivery::ObjectCount());
    while (TAsyncTask::ObjectCount() && dl > Now()) {
        NanoSleep(50);
    }
    CHECK_WITH_LOG(!TAsyncTask::ObjectCount());
}

void NAPHelper::DumpLog(const TString& logStr) {
    INFO_LOG << logStr;
}

TAtomicSharedPtr<NAPHelper::TSlowNehServer> NAPHelper::BuildServer(const ui32 port, const ui32 threads, const TDuration pause, const ui32 code, TString scheme /*= "http"*/, bool check /*= false*/) {
    THttpServerOptions httpOptions1(port);
    httpOptions1.SetThreads(threads);
    NUtil::TAbstractNehServer::TOptions options1(httpOptions1, scheme);
    auto server1 = MakeHolder<TSlowNehServer>(options1, pause, code, check);
    server1->Start();
    return server1.Release();
}
