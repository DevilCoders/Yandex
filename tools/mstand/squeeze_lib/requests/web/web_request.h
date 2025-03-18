#pragma once

#include <tools/mstand/squeeze_lib/requests/common/base_request.h>

#include <quality/user_metrics/vertical_metrics/video_metrics.h>

#include <quality/ab_testing/stat_collector_lib/common/logs/baobab.h>
#include <quality/user_sessions/request_aggregate_lib/requests_container.h>

#include <mapreduce/yt/interface/client.h>

#include <library/cpp/scheme/scheme.h>

namespace NMstand {

class TWebBlock;

class TWebClick : public TBaseClick {
public:
    explicit TWebClick(const NRA::TClick* click, const TString& blockID = "");

    bool operator<(const TWebClick& webClick) const;
    bool operator==(const TWebClick& webClick) const;
    bool operator!=(const TWebClick& webClick) const;

    const NYT::TNode GetClickData(const NYT::TNode& baseData) const;

    void SetBaobabParameters(const NRA::TClick* click);
    void SetBlock(const std::shared_ptr<TWebBlock>& block);

public:
    bool IsMiscClick = false;
    bool IsScroll = false;
    TMaybe<ui64> FraudBits;
    THashSet<TString> BaobabBlockMarkers;
    TString BaobabPath;
    TString BlockId;
    TString ClickId;
    TString OriginalUrl;

    NYT::TNode BaobabAttrs = NYT::TNode::CreateMap(); // TODO: remove later

private:
    std::weak_ptr<TWebBlock> Block;
};

class TWebBlock : public TBaseBlock {
public:
    explicit TWebBlock() = default;

    TWebBlock(
        const NRA::TBlock* block,
        TAtomicSharedPtr<const NBaobab::NTamus::TMarkersContainer> tamusMarkers,
        TMaybe<const NBaobab::TBlock> baobabBlock = Nothing(),
        double otherScore = 0.,
        bool isParallel = false
    );

    const NYT::TNode GetBlockData() const;

public:
    TVector<std::shared_ptr<TWebClick>> Clicks;

    // Properties
    bool HasWizardResultProperties = false;
    TString WizardName;
    TString CorrectedWizardName;
    TString Path;
    TString ConvertedPath;
    TString ParentPath;
    size_t WizardPosition = 0;

    bool HasOrganicResultProperties = false;
    THashMap<TString, TString> OrganicResultPropertiesMarkers;
    size_t OrganicPosition = 0;
    TString BaseType;

    bool HasVideoWizardResultProperties = false;

    // Result
    bool IsWebResult = false;
    TString WebShard;
    TString WebSource;
    TString WebUrl;
    TString YandexService;
    TString YandexTurboService;
    TVector<TGeoRegion> WebRegions;

    bool IsWizardResult = false;

    bool IsBlenderWizardResult = false;
    TString BlenderWizardName;
    TString BlenderWizardUrl;

    bool IsDirectResult = false;
    size_t DirectPosition = 0;
    ui64 FraudBits = 0;
    ui64 BannerID = 0;
    bool IsUrlMarketplace = false;

    bool IsRecommendationResult = false;

    bool IsLast = false;
    bool IsBna = false;

    bool IsSignificantBlock = false;
    bool IsMedicalHost = false;
    bool IsFinancialHost = false;
    bool IsLegalHost = false;
    bool IsFactResult = false;
    bool IsProgrammingResult = false;
    bool IsFactResultWithEntitySearch = false;
    float RelevPrediction = 0;
    TMaybe<float> ProximaPredict;

    bool IsVideoPlayer = false;
    bool IsExpVideoPlayer = false;
    bool IsEntitySearchPlayer = false;

    ui32 HeartbeatCount = 0;
    ui32 UnmutedHeartbeatCount = 0;
    ui32 Duration = 0;
    double WatchRatio = 0;

    float Attractiveness = 1.0f;
    float DefaultHeight = 1.0f;

    TString BlenderViewType;
    bool MediaTurboBna = false;
    THashMap<TString, bool> BlenderIntents;
    THashSet<TString> BlenderPlugins;

    TString SourceName;
    bool FreshSource = false;
    TString SnippetName;
    TString BlendedWebType;

    ui32 DocumentAge = 0;
    bool IsResultFromMercury = false;
    bool IsResultFromCallisto = false;
    bool IsResultFromJupiter = false;
    bool IsResultFromQuick = false;

    TString BlockId;
    TString FullBlockId;
    TString BaobabWizardName;
    TString BaobabSubType;
    NYT::TNode BaobabAttrs = NYT::TNode::CreateMap();

    bool HasMarketGallery = false;

    bool HasMouseTrackTop = false;
    ui32 MouseTrackTop = 0;
    bool HasMouseTrackHeight = false;
    ui32 MouseTrackHeight = 0;

    THashSet<TString> BaobabBlockMarkers;
    THashSet<TString> BaobabBlockChildrenMarkers;

    double OtherScore;
    TString ResultType;

private:
    void SetIsSignificantBlock();
    void SetHostType();
    void SetIsFactResult(bool hasEntitySearch);
    void SetBlockAttractiveness();
    void SetBlenderViewType();
    void SetMediaTurboBna();
    void SetIsVideoPlayer();
    void SetBlenderIntents();
    void SetPlugins();
    void SetResultType(bool isParallel);
    void ParseBaobabAttrs(const NBaobab::TBlock& baobabBlock);

public:
    const TString GetPlacement() const;
    void SetMouseTrackParameters(const NMouseTrack::TBlockData* blockData);
};

class TWebRequest : public TBaseRequest {
private:
    static const TString TOP_BLOCKS_RULE_NAME;
    static const TString MAIN_BLOCKS_RULE_NAME;
    static const TString PARALLEL_BLOCKS_RULE_NAME;

public:
    explicit TWebRequest(
        const NRA::TRequest* request,
        TAtomicSharedPtr<const NBaobab::NTamus::TMarkersContainer> tamusMarkers,
        const TRequestsSettings& settings,
        const THashMap<TString, double>* otherRequestScores = nullptr
    );

    const NYT::TNode GetSourcesSqueeze() const;
    const NYT::TNode GetRequestParamsSqueeze() const;
    const NYT::TNode GetBlocksSqueeze() const;
    const NYT::TNode GetMousetrackSqueeze() const;
    const NYT::TNode GetRequestData(const NYT::TNode& baseData) const;
    const NYT::TNode GetEnrichedData(const NYT::TNode& baseData, const NYT::TNode& extra) const;
    TVector<NYT::TNode> GetFullRequestData() const;
    TVector<NYT::TNode> GetVideoRequestData() const;

public:
    TVector<std::shared_ptr<TWebBlock>> MainBlocks;
    TVector<std::shared_ptr<TWebBlock>> ParallelBlocks;
    TVector<std::shared_ptr<TWebBlock>> BSBlocks;
    TVector<std::shared_ptr<TWebClick>> AllClicks;
    size_t StaticClicksCount = 0;
    size_t DynamicClicksCount = 0;
    THashSet<std::pair<TString, time_t>> ClickIDs;

    // Properties
    bool HasSerpProperties = false;
    TString Domain;
    TString UserCountryCode;

    bool HasWebRequestProperties = false;
    bool HasVideoRequestProperties = false;
    bool HasPortalRequestProperties = false;

    bool HasDesktopUIProperties = false;
    bool HasPadUIProperties = false;
    bool HasTouchUIProperties = false;

    bool HasBlockstatRequestProperties = false;
    bool HasHalfPremium = false;

    // Custom attributes
    bool MisspellBlendedRequest = false;
    bool IsEntoSerp = false;
    bool WizardGeneratedSerp = false;
    TString GeneratedWizardName = "";

    ui32 ScreenHeight = 0;
    ui32 MaxScrollYCoordinate = 0;
    double ScreensScrolled = 0;

    bool IsBna = false;
    size_t BnaPos = 0;
    TString BnaType;
    TString BnaExtension;

    ui32 MaxScrollYBottom = 0;
    ui32 MaxScrollYTop = 0;
    ui32 ViewportHeight = 0;

    ui32 HeartbeatCount = 0;
    ui32 UnmutedHeartbeatCount = 0;
    ui32 Duration = 0;
    double WatchRatio = 0;

    ui32 PlayerClicks = 0;
    bool HasEtherWizard = false;
    bool HasVideoWizard = false;

    TVector<size_t> PersonalizationNewOrder;

    bool ReRe = false;
    bool WeakReRe = false;
    bool HasMarketDelayedCpcClick = false;

    THashMap<TString, double> VideoWizardGeneratedRequestProperties;
    THashMap<TString, double> EtherWizardGeneratedRequestProperties;

    THashSet<ui64> ExistsBannerID;
    bool NeedParseSearchProps = false;
    bool MarketOnlyDirect = false;
//    double WeightSurplusScore = 1.;
//    double WeightOtherScore = 0.;

    TString ClusterID;
    TString DssmClusterID;
    float HpDetectorPredict = 0;
    float PayDetectorPredict = 0;
    float QueryAboutOneProduct = 0;
    float QueryAboutManyProducts = 0;

    TMaybe<bool> EcomClassifierResult;
    TMaybe<float> EcomClassifierProb;

    // TODO move to video
    bool IsVideoAcceptableRequest = false;
    NYT::TNode VideoData;
    TVector<NYT::TNode> PlayerData;
    TVector<NYT::TNode> VideoHeartbeatData;
    TVector<NYT::TNode> VideoDurationData;

    bool IsPermissionRequested = false;

    NYT::TNode RearrValues;
    NYT::TNode RelevValues;
    NYT::TNode UserHistory;
    NYT::TNode SearchProps;

    TString UserIP;

    bool isReload = false;
    bool isReloadBackForward = false;
private:
    void ParsePlayerEvent(
        const TString& path,
        const TMaybe<ui32>& time,
        const TMaybe<ui32>& duration,
        bool mute
    );
    void GetMiscClicks(const NRA::TRequest* request);
    void ParseSearchPropsAndReqParams(const NRA::TRequest* request);
    void FixMouseTrackParameters();
    void ParseVideoRequestProperties(const NRA::TRequest* request);
    void ParseBaobabProperties(
        const NRA::TRequest* request,
        TAtomicSharedPtr<const NBaobab::NTamus::TMarkersContainer> tamusMarkers,
        const THashMap<TString, double>* otherRequestScores
    );
    void ParseYandexTechEvents(const NRA::TRequest* request);
    void ParseVideoHeartbeat(const NRA::TRequest* request);
    void ParseVideoDurationInfo(const NRA::TVideoRequestProperties* request);

    bool IsMisspellBlendedRequest(const NRA::TRequest& request);
    size_t GetWizShow() const;
    size_t GetDirShow() const;


public:
    static const ui32 SCROLL_Y_LIMIT = 10000;
    static const ui32 UNKNOWN_SCREEN_HEIGHT = 0;
    constexpr static const double SCROLL_SCREENS_LIMIT = 10.;

    static ui32 GetBaobabScreenHeight(const NRA::TRequest* request);
    static ui32 GetBaobabMaxScrollYCoordinate(const NRA::TRequest* request);
    static double GetScreensScrolled(ui32 screenHeight, ui32 maxScrollY);

    void PostProcessBlocks();
};

class TWebRequestsContainer : public TBaseRequestsContainer {
public:
    explicit TWebRequestsContainer(
        const NRA::TRequestsContainer& requestsContainer,
        const TRequestsSettings& settings,
        const THashMap<TString, THashMap<TString, double>>* reqidToBaobabIdToOtherScore = nullptr
    );

    const TVector<TWebRequest>& GetRequestsContainer() const;
    const NUserMetrics::TRequestMetricsMap& GetSessionVideoWizardGeneratedRequestProperties() const;

    const TVector<NYT::TNode>& GetWebSqueeze();
    const TVector<NYT::TNode>& GetWebExtendedSqueeze();
    const TVector<NYT::TNode>& GetVideoSqueeze();

private:
    void SetSessionVideoWizardGeneratedRequestProperties(const NRA::TRequests& requests);
    void AddVideoEtherView(const TString& wizardName, const TVector<time_t>& timestamps);
    void SetSessionEtherWizardGeneratedRequestProperties(const NRA::TRequestsContainer& requestsContainer);
    void SetNavSuggest(const NRA::TRequestsContainer& requestsContainer);

private:
    TVector<TWebRequest> RequestsContainer;

    // Custom attributes
    NUserMetrics::TRequestMetricsMap SessionVideoWizardGeneratedRequestProperties;

    TVector<NYT::TNode> WebSqueeze;
    TVector<NYT::TNode> WebExtendedSqueeze;
    TVector<NYT::TNode> VideoSqueeze;
    TVector<NYT::TNode> NavSuggestData;

//public:
//    double WeightSurplusScore = 1.;
//    double WeightOtherScore = 0.;
};

}; //namespace NMstand
