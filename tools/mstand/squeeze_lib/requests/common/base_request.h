#pragma once

#include <quality/user_sessions/request_aggregate_lib/all.h>


namespace NMstand {

// tech wizards, skipped during surplus calculation
static const TVector<TString> TechWizards = {
    "lang_hint", "minuswords", "minuswords_obsolete", "misspell_source",
    "unquote", "pi_e", "switch_off_thes", "request_filter",
    "anti_pirate", "navigation_context"
};

enum class ERequestsContainerType {
    SRCT_UNKNOWN,
    SRCT_WEB,
    SRCT_MORDA,
    SRCT_ADS_VERTICAL,
    SRCT_NAV_SUGGEST,
    SRCT_MARKET,
};

enum EPlacement {
    P_UPPER /* "UPPER" */,
    P_MAIN /* "MAIN" */,
    P_PARALLEL /* "PARALLEL" */,
    P_UPPER_CLICK /* "UPPER_CLICK" */
};

enum ESerpType {
    ST_DESKTOP_SERP /* "desktop" */,
    ST_TOUCH_SERP /* "touch" */,
    ST_PAD_SERP /* "pad" */,
    ST_MOBILE_SERP /* "mobile" */,
    ST_MOBILE_APP_SERP /* "mobileapp" */,
    ST_TOUCH_OR_MOBILE_APP_SERP /* "touch_or_app" */,
    ST_UNKNOWN_SERP /* "unknown" */
};

struct TRequestsSettings {
    TRequestsSettings(
        ERequestsContainerType requestsContainerType
    );

    ERequestsContainerType RequestsContainerType = ERequestsContainerType::SRCT_UNKNOWN;
};

bool GetIsAcceptableRequest(const NRA::TRequest& request, const TRequestsSettings& settings);

class TBaseClick {
private:
    static constexpr time_t MAX_DWELL_TIME = 10000;

public:
    TBaseClick() {}

    explicit TBaseClick(const NRA::TClick* click);

    time_t DwellTime = 0;
    time_t DwellTimeOnService = 0;
    time_t DwellTimeOnServiceV2 = 0;
    time_t Timestamp = 0;
    TString Path;
    TString ConvertedPath;
    bool IsDynamic = false;
    bool ShouldBeUsedInDwellTimeOnService = true;

    bool HasUrl = false;
    TString Url;
};

class TBaseBlock {
public:
    TBaseBlock() {}

    explicit TBaseBlock(const NRA::TBlock* block);

public:

    size_t LibraPosition = 0;
    TVector<TString> Adapters;

    bool HasHeight = false;
    ui32 Height = 0;

    EPlacement Placement = EPlacement::P_MAIN;
    size_t Position = 0;

    TString BuildMetricName() const {
        return TString::Join(
            "block_",
            (Placement == EPlacement::P_PARALLEL ? "parallel_" : "main_"),
            ToString(Position)
        );
    }
};

ESerpType GetSerpType(const NRA::TRequest& request);

TString GetServiceType(ESerpType serpType);

class TBaseRequest {
public:
    TBaseRequest(const NRA::TRequest* request, ERequestsContainerType type);

    const NYT::TNode GetBaseData(bool addUI) const;
    const NYT::TNode GetCommonData(const NYT::TNode& baseData) const;

public:
    ERequestsContainerType Type;
    bool IsAcceptableRequest = false;

    const TString ReqID;
    const TString UID;
    TString FullQuery;
    const TString QueryText;
    const TMaybe<TString> CorrectedQuery;
    const time_t Timestamp = 0;
    const bool IsParallel = false;
    ESerpType SerpType;

    bool HasYandexRequestProperties = false;
    TCgiParameters Cgi;

    bool HasPageProperties = false;
    ui32 PageNo = 0;

    bool HasGeoInfoRequestProperties = false;
    TGeoRegion UserRegion = 0;

    TString Browser;
    TString Url;
    TString Referer;
    TMaybe<TString> DomRegion;
    NYT::TNode Suggest;
    bool HasMisspell = false;

    bool IsVisible;

    bool UserPersonalization = false;

    TMaybe<float> FilmListPrediction;
    TMaybe<float> MaxRelevPredict;
    TMaybe<float> MinRelevPredict;

private:
    void ParseSearchProps(const NRA::TRequest* request);
};

class TBaseRequestsContainer {
public:
    explicit TBaseRequestsContainer(
        const TRequestsSettings& settings
    );

public:
    TRequestsSettings Settings;
    THashMap<TString, TString> ReqIDToSerpType;
};

}; //namespace NMstand
