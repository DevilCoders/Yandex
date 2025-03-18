#include "async_lookups.h"

#include <aapi/lib/trace/trace.h>

#include <library/cpp/blockcodecs/stream.h>

#include <util/string/hex.h>

using namespace NThreading;
using namespace NYT;

namespace NAapi {

TAsyncLookups::TAsyncLookups(NYT::IClientPtr client, const TString& table, size_t minChunkSize, size_t maxChunkSize, IThreadPool* pool)
    : Client(client)
    , Table(table)
    , MinChunkSize(minChunkSize)
    , MaxChunkSize(maxChunkSize)
    , Pool(pool)
    , Done(false)
    , OnLookupCount(0)
{
}

void TAsyncLookups::Lookup(const TString& key) {
    with_lock(Mutex) {
        InKeys.emplace_back(ToYtKey(key));

        if (InKeys.size() >= Min(MaxChunkSize, Max(MinChunkSize, FastClp2(OnLookupCount)))) {
            Lookups.emplace_back(StartAsyncLookup(std::move(InKeys)));
        }
    }

    HasLookupsCV.Signal();
}

void TAsyncLookups::LookupsDone() {
    with_lock(Mutex) {
        if (InKeys) {
            Lookups.emplace_back(StartAsyncLookup(std::move(InKeys)));
        }
        Done = true;
    }

    HasLookupsCV.Signal();
}

bool TAsyncLookups::Next(TString& key, TString& data) {
    TMaybe<TLookup> lookup;

    do {
        with_lock(Mutex) {
            while (!InKeys && !Lookups && !OutObjects && !Done) {
                HasLookupsCV.Wait(Mutex);
            }

            if (OutObjects) {
                std::tie(key, data) = OutObjects.front();
                OutObjects.pop_front();
                return true;
            } else if (Lookups) {
                lookup.ConstructInPlace(Lookups.front());
                Lookups.pop_front();
            } else if (InKeys) {
                lookup.ConstructInPlace(StartAsyncLookup(std::move(InKeys)));
            } else if (Done) {
                return false;
            } else {
                ythrow yexception();  // never happens
            }
        }

        Y_ENSURE(lookup);
        const NThreading::TFuture<TVector<TRow> > lookupFuture = lookup->first;
        const size_t lookupSize = lookup->second;

        lookupFuture.Wait();

        if (lookupFuture.HasException()) {
            try {
                lookupFuture.GetValueSync();
            } catch (yexception e) {
                Error.ConstructInPlace(e.what());
            }

            with_lock(Mutex) {
                OnLookupCount -= lookupSize;
            }
        }


    } while (lookup->first.HasException());  // elimination of tail recursion

    with_lock(Mutex) {
        for (const TRow& row: lookup->first.GetValueSync()) {
            OutObjects.emplace_back(row);
        }

        Y_ENSURE(lookup->first.GetValueSync().size() == lookup->second);

        OnLookupCount -= lookup->second;

        std::tie(key, data) = OutObjects.front();
        OutObjects.pop_front();
    }

    return true;
}

const TMaybe<TString>& TAsyncLookups::LastError() const {
    return Error;
}

size_t TAsyncLookups::Size() const {
    with_lock(Mutex) {
        return InKeys.size() + OnLookupCount + OutObjects.size();
    }
}

NYT::TNode TAsyncLookups::ToYtKey(const TString& key) {
    return NYT::TNode()("hash", key);
}

TString TAsyncLookups::FromYtKey(const NYT::TNode& ytKey) {
    return ytKey["hash"].AsString();
}

TString TAsyncLookups::FromYtRow(const NYT::TNode& ytRow) {
    if (ytRow.IsNull()) {
        return TString();
    }

    const ui8 type = static_cast<ui8>(ytRow["type"].AsUint64());
    const TString& data = ytRow["data"].AsString();

    TString dump;
    TStringOutput out(dump);

    // data
    TStringInput in(data);

    if (type == 1) {
        NBlockCodecs::TDecodedInput decodedIn(&in);
        TransferData(&decodedIn, &out);
    } else {
        TransferData(&in, &out);
    }

    // type
    out.Write(&type, 1);

    return dump;
}

TLookup TAsyncLookups::StartAsyncLookup(TVector<NYT::TNode> keys) {
    auto promise = NewPromise<TVector<TRow>>();
    TLookup ret(promise.GetFuture(), keys.size());

    Pool->SafeAddFunc([client = Client, table = Table, keys = std::move(keys), promise = promise]() mutable {
        Trace(TLookupStarted(keys.size()));

        TVector<NYT::TNode> ytRows;

        try {
            ytRows = client->LookupRows(table, keys, NYT::TLookupRowsOptions().KeepMissingRows(true).Columns({"type", "data"}));
        } catch (const yexception& e) {
            promise.SetException(std::current_exception());
            Trace(TLookupFailed(keys.size(), e.what()));
            return;
        }

        TVector<TRow> value;
        for (size_t i = 0; i < keys.size(); ++i) {
            value.emplace_back(FromYtKey(keys[i]), FromYtRow(ytRows[i]));
        }

        Trace(TLookupFinished(keys.size()));
        promise.SetValue(value);
    });

    OnLookupCount += ret.second;
    return ret;
}

TLookupThread::TLookupThread(NYT::IClient* client, const TString* table, TVector<TString> keys)
    : Client(client)
    , Table(table)
    , Keys(std::move(keys))
    , Promise(NewPromise<TVector<TString>>())
{
    Thread = SystemThreadFactory()->Run(this);
}

TLookupThread::~TLookupThread() {
    if (Thread) {
        Thread->Join();
    }
}

void TLookupThread::Join() {
    Thread->Join();
}

const TVector<TString>& TLookupThread::GetKeys() const {
    return Keys;
}

TFuture<TVector<TString>> TLookupThread::GetFuture() const {
    return Promise.GetFuture();
}

void TLookupThread::DoExecute() {
    Trace(TLookupStarted(Keys.size()));

    TVector<TNode> ytKeys;
    ytKeys.reserve(Keys.size());
    for (const TString& key: Keys) {
        ytKeys.emplace_back(TNode()("hash", key));
    }

    try {
        const auto& rows = Client->LookupRows(*Table, ytKeys, TLookupRowsOptions().KeepMissingRows(true).Columns({"type", "data"}));
        TVector<TString> result(rows.size());
        for (size_t i = 0; i < rows.size(); ++i) {
            result[i] = TAsyncLookups::FromYtRow(rows[i]);
        }
        Trace(TLookupFinished(Keys.size()));
        Promise.SetValue(std::move(result));
    } catch (const yexception& e) {
        Promise.SetException(std::current_exception());
        Trace(TLookupFailed(Keys.size(), e.what()));
    }
}

}  // namespace NAapi

