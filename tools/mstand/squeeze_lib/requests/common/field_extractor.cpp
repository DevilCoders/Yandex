#include "field_extractor.h"

#include "library/cpp/uri/uri.h"
#include <library/cpp/string_utils/url/url.h>

#include <util/string/split.h>
#include <util/generic/vector.h>

#include <regex>


// GetRefererName: https://a.yandex-team.ru/arc/trunk/arcadia/tools/mstand/session_squeezer/referer.py?rev=r6098223#L29

TString StripProtocolWWW(const TString& url) {
    NUri::TUri parsedUrl;
    parsedUrl.Parse(url, NUri::TFeature::FeatureSchemeFlexible);
    TString host = TString{parsedUrl.GetField(NUri::TField::FieldHost)};
    TString path = TString{parsedUrl.GetField(NUri::TField::FieldPath)};
    if (host.StartsWith("www.")) {
        host = host.substr(4);
    }
    return host + path;
}

const std::regex RE_WEB("^(yandex\\.ru/yandsearch|ya\\.ru/yandsearch|yandsearch\\.yandex\\.ru/yandsearch|m\\.yandex\\.ru/yandsearch).*");
const std::regex RE_IMAGES("^(images\\.yandex\\.ru|yandex\\.ru/images|ya\\.ru/images|m\\.images\\.yandex\\.ru).*");
const std::regex RE_VIDEO("^(video\\.yandex\\.ru|yandex\\.ru/video|ya\\.ru/video|m\\.video\\.yandex\\.ru).*");
const std::regex RE_SERVICES("^(yandex\\.ru|ya\\.ru|.+\\.yandex\\.ru|m\\..+\\.yandex\\.ru).*");

const TVector<std::pair<TString, std::regex>> RE_REFERER {
    {"web", RE_WEB},
    {"images", RE_IMAGES},
    {"video", RE_VIDEO},
    {"services", RE_SERVICES},
};

TString GetRefererName(const TString& referer, const TCgiParameters& cgi) {
    TCgiParameters::const_iterator msidIt = cgi.find("msid");
    if (msidIt != cgi.end()) {
        return "morda";
    }

    const TString domainWithPath = StripProtocolWWW(referer);
    for (const auto& item : RE_REFERER) {
        if (std::regex_match(domainWithPath.begin(), domainWithPath.end(), item.second)) {
            return item.first;
        }
    }
    return "other";
}
// GetRefererName End

// GetDomRegionFromHost https://a.yandex-team.ru/arc/trunk/arcadia/tools/mstand/session_squeezer/squeezer_common.py?rev=r6098223#L765-778

TString ResolveDomRegionFromHost(const TString& host) {
    auto dot_position = host.rfind(".");
    if (dot_position == TString::npos) {
        return host;
    }
    return host.substr(dot_position + 1);
}

TMaybe<TString> GetDomRegion(const NRA::TRequest* request) {
    if (const auto* yandexRequestProperties = dynamic_cast<const NRA::TYandexRequestProperties*>(request)) {
        return yandexRequestProperties->GetServiceDomRegion();
    }
    else if (const auto* portalRequestProperties = dynamic_cast<const NRA::TPortalRequestProperties*>(request)) {
        return ResolveDomRegionFromHost(portalRequestProperties->GetHost());
    }
    return Nothing();
}
// GetDomRegionFromHost End

// PrepareSuggestData
const std::regex RE_TPATH_PART("\\[(\\w+),(p\\d+),(\\d+)\\]|\\[([\\w\\d,]+)\\]");

NYT::TNode ParseTpahLog(const TMaybe<TString>& tpathLog) {
    if (!tpathLog.Defined()) {
        return NYT::TNode::CreateEntity();
    }
    auto result = NYT::TNode::CreateList();
    std::smatch match;
    TString tmp = tpathLog.GetRef();
    while (std::regex_search(tmp.begin(), tmp.end(), match, RE_TPATH_PART)) {
        auto node = NYT::TNode::CreateList({NYT::TNode(match.str(0))});
        if (match.str(4).empty()) {
            node = NYT::TNode::CreateList({NYT::TNode(match.str(1)), NYT::TNode(match.str(2))});
            size_t milliseconds;
            if (TryFromString(match.str(3), milliseconds)) {
                node.Add(milliseconds);
            }
            else {
                node.Add(NYT::TNode(match.str(3)));
            }
        }
        result.Add(node);
        tmp = match.suffix().str();
    }
    return result;
}

template <class TValue>
NYT::TNode UnwrapOrNull(const TMaybe<TValue>& value) {
    if (value.Defined()) {
        return value.GetRef();
    }
    return NYT::TNode::CreateEntity();
}

NYT::TNode PrepareSuggestData(const NRA::TRequest* request) {
    const auto* yandexRequestProperties = dynamic_cast<const NRA::TYandexRequestProperties*>(request);
    if (yandexRequestProperties == nullptr) {
        return NYT::TNode::CreateEntity();
    }

    const NRA::TYandexSuggest* suggest = yandexRequestProperties->GetSuggest();
    if (suggest == nullptr) {
        return NYT::TNode::CreateEntity();
    }

    auto result = NYT::TNode::CreateMap();
    result["TimeSinceFirstChange"] = suggest->GetTimeSinceFirstChange();
    result["TimeSinceLastChange"] = suggest->GetTimeSinceLastChange();
    result["UserKeyPressesCount"] = suggest->CalcUserKeyPressesCount();
    result["UserInput"] = suggest->GetUserInput();
    result["TpahLog"] = ParseTpahLog(suggest->GetTpahLog());
    result["Status"] = suggest->GetStatus();
    result["TotalInputTime"] = UnwrapOrNull(suggest->GetTotalInputTimeInMilliseconds());
    return result;
}
// PrepareSuggestData End

bool GetHasMisspell(const NRA::TRequest* request) {
    if (const auto* misspellRequestProperties = dynamic_cast<const NRA::TMisspellRequestProperties*>(request)) {
        return misspellRequestProperties->IsMSP();
    }
    return false;
}

bool GetIsPermissionRequested(const NRA::TRequest* request) {
    for (const auto& techEvent : request->GetYandexTechEvents()) {
        const auto& path = techEvent->GetPath();
        if (path.find("690.471.287.614.1304") != TString::npos) {
            return true;
        }
    }
    return false;
}

NYT::TNode GetRearrValues(const NRA::TNameValueMap& rearrValues) {
    static THashMap<TString, TString> keys {
        {"IsFinancialQuery", "IsFinancialQuery"},
        {"IsLawQuery", "IsLawQuery"},
        {"IsMedicalQuery", "IsMedicalQuery"},
        {"wizdetection_beauty_ecom_classifier_prob", "beauty_ecom_classifier_prob"},
        {"wizdetection_cehac_ecom_classifier_prob", "cehac_ecom_classifier_prob"},
        {"wizdetection_diy_ecom_classifier_prob", "diy_ecom_classifier_prob"},
        {"wizdetection_ecom_classifier_prob", "ecom_classifier_prob"},
        {"wizdetection_ecom_classifier", "ecom_classifier"},
        {"wizdetection_fashion_ecom_classifier_prob", "fashion_ecom_classifier_prob"},
        {"wizdetection_home_ecom_classifier_prob", "home_ecom_classifier_prob"},
        {"wizdetection_kids_ecom_classifier_prob", "kids_ecom_classifier_prob"},
        {"wizdetection_pharma_classifier", "pharma_ecom_classifier"},
        {"wizdetection_pharma_ecom_classifier", "pharma_ecom_classifier"},
        {"wizdetection_pharma_ecom_classifier_prob", "pharma_ecom_classifier_prob"},
        {"class_query_is_cs_v2", "class_query_is_cs_v2"},
    };
    auto result = NYT::TNode::CreateMap();
    for (const auto& key : keys) {
        if (const auto* valuePtr = rearrValues.FindPtr(key.first)) {
            float value;
            if (TryFromString(*valuePtr, value)) {
                result[key.second] = value;
            }
        }
    }
    return result;
}

NYT::TNode GetRelevValues(const NRA::TNameValueMap& relevValues) {
    static TVector<TString> keys {
        "fresh_news_detector_predict",
        "hp_detector_predict",
        "pay_detector_predict",
        "purchase_total_predict",
        "query_about_many_products",
        "query_about_one_product",
        "query_conversion_detector_predict",
        "fresh_flow",
        "is_nav",
    };
    auto result = NYT::TNode::CreateMap();
    for (const auto& key : keys) {
        if (const auto* valuePtr = relevValues.FindPtr(key)) {
            float value;
            if (TryFromString(*valuePtr, value)) {
                result[key] = value;
            }
        }
    }
    return result;
}

NYT::TNode GetUserHistory(const NRA::TNameValueMap& searchProps) {
    static TVector<TString> keys {
        "WEB.UserHistory.OldestRequestTimestamp",
        "WEB.UserHistory.RequestAmount",
        "WEB.UserHistory.RequestAmountLastWeek",
    };
    auto result = NYT::TNode::CreateMap();
    for (const auto& key : keys) {
        if (const auto* valuePtr = searchProps.FindPtr(key)) {
            result[key] = *valuePtr;
        }
    }
    return result;
}

NYT::TNode GetSearchProps(const NRA::TNameValueMap& searchProps) {
    static TVector<TString> float_keys {
        "UPPER.EntitySearch.Accept",
        "UPPER.ShinyDiscovery.has_data",
        "UPPER.Facts.QueryIsGood",
        "UPPER.SameServicesChecker.has_sport",
    };
    static TVector<TString> string_keys {
        "UPPER.EntitySearch.Otype",
        "UPPER.EntitySearch.Osubtype",
        "UPPER.Stateful.add_to_blend",
    };
    auto result = NYT::TNode::CreateMap();
            
    for (const auto& key : string_keys) {
        if (const auto* valuePtr = searchProps.FindPtr(key)) {
            result[key] = *valuePtr;
        }
    }

    for (const auto& key : float_keys) {
        if (const auto* valuePtr = searchProps.FindPtr(key)) {
            float value;
            if(TryFromString(*valuePtr, value)) {
                result[key] = value;
            }
        }
    }
    return result;
}
