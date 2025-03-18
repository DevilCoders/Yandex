#pragma once

#include <mapreduce/yt/interface/client.h>
#include <util/thread/pool.h>
#include <util/memory/blob.h>
#include <util/generic/deque.h>
#include <util/system/condvar.h>

namespace NAapi {

using TRow = std::pair<TString, TString>;
using TLookup = std::pair<NThreading::TFuture<TVector<TRow>>, size_t>;

//
// Thread-safe for single producer and single consumer only
//
class TAsyncLookups {
public:
    TAsyncLookups(NYT::IClientPtr client, const TString& table, size_t minChunkSize, size_t maxChunkSize, IThreadPool* pool);

    // producer methods
    void Lookup(const TString& key);
    void LookupsDone();

    // consumer methods
    bool Next(TString& key, TString& data);
    const TMaybe<TString>& LastError() const;
    size_t Size() const;

public:
    static NYT::TNode ToYtKey(const TString& key);
    static TString FromYtKey(const NYT::TNode& ytKey);
    static TString FromYtRow(const NYT::TNode& ytRow);

private:
    TLookup StartAsyncLookup(TVector<NYT::TNode> keys);

private:
    TMutex Mutex;
    TCondVar HasLookupsCV;
    TVector<NYT::TNode> InKeys;
    TDeque<TLookup> Lookups;
    TDeque<TRow> OutObjects;
    TMaybe<TString> Error;

    NYT::IClientPtr Client;
    const TString Table;
    const size_t MinChunkSize;
    const size_t MaxChunkSize;
    IThreadPool* Pool;
    bool Done;
    size_t OnLookupCount;
};

class TLookupThread
    : public IThreadFactory::IThreadAble
    , public TNonCopyable
{
public:
     TLookupThread(NYT::IClient* client, const TString* table, TVector<TString> keys);
    ~TLookupThread();

    void Join();

    const TVector<TString>& GetKeys() const;

    NThreading::TFuture<TVector<TString>> GetFuture() const;

private:
    void DoExecute() override;

private:
    NYT::IClient* Client;
    const TString* Table;
    const TVector<TString> Keys;

    NThreading::TPromise<TVector<TString>> Promise;
    TAutoPtr<IThreadFactory::IThread> Thread;
};

using TLookupThreadHolder = THolder<TLookupThread>;

}  // namespace NAapi

