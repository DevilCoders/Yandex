#include "throttler_logger.h"

#include <cloud/blockstore/libs/diagnostics/request_stats.h>
#include <cloud/blockstore/libs/diagnostics/volume_stats.h>
#include <cloud/blockstore/libs/service/context.h>
#include <cloud/blockstore/libs/throttling/throttler_logger.h>

#include <cloud/storage/core/libs/common/format.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

namespace NCloud::NBlockStore {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TThrottlerLoggerDefault final
    : public IThrottlerLogger
{
private:
    const IRequestStatsPtr RequestStats;

    TLog Log;

public:
    TThrottlerLoggerDefault(
            IRequestStatsPtr requestStats,
            ILoggingServicePtr logging,
            const TString& loggerName)
        : RequestStats(std::move(requestStats))
        , Log(logging->CreateLog(loggerName))
    {}

#define BLOCKSTORE_IMPLEMENT_METHOD(name, ...)                                 \
    void LogPostponedRequest(                                                  \
        IVolumeInfo* volumeInfo,                                               \
        const NProto::T##name##Request& request,                               \
        TDuration postponeDelay) override                                      \
    {                                                                          \
        static constinit auto requestType = EBlockStoreRequest::name;          \
        RequestStats->RequestPostponed(requestType);                           \
        if (volumeInfo) {                                                      \
            volumeInfo->RequestPostponed(requestType);                         \
        }                                                                      \
                                                                               \
        STORAGE_DEBUG(                                                         \
            TRequestInfo(                                                      \
                requestType,                                                   \
                GetRequestId(request),                                         \
                GetDiskId(request),                                            \
                GetClientId(request))                                          \
            << GetRequestDetails(request)                                      \
            << " request postponed"                                            \
            << " (delay: " << FormatDuration(postponeDelay) << ")");           \
   }                                                                           \
                                                                               \
    void LogAdvancedRequest(                                                   \
        TInstant now,                                                          \
        TCallContext& callContext,                                             \
        IVolumeInfo* volumeInfo,                                               \
        const NProto::T##name##Request& request,                               \
        TInstant postponeTimestamp) override                                   \
    {                                                                          \
        static constinit auto requestType = EBlockStoreRequest::name;          \
        RequestStats->RequestAdvanced(requestType);                            \
        if (volumeInfo) {                                                      \
            volumeInfo->RequestAdvanced(requestType);                          \
        }                                                                      \
                                                                               \
        STORAGE_DEBUG(                                                         \
            TRequestInfo(                                                      \
                requestType,                                                   \
                GetRequestId(request),                                         \
                GetDiskId(request),                                            \
                GetClientId(request))                                          \
            << GetRequestDetails(request)                                      \
            << " request advanced");                                           \
                                                                               \
        callContext.AddTime(                                                   \
            EProcessingStage::Postponed,                                       \
            now - postponeTimestamp);                                          \
    }                                                                          \
                                                                               \
    void LogError(                                                             \
        const NProto::T##name##Request& request,                               \
        const TString& errorMessage) override                                  \
    {                                                                          \
        static constinit auto requestType = EBlockStoreRequest::name;          \
        STORAGE_ERROR(                                                         \
            TRequestInfo(                                                      \
                requestType,                                                   \
                GetRequestId(request),                                         \
                GetDiskId(request),                                            \
                GetClientId(request))                                          \
            << GetRequestDetails(request)                                      \
            << " exception in callback: " << errorMessage);                    \
    }                                                                          \
// BLOCKSTORE_IMPLEMENT_METHOD

    BLOCKSTORE_SERVICE(BLOCKSTORE_IMPLEMENT_METHOD)

#undef BLOCKSTORE_IMPLEMENT_METHOD
};

////////////////////////////////////////////////////////////////////////////////

class TThrottlerLoggerStub final
    : public IThrottlerLogger
{
public:

#define BLOCKSTORE_IMPLEMENT_METHOD(name, ...)                                 \
    void LogPostponedRequest(                                                  \
        IVolumeInfo* volumeInfo,                                               \
        const NProto::T##name##Request& request,                               \
        TDuration postponeDelay) override                                      \
    {                                                                          \
        Y_UNUSED(volumeInfo);                                                  \
        Y_UNUSED(request);                                                     \
        Y_UNUSED(postponeDelay);                                               \
    }                                                                          \
                                                                               \
    void LogAdvancedRequest(                                                   \
        TInstant now,                                                          \
        TCallContext& callContext,                                             \
        IVolumeInfo* volumeInfo,                                               \
        const NProto::T##name##Request& request,                               \
        TInstant postponeTimestamp) override                                   \
    {                                                                          \
        Y_UNUSED(now);                                                         \
        Y_UNUSED(callContext);                                                 \
        Y_UNUSED(volumeInfo);                                                  \
        Y_UNUSED(request);                                                     \
        Y_UNUSED(postponeTimestamp);                                           \
    }                                                                          \
                                                                               \
    void LogError(                                                             \
        const NProto::T##name##Request& request,                               \
        const TString& errorMessage) override                                  \
    {                                                                          \
        Y_UNUSED(request);                                                     \
        Y_UNUSED(errorMessage);                                                \
    }                                                                          \
// BLOCKSTORE_IMPLEMENT_METHOD

    BLOCKSTORE_SERVICE(BLOCKSTORE_IMPLEMENT_METHOD)

#undef BLOCKSTORE_IMPLEMENT_METHOD
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IThrottlerLoggerPtr CreateThrottlerLoggerDefault(
    IRequestStatsPtr requestStats,
    ILoggingServicePtr logging,
    const TString& loggerName)
{
    return std::make_shared<TThrottlerLoggerDefault>(
        std::move(requestStats),
        std::move(logging),
        loggerName);
}

IThrottlerLoggerPtr CreateThrottlerLoggerStub()
{
    return std::make_shared<TThrottlerLoggerStub>();
}

}   // namespace NCloud::NBlockStore
