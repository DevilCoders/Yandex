#pragma once
#include "context/replier.h"
#include "http_status_config.h"
#include <util/generic/object_counter.h>
#include <util/thread/pool.h>
#include <kernel/search_daemon_iface/reqtypes.h>

class TSearchRequestData;
class TSearchHandlers;

class IHttpReplier: public IObjectInQueue, public NCSUtil::TObjectCounter<IHttpReplier> {
private:
    const THttpStatusManagerConfig HttpStatusConfig;

public:
    IHttpReplier(IReplyContext::TPtr client, const THttpStatusManagerConfig* config);
    virtual ~IHttpReplier() = default;

    void Reply();
    // IObjectInQueue
    virtual void Process(void* /*ThreadSpecificResource*/) override;

    using TPtr = THolder<IHttpReplier>;

protected:
    virtual TDuration GetDefaultTimeout() const = 0;
    virtual double GetDefaultKffWaitingAvailable() const {
        return 0.9;
    }
    virtual void OnRequestExpired() = 0;
    virtual void OnQueueFailure();
    virtual void MakeErrorPage(ui32 code, const TString& error);
    virtual void SearchAndReply() final;
    virtual void DoSearchAndReply() = 0;
    virtual IThreadPool* DoSelectHandler() = 0;

protected:
    IReplyContext::TPtr Context;
};
