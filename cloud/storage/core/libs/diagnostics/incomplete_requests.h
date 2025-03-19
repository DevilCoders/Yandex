#pragma once

#include "public.h"

#include <cloud/storage/core/libs/common/verify.h>
#include <cloud/storage/core/protos/media.pb.h>

#include <util/datetime/base.h>
#include <util/generic/vector.h>

#include <array>

namespace NCloud {

////////////////////////////////////////////////////////////////////////////////

struct TIncompleteRequest
{
    NProto::EStorageMediaKind MediaKind = NProto::STORAGE_MEDIA_DEFAULT;
    TDiagnosticsRequestType RequestType = 0;
    TDuration RequestTime;
};

////////////////////////////////////////////////////////////////////////////////

template <ui32 RequestCount>
class TIncompleteRequestStats
{
private:
    std::array<TDuration, RequestCount> Ssd;
    std::array<TDuration, RequestCount> SsdNonrepl;
    std::array<TDuration, RequestCount> SsdMirror2;
    std::array<TDuration, RequestCount> SsdMirror3;
    std::array<TDuration, RequestCount> SsdLocal;
    std::array<TDuration, RequestCount> Hdd;

public:
    void Add(
        NProto::EStorageMediaKind mediaKind,
        TDiagnosticsRequestType requestType,
        TDuration requestTime)
    {
        STORAGE_VERIFY(
            requestType < RequestCount,
            "MediaKind",
            static_cast<ui32>(mediaKind));

        auto& maxTime = GetCounters(mediaKind)[requestType];
        if (maxTime < requestTime) {
            maxTime = requestTime;
        }
    }

    TVector<TIncompleteRequest> Finish()
    {
        TVector<TIncompleteRequest> result;
        CollectCounters(NProto::STORAGE_MEDIA_SSD, result);
        CollectCounters(NProto::STORAGE_MEDIA_SSD_NONREPLICATED, result);
        CollectCounters(NProto::STORAGE_MEDIA_SSD_MIRROR2, result);
        CollectCounters(NProto::STORAGE_MEDIA_SSD_MIRROR3, result);
        CollectCounters(NProto::STORAGE_MEDIA_SSD_LOCAL, result);
        CollectCounters(NProto::STORAGE_MEDIA_HDD, result);
        return result;
    }

private:
    TDuration* GetCounters(NProto::EStorageMediaKind mediaKind)
    {
        switch (mediaKind) {
            case NProto::STORAGE_MEDIA_SSD:
                return Ssd.begin();
            case NProto::STORAGE_MEDIA_SSD_NONREPLICATED:
                return SsdNonrepl.begin();
            case NProto::STORAGE_MEDIA_SSD_MIRROR2:
                return SsdMirror2.begin();
            case NProto::STORAGE_MEDIA_SSD_MIRROR3:
                return SsdMirror3.begin();
            case NProto::STORAGE_MEDIA_SSD_LOCAL:
                return SsdLocal.begin();
            case NProto::STORAGE_MEDIA_HDD:
            case NProto::STORAGE_MEDIA_HYBRID:
            case NProto::STORAGE_MEDIA_DEFAULT:
                return Hdd.begin();
            default: {
                Y_FAIL(
                    "unsupported media kind: %u",
                    static_cast<ui32>(mediaKind));
            }
        }
    }

    void CollectCounters(
        NCloud::NProto::EStorageMediaKind mediaKind,
        TVector<TIncompleteRequest>& result)
    {
        auto* maxTime = GetCounters(mediaKind);
        for (TDiagnosticsRequestType t = 0; t < RequestCount; ++t) {
            if (maxTime[t]) {
                result.push_back({ mediaKind, t, maxTime[t] });
            }
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

struct IIncompleteRequestProvider
{
    virtual ~IIncompleteRequestProvider() = default;

    virtual TVector<TIncompleteRequest> GetIncompleteRequests() = 0;
};

IIncompleteRequestProviderPtr CreateIncompleteRequestProviderStub();

}   // namespace NCloud
