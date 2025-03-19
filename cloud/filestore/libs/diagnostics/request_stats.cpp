#include "request_stats.h"

#include "config.h"
#include "critical_events.h"

#include <cloud/filestore/libs/service/context.h>
#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/format.h>
#include <cloud/storage/core/libs/common/timer.h>
#include <cloud/storage/core/libs/diagnostics/incomplete_requests.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>
#include <cloud/storage/core/libs/diagnostics/monitoring.h>
#include <cloud/storage/core/libs/diagnostics/request_counters.h>
#include <cloud/storage/core/libs/version/version.h>

#include <library/cpp/monlib/dynamic_counters/counters.h>

#include <util/generic/hash.h>
#include <util/system/mutex.h>

namespace NCloud::NFileStore {

using namespace NMonitoring;

namespace {

////////////////////////////////////////////////////////////////////////////////

TString MediaKindLabel(NProto::EStorageMediaKind kind)
{
    switch (kind) {
        case NProto::STORAGE_MEDIA_SSD:
            return "ssd";
        default:
            return "hdd";
    }
}

TRequestCountersPtr MakeRequestCounters(ITimerPtr timer, TDynamicCounters& counters)
{
    auto requestCounters = std::make_shared<TRequestCounters>(
        std::move(timer),
        FileStoreRequestCount,
        [] (TRequestCounters::TRequestType t) -> const TString& {
            return GetFileStoreRequestName(static_cast<EFileStoreRequest>(t));
        },
        [] (TRequestCounters::TRequestType t) {
            auto rt = static_cast<EFileStoreRequest>(t);
            return rt == EFileStoreRequest::WriteData
                || rt == EFileStoreRequest::ReadData;
        },
        TRequestCounters::EOption::ReportHistogram
    );
    requestCounters->Register(counters);
    return requestCounters;
}

////////////////////////////////////////////////////////////////////////////////

template<typename TContext>
class TBaseStats
{
protected:
    void RequestStarted(TLog& Log, TContext& callContext)
    {
        STORAGE_LOG(TLOG_RESOURCES, LogHeader(callContext) << " REQUEST");

        callContext.SetStartedCycles(GetRequestCounters(callContext).RequestStarted(
            static_cast<TRequestCounters::TRequestType>(callContext.RequestType),
            callContext.RequestSize));
    }

    void RequestCompleted(TLog& Log, TContext& callContext)
    {
        const auto errorKind = GetErrorKind(callContext.Error);

        const auto requestTime = GetRequestCounters(callContext).RequestCompleted(
            static_cast<TRequestCounters::TRequestType>(callContext.RequestType),
            callContext.GetStartedCycles(),
            {}, // postponedTime
            callContext.RequestSize,
            errorKind);

        ELogPriority logPriority;
        TStringBuf message;

        // TODO pass slow req threshold via diagnostic config
        if (requestTime >= TDuration::Seconds(10)) {
            logPriority = TLOG_WARNING;
            message = "request too slow";
        } else if (errorKind == EErrorKind::ErrorFatal) {
            logPriority = TLOG_WARNING;
            message = "request failed";
        } else {
            logPriority = TLOG_RESOURCES;
            message = "request completed";
        }

        STORAGE_LOG(logPriority, LogHeader(callContext)
            << " RESPONSE " << message
            << " (time: " << FormatDuration(requestTime)
            << ", size: " << FormatByteSize(callContext.RequestSize)
            << ", error: " << FormatError(callContext.Error)
            << ")");
    }

    virtual TStringBuilder LogHeader(const TContext& callContext) = 0;
    virtual TRequestCounters& GetRequestCounters(const TContext& callContext) = 0;
};

////////////////////////////////////////////////////////////////////////////////

class TServerStats final
    : public IServerStats
    , public TBaseStats<TServerCallContext>
{
    TDynamicCountersPtr RootCounters;

    TRequestCountersPtr TotalCounters;
    TRequestCountersPtr SsdCounters;
    TRequestCountersPtr HddCounters;

    TMutex Lock;
    THashMap<TString, NProto::EStorageMediaKind> FileSystems;

public:
    TServerStats(
            TDynamicCountersPtr counters,
            ITimerPtr timer)
        : RootCounters{std::move(counters)}
        , TotalCounters{MakeRequestCounters(timer, *RootCounters)}
        , SsdCounters{MakeRequestCounters(timer, *RootCounters->GetSubgroup("type", "ssd"))}
        , HddCounters{MakeRequestCounters(timer, *RootCounters->GetSubgroup("type", "hdd"))}
    {
        SsdCounters->Subscribe(TotalCounters);
        HddCounters->Subscribe(TotalCounters);

        auto revisionGroup =
            RootCounters->GetSubgroup("revision", GetFullVersionString());

        auto versionCounter = revisionGroup->GetCounter(
            "version",
            false);
        *versionCounter = 1;

        InitCriticalEventsCounter(RootCounters);
    }

    void RequestStarted(TLog& log, TServerCallContext& callContext) override
    {
        TBaseStats::RequestStarted(log, callContext);
    }

    void RequestCompleted(TLog& log, TServerCallContext& callContext) override
    {
        TBaseStats::RequestCompleted(log, callContext);
    }

    void ResponseSent(TServerCallContext& callContext) override
    {
        Y_UNUSED(callContext);
    }

    void UpdateRestartsCount(int cnt) override
    {
        auto counter = RootCounters->GetCounter(
            "RestartsCount",
            false);
        *counter = cnt;
    }

    void UpdateStats(bool updatePercentiles) override
    {
        TotalCounters->UpdateStats(updatePercentiles);
        SsdCounters->UpdateStats(updatePercentiles);
        HddCounters->UpdateStats(updatePercentiles);
    }

    void AddIncompleteRequest(const TIncompleteRequest& request)
    {
        if (request.MediaKind != NProto::STORAGE_MEDIA_DEFAULT) {
            GetCounters(request.MediaKind)->AddIncompleteStats(
                request.RequestType,
                request.RequestTime);
        } else {
            TotalCounters->AddIncompleteStats(
                request.RequestType,
                request.RequestTime);
        }
    }

    TRequestCountersPtr GetCounters(NProto::EStorageMediaKind media)
    {
        switch (media) {
            case NProto::STORAGE_MEDIA_SSD: return SsdCounters;
            default: return HddCounters;
        }
    }

    void RegisterFileSystem(const TString& fileSystemId, NProto::EStorageMediaKind media)
    {
        TGuard g{Lock};
        FileSystems[fileSystemId] = media;
    };

    void UnregisterFileSystem(const TString& fileSystemId)
    {
        TGuard g{Lock};
        FileSystems.erase(fileSystemId);
    }

protected:
    TStringBuilder LogHeader(const TServerCallContext& callContext) override
    {
        TStringBuilder sb;

        sb << GetFileStoreRequestName(callContext.RequestType)
            << " " << callContext.RequestId
            << " " << callContext.FileSystemId;

        return sb;
    }

    TRequestCounters& GetRequestCounters(const TServerCallContext& callContext) override
    {
        TGuard g{Lock};
        auto it = FileSystems.find(callContext.FileSystemId);
        if (it == FileSystems.end()) {
            return *TotalCounters;
        }
        return *GetCounters(it->second);
    }
};

////////////////////////////////////////////////////////////////////////////////

class TRequestStats final
    : public IRequestStats
    , public TBaseStats<TCallContext>
{
private:
    const TString FileSystemId;
    const TString ClientId;

    TRequestCountersPtr RequestCounters;

public:
    TRequestStats(
            TString fileSystemId,
            TString clientId,
            ITimerPtr timer,
            TDynamicCountersPtr counters)
        : FileSystemId(std::move(fileSystemId))
        , ClientId(std::move(clientId))
        , RequestCounters(MakeRequestCounters(timer, *counters))
    {
    }

    void Subscribe(TRequestCountersPtr counters)
    {
        RequestCounters->Subscribe(counters);
    }

    void RequestStarted(TLog& log, TCallContext& callContext) override
    {
        TBaseStats::RequestStarted(log, callContext);
    }

    void RequestCompleted(TLog& log, TCallContext& callContext) override
    {
        TBaseStats::RequestCompleted(log, callContext);
    }

    void ResponseSent(TCallContext& callContext) override
    {
        Y_UNUSED(callContext);
    }

    void UpdateStats(bool updatePercentiles) override
    {
        RequestCounters->UpdateStats(updatePercentiles);
    }

protected:
    TStringBuilder LogHeader(const TCallContext& callContext) override
    {
        TStringBuilder sb;

        sb << GetFileStoreRequestName(callContext.RequestType)
            << " " << callContext.RequestId
            << " " << FileSystemId
            << " " << ClientId;

        return sb;
    }

    TRequestCounters& GetRequestCounters(const TCallContext& callContext) override
    {
        Y_UNUSED(callContext);
        return *RequestCounters;
    }
};

////////////////////////////////////////////////////////////////////////////////

class TServerStatsStub final
    : public IServerStats
{
    void RequestStarted(TLog& log, TServerCallContext& callContext) override
    {
        Y_UNUSED(log);
        Y_UNUSED(callContext);
    }

    void RequestCompleted(TLog& log, TServerCallContext& callContext) override
    {
        Y_UNUSED(log);
        Y_UNUSED(callContext);
    }

    void ResponseSent(TServerCallContext& callContext) override
    {
        Y_UNUSED(callContext);
    }

    void UpdateRestartsCount(int cnt) override
    {
        Y_UNUSED(cnt);
    }

    void UpdateStats(bool updatePercentiles) override
    {
        Y_UNUSED(updatePercentiles);
    }
};

////////////////////////////////////////////////////////////////////////////////

class TRequestStatsStub final
    : public IRequestStats
{
public:
    void RequestStarted(TLog& log, TCallContext& callContext) override
    {
        Y_UNUSED(log);
        Y_UNUSED(callContext);
    }

    void RequestCompleted(TLog& log, TCallContext& callContext) override
    {
        Y_UNUSED(log);
        Y_UNUSED(callContext);
    }

    void ResponseSent(TCallContext& callContext) override
    {
        Y_UNUSED(callContext);
    }

    void UpdateStats(bool updatePercentiles) override
    {
        Y_UNUSED(updatePercentiles);
    }
};

////////////////////////////////////////////////////////////////////////////////

class TRequestStatsRegistry final
    : public IRequestStatsRegistry
{
private:
    TDiagnosticsConfigPtr DiagnosticsConfig;
    NMonitoring::TDynamicCountersPtr RootCounters;
    ITimerPtr Timer;

    std::shared_ptr<TServerStats> ServerStats;

    TDynamicCountersPtr FsCounters;
    TMutex Lock;
    THashMap<std::pair<TString, TString>, IRequestStatsPtr> StatsMap;

public:
    TRequestStatsRegistry(
            TString component,
            TDiagnosticsConfigPtr diagnosticsConfig,
            NMonitoring::TDynamicCountersPtr rootCounters,
            ITimerPtr timer)
        : DiagnosticsConfig(std::move(diagnosticsConfig))
        , RootCounters(std::move(rootCounters))
        , Timer(std::move(timer))
    {
        auto totalCounters = RootCounters
            ->GetSubgroup("component", component);
        ServerStats = std::make_shared<TServerStats>(std::move(totalCounters), Timer);

        FsCounters = RootCounters
            ->GetSubgroup("component", component + "_fs")
            ->GetSubgroup("host", "cluster");
    }

    IRequestStatsPtr GetFileSystemStats(
        const TString& fileSystemId,
        const TString& clientId,
        NProto::EStorageMediaKind media) override
    {
        std::pair<TString, TString> key = std::make_pair(fileSystemId, clientId);

        TGuard guard(Lock);
        if (auto it = StatsMap.find(key); it != StatsMap.end()) {
            return it->second;
        }

        auto counters = FsCounters
            ->GetSubgroup("filesystem", fileSystemId)
            ->GetSubgroup("client", clientId)
            ->GetSubgroup("type", MediaKindLabel(media));

        auto stats = CreateRequestStats(
            fileSystemId,
            clientId,
            counters,
            Timer);
        stats->Subscribe(ServerStats->GetCounters(media));

        StatsMap.insert(std::make_pair(key, stats));
        ServerStats->RegisterFileSystem(fileSystemId, media);
        return stats;
    }

    IServerStatsPtr GetServerStats() override
    {
        return ServerStats;
    }

    void Unregister(
        const TString& fileSystemId,
        const TString& clientId) override
    {
        std::pair<TString, TString> key = std::make_pair(fileSystemId, clientId);

        TGuard guard(Lock);
        if (auto it = StatsMap.find(key); it != StatsMap.end()) {
            StatsMap.erase(it);
        }
        FsCounters->GetSubgroup("filesystem", fileSystemId)
            ->RemoveSubgroup("client", clientId);
        ServerStats->UnregisterFileSystem(fileSystemId);
    }

    void AddIncompleteRequest(const TIncompleteRequest& request) override
    {
        ServerStats->AddIncompleteRequest(request);
    }

    void UpdateStats(bool updatePercentiles) override
    {
        ServerStats->UpdateStats(updatePercentiles);

        TGuard guard(Lock);
        for (auto& [_, stats]: StatsMap) {
            stats->UpdateStats(updatePercentiles);
        }
    }

private:
    std::shared_ptr<TRequestStats> CreateRequestStats(
        TString fileSystemId,
        TString clientId,
        TDynamicCountersPtr counters,
        ITimerPtr timer)
    {
        return std::make_shared<TRequestStats>(
            std::move(fileSystemId),
            std::move(clientId),
            std::move(timer),
            std::move(counters));
    }
};

////////////////////////////////////////////////////////////////////////////////

class TRequestStatsRegistryStub final
    : public IRequestStatsRegistry
{
public:
    TRequestStatsRegistryStub() = default;

    IRequestStatsPtr GetFileSystemStats(
        const TString& fileSystemId,
        const TString& clientId,
        NProto::EStorageMediaKind media) override
    {
        Y_UNUSED(fileSystemId);
        Y_UNUSED(clientId);
        Y_UNUSED(media);

        return std::make_shared<TRequestStatsStub>();
    }

    IServerStatsPtr GetServerStats() override
    {
        return std::make_shared<TServerStatsStub>();
    }

    void Unregister(
        const TString& fileSystemId,
        const TString& clientId) override
    {
        Y_UNUSED(fileSystemId);
        Y_UNUSED(clientId);
    }

    void AddIncompleteRequest(const TIncompleteRequest& request) override
    {
        Y_UNUSED(request);
    }

    void UpdateStats(bool updatePercentiles) override
    {
        Y_UNUSED(updatePercentiles);
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IRequestStatsRegistryPtr CreateRequestStatsRegistry(
    TString component,
    TDiagnosticsConfigPtr diagnosticsConfig,
    NMonitoring::TDynamicCountersPtr rootCounters,
    ITimerPtr timer)
{
    return std::make_shared<TRequestStatsRegistry>(
        std::move(component),
        std::move(diagnosticsConfig),
        std::move(rootCounters),
        std::move(timer));
}

IRequestStatsRegistryPtr CreateRequestStatsRegistryStub()
{
    return std::make_shared<TRequestStatsRegistryStub>();
}

}   // namespace NCloud::NFileStore
