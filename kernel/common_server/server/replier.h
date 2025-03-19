#pragma once

#include "server.h"

#include <kernel/common_server/library/searchserver/simple/replier.h>
#include <kernel/common_server/util/accessor.h>

class TReplier: public IHttpReplier {
private:
    class THandlersCounterGuard {
    private:
        const TString Key;
        const TString Handler;
    public:
        THandlersCounterGuard(const TString& key, const TString& metric);
        ~THandlersCounterGuard();
    };

    const TBaseServerConfig* Config;
    const IBaseServer* Server;
    IRequestProcessor::TPtr Processor;
    THolder<THandlersCounterGuard> SignalGuard;
    CSA_READONLY_DEF(TString, HandlerName);
    virtual void MakeErrorPage(ui32 code, const TString& error) override;

public:
    TReplier(IReplyContext::TPtr context, const TBaseServerConfig* config, const IBaseServer* server);
    ~TReplier();

    virtual double GetDefaultKffWaitingAvailable() const override;
    virtual TDuration GetDefaultTimeout() const override;
    virtual void OnRequestExpired() override;
    virtual void DoSearchAndReply() override;
    virtual IThreadPool* DoSelectHandler() override;
    virtual void OnQueueFailure() override;
private:
    bool InitializeEncryptor(IReplyContext::TPtr context, TString& errorCode) const;
};
