#pragma once

#include "api.h"
#include "sky_share.h"

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/mutex.h>
#include <util/thread/pool.h>

#include <library/cpp/tvmauth/client/facade.h>
#include <library/cpp/threading/future/future.h>

namespace NSkyboneD {

struct TSkyShareManagerConfig {
    TString TvmSecret;
    int SrcTvmId;
    int DstTvmId = 2021848;
    int ApiThreads = 4;

    NSkyboneD::NApi::TApiConfig ApiConfig;
};

class TSkyShareManager {
public:
    TSkyShareManager(TSkyShareManagerConfig config);

    TSkySharePtr MakeSkyShare();

    // takes 3-5 seconds to register a share
    void RegisterSkyShareAsync(TSkySharePtr&& skyShare);
    void FinishAllAsyncShares();

    // several milliseconds hopefully
    void DeleteSkyShare(const TString& rbtorrent,
                        const TString& sourceId = "");
    void DeleteSkyShareAsync(const TString& rbtorrent,
                             const TString& sourceId = "");
    void FinishAllAsyncDeletions();

private:
    TSkyShareManagerConfig Config_;
    THolder<NTvmAuth::TTvmClient> TvmClient_;

    TVector<NThreading::TFuture<void>> Registrations_;
    TMutex RegistrationsMutex_;

    TVector<NThreading::TFuture<void>> Deletions_;
    TMutex DeletionsMutex_;

    TThreadPool ThreadPool_;
};
using TSkyShareManagerPtr = TAtomicSharedPtr<TSkyShareManager>;

TSkyShareManagerPtr MakeSkyShareManager(TSkyShareManagerConfig config);

}
