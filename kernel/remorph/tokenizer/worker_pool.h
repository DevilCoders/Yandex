#pragma once

#include "remorph_tokenizer.h"
#include "blocking_queue.h"

#include <util/generic/ptr.h>
#include <util/string/vector.h>
#include <util/thread/factory.h>

namespace NToken {

struct TInputBlock {
    TInputBlock()
        : Callback(nullptr)
    {
    }
    TInputBlock(const TSentenceInfo& sentInfo, ITokenizerCallback& cb)
        : SentInfo(sentInfo)
        , Callback(&cb)
    {
    }
    TSentenceInfo SentInfo;
    ITokenizerCallback* Callback;
};

class TWorkerPool: public IThreadFactory::IThreadAble, public TSimpleRefCount<TWorkerPool> {
private:
    typedef TBlockingQueue<TInputBlock> TInputQueue;
    typedef TSimpleSharedPtr<IThreadFactory::IThread> TThreadPtr;
private:
    size_t NumThreads;
    TInputQueue Queue;
    TVector<TThreadPtr> Threads;

private:
    void DoExecute() override {
        TInputBlock block;
        while (Queue.Pop(block)) {
            Y_ASSERT(block.Callback != nullptr);
            block.Callback->OnSentence(block.SentInfo);
        }
    }

public:
    TWorkerPool(size_t numThreads)
        : NumThreads(numThreads)
        , Queue(1000 * numThreads)
    {
    }

    ~TWorkerPool() override {
        Stop();
    }

    void Start() {
        Y_ASSERT(Threads.empty());
        Queue.SetFinished(false);
    }

    void Stop() {
        Queue.SetFinished();
        for (TVector<TThreadPtr>::const_iterator i = Threads.begin(); i != Threads.end(); ++i) {
            i->Get()->Join();
        }
        Threads.clear();
    }

    inline void Put(const TSentenceInfo& sentInfo, ITokenizerCallback& cb) {
        Queue.Push(TInputBlock(sentInfo, cb));
        // If workers have no time to process the next chunk and we don't start all threads then start the new one
        if (Threads.size() < NumThreads && !Queue.Empty()) {
            Threads.push_back(SystemThreadFactory()->Run(this));
        }
    }
};

typedef TIntrusivePtr<TWorkerPool> TWorkerPoolPtr;

struct TPoolCallbackProxy: public ITokenizerCallback {
    TWorkerPool& Pool;
    ITokenizerCallback& Callback;

    TPoolCallbackProxy(TWorkerPool& pool, ITokenizerCallback& cb)
        : Pool(pool)
        , Callback(cb)
    {
        Pool.Start();
    }

    ~TPoolCallbackProxy() override {
        Pool.Stop();
    }

    void OnSentence(const TSentenceInfo& sentInfo) override {
        Pool.Put(sentInfo, Callback);
    }
};

} // NToken
