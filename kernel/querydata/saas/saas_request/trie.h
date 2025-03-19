#pragma once

#include "saas_service_opts.h"

#include <kernel/querydata/saas/qd_saas_key.h>
#include <kernel/querydata/saas/qd_saas_request.h>
#include <kernel/saas_trie/idl/saas_trie.pb.h>

namespace NQuerySearch {
    class TSaaSRequestStats;
}

namespace NQueryDataSaaS {
    struct TSplitDim {
        ui32 Index = 0;
        ui32 Weight = 0;
    };

    struct TSaasKey {
        TSaaSKeyType KeyType;
        NSaasTrie::TComplexKey Key;
        ui32 KeysToSearch = 0;
        ui32 KeysToSend = 0;
        TSplitDim SplitDim;
    };

    struct IRequestSender {
        virtual ~IRequestSender() = default;

        virtual void SendRequest(const TString& client,
                                 const TSaasKey& saasKey,
                                 TSaaSRequestRec::TRef requestContext,
                                 TStringBuf additionalCgiParams) = 0;
    };

    void GenerateSaasTrieRequests(const TString& client,
                                  const NQueryData::TRequestRec& rec,
                                  const NQuerySearch::TSaaSServiceOpts& opts,
                                  NQuerySearch::TSaaSRequestStats& stats,
                                  IRequestSender& requestProcessor);
}

