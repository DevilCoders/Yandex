#include "neh.h"

#include <library/cpp/logger/global/global.h>

TSearchNehServer::TSearchNehServer(const TOptions& config)
    : TAbstractNehServer(config)
{
    YServer.HttpServerOptions.Host = config.Host;
    YServer.HttpServerOptions.Port = config.Port;
    YServer.CreateImpl();
}

void TSearchNehServer::SuperMind(const TStringBuf& postdata, IOutputStream& conn, IOutputStream& headers, bool isLocal) {
    YServer.SuperMind(postdata, conn, headers, isLocal);
}

void TNehReplyContext::DoMakeSimpleReply(const TBuffer& buf, int /*code*/) {
    GetReplyOutput() << TStringBuf(buf.data(), buf.size());
    GetReplyOutput().Finish();
}
