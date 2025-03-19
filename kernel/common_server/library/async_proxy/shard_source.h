#pragma once

#include "shard.h"

#include <util/system/mutex.h>

class IConnIterator;

class TShardISource: public TShardDelivery {
private:
    THolder<IConnIterator> Iterator;
    TMutex IteratorLock;

protected:
    virtual IAddrDelivery* DoNext(const ui64 att) override;

public:
    TShardISource(TAsyncTask* owner, NScatter::ISource* source, const NSimpleMeta::TConfig& metaConfig, IAsyncDelivery* asyncDelivery, const ui32 shardId);
    ~TShardISource();

    virtual bool Prepare() override;
};

namespace NAsyncProxyMeta {
    class TSimpleShard: public TShardISource {
    private:
        const TString Request;
        NNeh::TMessage Message;

    public:
        TSimpleShard(
            TAsyncTask* owner,
            NScatter::ISource* source,
            const NSimpleMeta::TConfig& metaConfig,
            const TString& request,
            IAsyncDelivery* asyncDelivery,
            const ui32 shardId,
            IReaskLimiter* limiter = nullptr
        );

        virtual NNeh::TMessage BuildMessage(const ui32 att) const override;
        virtual bool Prepare() override;
    };
}
