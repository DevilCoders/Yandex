#include "shard_source.h"
#include "addr.h"
#include "message.h"

#include <search/meta/scatter/source.h>

#include <kernel/httpsearchclient/httpsearchclient.h>

#include <util/generic/hash.h>

IAddrDelivery* TShardISource::DoNext(const ui64 att) {
    TInstant dl = Owner->GetDeadline();
    if (dl < Now()) {
        return nullptr;
    }
    const TConnData* data = nullptr;
    {
        TGuard<TMutex> g(IteratorLock);
        data = Iterator->Next(att);
    }
    if (data) {
        NNeh::TMessage mess = BuildMessage(att);
        if (!mess.Addr) {
            mess.Addr = data->SearchScript();
        }
        return new TAddrDelivery(this, mess, dl, att);
    }
    return nullptr;
}

bool TShardISource::Prepare() {
    const NNeh::TMessage mess = BuildMessage(0);
    NHttpSearchClient::TSeed seed(ComputeHash(mess.Addr) + ComputeHash(mess.Data));
    Iterator = GetSource()->GetSearchConnections(seed);
    return true;
}

TShardISource::TShardISource(TAsyncTask* owner, NScatter::ISource* source, const NSimpleMeta::TConfig& metaConfig, IAsyncDelivery* asyncDelivery, const ui32 shardId)
    : TShardDelivery(owner, source, metaConfig, asyncDelivery, shardId)
{
}

TShardISource::~TShardISource() {
}

NAsyncProxyMeta::TSimpleShard::TSimpleShard(TAsyncTask* owner, NScatter::ISource* source, const NSimpleMeta::TConfig& metaConfig, const TString& request, IAsyncDelivery* asyncDelivery, const ui32 shardId, IReaskLimiter* limiter /*= nullptr*/)
    : TShardISource(owner, source, metaConfig, asyncDelivery, shardId)
    , Request(request)
{
    SetReasksLimiter(limiter);
}

NNeh::TMessage NAsyncProxyMeta::TSimpleShard::BuildMessage(const ui32 att) const {
    NNeh::TMessage result = Message;
    result.Data += att ? (TString(!result.Data ? "?" : "&") + "ap_reask=" + ToString(att)) : "";
    return result;
}

bool NAsyncProxyMeta::TSimpleShard::Prepare() {
    Message.Data = Request;
    return TShardISource::Prepare();
}
