#include "base_request.h"
#include "field_extractor.h"

#include <quality/ab_testing/lib_calc_session_metrics/common/words.h>

#include <quality/logs/baobab/api/cpp/common/interface_adapters.h>

namespace NMstand {

TRequestsSettings::TRequestsSettings(
    ERequestsContainerType requestsContainerType
//    double weightSurplusScore,
//    double weightOtherScore
)
    : RequestsContainerType(requestsContainerType)
//    , WeightSurplusScore(weightSurplusScore)
//    , WeightOtherScore(weightOtherScore)
{}

TBaseClick::TBaseClick(
    const NRA::TClick* click
)
    : DwellTime(Min(click->GetDwellTime(), MAX_DWELL_TIME))
    , DwellTimeOnService(Min(click->GetDwellTimeOnService(), MAX_DWELL_TIME))
    , DwellTimeOnServiceV2(Min(click->GetDwellTimeV2OnService(), MAX_DWELL_TIME))
//    , DwellTimeOnService(click->GetDwellTimeOnService())
//    , DwellTimeOnServiceV2(click->GetDwellTimeV2OnService())
    , Timestamp(click->GetTimestamp())
    , IsDynamic(click->IsDynamic())
    , ShouldBeUsedInDwellTimeOnService(click->ShouldBeUsedInDwellTimeOnService())
{
    if (click->GetUrl().Defined()) {
        HasUrl = true;
        Url = click->GetUrl().GetRef();
    }

//    if (click->GetBaobabBlock().Defined()) {
//        BlockId = click->GetBaobabBlock()->GetID();
////        TMaybe<NBaobab::TBlock> baobabBlock = NBaobab::GetAncestorResultBlock(click->GetBaobabBlock().GetRef());
////        if (baobabBlock.Defined()) {
////            Ba
////        }
//    }
}

TBaseBlock::TBaseBlock(const NRA::TBlock* block)
    : LibraPosition(block->GetPosition())
    , Adapters(block->GetAdapters())
{
    if (block->GetHeight().Defined()) {
        Height = block->GetHeight().GetRef();
        HasHeight = Height > 0;
    }
}

ESerpType GetSerpType(const NRA::TRequest& request) {
    const NRA::TYandexRequestProperties* ydx = dynamic_cast<const NRA::TYandexRequestProperties*>(&request);
    const NRA::TPortalRequestProperties* portal = dynamic_cast<const NRA::TPortalRequestProperties*>(&request);
    const NRA::TMarketRequestProperties* market = dynamic_cast<const NRA::TMarketRequestProperties*>(&request);
    if (!ydx && !portal && !market) {
        return ESerpType::ST_UNKNOWN_SERP;
    }
    if (dynamic_cast<const NRA::TPadUIProperties*>(&request)) {
        return ESerpType::ST_PAD_SERP;
    }
    if (dynamic_cast<const NRA::TTouchUIProperties*>(&request)) {
        return ESerpType::ST_TOUCH_SERP;
    }
    if (dynamic_cast<const NRA::TMobileAppUIProperties*>(&request)) {
        return ESerpType::ST_MOBILE_APP_SERP;
    }
    if (dynamic_cast<const NRA::TMobileUIProperties*>(&request)) {
        return ESerpType::ST_MOBILE_SERP;
    }
    if (dynamic_cast<const NRA::TDesktopUIProperties*>(&request)) {
        return ESerpType::ST_DESKTOP_SERP;
    }
    return ESerpType::ST_UNKNOWN_SERP;
}

TString GetServiceType(ESerpType serpType) {
    if (serpType == ESerpType::ST_TOUCH_SERP || serpType == ESerpType::ST_MOBILE_SERP
        || serpType == ESerpType::ST_MOBILE_APP_SERP) {
        return "touch";
    }
    return "web";
}

void InitFromSearchProps(const NRA::TNameValueMap& searchProps, const TString& key, TMaybe<float>& value) {
    NRA::TNameValueMap::const_iterator strValuePtr = searchProps.find(key);
    if (strValuePtr != searchProps.end()) {
        float tmpValue;
        if (TryFromString(strValuePtr->second, tmpValue)) {
            value = tmpValue;
        }
    }
}

TBaseRequest::TBaseRequest(
    const NRA::TRequest* request,
    ERequestsContainerType type)
    : Type(type)
    , ReqID(request->GetReqID())
    , UID(request->GetUID())
    , QueryText(request->GetQuery())
    , CorrectedQuery(NSessionsMetrics::GetQuery(request))
    , Timestamp(request->GetTimestamp())
    , IsParallel(request->IsParallel())
    , SerpType(GetSerpType(*request))
    , Browser(request->GetBrowser())
    , DomRegion(GetDomRegion(request))
    , Suggest(PrepareSuggestData(request))
    , HasMisspell(GetHasMisspell(request))
    , IsVisible(request->IsVisible())
{
    if (const auto* yandexRequestProperties = dynamic_cast<const NRA::TYandexRequestProperties*>(request)) {
        HasYandexRequestProperties = true;
        Cgi = yandexRequestProperties->GetCGI();
        FullQuery = yandexRequestProperties->GetFullRequest();
    }
    if (const auto* pageProperties = dynamic_cast<const NRA::TPageProperties*>(request)) {
        HasPageProperties = true;
        PageNo = pageProperties->GetPageNo();
    }
    if (const auto* geoInfoRequestProperties = dynamic_cast<const NRA::TGeoInfoRequestProperties*>(request)) {
        HasGeoInfoRequestProperties = true;
        UserRegion = geoInfoRequestProperties->GetUserRegion();
    }
    Referer = GetRefererName(request->GetReferer(), Cgi);
    ParseSearchProps(request);
}

const NYT::TNode TBaseRequest::GetBaseData(bool addUI) const {
    NYT::TNode data;
    data["yuid"] = UID;
    data["page"] = PageNo;
    data["reqid"] = ReqID;

    if (DomRegion.Defined()) {
        data["domregion"] = DomRegion.GetRef();
    }
    if (addUI) {
        data["ui"] = ToString(SerpType);
    }

    return data;
}

void TBaseRequest::ParseSearchProps(const NRA::TRequest* request) {
    const auto* misc = dynamic_cast<const NRA::TMiscRequestProperties*>(request);
    if (!misc || !HasYandexRequestProperties) {
        return;
    }

    const NRA::TNameValueMap& searchProps = misc->GetSearchPropsValues();
    UserPersonalization = searchProps.find("UPPER.Personalization.YandexUid") != searchProps.end();

    InitFromSearchProps(searchProps, "WIZARD.IsFilmList.predict", FilmListPrediction);
    InitFromSearchProps(searchProps, "WEB.HoleDetector.relev_predict_quantile_1", MaxRelevPredict);
    InitFromSearchProps(searchProps, "WEB.HoleDetector.relev_predict_quantile_0", MinRelevPredict);
}

const NYT::TNode TBaseRequest::GetCommonData(const NYT::TNode& baseData) const {
    NYT::TNode data = baseData;
    data["browser"] = Browser;
    data["correctedquery"] = CorrectedQuery.Defined()
        ? CorrectedQuery.GetRef()
        : "";
    data["hasmisspell"] = HasMisspell;
    data["query"] = QueryText;
    data["referer"] = Referer;
    data["ts"] = Timestamp;
    data["type"] = "request";
    data["userpersonalization"] = UserPersonalization;
    // mstandRequest["suggest"] = Suggest; // TODO: need fix

    if (MaxRelevPredict.Defined()) {
        data["maxrelevpredict"] = MaxRelevPredict.GetRef();
    }
    if (MinRelevPredict.Defined()) {
        data["minrelevpredict"] = MinRelevPredict.GetRef();
    }
    if (FilmListPrediction.Defined()) {
        data["filmlistprediction"] = FilmListPrediction.GetRef();
    }
    TCgiParameters::const_iterator clidIt = Cgi.find("clid");
    if (clidIt != Cgi.end()) {
        data["clid"] = clidIt->second;
    }
    TCgiParameters::const_iterator msidIt = Cgi.find("msid");
    if (msidIt != Cgi.end()) {
        data["msid"] = msidIt->second;
    }
    if (UserRegion != 0) {
        data["userregion"] = UserRegion;
    }
    TCgiParameters::const_iterator queryregionIt = Cgi.find("lr");
    if (queryregionIt != Cgi.end()) {
        i64 queryRegion;
        if (TryFromString(queryregionIt->second, queryRegion)) {
            data["queryregion"] = queryRegion;
        }
    }
    return data;
}

TBaseRequestsContainer::TBaseRequestsContainer(
    const TRequestsSettings& settings
)
    : Settings(settings)
{}

}; //namespace NMstand
