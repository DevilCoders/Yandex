#include "market_request.h"

#include <quality/user_sessions/request_aggregate_lib/market.h>

#include <scarab/api/cpp/all.h>


namespace NMstand {

const ui32 OVL_TIME = 120;

TMarketClick::TMarketClick(
        const NRA::TClick* click,
        TAtomicSharedPtr<const NBaobab::NTamus::TMarkersContainer> /* tamusMarkers */,
        const TString& blockID
)   : TBaseClick(click)
        , ClickId(click->GetBaobabClick().Get() != nullptr ? click->GetBaobabClick()->GetEventID().GetOrElse("") : "")
        , BlockId(blockID)
{
}


TMarketRequest::TMarketRequest(
        const NRA::TRequest *request,
        const TRequestsSettings& settings
)   : TBaseRequest(request, settings.RequestsContainerType)
{
}

void TMarketRequest::InitScarabFeatures(const TMarketEventsDict& dict) {
    ScarabFeatures_.insert({"req_cpc_clicks", 0});
    ScarabFeatures_.insert({"req_cpc_orders", 0});
    ScarabFeatures_.insert({"req_cpa_clicks", 0});
    ScarabFeatures_.insert({"req_cpa_orders", 0});

    if (dict.contains(ReqID)) {
        for (auto& [eventType, _] : dict.at(ReqID).at("req_total")) {
            ScarabFeatures_["req_" + eventType] = 1;
        }
    }
}

void TMarketRequest::InitUi(const NRA::TRequest& request) {
    if (dynamic_cast<const NRA::TYandexMarketSearchxmlRequest *>(&request) ||
        dynamic_cast<const NRA::TYandexBlueMarketSearchxmlRequest *>(&request)) {
        Ui = "desktop";
    } else if (dynamic_cast<const NRA::TTouchYandexMarketSearchxmlRequest *>(&request) ||
        dynamic_cast<const NRA::TTouchYandexBlueMarketSearchxmlRequest *>(&request)) {
        Ui = "touch";
    } else if (dynamic_cast<const NRA::TBlueMarketAppSearchxmlRequest *>(&request)) {
        Ui = "app";
    } else {
        Ui = "unrecognized";
    }
}

void TMarketRequest::Init(const NRA::TRequest& request, const TMarketEventsDict& marketDict) {
    InitMainBlocks(request, marketDict);
    InitScarabFeatures(marketDict);
    InitUi(request);
    if (const auto* marketRequestProperties = dynamic_cast<const NRA::TMarketRequestProperties*>(&request)) {
        if (Ui == "app") {
            Url = marketRequestProperties->GetReportRequest();
        } else {
            Url = marketRequestProperties->GetUrl();
        }
        Viewtype = marketRequestProperties->GetViewtype().GetOrElse("");

    }
}

bool GetIsAcceptableRequest(const NRA::TRequest& request) {
    if (!dynamic_cast<const NRA::TPageProperties*>(&request)) {
        return false;
    }
    if (const auto marketReqProps = dynamic_cast<const NRA::TMarketSearchxmlProperties*>(&request)) {
        return marketReqProps;
    }
    const auto marketBlueReqProps = dynamic_cast<const NRA::TBlueMarketSearchxmlProperties*>(&request);
    return marketBlueReqProps;
}

TMarketRequest TMarketRequest::Build(
        const NRA::TRequest *request,
        const TRequestsSettings& settings,
        const TMarketEventsDict& marketDict
) {
    TMarketRequest marketRequest(request, settings);
    marketRequest.Init(*request, marketDict);
    marketRequest.IsAcceptableRequest = GetIsAcceptableRequest(*request);
    return marketRequest;
}

TMarketRequestsContainer::TMarketRequestsContainer(
        const NRA::TRequestsContainer& requestsContainer,
        const TRequestsSettings& settings,
        const TMarketEventsDict& marketDict
)   :   TBaseRequestsContainer(settings)
{
    RequestsContainer.reserve(requestsContainer.GetRequests().size());
    for (const auto& request : requestsContainer.GetRequests()) {
        RequestsContainer.emplace_back(TMarketRequest::Build(request, settings, marketDict));
    }
}

const TVector<NYT::TNode>& TMarketRequestsContainer::GetMarketSqueeze() {
    if (!MarketSqueeze.empty()) {
        return MarketSqueeze;
    }

    size_t actionIndex = 0;
    for (auto& marketRequest : RequestsContainer) {
        if (!marketRequest.IsAcceptableRequest) {
            continue;
        }

        NYT::TNode mstandRequest;
        mstandRequest["type"] = "MARKET_SEARCH_REQUEST";
        mstandRequest["action_index"] = actionIndex++;
        marketRequest.Dump(mstandRequest);
        MarketSqueeze.push_back(mstandRequest);

        for (auto& block : marketRequest.GetMainBlocks()) {
            NYT::TNode mstandBlock;
            if (block.IsValid()){
                block.Dump(mstandBlock);
                marketRequest.Dump(mstandBlock);

                mstandBlock["action_index"] = actionIndex++;
                mstandBlock["type"] = "MARKET_SEARCH_SHOW";

                MarketSqueeze.push_back(mstandBlock);
            }
        }
    }

    return MarketSqueeze;
}

inline bool Contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

bool IsText(const TString& url) {
    std::string normal_string = ToString(url);
    bool is_sorted = Contains(normal_string, "how=");
    bool is_first_page = !Contains(normal_string, "page=");
    bool is_not_redirect_and_not_wizard =
            Contains(normal_string, "cvredirect=0") && !Contains(normal_string, "clid=");
    bool is_filtered_without_redirect =
            Contains(normal_string, "glfilter=") && !Contains(normal_string, "&rt=");
    bool has_text = Contains(normal_string, "?text=") || Contains(normal_string, "&text=");

    return !is_sorted &&
           is_first_page &&
           !is_not_redirect_and_not_wizard &&
           !is_filtered_without_redirect &&
           has_text;
}

TMarketBlock TMarketBlock::Build(const TObj<NRA::TBlock>& block,
    const TMarketRequest& request, const TMarketEventsDict& marketDict) {
    TMarketBlock marketBlock;
    marketBlock.Init(block, request, marketDict);
    return marketBlock;
}

void TMarketBlock::Validate(const TObj<NRA::TBlock>& block) {
    IsValid_ = false;
    if (!block) {
        return;
    }

    IsValid_ = true;
}

void TMarketBlock::Init(const TObj<NRA::TBlock>& block, const TMarketRequest& request,
    const TMarketEventsDict& marketDict) {
    Validate(block);
    if (!IsValid_) {
        return;
    }

    IsCpa = true;

    if (auto marketSearchxmlResult =
        dynamic_cast<const NRA::TMarketSearchxmlResult*>(block->GetMainResult())) {
        ShowUID = marketSearchxmlResult->GetShowUID();
        Pos = marketSearchxmlResult->GetPosition();
        Inclid = marketSearchxmlResult->GetInclid().GetOrElse(0);
        BlockType = TString("org");
    }
    if (dynamic_cast<const NRA::TMarketIncutSearchxmlResult*>(block->GetMainResult())) {
        BlockType = TString("incut");
    }
    if (dynamic_cast<const NRA::TMarketAdsResult*>(block->GetMainResult())) {
        BlockType = TString("ads");
    }

    DwellTime = 0;


    for (auto& child : block->GetChildren()) {
        auto marketChild = dynamic_cast<const NRA::TMarketResultProperties *>(child.Get());
        if (marketDict.contains(request.ReqID) &&
            marketDict.at(request.ReqID).contains(marketChild->GetShowUID())) {
            for (auto& [eventType, _] : marketDict.at(request.ReqID).at(marketChild->GetShowUID())) {
                ScarabFeatures_["block_" + eventType] = 1;
            }
        }

        for (auto& result : child->GetClicks()) {
            DwellTime = std::max(DwellTime, result->GetDwellTimeV2OnService());
        }
    }

    Clicked = DwellTime > 0;
    OvlClicked = DwellTime > OVL_TIME;

    ClicksCnt = block->GetClicks().size();
    CpcClick = std::count_if(block->GetClicks().begin(), block->GetClicks().end(), [](auto& click)
        { return click && click->GetUrl() && !Contains(click->GetUrl().GetRef(), "market.yandex."); });

    CpaOrder = 0;
    HasCpaClick = 0;
    HasCpaClickWithoutProperties = 0;
    HasCpaSearchClick = 0;
    HasCpaModelClick = 0;
    auto marketLinearBlock = dynamic_cast<const NRA::TMarketLinearBlock *>(block.Get());
    if (marketLinearBlock) {
        for (auto& cpaSearch : marketLinearBlock->GetCPAClicksSearch()) {
            HasCpaClickWithoutProperties = 1;
            HasCpaSearchClick = 1;
            auto cpaProps = cpaSearch->GetProperties<NRA::TMarketCPAClickProperties>();
            if (cpaProps) {
                HasCpaClick = 1;
                if (cpaProps->GetOrders() > 0) {
                    CpaOrder = 1;
                    break;
                }
            }
        }

        auto& modelClicks = marketLinearBlock->GetCPAClicksModel();
        for (auto& cpaModel : modelClicks) {
            HasCpaClickWithoutProperties = 1;
            HasCpaModelClick = 1;
            auto cpaProps = cpaModel->GetProperties<NRA::TMarketCPAClickProperties>();
            if (cpaProps) {
                HasCpaClick = 1;
                if (cpaProps->GetOrders() > 0) {
                    CpaOrder = 1;
                    break;
                }
            }
        }
    }
}

void TMarketBlock::Dump(NYT::TNode& node) const {
    node["show_uid"] = ShowUID;
    node["pos"] = Pos;
    node["inclid"] = Inclid;
    node["block_type"] = BlockType;
    node["clicked"] = Clicked;
    node["ovl_clicked"] = OvlClicked;
    node["cpa_order"] = CpaOrder;
    node["cpc_click"] = CpcClick;
    node["has_cpa_click"] = HasCpaClick;
    node["has_cpa_click_without_properties"] = HasCpaClickWithoutProperties;
    node["has_cpa_search_click"] = HasCpaSearchClick;
    node["has_cpa_model_click"] = HasCpaModelClick;
    node["is_cpa"] = IsCpa;
    node["dwelltime"] = DwellTime;
    node["clicks"] = ClicksCnt;

    if (ScarabFeatures_.contains("block_cpc_clicks")) {
        node["block_cpc_clicks"] = ScarabFeatures_.at("block_cpc_clicks");
    }
    if (ScarabFeatures_.contains("block_cpc_orders")) {
        node["block_cpc_orders"] = ScarabFeatures_.at("block_cpc_orders");
    }
    if (ScarabFeatures_.contains("block_cpa_clicks")) {
        node["block_cpa_clicks"] = ScarabFeatures_.at("block_cpa_clicks");
    }
    if (ScarabFeatures_.contains("block_cpa_orders")) {
        node["block_cpa_orders"] = ScarabFeatures_.at("block_cpa_orders");
    }
}

bool TMarketBlock::IsValid() const {
    return IsValid_;
}

void TMarketRequest::InitMainBlocks(const NRA::TRequest& request,
    const TMarketEventsDict& marketDict) {
    MainBlocks_.reserve(request.GetMainBlocks().size());
    for (const auto& block : request.GetMainBlocks()) {
        MainBlocks_.push_back(TMarketBlock::Build(block, *this, marketDict));
    }
}

const TVector<TMarketBlock>& TMarketRequest::GetMainBlocks() const {
    return MainBlocks_;
}

template <typename IEvent>
inline void AddEventToDict(
    TMarketEventsDict& marketEventsDict,
    const IEvent& event,
    const TString& field = "clicks",
    int value = 1)
{
    const TString& reqId = event.GetRequestId()->GetValue();
    const TString& showUid = event.GetShowUid();
    const bool isFine = (
        event.GetScarabType() == "MARKET_CPA_CLICK_EVENT" &&
        event.GetScarabVersion() >= 3 &&
        event.GetFraudDiagnosis() == "0"
    ) || (
        event.GetScarabType() == "MARKET_CLICK_EVENT" &&
        event.GetFraudDiagnosis() == "0"
    );

    if (!reqId.empty() && !showUid.empty() && isFine) {
        marketEventsDict[reqId][showUid][field] += value;
        marketEventsDict[reqId]["req_total"][field] += value;
    }
}

void TMarketRequestsContainer::TryAddEvent(TMarketEventsDict& marketEventsDict, const TString& value) {
    TStringInput si(value);
    try {
        THolder<NScarab::NCommon::IEvent> event = NScarab::Deserialize(si);
        if (!event) {
            return;
        }

        if (event->GetScarabType() == "MARKET_CLICK_EVENT") {
            if (auto marketEvent = dynamic_cast<NScarab::NMarket::IClickEvent *>(event.Get())) {
                AddEventToDict(marketEventsDict, *marketEvent, "cpc_clicks");
            }
        } else if (event->GetScarabType() == "MARKET_CPA_CLICK_EVENT") {
            if (auto marketEvent = dynamic_cast<NScarab::NMarket::ICpaClickEvent3 *>(event.Get())) {
                AddEventToDict(marketEventsDict, *marketEvent, "cpa_clicks");
            }
        }

        // TODO: MARKET_CLICK_ORDER_PLACED_EVENT -- NScarab::NMarket::IClickOrderPlacedEvent
        // TODO: MARKET_ORDER_EVENT -- NScarab::NMarket::IOrderEvent
    } catch(...) {
        // do nothing;
    }
}

void TMarketRequest::Dump(NYT::TNode& node) const {
    node["yuid"] = UID;
    node["ts"] = Timestamp;
    node["browser"] = Browser;
    // node["testid"];

    // TODO: node["domregion"];
    node["reqid"] = ReqID;
    node["page"] = PageNo;
    node["ui"] = Ui;
    //node["url"] = FullQuery;
    node["url"] = Url;
    node["viewtype"] = Viewtype;

    node["servicetype"] = "market-search-sessions";
    node["is_match"] = true;
    node["is_text"] = IsText(Url);

    if (ScarabFeatures_.contains("req_cpc_clicks")) {
        node["req_cpc_clicks"] = ScarabFeatures_.at("req_cpc_clicks");
    }
    if (ScarabFeatures_.contains("req_cpc_orders")) {
        node["req_cpc_orders"] = ScarabFeatures_.at("req_cpc_orders");
    }
    if (ScarabFeatures_.contains("req_cpa_clicks")) {
        node["req_cpa_clicks"] = ScarabFeatures_.at("req_cpa_clicks");
    }
    if (ScarabFeatures_.contains("req_cpa_orders")) {
        node["req_cpa_orders"] = ScarabFeatures_.at("req_cpa_orders");
    }
}

} // namespace NMstand
