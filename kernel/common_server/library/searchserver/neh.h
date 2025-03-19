#pragma once

#include <search/daemons/httpsearch/yshttp.h>
#include <kernel/common_server/library/neh/server/neh_server.h>
#include "replier.h"
#include <library/cpp/neh/rpc.h>
#include <util/generic/buffer.h>
#include "client.h"

class TSearchNehServer: public NUtil::TAbstractNehServer {
protected:
    YandexHttpServer YServer;

public:
    TSearchNehServer(const TOptions& config);

    virtual void SuperMind(const TStringBuf& postdata, IOutputStream& conn, IOutputStream& headers, bool isLocal);
};

class TNehReplyContext: public IRDReplyContext<IReplyContext> {
public:
    virtual ~TNehReplyContext() {

    }
    virtual NNeh::IRequest& GetRequest() = 0;
    virtual NNeh::TRequestOut& GetReplyOutput() = 0;

    virtual bool IsHttp() const override {
        return false;
    }

    virtual void DoMakeSimpleReply(const TBuffer& buf, int code) override final;
    virtual void AddReplyInfo(const TString&, const TString&, const bool /*rewrite*/) override final {}
};
