#include "emulator.h"
#include <library/cpp/logger/global/global.h>

void TBaseEmulatorServer::Run(ui16 httpPort) {
    THttpServerOptions httpOptions(httpPort);
    Server = MakeHolder<THttpServer>(this, httpOptions);
    try {
        Server->Start();
    } catch (...) {
        ERROR_LOG << CurrentExceptionMessage();
        FAIL_LOG("Can't create emulator");
    }
}


TClientRequest* TBaseEmulatorServer::CreateClient() {
    return new TEmulatorReplier(*this);
}

TBaseEmulatorServer::~TBaseEmulatorServer() {
    if (Server) {
        Server->Stop();
    }
}
