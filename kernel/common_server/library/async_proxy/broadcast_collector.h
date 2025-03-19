#pragma once
#include "async_delivery.h"
#include "shard_source.h"

#include <kernel/common_server/library/metasearch/simple/config.h>
#include <search/meta/scatter/source.h>
#include <search/meta/scatter/options/options.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>

class TBroadcastCollector {
private:
    TAsyncDelivery& AD;
    TVector<THolder<NScatter::ISource>> Sources;
    const NSimpleMeta::TConfig Config;
public:

    TBroadcastCollector(const TVector<TString>& searchSources, const NSimpleMeta::TConfig& config, TAsyncDelivery& aD)
        : AD(aD)
        , Config(config)
    {
        NScatter::TSourceOptions opts;
        opts.EnableIpV6 = true;
        opts.MaxAttempts = config.GetMaxAttempts();
        opts.AllowDynamicWeights = config.GetAllowDynamicWeights();
        opts.ConnectTimeouts = { config.GetConnectTimeout() };
        opts.SendingTimeouts = { config.GetSendingTimeout() };
        ui32 indexSource = 0;
        for (auto&& i : searchSources) {
            Sources.push_back(std::move(NScatter::CreateSimpleSource(ToString(indexSource++), i, opts)));
        }
    }

    void Collect(IReportBuilder::TPtr reportBuilder, const TInstant deadline) {
        auto task = MakeHolder<TAsyncTask>(reportBuilder, deadline);
        ui32 shardIdx = 0;
        for (auto&& i : Sources) {
            task->AddShard(MakeAtomicShared<NAsyncProxyMeta::TSimpleShard>(task.Get(), i.Get(), Config, "", &AD, shardIdx++, nullptr));
        }
        AD.Send(task.Release());
    }
};
