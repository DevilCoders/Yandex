#pragma once

#include <kernel/knn_service/protos/knn_multi_request.pb.h>
#include <search/idl/meta.pb.h>

#include <library/cpp/yson/node/node.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/maybe.h>
#include <util/generic/array_ref.h>
#include <util/generic/string.h>

namespace NKnnService {

struct TKnnRequest {
    static TString GenerateSaasCgiParams(const NKnnService::NProtos::TMultiRequest& multiRequest) noexcept(false);
    static TString GenerateExtraCgiForSaasSearchProxy(const NKnnService::NProtos::TMultiRequest& multiRequest) noexcept(false);
};

using TResponceTransport = NMetaProtocol::TReport;

struct TResponseTraits {
    static float GetDistanceFromResultToQuery(const NMetaProtocol::TDocument&);
    static void SetDistanceFromQuery(NMetaProtocol::TDocument&, float dist);
};


struct TDocumentResult {
    ui32 ComeFromRequestId = 0;
    ui32 DocId = Max();
    TString ShardId;

    float DistanceFromQuery = 0;
    TString DocIdentifier;

    TMaybe<TString> GroupingName;
    TMaybe<TString> GroupName;

    THashMap<TString, TString> FetchedDocFields;
    TMaybe<TString> BodyPacked;

    TStringBuf UnpackBodyAsString() const;
};

struct TResponse {
    const TResponceTransport& DataHolderRef;

    TVector<TDocumentResult> Documents;

    TResponse(const TResponceTransport& metaResponceToParse) noexcept(false);
};

namespace NDetail {
    TString PackBody(const NYT::TNode&);
    NYT::TNode UnPackBody(TStringBuf d);
}

}
