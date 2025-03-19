#pragma once
#include <kernel/common_server/library/searchserver/simple/client.h>
#include <kernel/common_server/library/searchserver/simple/replier.h>
#include "config.h"

class TFrontend;

class TSimpleHttpReplyContext: public TCommonHttpReplyContext<TServerRequestData> {
private:
    using TBase = TCommonHttpReplyContext<TServerRequestData>;
    using TBase::Client;
protected:
    virtual TStringBuf DoGetUri() const override {
        return Client->GetRequestData().ScriptName();
    }

public:
    using TRequestData = TServerRequestData;
    using TBase::TBase;

    virtual const TCgiParameters& GetCgiParameters() const override {
        return Client->GetRequestData().CgiParam;
    }

    virtual TCgiParameters& MutableCgiParameters() override {
        return Client->MutableRequestData().CgiParam;
    }

    virtual TSearchRequestData& MutableRequestData() override {
        Y_UNREACHABLE();
    }

    virtual const TSearchRequestData& GetRequestData() const override {
        Y_UNREACHABLE();
    }

};

class TFrontendClientRequest: public THttpClientImpl<TSimpleHttpReplyContext> {
private:
    const TBaseServerConfig* Config;
    const IBaseServer* Server;
protected:
    virtual void MakeErrorPage(IReplyContext::TPtr context, ui32 code, const TString& error) const override;
    virtual IHttpReplier::TPtr DoSelectHandler(IReplyContext::TPtr context) override;
public:

    TFrontendClientRequest() {
        Y_UNREACHABLE();
    }

    TFrontendClientRequest(const TBaseServerConfig* config, const IBaseServer* server);

};

