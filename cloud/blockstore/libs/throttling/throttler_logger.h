#pragma once

#include "public.h"

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

struct IThrottlerLogger
{
    virtual ~IThrottlerLogger() = default;

#define BLOCKSTORE_DECLARE_METHOD(name, ...)                                   \
    virtual void LogPostponedRequest(                                          \
        IVolumeInfo* volumeInfo,                                               \
        const NProto::T##name##Request& request,                               \
        TDuration postponeDelay) = 0;                                          \
                                                                               \
    virtual void LogAdvancedRequest(                                           \
        TInstant now,                                                          \
        TCallContext& callContext,                                             \
        IVolumeInfo* volumeInfo,                                               \
        const NProto::T##name##Request& request,                               \
        TInstant postponeTimestamp) = 0;                                       \
                                                                               \
    virtual void LogError(                                                     \
        const NProto::T##name##Request& request,                               \
        const TString& errorMessage) = 0;                                      \
// BLOCKSTORE_DECLARE_METHOD

    BLOCKSTORE_SERVICE(BLOCKSTORE_DECLARE_METHOD)

#undef BLOCKSTORE_DECLARE_METHOD
};

////////////////////////////////////////////////////////////////////////////////

IThrottlerLoggerPtr CreateThrottlerLoggerDefault(
    IRequestStatsPtr requestStats,
    ILoggingServicePtr logging,
    const TString& loggerName);

IThrottlerLoggerPtr CreateThrottlerLoggerStub();

}   // namespace NCloud::NBlockStore
