#pragma once

#include <infra/yp_service_discovery/api/api.pb.h>

#include <nginx/modules/strm_packager/src/proto/resolve_data.pb.h>

#include <util/datetime/base.h>

namespace NStrm::NPackager::NPackagerGrpcResolver {
    using TDataProto = NStrm::NPackager::NResolveDataProto::TData;

    constexpr char Scheme[] = "pgr";

    void Init();

    i64 GetDataVersion();

    size_t GetRunningResolversCount();

    TDataProto SaveToProto();

    void LoadFromProto(const i64 version, ui8 const* const buffer, size_t bufferLength);

    void UpdateResolve(
        const TString& id,
        const TString& cluster,
        const ::NYP::NServiceDiscovery::NApi::TRspResolveEndpoints& resp);

    void CheckAddress(const NYP::NServiceDiscovery::NApi::TEndpoint& endpoint);

    // return pair <resolved, first check time>
    std::pair<bool, TInstant> CheckEndpointSedIdResolved(const TString& endpointSetId);
}
