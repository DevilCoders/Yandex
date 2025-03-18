#pragma once

#include "async_lookups.h"

#include <aapi/lib/trace/trace.h>
#include <util/generic/ptr.h>
#include <util/generic/queue.h>
#include <util/string/hex.h>
#include <util/thread/pool.h>

#include <contrib/libs/grpc/include/grpc++/impl/codegen/status_code_enum.h>
#include <contrib/libs/grpc/include/grpc++/support/status.h>

namespace NAapi {

using TGetObjectCallback = std::function<void (const TString&, const TString&, const grpc::Status&)>;

template <class TStore>
class TAsyncLookups2 {

    struct TLookupRequest {
        TString Hash;
        ui64 Weight;
        TGetObjectCallback Cb;

        TLookupRequest(const TString& hash, ui64 weight, TGetObjectCallback cb)
            : Hash(hash)
            , Weight(weight)
            , Cb(cb)
        {
        }
    };

    using TRequestsChunk = TVector<TLookupRequest>;

public:
    TAsyncLookups2(IThreadPool* pool, ui64 threadsCount, ui64 maxChunkWeight, TStore* store)
        : CurChunkWeight(0)
        , MaxChunkWeight(maxChunkWeight)
        , Store(store)
        , Stopped(false)
    {
        for (size_t i = 0 ; i < threadsCount; ++i) {
            pool->SafeAddFunc([this]() {
                while (true) {
                    TRequestsChunk chunk(WaitNextChunk());

                    if (chunk.empty()) {
                        break;  // Stopped

                    } else {
                        DoLookupNoLock(chunk);
                    }
                }
            });
        }
    }

    ~TAsyncLookups2() {
        {
            auto g(Guard(Lock));
            Stopped = true;
            HasRequestsOrStopped.Signal();
        }
    }

    void ScheduleLookup(const TString& hash, ui64 weight = 1, TGetObjectCallback cb = {}) {
        auto g(Guard(Lock));  // TODO: use lock-free queue?

        CurChunk.emplace_back(hash, weight, cb);
        CurChunkWeight += CurChunk.back().Weight;

        if (CurChunkWeight >= MaxChunkWeight) {
            Chunks.emplace(std::move(CurChunk));
            CurChunkWeight = 0;
        }

        HasRequestsOrStopped.Signal();
    }

private:
    TRequestsChunk WaitNextChunk() {
        TRequestsChunk chunk;

        auto g(Guard(Lock));

        while (CurChunk.empty() && Chunks.empty() && !Stopped) {
            HasRequestsOrStopped.Wait(Lock);
        }

        if (Stopped) {
            HasRequestsOrStopped.Signal();
            return chunk;
        }

        if (Chunks) {
            chunk.swap(Chunks.front());
            Chunks.pop();
        } else {
            chunk.swap(CurChunk);
        }

        if (!Chunks.empty() || !CurChunk.empty()) {
            HasRequestsOrStopped.Signal();
        }

        return chunk;
    }

    void DoLookupNoLock(const TRequestsChunk& lookups) {
        TVector<NYT::TNode> ytKeys;

        for (const TLookupRequest& lookup: lookups) {
            ytKeys.push_back(NYT::TNode()("hash", lookup.Hash));
        }

        TVector<NYT::TNode> ytValues;
        Trace(TLookupStarted(ytKeys.size()));
        try {
            ytValues = Store->LookupRows(ytKeys);
        } catch (yexception e) {
            Trace(TLookupFailed(ytKeys.size(), e.what()));
            for (const TLookupRequest& lookup: lookups) {
                if (lookup.Cb) {
                    lookup.Cb(lookup.Hash, TString(), grpc::Status(grpc::StatusCode::UNAVAILABLE, e.what()));
                }
            }
            return;
        }
        Trace(TLookupFinished(ytKeys.size()));

        for (size_t i = 0; i < lookups.size(); ++i) {
            if (ytValues[i].IsNull()) {
                if (lookups[i].Cb) {
                    lookups[i].Cb(lookups[i].Hash, TString(), grpc::Status(grpc::StatusCode::NOT_FOUND, TString()));
                }
            } else {
                const TString data = TAsyncLookups::FromYtRow(ytValues[i]);
                Store->Put(lookups[i].Hash, data);
                if (lookups[i].Cb) {
                    lookups[i].Cb(lookups[i].Hash, data, grpc::Status::OK);
                }
            }
        }
    }

private:
    TMutex Lock;
    ui64 CurChunkWeight;
    TRequestsChunk CurChunk;
    TQueue<TRequestsChunk> Chunks;
    TCondVar HasRequestsOrStopped;

    const ui64 MaxChunkWeight;
    TStore* Store;

    bool Stopped;
};

}  // namespace NAapi
