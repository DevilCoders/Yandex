#include "component_api.h"
#include "float_packing.h"
#include "string_constants.h"

#include <quality/ytlib/tools/nodeio/transform.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/generic/xrange.h>
#include <util/string/escape.h>

static bool HasProhibitedSymbols(TString str, TStringBuf charsList) {
    for (char c : charsList) {
        if (str.Contains(c)) {
            return true;
        }
    }
    return false;
}

float NKnnService::TResponseTraits::GetDistanceFromResultToQuery(const NMetaProtocol::TDocument& d) {
    return IntegerToDistance(d.GetSRelevance());
}

void NKnnService::TResponseTraits::SetDistanceFromQuery(NMetaProtocol::TDocument& d, float dist) {
    d.SetRelevance(DistanceToInteger(dist));
    d.SetSRelevance(DistanceToInteger(dist));
    d.SetRelevPredict(dist);//NOTE: can be deleted after boosting-wizard release, see SEARCH-6214
}

TString NKnnService::TKnnRequest::GenerateSaasCgiParams(const NKnnService::NProtos::TMultiRequest & multiRequest) noexcept(false) {
    TStringStream buf;

    buf << "comp:" << SaasComponentName << ";";
    TString packed = Base64EncodeUrl(multiRequest.SerializeAsString());

    constexpr TStringBuf prohibitedSymbols = ":;";
    Y_ENSURE(!HasProhibitedSymbols(packed, prohibitedSymbols));

    buf << MultiRequestParamName << ":" << packed << ";";

    return buf.Str();
}

TString NKnnService::TKnnRequest::GenerateExtraCgiForSaasSearchProxy(const NKnnService::NProtos::TMultiRequest & multiRequest) noexcept(false) {
    TCgiParameters extraParams;
    extraParams.InsertUnescaped("comp_search", GenerateSaasCgiParams(multiRequest));
    extraParams.InsertEscaped("component", "HNSW");
    extraParams.InsertEscaped("normal_kv_report", "da");
    extraParams.InsertEscaped("skip-wizard", "1");
    extraParams.InsertEscaped("meta_search", "first_found");
    extraParams.InsertEscaped("noqtree", "1");
    return "&" + extraParams.Print();
}

NKnnService::TResponse::TResponse(const TResponceTransport & metaResponceToParse) noexcept(false)
    : DataHolderRef(metaResponceToParse)
{
    for (auto& grouping : metaResponceToParse.GetGrouping()) {
        for (auto& group : grouping.GetGroup()) {
            for (auto& doc : group.GetDocument()) {
                try {
                    TDocumentResult& dst = Documents.emplace_back();
                    dst.DistanceFromQuery = doc.GetRelevPredict();
                    dst.GroupingName = UnescapeC(grouping.GetAttr());
                    dst.GroupName = UnescapeC(group.GetCategoryName());
                    TryFromString(doc.GetDocId(), dst.DocId);
                    dst.DocIdentifier = doc.GetUrl();

                    for (auto& atr : doc.GetArchiveInfo().GetGtaRelatedAttribute()) {
                        if (atr.GetKey() == ShardIdDocAttrName) {
                            dst.ShardId = UnescapeC(atr.GetValue());
                            continue;
                        }
                        if (atr.GetKey() == BodyGtaAttrName) {
                            dst.BodyPacked = Base64Decode(atr.GetValue());
                            continue;
                        }
                        if (atr.GetKey() == RequestIdInDocAttrName) {
                            TryFromString(atr.GetValue(), dst.ComeFromRequestId);
                            continue;
                        }

                        dst.FetchedDocFields[UnescapeC(atr.GetKey())] = Base64Decode(atr.GetValue());
                    }
                } catch (yexception& e) {
                    e.Append("error unpacking at document " + doc.GetUrl());
                    throw;
                }
            }
        }
    }
}


namespace {
enum class EPackageType {
    String,
    BinNode,
    TextNode
};
}

TString NKnnService::NDetail::PackBody(const NYT::TNode& n) {
    if (n.IsString()) {
        char t = char(EPackageType::String);
        return TString::Join(TStringBuf(&t, 1), n.AsString());
    } else {
        //char t = char(EPackageType::TextNode);
        //return TString::Join(TStringBuf(&t, 1), NNodeIo::ToOneLineString(n));
        char t = char(EPackageType::BinNode);
        return TString::Join(TStringBuf(&t, 1), NNodeIo::ToBinaryString(n));
    }
}

NYT::TNode NKnnService::NDetail::UnPackBody(TStringBuf d) {
    Y_ENSURE(!d.empty());
    EPackageType t = EPackageType(d[0]);
    switch (t) {
        case EPackageType::String:
        {
            return TStringBuf(d.cbegin() + 1, d.cend());
        } break;
        case EPackageType::BinNode:
        case EPackageType::TextNode:
        {
            return NNodeIo::NodeFromString(TString(d.cbegin() + 1, d.cend()));
        } break;
        default:
        Y_ENSURE(false, "unnown package type " << ui32(t));
    }
}


TStringBuf NKnnService::TDocumentResult::UnpackBodyAsString() const {
    Y_ENSURE(BodyPacked);
    Y_ENSURE(!BodyPacked->empty());
    TStringBuf d = *BodyPacked;
    EPackageType t = EPackageType(*d.cbegin());
    switch (t) {
        case EPackageType::String:
        {
            return TStringBuf(d.cbegin() + 1, d.cend());
        } break;
        default:
        Y_ENSURE(false, "body is not a string, pack-type: " << ui32(t));
    }
}
