#pragma once

#include "async_writer.h"

#include <aapi/lib/proto/vcs.grpc.pb.h>
#include <aapi/lib/store/istore.h>
#include <aapi/lib/yt/async_lookups.h>

#include <library/cpp/threading/blocking_queue/blocking_queue.h>

#include <util/thread/factory.h>

#include <util/string/hex.h>

namespace NAapi {

template <class TWriter, class TDataProducer>
class TAsyncTransfer: IThreadFactory::IThreadAble {
public:
    inline TAsyncTransfer(TAsyncLookups* lookups, NStore::IStore* discStore, NStore::IStore* ramStore, TWriter* writer)
        : Lookups(lookups)
        , DiscStore(discStore)
        , RamStore(ramStore)
        , Writer(writer)
    {
        Thread = SystemThreadFactory()->Run(this);
    }

    inline void Join() {
        Thread->Join();
    }

    inline ~TAsyncTransfer() {
        Join();
    }

private:
    TAsyncLookups* Lookups;
    NStore::IStore* DiscStore;
    NStore::IStore* RamStore;
    TWriter* Writer;
    TAutoPtr<IThreadFactory::IThread> Thread;

    inline void DoExecute() {
        TString key;
        TString data;
        while (Lookups->Next(key, data)) {
            Writer->Write(TDataProducer::Produce(key, data));
            RamStore->Put(key, data);
            DiscStore->Put(key, data);
        }
    }
};

struct TObjectProducer {
    inline static TObject Produce(const TString& key, const TString& data) {
        TObject object;
        object.SetHash(key);
        object.SetBlob(data);
        return object;
    }
};

template <class TWriter>
using TAsyncObjectsTransfer = TAsyncTransfer<TWriter, TObjectProducer>;

}  // namespace NAapi
