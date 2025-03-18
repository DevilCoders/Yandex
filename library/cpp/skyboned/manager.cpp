#include "manager.h"
#include "api.h"

#include <library/cpp/threading/future/async.h>

#include <util/generic/scope.h>

namespace NSkyboneD {

TSkyShareManager::TSkyShareManager(TSkyShareManagerConfig config)
    : Config_(std::move(config))
{
    Y_ENSURE(!Config_.TvmSecret.empty());
    Y_ENSURE(Config_.SrcTvmId && Config_.DstTvmId);

    NTvmAuth::NTvmApi::TClientSettings tvmSettings;
    tvmSettings.SetSelfTvmId(Config_.SrcTvmId);
    tvmSettings.EnableServiceTicketsFetchOptions(Config_.TvmSecret, NTvmAuth::NTvmApi::TClientSettings::TDstVector{Config_.DstTvmId});

    TvmClient_ = MakeHolder<NTvmAuth::TTvmClient>(tvmSettings, NTvmAuth::TDevNullLogger::IAmBrave());

    ThreadPool_.Start(Config_.ApiThreads);
}

THolder<TSkyShare> TSkyShareManager::MakeSkyShare() {
    return MakeHolder<TSkyShare>(TvmClient_->GetServiceTicketFor(Config_.DstTvmId), Config_.ApiConfig);
}

void TSkyShareManager::DeleteSkyShare(const TString& rbtorrent, const TString& sourceId) {
    NSkyboneD::NApi::RemoveResource(Config_.ApiConfig,
                                    "X-Ya-Service-Ticket: " + TvmClient_->GetServiceTicketFor(Config_.DstTvmId),
                                    rbtorrent,
                                    sourceId);
}

void TSkyShareManager::DeleteSkyShareAsync(const TString& rbtorrent, const TString& sourceId) {
    TGuard guard{DeletionsMutex_};
    Deletions_.push_back(NThreading::Async([=]() {
        DeleteSkyShare(rbtorrent, sourceId);
    }, ThreadPool_));
}

void TSkyShareManager::FinishAllAsyncDeletions() {
    TGuard guard{DeletionsMutex_};
    Y_DEFER {
        Deletions_.clear();
    };
    NThreading::WaitAll(Deletions_).GetValueSync();
}

void TSkyShareManager::RegisterSkyShareAsync(TSkySharePtr&& skyShare) {
    TGuard guard{RegistrationsMutex_};
    Registrations_.push_back(NThreading::Async([skyShare=std::move(skyShare)]() {
        skyShare->FinalizeAndGetRBTorrent();
    }, ThreadPool_));
}

void TSkyShareManager::FinishAllAsyncShares() {
    TGuard guard{RegistrationsMutex_};
    Y_DEFER {
        Registrations_.clear();
    };
    NThreading::WaitAll(Registrations_).GetValueSync();
}

TSkyShareManagerPtr MakeSkyShareManager(TSkyShareManagerConfig config) {
    return MakeAtomicShared<TSkyShareManager>(std::move(config));
}

}
