#pragma once

#include "fetched_doc.h"
#include "queue.h"

#include <library/cpp/http/fetch/exthttpcodes.h>
#include <util/generic/noncopyable.h>
#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/list.h>
#include <util/system/spinlock.h>
#include <util/system/guard.h>
#include <util/datetime/base.h>

namespace NSteam
{
    class TAsyncFetchResult: public TCondVarQueue<TSimpleSharedPtr<TFetchedDoc> >
    {
    public:
        virtual ~TAsyncFetchResult()
        {
        }
    };

    struct TFetchTask
    {
        TString Url;
        TString JobId;
        TInstant Deadline;
        TSimpleSharedPtr<TAsyncFetchResult> Sink;

        TFetchTask(const TString& url, const TString& reqId, TInstant deadline, TSimpleSharedPtr<TAsyncFetchResult> sink)
            : Url(url)
            , JobId(reqId)
            , Deadline(deadline)
            , Sink(sink)
        {
        }
    };

    class TBaseFetcher : private TNonCopyable
    {
    protected:
        typedef TList<TFetchTask> TTaskList;
        typedef THashMultiMap<TString, TFetchTask> TTaskMap;

        TAdaptiveLock InGuard;
        TTaskList InTasks;
        TTaskMap InFlight;

        void SetInFlight(TFetchTask& task);
        void Complete(TFetchedDoc& result);
        void Fail(TFetchTask& task, const TString& reason, int httpCode);

    public:
        virtual ~TBaseFetcher() = default;
        virtual const char* GetName() const = 0;
        virtual TDuration GetTimeout() const = 0;
        virtual void Add(const TFetchTask& task);
    };
}
