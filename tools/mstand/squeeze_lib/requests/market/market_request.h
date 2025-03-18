#pragma once

#include <tools/mstand/squeeze_lib/requests/common/base_request.h>

#include <quality/user_metrics/vertical_metrics/video_metrics.h>

#include <quality/ab_testing/stat_collector_lib/common/logs/baobab.h>
#include <quality/user_sessions/request_aggregate_lib/requests_container.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>
#include <library/cpp/scheme/scheme.h>


namespace NMstand {

using TMarketEventsDict = THashMap<TString, THashMap<TString, THashMap<TString, int>>>;

class TMarketRequest;

class TMarketClick : public TBaseClick {
public:
    explicit TMarketClick(
        const NRA::TClick* click,
        TAtomicSharedPtr<const NBaobab::NTamus::TMarkersContainer> tamusMarkers = nullptr,
        const TString& blockID = ""
    );

    bool operator<(const TMarketClick& MarketClick) const;
    bool operator==(const TMarketClick& MarketClick) const;
    bool operator!=(const TMarketClick& MarketClick) const;

public:
    bool IsMiscClick = false;
    ui64 FraudBits = 0;
    TString ClickId;
    NYT::TNode BaobabAttrs = NYT::TNode::CreateMap();

    TString BlockId;
    THashSet<TString> BaobabBlockMarkers;
};

class TMarketBlock : public TBaseBlock {
public:
    explicit TMarketBlock() = default;

    /*
    TMarketBlock(TMarketBlock&& other)
    :   Pos(other.Pos)
    ,   Clicked(other.Clicked)
    ,   OvlClicked(other.OvlClicked)
    ,   CpcClick(other.CpcClick)
    ,   ClicksCnt(other.ClicksCnt)
    ,   CpaOrder(other.CpaOrder)
    ,   HasCpaClick(other.HasCpaClick)
    ,   HasCpaClickWithoutProperties(other.HasCpaClickWithoutProperties)
    ,   IsCpa(other.IsCpa)
    ,   DwellTime(other.DwellTime)
    ,   Clicks(std::move(other.Clicks))
    ,   AllClicks(std::move(other.AllClicks))
    ,   IsValid_(other.IsValid_) {
    }
    */

    static TMarketBlock Build(
        const TObj<NRA::TBlock>& block,
        const TMarketRequest& request,
        const TMarketEventsDict& marketEventsDict);

    void Init(
        const TObj<NRA::TBlock>& block,
        const TMarketRequest& request,
        const TMarketEventsDict& marketEventsDict
    );

    void Dump(NYT::TNode& node) const;

    bool IsValid() const;

public:
    int64_t Pos;
    int64_t Inclid;
    TString BlockType;
    bool Clicked;
    bool OvlClicked;

    int64_t CpcClick;
    int64_t ClicksCnt;

    int64_t CpaOrder;
    int64_t HasCpaClick;
    int64_t HasCpaClickWithoutProperties;
    int64_t HasCpaSearchClick;
    int64_t HasCpaModelClick;

    int64_t IsCpa;
    int64_t DwellTime;

    TString ShowUID;

    TVector<TMarketClick> Clicks;
    TVector<TMarketClick> AllClicks;

    NSc::TValue Features;

private:
    bool IsValid_;

    THashMap <TString, int> ScarabFeatures_;

private:
    void Validate(const TObj<NRA::TBlock>& block);
};

bool GetIsAcceptableRequest(const NRA::TRequest& request, const TRequestsSettings& settings);

class TMarketRequest : public TBaseRequest {
public:
    static TMarketRequest Build(
        const NRA::TRequest *request,
        const TRequestsSettings& settings,
        const TMarketEventsDict& marketEventsDict
    );

    explicit TMarketRequest(
        const NRA::TRequest *request,
        const TRequestsSettings& settings
    );

    void Init(const NRA::TRequest& request, const TMarketEventsDict& marketEventsDict);

    void InitMainBlocks(const NRA::TRequest& request, const TMarketEventsDict& marketEventsDict);
    const TVector<TMarketBlock>& GetMainBlocks() const;

    void Dump(NYT::TNode& node) const;

    void InitScarabFeatures(const TMarketEventsDict& dict);

    void InitUi(const NRA::TRequest& request);

private:
    TString Ui;
    TString Viewtype;

    TVector<TMarketBlock> MainBlocks_;

    THashMap<TString, int> ScarabFeatures_;

public:
    TVector<TMarketBlock> ParallelBlocks;
    TVector<TMarketBlock> BSBlocks;
    TVector<TMarketClick> AllClicks;
    THashSet<std::pair<TString, time_t>> ClickIDs;
};

class TMarketRequestsContainer : public TBaseRequestsContainer {
public:

    static void TryAddEvent(TMarketEventsDict& marketEventsDict, const TString& value);

    explicit TMarketRequestsContainer(
        const NRA::TRequestsContainer& requestsContainer,
        const TRequestsSettings& settings,
        const TMarketEventsDict& marketDict
    );

    const TVector<TMarketRequest>& GetRequestsContainer() const;
    const NUserMetrics::TRequestMetricsMap& GetSessionVideoWizardGeneratedRequestProperties() const;

    const TVector<NYT::TNode>& GetMarketSqueeze();

private:
    void SetSessionVideoWizardGeneratedRequestProperties(const NRA::TRequests& requests);
    void AddVideoEtherView(const TString& wizardName, const TVector<time_t>& timestamps);
    void SetSessionEtherWizardGeneratedRequestProperties(const NRA::TRequestsContainer& requestsContainer);

private:
    TVector<TMarketRequest> RequestsContainer;

    THolder<TMarketEventsDict> MarketEventsDict;

    // Custom attributes
    NUserMetrics::TRequestMetricsMap SessionVideoWizardGeneratedRequestProperties;

    TVector<NYT::TNode> MarketSqueeze;

    THashMap<TString, THashMap<TString, THashMap<TString, int>>> ReqToShowuidToEvent;
};

bool IsText(const TString& url);

} // NMstand
