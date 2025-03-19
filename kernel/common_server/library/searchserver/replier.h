#pragma once
#include "simple/replier.h"
#include <kernel/search_daemon_iface/pagecb.h>

class TUnistatFrame;
class TCommonSearch;
class TSearchHandlers;
class TSelfFlushLogFrame;
class TSearchRequestData;

using TUnistatFramePtr = TIntrusivePtr<TUnistatFrame>;
using TSelfFlushLogFramePtr = TIntrusivePtr<TSelfFlushLogFrame>;

class ISearchReplier: public IHttpReplier {
private:
    using TBase = IHttpReplier;
protected:

    virtual const TSearchHandlers* GetSearchHandlers() const {
        return nullptr;
    }
    IThreadPool* GetHandler(ERequestType reqType);
public:
    using TBase::TBase;
};

class TCommonSearchReplier: public ISearchReplier, public IReportCallback {
protected:
    struct TStatistics {
        ui64 TotalDocCount = 0;
        ui32 UnanswerCount = 0;
        float CacheHit = 0;
    };
protected:
    mutable TStatistics Stat;
protected:
    virtual int BeforeReport(ISearchContext* ctx) const override;
public:
    TCommonSearchReplier(IReplyContext::TPtr context, const TCommonSearch* commonSearcher, const THttpStatusManagerConfig* httpStatusConfig);
protected:
    virtual void DoSearchAndReply() override;
    virtual IThreadPool* DoSelectHandler() override;
private:
    void CreateLogFrame();
    virtual const TSearchHandlers* GetSearchHandlers() const override;
protected:
    virtual bool ProcessInfo();
    virtual bool ProcessFetch();
    virtual bool ProcessRequest();

    TMakePageContext CreateMakePageContext();

protected:
    const TCommonSearch* CommonSearcher;

    TSelfFlushLogFramePtr LogFrame;
    TUnistatFramePtr StatFrame;
    ERequestType RequestType;
};
