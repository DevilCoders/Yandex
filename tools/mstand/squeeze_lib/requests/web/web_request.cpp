#include "web_request.h"

#include <tools/mstand/squeeze_lib/requests/common/field_extractor.h>
#include <quality/logs/parse_lib/parse_lib.h>

#include <quality/user_metrics/common/blocks.h>  // NUserMetrics::GetSourceName, NUserMetrics::GetSnippetName, NUserMetrics::GetBlendedWebType
#include <quality/user_metrics/common/bno_surplus.h>  // NUserMetrics::
#include <quality/user_metrics/common/clicks.h>  // NUserMetrics::GetAllClicks

#include <quality/ab_testing/cost_join_lib/joiner.h>
#include <quality/ab_testing/lib_calc_session_metrics/common/words.h>
#include <quality/ab_testing/stat_collector_lib/common/logs/baobab.h>
#include <quality/ab_testing/stat_collector_lib/common/logs/related_requests.h>
#include <quality/ab_testing/stat_collector_lib/common/url/yandex_service.h>

#include <quality/functionality/turbo/urls_lib/cpp/lib/turbo_urls.h>

#include <quality/logs/baobab/api/cpp/common/block_attributes.h>

#include <quality/webfresh/learn/parser_lib/freshness.h>

#include <search/web/blender/util/valid_intent.h>
#include <search/web/util/sources/sources.h>

#include <util/string/cast.h>
#include <util/string/split.h>

namespace NMstand {

void ParseAttrs(const NBaobab::TAttr& attrs, NYT::TNode& parsedAttrs) {
    switch (attrs.GetType()) {
        case NBaobab::EAttrType::Null:
            parsedAttrs = NYT::TNode::CreateEntity();
            break;
        case NBaobab::EAttrType::Bool:
            parsedAttrs = attrs.Get<bool>();
            break;
        case NBaobab::EAttrType::UnsignedInt:
            parsedAttrs = attrs.Get<ui64>();
            break;
        case NBaobab::EAttrType::Int:
            parsedAttrs = attrs.Get<i64>();
            break;
        case NBaobab::EAttrType::Double:
            parsedAttrs = attrs.Get<double>();
            break;
        case NBaobab::EAttrType::String:
            parsedAttrs = attrs.Get<TString>();
            break;
        case NBaobab::EAttrType::Dict:
        {
            parsedAttrs = NYT::TNode::CreateMap();
            for (const auto& kv : attrs.Get<NBaobab::TAttrsMap>()) {
                ParseAttrs(kv.second, parsedAttrs[kv.first]);
            }
            break;
        }
        case NBaobab::EAttrType::Array:
        {
            const auto& attrsArray = attrs.Get<NBaobab::TAttrsArray>();
            parsedAttrs = NYT::TNode::CreateList();
            parsedAttrs.AsList().resize(attrsArray.size());
            for (size_t index = 0; index < attrsArray.size(); ++index) {
                ParseAttrs(attrsArray[index], parsedAttrs[index]);
            }
            break;
        }
    }
}

TWebClick::TWebClick(const NRA::TClick* click, const TString& blockID)
    : TBaseClick(click)
    , BlockId(blockID)
    , ClickId(click->GetBaobabClick().Get() != nullptr ? click->GetBaobabClick()->GetEventID().GetOrElse("") : "")
{
    if (dynamic_cast<const NRA::TMiscResultClick*>(click)) {
        IsMiscClick = true;
    }

    if (click->GetBaobabScroll()) {
        IsScroll = true;
    }

    if (const auto* directClickProperties = click->GetProperties<NRA::TDirectClickProperties>()) {
        FraudBits = directClickProperties->GetFraudBits();
    }

    SetBaobabParameters(click);

    if (click->GetUrl().Defined() && !NTurbo::TryDecodeTurboUrl(click->GetUrl().GetRef(), OriginalUrl)) {
        OriginalUrl = click->GetUrl().GetRef();
    }
}

bool TWebClick::operator<(const TWebClick& webClick) const {
    return Timestamp < webClick.Timestamp;
}

bool TWebClick::operator==(const TWebClick& webClick) const {
    return !(*this < webClick) && !(webClick < *this);
}

bool TWebClick::operator!=(const TWebClick& webClick) const {
    return !(*this == webClick);
}

TString GetBaobabPath(const NBaobab::TBlock& block) {
    NBaobab::TResultInterfaceAdapter::TMyFilter resultFilter;

    TVector<TString> blocksNames;
    for (const auto& currentBlock : NBaobab::TAncestors(block)) {
        blocksNames.push_back(currentBlock.GetName());

        if (resultFilter.Accept(currentBlock)) {
            break;
        }
    }

    TStringStream path;
    for (auto rit = blocksNames.rbegin(); rit != blocksNames.rend(); ++rit) {
        if (!path.Empty()) {
            path << "/";
        }
        path << *rit;
    }
    return path.Str();
}

void TWebClick::SetBaobabParameters(const NRA::TClick* click) {
    if (click->GetBaobabBlock().Defined()) {
        auto& block = click->GetBaobabBlock().GetRef();
        if (BlockId.Empty()) {
            TMaybe<NBaobab::TBlock> resultBlock = NBaobab::GetAncestorResultBlock(block);
            if (resultBlock.Defined()) {
                BlockId = resultBlock->GetID();
            } else {
                BlockId = block.GetID();
            }
        }
        BaobabPath = GetBaobabPath(block);
    }

    BaobabAttrs["block_id"] = BlockId;
    BaobabAttrs["path"] = ""; // TODO: real BaobabPath value breaks tests
}

void TWebClick::SetBlock(const std::shared_ptr<TWebBlock>& block) {
    Block = block;
}

const NYT::TNode TWebClick::GetClickData(const NYT::TNode& baseData) const {
    NYT::TNode result = baseData;
    result["ts"] = Timestamp;
    result["dwelltime_on_service"] = DwellTimeOnService;
    if (FraudBits.Defined()) {
        result["fraud_bits"] = FraudBits.GetRef();
    }
    result["path"] = Path;
    result["type"] = IsDynamic ? "dynamic-click" : "click";
    result["url"] = Url;
    result["is_dynamic_click"] = IsDynamic;
    result["is_misc_click"] = IsMiscClick;
    result["is_scroll"] = IsScroll;
    result["original_url"] = OriginalUrl;

    if (!BaobabAttrs.Empty()) {
        result["baobab_attrs"] = BaobabAttrs;
    }
    if (!BaobabPath.Empty()) {
        result["baobab_path"] = BaobabPath;
    }
    if (!BlockId.Empty()) {
        result["block_id"] = BlockId;
    }
    if (!ClickId.Empty()) {
        result["click_id"] = ClickId;
    }

    if (std::shared_ptr<TWebBlock> blockPtr = Block.lock()) {
        result["pos"] = blockPtr->Position;
        result["restype"] = blockPtr->ResultType;
        result["placement"] = blockPtr->GetPlacement();
        result["wizard_name"] = blockPtr->BaobabWizardName;
    }
    return result;
}

TWebBlock::TWebBlock(
    const NRA::TBlock* block,
    TAtomicSharedPtr<const NBaobab::NTamus::TMarkersContainer> tamusMarkers,
    TMaybe<const NBaobab::TBlock> baobabBlock,
    double otherScore,
    bool isParallel
)
    : TBaseBlock(block)
    , BlockId(baobabBlock ? baobabBlock->GetID() : "")
    , OtherScore(otherScore)
{
    if (baobabBlock.Defined()) {
        BaobabBlockMarkers = tamusMarkers->GetMarkersWithAncestors(BlockId);
        for (const auto& childBlock : NBaobab::TChildren(*baobabBlock)) {
            const auto& childMarkers = tamusMarkers->GetMarkers(childBlock.GetID());
            BaobabBlockChildrenMarkers.insert(childMarkers.begin(), childMarkers.end());
        }
        ParseBaobabAttrs(baobabBlock.GetRef());
    }

    if (isParallel) {
        Placement = EPlacement::P_PARALLEL;
    }
    else if (BaobabBlockMarkers.contains("top") || MediaTurboBna) {
        Placement = EPlacement::P_UPPER;
    }

    const NRA::TResult* blockMainResult = block->GetMainResult();

    // Properties
    if (const auto* wizardProperties = dynamic_cast<const NRA::TWizardResultProperties*>(blockMainResult)) {
        HasWizardResultProperties = true;
        WizardName = wizardProperties->GetName();
        BlenderWizardName = wizardProperties->GetName();
    }
    if (const auto* organicProperties = dynamic_cast<const NRA::TOrganicResultProperties*>(blockMainResult)) {
        HasOrganicResultProperties = true;
        OrganicResultPropertiesMarkers = organicProperties->GetMarkers();
        WebUrl = organicProperties->GetUrl();
        YandexService = NStatCollector::GetYandexServiceNameByUrl(WebUrl);
        if (YandexService == "turbo") {
            YandexTurboService = NStatCollector::GetYandexServiceNameByTurboUrl(WebUrl);
        }
        if (YandexService == "thequestion.ru" || YandexService == "q") {
            YandexService = "thequestion";
        }
        if (YandexService == "eda.yandex") {
            YandexService = "eda";
        }
        BlenderWizardUrl = organicProperties->GetUrl();
        BaseType = organicProperties->GetBaseType();
    }
    if (const auto* videoProperties = dynamic_cast<const NRA::TVideoWizardResultProperties*>(blockMainResult)) {
        HasVideoWizardResultProperties = true;
    }

    // Result
    if (const auto* webResult = dynamic_cast<const NRA::TWebResult*>(blockMainResult)) {
        IsWebResult = true;
        WebShard = webResult->GetShard();
        WebSource = webResult->GetSource();
        WebRegions = webResult->GetRegions();
    }
    if (const auto* wizardResult = dynamic_cast<const NRA::TWizardResult*>(blockMainResult)) {
        IsWizardResult = true;
    }
    if (const auto* blenderWizard = dynamic_cast<const NRA::TBlenderWizardResult*>(blockMainResult)) {
        IsBlenderWizardResult = true;
        if (const auto* tvProperties = dynamic_cast<const NRA::TTVOnlineStreamResultProperties*>(blockMainResult)) {
            IsVideoPlayer = true;
            if (WizardName == "entity_search") {
                IsEntitySearchPlayer = true;
            }
            for (const auto& event : tvProperties->GetPlayerEvents()) {
                if (event->GetPath() == "player-events.heartbeat") {
                    HeartbeatCount += 1;
                }
            }
        }
    }
    if (const auto* directResult = dynamic_cast<const NRA::TDirectResult*>(blockMainResult)) {
        IsDirectResult = true;
        FraudBits = directResult->GetFraudBits();
        BannerID = directResult->GetBannerID().GetOrElse(0);
        WebUrl = directResult->GetUrl().GetOrElse("");
        IsUrlMarketplace = NStatCollector::GetCanonDom(WebUrl).StartsWith("pokupki.market.yandex");
    }
    if (const auto* recommendationResult = dynamic_cast<const NRA::TRecommendationResult*>(blockMainResult)) {
        IsRecommendationResult = true;
    }

    SetIsSignificantBlock();
    SetHostType();
    SetIsFactResult(tamusMarkers->HasBlocks("entity_search_wizard"));
    SetBlockAttractiveness();

    const NRA::TClicks& allClicks = NUserMetrics::GetAllClicks(block);
    for (const auto& click : allClicks) {
        if (!click->ShouldBeUsedInDwellTimeOnService()) {
            continue;
        }

        auto webClick = std::make_shared<TWebClick>(click, BlockId);
        Clicks.push_back(webClick);
    }

    SetBlenderViewType();
    SetMediaTurboBna();
    SetBlenderIntents();
    SetPlugins();
    SetIsVideoPlayer();
    SetResultType(isParallel);

    if (const auto* webResult = dynamic_cast<const NRA::TWebResult*>(blockMainResult)) {
        SourceName = NUserMetrics::GetSourceName(webResult);
        FreshSource = IsFreshSource(SourceName);
        SnippetName = NUserMetrics::GetSnippetName(webResult);
        BlendedWebType = NUserMetrics::GetBlendedWebType(webResult);
        DocumentAge = NWebFresh::GetDocumentAge(webResult);
        IsResultFromMercury = NWebFresh::IsResultFromMercury(webResult);
        IsResultFromCallisto = NWebFresh::IsResultFromCallisto(webResult);
        IsResultFromJupiter = NWebFresh::IsResultFromJupiter(webResult);
        IsResultFromQuick = NWebFresh::IsResultFromQuick(webResult);
    }
}

void TWebBlock::SetIsSignificantBlock() {
    if (HasWizardResultProperties) {
        // todo: remove this crutch
        if (IsBlenderWizardResult) {
            if (BlenderWizardName.Empty() && BlenderWizardUrl.find("maps.yandex.ru") != TString::npos) {
                IsSignificantBlock = false;
                return;
            }
        }
        IsSignificantBlock = !IsIn(NMstand::TechWizards, WizardName);
        return;
    }

    if (IsDirectResult && FraudBits > 0) {
        IsSignificantBlock = false;
        return;
    }
    IsSignificantBlock = true;
}

const THashSet<TString> UNION_FACTS_WIZNAMES = {
    "calories_fact",
    "currencies",
    "distance_fact",
    "graph",
    "entity-fact",
    "entity_fact",
    "suggest_fact",
    "table_fact",
    "wikipedia_fact",
    "rich_fact",
    "fact_instruction",
    "multiple_facts",
    "dict_fact",
    "time",
    "colors",
    "image_fact",
    "poetry_lover"
};

void TWebBlock::SetHostType() {
    if (HasOrganicResultProperties) {
        if (const TString *marker = OrganicResultPropertiesMarkers.FindPtr("SP_MedDssmGood")) {
            if (*marker == "1") {
                IsMedicalHost = true;
            }
        }
        if (const TString *marker = OrganicResultPropertiesMarkers.FindPtr("SP_FinDssmGood")) {
            if (*marker == "1") {
                IsFinancialHost = true;
            }
        }
        if (const TString *marker = OrganicResultPropertiesMarkers.FindPtr("SP_LegDssmGood")) {
            if (*marker == "1") {
                IsLegalHost = true;
            }
        }
        if (const TString *marker = OrganicResultPropertiesMarkers.FindPtr("RelevPrediction")) {
            TryFromString(*marker, RelevPrediction);
        }
        if (const TString *marker = OrganicResultPropertiesMarkers.FindPtr("ProximaPredict")) {
            float value;
            if (TryFromString(*marker, value)){
                ProximaPredict = value;
            }
        }
    }
}

void TWebBlock::SetIsFactResult(bool hasEntitySearch) {
    if (HasOrganicResultProperties) {
        if (const TString *rule = OrganicResultPropertiesMarkers.FindPtr("Rule")) {
            if (*rule == "Facts") {
                IsFactResult = true;
                IsFactResultWithEntitySearch = hasEntitySearch;
                return;
            }
        }
        if (OrganicResultPropertiesMarkers.FindPtr("FactType")) {
            IsFactResult = true;
            IsFactResultWithEntitySearch = hasEntitySearch;
            return;
        }
    }
    if (IsWizardResult) {
        if (UNION_FACTS_WIZNAMES.contains(WizardName)) {
            IsFactResult = true;
            IsFactResultWithEntitySearch = hasEntitySearch;
            return;
        }
    }
    if (IsBlenderWizardResult) {
        if (UNION_FACTS_WIZNAMES.contains(BlenderWizardName)) {
            IsFactResult = true;
            IsFactResultWithEntitySearch = hasEntitySearch;
            return;
        }
    }
    IsFactResult = false;
    IsFactResultWithEntitySearch = false;
}

void TWebBlock::SetBlockAttractiveness() {
    if (HasWizardResultProperties) {
        if (WizardName == "panoramas"
            || WizardName == "maps"
            || WizardName == "adresa"
            || WizardName == "companies")
        {
            Attractiveness = 0.5f;
        }
        if (WizardName == "news") {
            Attractiveness = 0.41f;
        }
        if (WizardName == "weather") {
            Attractiveness = 0.34f;
        }
    }
}

void TWebBlock::SetBlenderViewType() {
    if (HasOrganicResultProperties) {
        if (const auto viewType = OrganicResultPropertiesMarkers.FindPtr("blndrViewType")) {
            BlenderViewType = *viewType;
        }
    }
}

void TWebBlock::SetMediaTurboBna() {
    if (HasOrganicResultProperties) {
        if (const auto uniAnswerComb = OrganicResultPropertiesMarkers.FindPtr("UniAnswerComb")) {
            MediaTurboBna = (*uniAnswerComb == "media_turbo_bna");
        }
    }
}

void TWebBlock::SetIsVideoPlayer() {
    if (HasOrganicResultProperties) {
        if (OrganicResultPropertiesMarkers.contains("xl_video_player")) {
            IsVideoPlayer = true;
            if (WizardName == "entity_search") {
                IsEntitySearchPlayer = true;
            }
        }
    }
}

void TWebBlock::SetBlenderIntents() {
    if (const auto* strPtr = OrganicResultPropertiesMarkers.FindPtr("Slices")) {
        TStringBuf buf(*strPtr);
        while (!buf.empty()) {
            const auto intent = ToString(buf.NextTok(':'));
            if (!intent.empty() && NBlender::IsValidIntentForMetric(intent)) {
                BlenderIntents[intent] = NBlender::IsValidIntentForMetric(intent);
                if (intent == "WEB_Q") {
                    CorrectedWizardName = "thequestion";
                }
                if (intent == "INTENT_FOR_ASAP_WARNING" || intent == "INTENT_FOR_ASAP_WARNING_FAKE") {
                    IsFactResult = false;
                }
            }
            buf.NextTok('|');
        }
    }
}

void TWebBlock::SetPlugins() {
    for (TStringBuf marker : { "Plugins", "FilteredP", "DisabledP" }) {
        if (const auto* plugins = OrganicResultPropertiesMarkers.FindPtr(marker)) {
            TStringBuf buf(*plugins);
            while (!buf.empty()) {
                const auto plugin = ToString(buf.NextTok('|'));
                BlenderPlugins.insert(plugin);
                if (marker == "Plugins" && plugin == "bno") {
                    IsBna = true;
                }
            }
        }
    }
}

void TWebBlock::ParseBaobabAttrs(const NBaobab::TBlock& baobabBlock) {
    ParseAttrs(baobabBlock.GetAttrsRoot(), BaobabAttrs);
    BaobabAttrs["block_id"] = baobabBlock.GetID();
    BaobabAttrs["parent_name"] = baobabBlock.GetParent().Defined() ? baobabBlock.GetParent()->GetName() : "";
}

const TString TWebBlock::GetPlacement() const {
    return Placement == EPlacement::P_PARALLEL ? "parallel" : "main";
}

void TWebBlock::SetMouseTrackParameters(const NMouseTrack::TBlockData* blockData) {
    if (!blockData) {
        return;
    }
    if (blockData->HasTop()) {
        double value = blockData->GetTop();
        if (value > 0 && value < 20000) {
            HasMouseTrackTop = true;
            MouseTrackTop =  (ui32)value;
        }
    }
    if (blockData->HasHeight()) {
        double value = blockData->GetHeight();
        if (value > 0 && value < 20000) {
            HasMouseTrackHeight = true;
            MouseTrackHeight = (ui32)value;
        }
    }
}

void TWebBlock::SetResultType(bool isParallel) {
    if (IsDirectResult) {
        if (BaobabBlockMarkers.contains("premium") || BaobabBlockMarkers.contains("sticky")) {
            ResultType = "dir";
        }
        else {
            ResultType = "guarantee";
        }
    }
    else if (HasWizardResultProperties) {
        if (isParallel) {
            ResultType = "wiz_parallel";
        }
        else {
            ResultType = "wiz";
        }
    }
    else if (HasOrganicResultProperties) {
        ResultType = "web";
    }
    else if (IsRecommendationResult) {
        ResultType = "recommendation";
    }
    else {
        ResultType = "other";
    }
}

const THashMap<TString, TString> MARKERS_FIELD_MAP = {
    {"blndrViewType", "blndr_view_type"},
    {"FreshAge", "fresh_age"},
    {"news_wizard_video_thumb", "news_wizard_video_thumb"},
    {"RobotDaterFreshAge", "robot_dater_fresh_age"},
    {"xl_video_player", "xl_video_player"},
    {"MedicalHostQuality", "medical_host_quality"}
};

const THashMap<TString, TString> MARKERS_INT_FIELD_MAP = {
    {"CostPlus", "cost_plus"}
};

const THashMap<TString, TString> MARKERS_PLUGIN_FIELD_MAP = {
    {"DisabledP", "disabled_p"},
    {"FilteredP", "filtered_p"},
    {"Plugins", "plugins"}
};

NYT::TNode GetBlockMarkers(const TWebBlock& webBlock) {
    NYT::TNode markers = NYT::TNode::CreateMap();
    for (const auto& kv : MARKERS_FIELD_MAP) {
        if (webBlock.OrganicResultPropertiesMarkers.contains(kv.first)) {
            markers[kv.second] = webBlock.OrganicResultPropertiesMarkers.at(kv.first);
        }
    }
    for (const auto& kv : MARKERS_INT_FIELD_MAP) {
        if (webBlock.OrganicResultPropertiesMarkers.contains(kv.first)) {
            int value;
            if (TryFromString(webBlock.OrganicResultPropertiesMarkers.at(kv.first), value)) {
                markers[kv.second] = value;
            }
        }
    }
    for (const auto& kv : MARKERS_PLUGIN_FIELD_MAP) {
        if (webBlock.OrganicResultPropertiesMarkers.contains(kv.first)) {
            markers[kv.second] = NYT::TNode::CreateList();
            StringSplitter(
                webBlock.OrganicResultPropertiesMarkers.at(kv.first))
                    .Split('|')
                    .AddTo(&markers[kv.second].AsList());
        }
    }
    return markers;
}

const NYT::TNode TWebBlock::GetBlockData() const {
    NYT::TNode mstandBlock;
    mstandBlock["mstand_id"] = FullBlockId;
    mstandBlock["baobab_attrs"] = BaobabAttrs;
    mstandBlock["height"] = Height;
    mstandBlock["clicks_count"] = Clicks.size();

    if (MouseTrackHeight != 0 || MouseTrackTop != 0){
        mstandBlock["mousetrack"]["height"] = MouseTrackHeight;
        mstandBlock["mousetrack"]["top"] = MouseTrackTop;
    }

    mstandBlock["pos"] = Position;
    mstandBlock["visual_pos"] = Position;
    mstandBlock["url"] = WebUrl;
    mstandBlock["placement"] = GetPlacement();
    mstandBlock["restype"] = ResultType;

    if (IsBna) {
        mstandBlock["is_bno"] = IsBna;
    }
    if (MediaTurboBna) {
        mstandBlock["is_big_bno"] = MediaTurboBna;
    }
    if (IsFactResult) {
        mstandBlock["is_fact"] = IsFactResult;
    }
    if (IsDirectResult) {
        mstandBlock["banner_id"] = BannerID;
        mstandBlock["fraud_bits"] = FraudBits;
    } else if (HasWizardResultProperties) {
        for (const auto& intent : BlenderIntents) {
            if (intent.second) {
                mstandBlock["intent"] = intent.first;
            }
        }
        mstandBlock["markers"] = GetBlockMarkers(*this);
        mstandBlock["name"] = WizardName;
        mstandBlock["path"] = Path;
    } else if (HasOrganicResultProperties) {
        for (const auto& intent : BlenderIntents) {
            if (intent.second) {
                mstandBlock["intent"] = intent.first;
            }
        }
        mstandBlock["markers"] = GetBlockMarkers(*this);
        mstandBlock["source"] = WebSource;
        mstandBlock["shard"] = WebShard;
        mstandBlock["base_type"] = BaseType;
        mstandBlock["relevpredict"] = RelevPrediction;
        if (ProximaPredict.Defined()){
            mstandBlock["proximapredict"] = ProximaPredict.GetRef();
        }
    }
    return mstandBlock;
}

const TString TWebRequest::TOP_BLOCKS_RULE_NAME = "top_blocks";
const TString TWebRequest::MAIN_BLOCKS_RULE_NAME = "main_blocks";
const TString TWebRequest::PARALLEL_BLOCKS_RULE_NAME = "parallel_blocks";

bool IsValidClick(const NRA::TClick* click) {
    if (click->GetBaobabBlock().Defined()
        && click->GetBaobabBlock()->GetName() == "suggest"
        && click->GetBaobabBlock()->GetParent().Defined()
        && click->GetBaobabBlock()->GetParent()->GetName() == "$header")
    {
        return false;
    }
    return true;
}

bool GetIsAcceptableRequest(const NRA::TRequest& request, const TRequestsSettings& settings) {
    const auto pageProps = dynamic_cast<const NRA::TPageProperties*>(&request);
    if (!pageProps) {
        return false;
    }
    if (settings.RequestsContainerType == ERequestsContainerType::SRCT_ADS_VERTICAL) {
        const auto adsReqProps = dynamic_cast<const NRA::TYandexDirectVerticalBaseRequestProperties*>(&request);
        return adsReqProps;
    } else if (settings.RequestsContainerType == ERequestsContainerType::SRCT_WEB) {
        const auto webReqProps = dynamic_cast<const NRA::TWebRequestProperties*>(&request);
        if(!webReqProps){
            const auto videoReqProps = dynamic_cast<const NRA::TVideoRequestProperties*>(&request);
            return videoReqProps;
        }
        return webReqProps;
    }
    return false;
}

bool IsWizardGeneratedRequest(
    const NRA::TRequest* request,
    TString& producingReqId,
    TString& source,
    TString& path,
    TString& relatedUrl)
{
    if (const auto* yaProps = dynamic_cast<const NRA::TYandexRequestProperties *>(request)) {
        const TCgiParameters cgi = yaProps->GetCGI();

        TCgiParameters::const_iterator sourceIt = cgi.find("source");
        if (sourceIt != cgi.end()) {
            source = sourceIt->second;
        }

        TCgiParameters::const_iterator pathIt = cgi.find("path");
        if (pathIt != cgi.end()) {
            path = pathIt->second;
        }

        TCgiParameters::const_iterator relatedUrlIt = cgi.find("related_url");
        if (relatedUrlIt != cgi.end()) {
            relatedUrl = relatedUrlIt->second;
        }

        producingReqId = request->GetProducingReqID().GetOrElse("");
        if (producingReqId) {
            return true;
        }

        TCgiParameters::const_iterator producingReqIdIt = cgi.find("parent-reqid");
        if (producingReqIdIt == cgi.end()) {
            producingReqIdIt = cgi.find("parent_reqid");
        }
        if (producingReqIdIt == cgi.end()) {
            producingReqIdIt = cgi.find("related_reqid");
        }
        if (producingReqIdIt == cgi.end()) {
            producingReqIdIt = cgi.find("reqid");
        }
        if (producingReqIdIt != cgi.end()) {
            producingReqId = producingReqIdIt->second;
            return true;
        }
    }
    return false;
}

TWebRequest::TWebRequest(
    const NRA::TRequest* request,
    TAtomicSharedPtr<const NBaobab::NTamus::TMarkersContainer> tamusMarkers,
    const TRequestsSettings& settings,
    const THashMap<TString, double>* otherRequestScores
)
    : TBaseRequest(request, settings.RequestsContainerType)
    , IsPermissionRequested(GetIsPermissionRequested(request))
    , UserIP(request->GetUserIPStr())
{
    // Base Properties
    if (const auto* serpProperties = dynamic_cast<const NRA::TSerpProperties*>(request)) {
        HasSerpProperties = true;
    }
    if (const auto* webRequestProperties = dynamic_cast<const NRA::TWebRequestProperties*>(request)) {
        HasWebRequestProperties = true;
    }

    ParseVideoRequestProperties(request);

    if (const auto* portalRequestProperties = dynamic_cast<const NRA::TPortalRequestProperties*>(request)) {
        HasPortalRequestProperties = true;
    }

    if (request->GetMainBlocks().size() > 100 || request->GetParallelBlocks().size() > 100) {
        return;
    }

    IsAcceptableRequest = GetIsAcceptableRequest(*request, settings);
    if (!IsAcceptableRequest) {
        return;
    }

    if (!tamusMarkers->HasBlocks(TOP_BLOCKS_RULE_NAME)
        && !tamusMarkers->HasBlocks(MAIN_BLOCKS_RULE_NAME)
        && !tamusMarkers->HasBlocks(PARALLEL_BLOCKS_RULE_NAME))
    {
        IsAcceptableRequest = false;
        if(!IsVideoAcceptableRequest) {
            return;
        }
    }
    // Blocks
    ParseBaobabProperties(request, tamusMarkers, otherRequestScores);
    if (!MainBlocks.empty()) {
        MainBlocks.back()->IsLast = true;
    }

    for (const auto& blocks : { MainBlocks, ParallelBlocks }) {
        for (const auto& block : blocks) {
            for (const auto& click : block->Clicks) {
                const auto key = std::make_pair(click->ClickId, click->Timestamp);
                if (click->ClickId == "" || !ClickIDs.contains(key)) {
                    ClickIDs.insert(key);
                    AllClicks.push_back(click);
                }
            }
        }
    }

    // Clicks
    for (const auto& click : request->GetClicks()) {
        if (!click->ShouldBeUsedInDwellTimeOnService()) {
            continue;
        }

        auto webClick = std::make_shared<TWebClick>(click);

        // TODO: REMOVE AFTER VALIDATION
        // webClick.BaobabAttrs["block_id"] = "";
        // TODO: REMOVE AFTER VALIDATION

        const auto key = std::make_pair(webClick->ClickId, webClick->Timestamp);
        if (!ClickIDs.contains(key)) {
            ClickIDs.insert(key);
            AllClicks.push_back(webClick);
        }
    }

    for (const auto& blocks : { MainBlocks, ParallelBlocks }) {
        for (const auto& block : blocks) {
            if (block->WizardName == "market_carousel") {
                NeedParseSearchProps = true;
            }
            if (block->WizardName == "ether") {
                HasEtherWizard = true;
            }
            if (block->WizardName == "video") {
                HasVideoWizard = true;
            }
        }
    }

    // Properties
    if (const auto* padUIProperties = dynamic_cast<const NRA::TPadUIProperties*>(request)) {
        HasPadUIProperties = true;
    }
    if (const auto* touchUIProperties = dynamic_cast<const NRA::TTouchUIProperties*>(request)) {
        HasTouchUIProperties = true;
    }
    if (const auto* blockstatRequestProperties = dynamic_cast<const NRA::TBlockstatRequestProperties*>(request)) {
        HasBlockstatRequestProperties = true;
    }
    if (const auto* marketProducerProperties = dynamic_cast<const NRA::TMarketProducerProperties*>(request)) {
        for (const auto& pe : marketProducerProperties->GetProducedEvents()) {
            if (pe) {
                auto pof = pe->GetPof();
                // https://st.yandex-team.ru/ONLINE-51#5cebeeeab1c761001fb34033
                if (pof >= 500 && pof <= 767) {
                    HasMarketDelayedCpcClick = true;
                }
            }
        }
    }

    MisspellBlendedRequest = IsMisspellBlendedRequest(*request);
    IsEntoSerp = HasYandexRequestProperties && Cgi.Has("ento");
    WizardGeneratedSerp = NStatCollector::IsWizardGeneratedSerp(request, GeneratedWizardName);
//    Domain = NMstand::GetDomain(*request);
//    UserCountryCode = GetUserCountryCode(request);

    ScreenHeight = GetBaobabScreenHeight(request);
    MaxScrollYCoordinate = GetBaobabMaxScrollYCoordinate(request);
    ScreensScrolled = GetScreensScrolled(ScreenHeight, MaxScrollYCoordinate);

    NUserMetrics::TBnaInfo bnaInfo;
    IsBna = NUserMetrics::GetBnaPosTypeAndExtensionByBaobab(*request, *tamusMarkers, bnaInfo);
    BnaPos = bnaInfo.Pos;
    BnaType = bnaInfo.Type;
    BnaExtension = bnaInfo.Extension;

    // MouseTrack use Path and Vars
    NMouseTrack::TMouseTrackRequestData mouseTrackRequestData = NMouseTrack::ExtractFromRequest(*request);
    MaxScrollYTop = mouseTrackRequestData.MaxScrollYTop;
    MaxScrollYBottom = mouseTrackRequestData.MaxScrollYBottom;
    ViewportHeight = mouseTrackRequestData.ViewportHeight;

    for (auto& block : MainBlocks) {
        for (const auto& mouseTrackBlock : mouseTrackRequestData.BlocksData) {
            if (mouseTrackBlock.Get()->HasBlockCounterId()
                && block->BlockId == mouseTrackBlock.Get()->GetBlockCounterId())
            {
                block->SetMouseTrackParameters(mouseTrackBlock.Get());
            }
        }
    }
    for (auto& block : ParallelBlocks) {
        for (const auto& mouseTrackBlock : mouseTrackRequestData.BlocksData) {
            if (mouseTrackBlock.Get()->HasBlockCounterId()
                && block->BlockId == mouseTrackBlock.Get()->GetBlockCounterId())
            {
                block->SetMouseTrackParameters(mouseTrackBlock.Get());
            }
        }
    }
    FixMouseTrackParameters();

    PostProcessBlocks();

    GetMiscClicks(request);
    std::sort(
        AllClicks.begin(),
        AllClicks.end(),
        [&](const auto& first, const auto& second) {
            return first->Timestamp < second->Timestamp;
        }
    );
    for (const auto& click : AllClicks) {
        if (click->IsDynamic) {
            ++DynamicClicksCount;
        } else {
            ++StaticClicksCount;
        }
    }

    ParseSearchPropsAndReqParams(request);
    ParseYandexTechEvents(request);

    isReload = request->IsReloaded();
    isReloadBackForward = request->IsBackForwardReloaded();
}

void TWebRequest::ParsePlayerEvent(
    const TString& path,
    const TMaybe<ui32>& time,
    const TMaybe<ui32>& duration,
    bool mute)
{
    if (path == "player-events.heartbeat") {
        HeartbeatCount += 1;
        if (!mute) {
            UnmutedHeartbeatCount += 1;
            if (duration.GetOrElse(0) > 0) {
                WatchRatio += 30.0 / duration.GetOrElse(0);
            }
        }
        if (duration.GetOrElse(0) > Duration) {
            Duration = duration.GetOrElse(0);
        }
    } else if (time.Defined() && time > 0
               && (path == "player-events.play" || path == "player-events.pause"))
    {
        PlayerClicks += 1;
    }
}

void TWebRequest::GetMiscClicks(const NRA::TRequest* request) {
    // TODO: https://st.yandex-team.ru/ONLINE-294
    // const NRA::TMiscResultClicks &miscClicks = request->GetMiscClicks();
    // for (const auto& click : miscClicks) {
    //     TWebClick webClick(click);
    // }

    NRA::TEvents::const_iterator event = request->GetOwnEvents().begin(), end = request->GetOwnEvents().end();
    for (; event != end; ++event) {
        if (TObj<NRA::TClick> click = dynamic_cast<NRA::TResultClick *>(&(**event))) {
            if (!click->ShouldBeUsedInDwellTimeOnService()) {
                continue;
            }

            auto webClick = std::make_shared<TWebClick>(click);

            // TODO: REMOVE AFTER VALIDATION
            // webClick.BaobabAttrs["block_id"] = "";
            // TODO: REMOVE AFTER VALIDATION

            const auto key = std::make_pair(webClick->ClickId, webClick->Timestamp);
            if (!ClickIDs.contains(key)) {
                ClickIDs.insert(key);
                AllClicks.push_back(webClick);
            }
        }
    }
}

ui32 TWebRequest::GetBaobabMaxScrollYCoordinate(const NRA::TRequest* request) {
    if (dynamic_cast<const NRA::TWebRequestProperties*>(request) == nullptr) {
        return 0;
    }

    ui32 maxScrollY = 0;
    for (const auto& tech : request->GetYandexTechEvents()) {
        const auto& btech = tech->GetBaobabTech();
        if (btech && btech->GetType() == "serp-page-scroll") {
            if (const auto& scrollY = NBaobab::GetAttrValueUnsafe<double>(btech->GetData(), "y")) {
                if (scrollY && *scrollY > maxScrollY) {
                    maxScrollY = *scrollY;
                }
            }
        }
    }

    if (maxScrollY > SCROLL_Y_LIMIT)
        maxScrollY = SCROLL_Y_LIMIT;
    return maxScrollY;
}

ui32 TWebRequest::GetBaobabScreenHeight(const NRA::TRequest* request) {
    if (dynamic_cast<const NRA::TWebRequestProperties*>(request) == nullptr) {
        return UNKNOWN_SCREEN_HEIGHT;
    }

    ui32 screenHeight = UNKNOWN_SCREEN_HEIGHT;
    for (const auto& tech : request->GetYandexTechEvents()) {
        const auto& btech = tech->GetBaobabTech();
        if (btech && btech->GetType() == "serp-page-loaded") {
            if (const auto& viewportSize = NBaobab::GetAttrValueUnsafe<NBaobab::TAttrsMap>(btech->GetData(), "viewportSize")) {
                if (const auto& maybeScreenHeight = NBaobab::GetAttrValueUnsafe<ui64>(*viewportSize, "height")) {
                    screenHeight = *maybeScreenHeight;
                }
            }
        }
    }

    return screenHeight;
}

double TWebRequest::GetScreensScrolled(ui32 screenHeight, ui32 maxScrollY) {
    if (screenHeight <= 0.)
        return 1.;
    double screensScrolled = (maxScrollY + screenHeight + 0.) / screenHeight;
    if (screensScrolled > SCROLL_SCREENS_LIMIT)
        screensScrolled = SCROLL_SCREENS_LIMIT;
    return screensScrolled;
}

void TWebRequest::ParseSearchPropsAndReqParams(const NRA::TRequest* request) {
    const auto* misc = dynamic_cast<const NRA::TMiscRequestProperties*>(request);
    if (!misc || !HasYandexRequestProperties) {
        return;
    }

    const NRA::TNameValueMap& searchProps = misc->GetSearchPropsValues();
    NRA::TNameValueMap::const_iterator onlyDirect = searchProps.find("UPPER.DirectMarket.OnlyDirect");
    if (searchProps.end() != onlyDirect) {
        TryFromString(onlyDirect->second, MarketOnlyDirect);
    }

    UserHistory = GetUserHistory(searchProps);

    NRA::TNameValueMap rearrValues = misc->GetRearrValues();
    if (const auto* qfcId = rearrValues.FindPtr("qfc_id_1")) {
        ClusterID = *qfcId;
    }
    if (const auto* dssmCtrCluster = rearrValues.FindPtr("dssm_ctr_cluster")) {
        DssmClusterID = *dssmCtrCluster;
    }
    if (const auto* ecomClassifierProb = rearrValues.FindPtr("wizdetection_ecom_classifier_prob")) {
        float value;
        TryFromString(*ecomClassifierProb, value);
        EcomClassifierProb = value;
    }
    if (const auto* ecomClassifierResult = rearrValues.FindPtr("wizdetection_ecom_classifier")) {
        EcomClassifierResult = *ecomClassifierResult == "1";
    }
    RearrValues = GetRearrValues(rearrValues);

    NRA::TNameValueMap relevValues = misc->GetRelevValues();
    if (const auto* hpDetectorPredict = relevValues.FindPtr("hp_detector_predict")) {
        TryFromString(*hpDetectorPredict, HpDetectorPredict);
    }
    if (const auto* payDetectorPredict = relevValues.FindPtr("pay_detector_predict")) {
        TryFromString(*payDetectorPredict, PayDetectorPredict);
    }
    if (const auto* queryAboutOneProduct = relevValues.FindPtr("query_about_one_product")) {
        TryFromString(*queryAboutOneProduct, QueryAboutOneProduct);
    }
    if (const auto* queryAboutManyProducts = relevValues.FindPtr("query_about_many_products")) {
        TryFromString(*queryAboutManyProducts, QueryAboutManyProducts);
    }
    RelevValues = GetRelevValues(relevValues);

    SearchProps = GetSearchProps(searchProps);
}

void TWebRequest::FixMouseTrackParameters() {
    size_t blocksCount = MainBlocks.size();
    for (size_t index = 0; index < blocksCount; ++index) {
        auto& block = *MainBlocks[index].get();
        if (!block.HasMouseTrackHeight) {
            if (index == 0 && index + 1 < blocksCount && MainBlocks[index + 1]->HasMouseTrackTop) {
                block.HasMouseTrackTop = true;
                block.MouseTrackTop = 121;
                block.HasMouseTrackHeight = true;
                block.MouseTrackHeight = MainBlocks[index + 1]->MouseTrackTop - block.MouseTrackTop;
            } else if (index > 0 && index + 1 < blocksCount) {
                if (block.HasMouseTrackTop && MainBlocks[index + 1]->HasMouseTrackTop) {
                    block.HasMouseTrackHeight = true;
                    block.MouseTrackHeight = MainBlocks[index + 1]->MouseTrackTop - block.MouseTrackTop;
                } else if (MainBlocks[index - 1]->HasMouseTrackTop && MainBlocks[index + 1]->HasMouseTrackTop) {
                    if (MainBlocks[index - 1]->HasMouseTrackHeight) {
                        block.HasMouseTrackTop = true;
                        block.MouseTrackTop =
                            MainBlocks[index - 1]->MouseTrackTop + MainBlocks[index - 1]->MouseTrackHeight;
                        block.HasMouseTrackHeight = true;
                        block.MouseTrackHeight = MainBlocks[index + 1]->MouseTrackTop - block.MouseTrackTop;
                    }
                }
            }
        }
    }
}

bool TWebRequest::IsMisspellBlendedRequest(const NRA::TRequest& request) {
    for (const auto& block : request.GetMainBlocks()) {
        if (const auto* wr = dynamic_cast<const NRA::TWebResult*>(block->GetMainResult())) {
            if (IsMisspellSource(wr->GetSource())) {
                return true;
            }
        }
    }
    return false;
}

size_t TWebRequest::GetWizShow() const {
    size_t result = 0;
    for (const auto& block : MainBlocks) {
        result += block->IsWizardResult;
    }
    return result;
}

size_t TWebRequest::GetDirShow() const {
    size_t result = 0;
    for (const auto& block : MainBlocks) {
        result += block->IsDirectResult;
    }
    return result;
}

void TWebRequest::ParseVideoRequestProperties(const NRA::TRequest* request) {
    const auto* videoRequestProperties = dynamic_cast<const NRA::TVideoRequestProperties*>(request);
    if (!videoRequestProperties) {
        return;
    }

    HasVideoRequestProperties = true;
    IsVideoAcceptableRequest = true;
    TString producingReqId;
    TString source;
    TString path;
    TString relatedUrl;

    IsWizardGeneratedRequest(
        request,
        producingReqId,
        source,
        path,
        relatedUrl
    );

    ParseVideoHeartbeat(request);
    ParseVideoDurationInfo(videoRequestProperties);

    VideoData["results"] = NYT::TNode::CreateList();
    VideoData["wizard_results"] = NYT::TNode::CreateList();
    VideoData["wizard_clicks"] = NYT::TNode::CreateList();
    VideoData["player_events"] = NYT::TNode::CreateList();
    VideoData["parent_reqid"] = producingReqId;
    VideoData["source"] = source;
    VideoData["path"] = path;
    VideoData["related_url"] = relatedUrl;

    for (NRA::TResultIterator<NRA::TResult> resultIter(request->GetMainBlocks()); resultIter.Valid(); ++resultIter) {
        NYT::TNode resultData = NYT::TNode::CreateMap();
        if (const auto* videoResult = dynamic_cast<const NRA::TVideoResultProperties*>(&*resultIter)) {
            resultData["url"] = videoResult->GetUrl();
            resultData["player_id"] = videoResult->GetPlayerId().GetOrElse("");
            resultData["duration"] = videoResult->GetDuration();
            resultData["source"] = videoResult->GetVideoSourceName().GetOrElse("");
            resultData["modification_time"] = videoResult->GetModificationTime().GetOrElse(0);
            resultData["clicks"] = NYT::TNode::CreateList();
            for (const auto& click : resultIter->GetClicks()) {
                NYT::TNode clickData = NYT::TNode::CreateMap();
                clickData["ts"] = click->GetTimestamp();
                clickData["path"] = click->GetPath();
                if (const auto& vd = videoRequestProperties->FindVideoDurationInfo(click)) {
                    clickData["view_duration"] = vd->GetPlayingDuration();
                }
                if (const auto& vh = videoRequestProperties->FindVideoHeartbeat(click, EVideoHeartbeatType::VH_SINGLE)) {
                    clickData["hb-single"] = vh->GetTicks();
                }
                if (const auto& vh = videoRequestProperties->FindVideoHeartbeat(click, EVideoHeartbeatType::VH_REPEAT)) {
                    clickData["hb-repeat"] = vh->GetTicks();
                }
                resultData["clicks"].Add(clickData);
            }
            resultData["view_time_info"] = NYT::TNode::CreateMap();
            if (const auto& vd = videoRequestProperties->FindVideoDurationInfo(&*resultIter)) {
                resultData["view_time_info"]["view_duration"] = vd->GetPlayingDuration();
            }
            if (const auto& vh = videoRequestProperties->FindVideoHeartbeat(&*resultIter, EVideoHeartbeatType::VH_SINGLE)) {
                resultData["view_time_info"]["hb-single"] = vh->GetTicks();
            }
            if (const auto& vh = videoRequestProperties->FindVideoHeartbeat(&*resultIter, EVideoHeartbeatType::VH_REPEAT)) {
                resultData["view_time_info"]["hb-repeat"] = vh->GetTicks();
            }
        }
        VideoData["results"].Add(resultData);
    }

    if (const auto* relatedVideoReqProps = dynamic_cast<const NRA::TRelatedVideoRequestProperties*>(request)) {
        VideoData["type"] = "related_request";
    } else if (const auto* yaVideoMordaReq = dynamic_cast<const NRA::TYandexVideoMordaRequest*>(request)) {
        VideoData["type"] = "morda_request";
    } else {
        VideoData["type"] = "search_request";
    }
}

void TWebRequest::ParseBaobabProperties(
    const NRA::TRequest* request,
    TAtomicSharedPtr<const NBaobab::NTamus::TMarkersContainer> tamusMarkers,
    const THashMap<TString, double>* otherRequestScores
) {
    const auto* baobab = dynamic_cast<const NRA::TBaobabProperties*>(request);
    if (!baobab) {
        return;
    }

    THashSet<TString> directOffersGalleryBlocks;
    for (const auto& block : tamusMarkers->GetBlocks("direct_offers_gallery")) {
        if (block.GetParent().Defined()) {
            directOffersGalleryBlocks.insert(block.GetParent()->GetID());
        }
    }

    THashSet<TString> blockIDs;
    for (const auto& groupName : {TOP_BLOCKS_RULE_NAME, MAIN_BLOCKS_RULE_NAME, PARALLEL_BLOCKS_RULE_NAME}) {
        const auto& blocks = tamusMarkers->GetBlocks(groupName);
        for (const auto& baobabBlock : blocks) {
            TString blockID = baobabBlock.GetID();
            NRA::TResult* result = baobab->FindResultByBaobabID(blockID);
            if (!result) {
                for (const auto& child : NBaobab::TChildren(baobabBlock)) {
                    result = baobab->FindResultByBaobabID(child.GetID());
                    if (result) {
                        blockID = child.GetID();
                        break;
                    }
                }
                if (!result) {
                    continue;
                }
            }

            if (blockIDs.contains(blockID)) {
                continue;
            }
            blockIDs.insert(blockID);

            auto ralibBlock = dynamic_cast<const NRA::TBlock*>(result);
            if (!ralibBlock) {
                continue;
            }

            double otherScore = 0.;
            if (otherRequestScores != nullptr) {
                auto itFindOtherScore = otherRequestScores->find(blockID);
                if (itFindOtherScore != otherRequestScores->end()) {
                    otherScore = itFindOtherScore->second;
                }
            }

            auto isParallel = groupName == PARALLEL_BLOCKS_RULE_NAME;
            auto webBlock = std::make_shared<TWebBlock>(ralibBlock, tamusMarkers, baobabBlock, otherScore, isParallel);
            for (auto& webClick : webBlock->Clicks) {
                webClick->SetBlock(webBlock);
            }

            if (webBlock->IsDirectResult) {
                // TODO: remove after fix in logs
                if (ExistsBannerID.contains(webBlock->BannerID)) {
                    webBlock->Clicks.clear();
                } else {
                    ExistsBannerID.insert(webBlock->BannerID);
                }
            }

            for (const auto& attr : baobabBlock.GetAttrs()) {
                if (attr.second.IsString()) {
                    if (attr.first == "wizard_name") {
                        webBlock->BaobabWizardName = attr.second.Get<TString>();
                    }
                    if (attr.first == "subtype") {
                        webBlock->BaobabSubType = attr.second.Get<TString>();
                    }
                }
                if (attr.second.IsMap()) {
                    if (attr.first == "tags") {
                        for (const auto& tag : attr.second.Get<NBaobab::TAttrsMap>()) {
                            if (tag.first == "video_content") {
                                webBlock->IsExpVideoPlayer = true;
                                if (webBlock->WizardName == "entity_search") {
                                    webBlock->IsEntitySearchPlayer = true;
                                }
                            }
                        }
                    }
                }
            }

            if (directOffersGalleryBlocks.contains(baobabBlock.GetID())) {
                webBlock->HasMarketGallery = true;
            }

            webBlock->FullBlockId = TString::Join(webBlock->BlockId, "_", ReqID);
            if (isParallel) {
                ParallelBlocks.push_back(webBlock);
            } else {
                MainBlocks.push_back(webBlock);
            }
        }
    }
}

void TWebRequest::ParseYandexTechEvents(const NRA::TRequest* request) {
    for (const auto& techEvent : request->GetYandexTechEvents()) {
        NYT::TNode playerEvent = NYT::TNode::CreateMap();
        if (const auto* videoPlayerEvent = dynamic_cast<const NRA::TVideoPlayerEvent*>(techEvent.Get())) {
            playerEvent["type"] = "player-event";
            playerEvent["ts"] = techEvent->GetTimestamp();
            playerEvent["path"] = videoPlayerEvent->GetPath();
            playerEvent["duration"] = videoPlayerEvent->GetDuration();
            playerEvent["watched_time"] = (double)videoPlayerEvent->GetTime().GetOrElse(0);
            playerEvent["mute"] = false;
            ParsePlayerEvent(
                videoPlayerEvent->GetPath(),
                videoPlayerEvent->GetTime(),
                videoPlayerEvent->GetDuration(),
                false
            );
        }
        if (const auto* tvOnlinePlayerEvent = dynamic_cast<const NRA::TTVOnlinePlayerEvent*>(techEvent.Get())) {
            playerEvent["type"] = "player-event";
            playerEvent["from_block"] = tvOnlinePlayerEvent->GetFromBlock().GetOrElse("");
            playerEvent["ts"] = techEvent->GetTimestamp();
            playerEvent["path"] = tvOnlinePlayerEvent->GetPath();
            playerEvent["duration"] = tvOnlinePlayerEvent->GetDuration().GetOrElse(0);
            playerEvent["watched_time"] = (double)tvOnlinePlayerEvent->GetWatchedTime().GetOrElse(0);
            playerEvent["mute"] = (tvOnlinePlayerEvent->GetMute().Defined() && tvOnlinePlayerEvent->GetMute().GetRef()) ? true : false;
            if (tvOnlinePlayerEvent->GetChannelID().Defined()) {
                playerEvent["channel_id"] = tvOnlinePlayerEvent->GetChannelID().GetRef();
            }
            playerEvent["full_screen"] = tvOnlinePlayerEvent->GetFullScreen().GetOrElse(false);
            ParsePlayerEvent(
                tvOnlinePlayerEvent->GetPath(),
                tvOnlinePlayerEvent->GetWatchedTime(),
                tvOnlinePlayerEvent->GetDuration(),
                tvOnlinePlayerEvent->GetMute().Defined() && tvOnlinePlayerEvent->GetMute().GetRef()
            );
        }
        if (!playerEvent.Empty()) {
            PlayerData.push_back(playerEvent);
        }
    }
}

void TWebRequest::PostProcessBlocks() {
    size_t position = 0;
    size_t directPosition = 0;
    size_t organicPosition = 0;
    size_t wizardPosition = 0;
    for (size_t index = 0; index < MainBlocks.size(); ++index) {
        auto& block = *MainBlocks[index].get();
        if (block.IsSignificantBlock) {
            block.Position = position;
            ++position;
        }
        if (block.BaobabWizardName == "entity/movie" && block.BaobabSubType == "cinema") {
            block.IsExpVideoPlayer = true;
            block.IsEntitySearchPlayer = true;
        }
        if (block.HasOrganicResultProperties) {
            if (block.HasWizardResultProperties) {
                block.WizardPosition = wizardPosition;
                ++wizardPosition;
            } else {
                block.OrganicPosition = organicPosition;
                ++organicPosition;
            }
        }
        if (block.IsDirectResult) {
            block.DirectPosition = directPosition;
            ++directPosition;
        }
    }

    position = 0;
    directPosition = 0;
    organicPosition = 0;
    wizardPosition = 0;
    for (size_t index = 0; index < ParallelBlocks.size(); ++index) {
        auto& block = *ParallelBlocks[index].get();
        if (block.IsSignificantBlock) {
            block.Position = position;
            ++position;
        }
        if (block.HasOrganicResultProperties) {
            if (block.HasWizardResultProperties) {
                block.WizardPosition = wizardPosition;
                ++wizardPosition;
            } else {
                block.OrganicPosition = organicPosition;
                ++organicPosition;
            }
        }
        if (block.IsDirectResult) {
            block.DirectPosition = directPosition;
            ++directPosition;
        }
    }
}

const NYT::TNode TWebRequest::GetSourcesSqueeze() const {
    NYT::TNode sourcesSqueeze = NYT::TNode::CreateList();
    for (const auto& webBlock : MainBlocks) {
        sourcesSqueeze.Add(webBlock->SourceName);
    }
    for (const auto& webBlock : ParallelBlocks) {
        sourcesSqueeze.Add(webBlock->SourceName);
    }
    return sourcesSqueeze;
}

const NYT::TNode TWebRequest::GetRequestParamsSqueeze() const {
    NYT::TNode requestParamsSqueeze = NYT::TNode::CreateList();
    for (const auto& cgi : Cgi) {
        requestParamsSqueeze.Add(cgi.first);
    }
    return requestParamsSqueeze;
}

const NYT::TNode TWebRequest::GetBlocksSqueeze() const {
    NYT::TNode blocksSqueeze = NYT::TNode::CreateList();
    for (const auto& blocks : { MainBlocks, ParallelBlocks }) {
        for (const auto& webBlock : blocks) {
            NYT::TNode blockNode = webBlock->GetBlockData();
            blocksSqueeze.Add(blockNode);
        }
    }
    return blocksSqueeze;
}

const NYT::TNode TWebRequest::GetMousetrackSqueeze() const {
    if (MaxScrollYTop == 0
        && MaxScrollYBottom == 0
        && ViewportHeight == 0)
    {
        return NYT::TNode::CreateEntity();
    }
    NYT::TNode mousetrackSqueeze = NYT::TNode::CreateMap();
    mousetrackSqueeze["max_scroll_y_top"] = MaxScrollYTop;
    mousetrackSqueeze["max_scroll_y_bottom"] = MaxScrollYBottom;
    mousetrackSqueeze["viewport_height"] = ViewportHeight;
    return mousetrackSqueeze;
}

const NYT::TNode TWebRequest::GetRequestData(const NYT::TNode& baseData) const {
    NYT::TNode result = GetCommonData(baseData);
    result["dir_show"] = GetDirShow();
    result["sources"] = GetSourcesSqueeze();
    result["wiz_show"] = GetWizShow();
    result["pay_detector_predict"] = PayDetectorPredict;
    result["query_about_one_product"] = QueryAboutOneProduct;
    result["query_about_many_products"] = QueryAboutManyProducts;

    if (EcomClassifierProb.Defined()) {
        result["ecom_classifier_prob"] = EcomClassifierProb.GetRef();
    }
    if (EcomClassifierResult.Defined()) {
        result["ecom_classifier_result"] = EcomClassifierResult.GetRef();
    }
    return result;
}

const NYT::TNode TWebRequest::GetEnrichedData(const NYT::TNode& baseData, const NYT::TNode& extra) const {
    NYT::TNode result = baseData;
    for (const auto& item : extra.AsMap()) {
        result[item.first] = item.second;
    }
    return result;
}

TVector<NYT::TNode> TWebRequest::GetFullRequestData() const {
    TVector<NYT::TNode> result;

    NYT::TNode baseData = GetBaseData(/*addUI*/ true);
    baseData["is_match"] = true;
    TString serpType = ToString(SerpType);
    if (serpType == "touch" || serpType == "mobile" || serpType == "mobileapp") {
        baseData["servicetype"] = "touch";
    } else {
        baseData["servicetype"] = "web";
    }

    NYT::TNode mstandRequest = GetRequestData(baseData);
    // TODO: request_params need fix, bag example 2021-07-01, y8212000391603954091
    mstandRequest["request_params"] = GetRequestParamsSqueeze();
    mstandRequest["is_permission_requested"] = IsPermissionRequested;
    mstandRequest["cluster"] = DssmClusterID;
    mstandRequest["cluster_3k"] = ClusterID;
    mstandRequest["hp_detector_predict"] = HpDetectorPredict;

    mstandRequest["rearr_values"] = RearrValues;
    mstandRequest["relev_values"] = RelevValues;
    mstandRequest["user_history"] = UserHistory;
    mstandRequest["is_visible"] = IsVisible;
    
    if (!UserIP.empty()) {
        mstandRequest["user_ip"] = UserIP;
    }

    mstandRequest["search_props"] = SearchProps;
    mstandRequest["is_reload"] = isReload;
    mstandRequest["is_reload_back_forward"] = isReloadBackForward;

    result.push_back(mstandRequest);

    for (const auto& click : AllClicks) {
        NYT::TNode mstandClick = click->GetClickData(baseData);
        result.push_back(mstandClick);
    }

    for (const auto& player : PlayerData) {
        NYT::TNode mstandHeartbeats = GetEnrichedData(baseData, player);
        result.push_back(mstandHeartbeats);
    }

    return result;
}

void TWebRequest::ParseVideoHeartbeat(const NRA::TRequest* request) {
    for (const auto& event : request->GetOwnEvents()) {
        if (const auto* yandexVideoHeartbeat = dynamic_cast<const NRA::TVideoHeartbeat *>(event.Get())) {
            if(yandexVideoHeartbeat->GetType() == VH_REPEAT){
                auto data = NYT::TNode::CreateMap({
                    {"ts", yandexVideoHeartbeat->GetTimestamp()},
                    {"type", "heartbeat"},
                    {"ticks", yandexVideoHeartbeat->GetTicks()},
                    {"url", yandexVideoHeartbeat->GetUrl()},
                    {"duration",yandexVideoHeartbeat->GetDuration()}
                });
                VideoHeartbeatData.push_back(data);
            }
        }
    }
}
void TWebRequest::ParseVideoDurationInfo(const NRA::TVideoRequestProperties* request) {
    for (const auto& duration : request->GetVideoDurationInfos()) {
        auto data = NYT::TNode::CreateMap({
            {"ts", duration->GetTimestamp()},
            {"type", "duration"},
            {"duration", duration->GetDuration()},
            {"playing_duration", duration->GetPlayingDuration()},
            {"url", duration->GetUrl()}
        });
        VideoDurationData.push_back(data);
    }
}

TVector<NYT::TNode> TWebRequest::GetVideoRequestData() const {
    TVector<NYT::TNode> result;

    NYT::TNode baseData = GetBaseData(true);
    baseData["is_match"] = true;
    baseData["servicetype"] = "video";

    NYT::TNode mstandRequest = GetRequestData(baseData);
    mstandRequest["videodata"] = VideoData;
    result.push_back(mstandRequest);

    for (const auto& click : AllClicks) {
        NYT::TNode mstandClick = click->GetClickData(baseData);
        result.push_back(mstandClick);
    }     

    for (const auto& heartbeat : VideoHeartbeatData) {
        NYT::TNode mstandHeartbeats = GetEnrichedData(baseData, heartbeat);
        result.push_back(mstandHeartbeats);
    }

    for (const auto& heartbeat : VideoDurationData) {
        NYT::TNode mstandDuration = GetEnrichedData(baseData, heartbeat);
        result.push_back(mstandDuration);
    }

    return result;
}

TWebRequestsContainer::TWebRequestsContainer(
    const NRA::TRequestsContainer& requestsContainer,
    const TRequestsSettings& settings,
    const THashMap<TString, THashMap<TString, double>>* reqidToBaobabIdToOtherScore)
    : TBaseRequestsContainer(settings)
{
    for (const auto& request : requestsContainer.GetRequests()) {
        const auto tamusMarkers = NStatCollector::GetBaobabInfo(request.Get(), requestsContainer.GetBaobabWeakTies());
        if (reqidToBaobabIdToOtherScore == nullptr) {
            RequestsContainer.emplace_back(request, tamusMarkers, settings, nullptr);
        } else {
            auto itReqid = reqidToBaobabIdToOtherScore->find(request->GetReqID());
            if (itReqid == reqidToBaobabIdToOtherScore->end()) {
                RequestsContainer.emplace_back(request, tamusMarkers, settings, nullptr);
            } else {
                RequestsContainer.emplace_back(request, tamusMarkers, settings, &itReqid->second);
            }
        }
    }

    SetNavSuggest(requestsContainer);
}

const TVector<TWebRequest>& TWebRequestsContainer::GetRequestsContainer() const {
    return RequestsContainer;
}

const NUserMetrics::TRequestMetricsMap& TWebRequestsContainer::GetSessionVideoWizardGeneratedRequestProperties() const {
    return SessionVideoWizardGeneratedRequestProperties;
}

void TWebRequestsContainer::SetSessionVideoWizardGeneratedRequestProperties(const NRA::TRequests& requests) {
    NUserMetrics::TVideoRequestsContainer videoCont;
    for (const auto& request : requests) {
        videoCont.AddRequest(request);
    }
    videoCont.GetWizardGeneratedVideoMetrics(SessionVideoWizardGeneratedRequestProperties);
    for (auto& request : RequestsContainer) {
        if (const auto* metrics = SessionVideoWizardGeneratedRequestProperties.FindPtr(request.ReqID)) {
            request.VideoWizardGeneratedRequestProperties = *metrics;
        }
    }
}

void TWebRequestsContainer::AddVideoEtherView(const TString& wizardName, const TVector<time_t>& timestamps) {
    const ui32 SIMILARITY_TIMEDELTA = 5 * 60;
    size_t heardbeatIndex = 0;
    for (size_t index = 0; index < RequestsContainer.size(); ++index) {
        auto& webRequest = RequestsContainer[index];
        THashMap<TString, double>* wizardGeneratedRequestPropertiesPtr = nullptr;
        if (wizardName == "ether" && webRequest.HasEtherWizard) {
            wizardGeneratedRequestPropertiesPtr = &webRequest.EtherWizardGeneratedRequestProperties;
        } else if (wizardName == "video" && webRequest.HasVideoWizard) {
            wizardGeneratedRequestPropertiesPtr = &webRequest.VideoWizardGeneratedRequestProperties;
        }
        if (!wizardGeneratedRequestPropertiesPtr) {
            continue;
        }
        if (!wizardGeneratedRequestPropertiesPtr->contains("VideoViewTimeAfterWizardGeneratedRequest")) {
            (*wizardGeneratedRequestPropertiesPtr)["VideoViewTimeAfterWizardGeneratedRequest"] = 0;
        }
        if (!wizardGeneratedRequestPropertiesPtr->contains("VideoViewTime5MinsAfterWizardGeneratedRequest")) {
            (*wizardGeneratedRequestPropertiesPtr)["VideoViewTime5MinsAfterWizardGeneratedRequest"] = 0;
        }
        for (const auto& blocks : { webRequest.MainBlocks, webRequest.ParallelBlocks }) {
            for (const auto& block : blocks) {
                if (block->WizardName == wizardName && !block->Clicks.empty()) {
                    const auto clickTimestamps = std::minmax_element(
                        block->Clicks.begin(),
                        block->Clicks.end(),
                        [&](const auto& first, const auto& second) {
                            return first->Timestamp < second->Timestamp;
                        }
                    );
                    while (heardbeatIndex < timestamps.size()) {
                        if ((*clickTimestamps.first)->Timestamp > timestamps[heardbeatIndex]) {
                            break;
                        }
                        size_t nextWizIndex = index + 1;
                        bool nextHasWizard = false;
                        for (nextWizIndex = index + 1; nextWizIndex < RequestsContainer.size(); ++nextWizIndex) {
                            if (wizardName == "ether" && RequestsContainer[nextWizIndex].HasEtherWizard) {
                                nextHasWizard = true;
                                break;
                            }
                            if (wizardName == "video" && RequestsContainer[nextWizIndex].HasVideoWizard) {
                                nextHasWizard = true;
                                break;
                            }
                        }
                        if (nextWizIndex < RequestsContainer.size()
                            && RequestsContainer[nextWizIndex].HasWebRequestProperties
                            && nextHasWizard
                            && timestamps[heardbeatIndex] >= RequestsContainer[nextWizIndex].Timestamp)
                        {
                            break;
                        }
                        (*wizardGeneratedRequestPropertiesPtr)["VideoViewTimeAfterWizardGeneratedRequest"] += 30;
                        if (timestamps[heardbeatIndex] - (*clickTimestamps.second)->Timestamp <= SIMILARITY_TIMEDELTA) {
                            (*wizardGeneratedRequestPropertiesPtr)["VideoViewTime5MinsAfterWizardGeneratedRequest"] += 30;
                        }
                        ++heardbeatIndex;
                    }
                }
            }
        }
    }
}

void TWebRequestsContainer::SetSessionEtherWizardGeneratedRequestProperties(
    const NRA::TRequestsContainer& requestsContainer)
{
    TVector<time_t> etherTimestamps;
    TVector<time_t> videoTimestamps;
    for (const auto& techEvent : requestsContainer.GetSessionYandexTechEvents()) {
        if (const auto *tvOnlinePlayerEvent = dynamic_cast<const NRA::TTVOnlinePlayerEvent *>(techEvent.Get())) {
            if (tvOnlinePlayerEvent->GetPath() == "player-events.heartbeat"
                && tvOnlinePlayerEvent->GetFromBlock().Defined()
                && tvOnlinePlayerEvent->GetMute().Defined()
                && !tvOnlinePlayerEvent->GetMute().GetRef())
            {
                if (tvOnlinePlayerEvent->GetFromBlock() == "ether_wizard") {
                    etherTimestamps.push_back(tvOnlinePlayerEvent->GetTimestamp());
                }
                if (tvOnlinePlayerEvent->GetFromBlock() == "ya_serp_video") {
                    videoTimestamps.push_back(tvOnlinePlayerEvent->GetTimestamp());
                }
            }
        }
    }

    if (!etherTimestamps.empty()) {
        AddVideoEtherView("ether", etherTimestamps);
    }

    if (!videoTimestamps.empty()) {
        AddVideoEtherView("video", videoTimestamps);
    }
}

void TWebRequestsContainer::SetNavSuggest(const NRA::TRequestsContainer& requestsContainer) {
    for (const auto& ownEvent : requestsContainer.GetOwnEvents()) {
        if (const auto* yandexNavSuggestClick = dynamic_cast<const NRA::TYandexNavSuggestClick*>(ownEvent.Get())) {
            auto data = NYT::TNode::CreateMap({
                {"ts", yandexNavSuggestClick->GetTimestamp()},
                {"type", "nav-suggest-click"},
                {"url", yandexNavSuggestClick->GetUrl()},
                {"suggestion", yandexNavSuggestClick->GetSuggestion()},
            });
            NavSuggestData.push_back(data);
        }
    }
}

bool SqueezeCmp(const NYT::TNode& first, const NYT::TNode& second) {
    if (first["ts"].AsInt64() == second["ts"].AsInt64()) {
        return first["action_index"].AsUint64() < second["action_index"].AsUint64();
    }
    return first["ts"].AsInt64() < second["ts"].AsInt64();
}

const TVector<NYT::TNode>& TWebRequestsContainer::GetWebSqueeze() {
    if (!WebSqueeze.empty()) {
        return WebSqueeze;
    }

    size_t actionIndex = 0;
    for (const auto& webRequest : RequestsContainer) {
        if (!webRequest.IsAcceptableRequest) {
            continue;
        }

        for (auto& data : webRequest.GetFullRequestData()) {
            data["action_index"] = actionIndex++;
            WebSqueeze.push_back(data);
        }
    }

    if (!WebSqueeze.empty()) {
        auto requestData = WebSqueeze[0];
        for (auto& data : NavSuggestData) {
            data["yuid"] = requestData["yuid"];
            data["is_match"] = false;
            data["servicetype"] = requestData["servicetype"];
            data["action_index"] = actionIndex++;
            WebSqueeze.push_back(data);
        }
    }

    std::sort(WebSqueeze.begin(), WebSqueeze.end(), SqueezeCmp);
    return WebSqueeze;
}

const TVector<NYT::TNode>& TWebRequestsContainer::GetWebExtendedSqueeze() {
    if (!WebExtendedSqueeze.empty()) {
        return WebExtendedSqueeze;
    }

    size_t actionIndex = 0;
    for (const auto& webRequest : RequestsContainer) {
        if (!webRequest.IsAcceptableRequest) {
            continue;
        }

        NYT::TNode mstandRequest = webRequest.GetBaseData(/*addUI*/ true);
        mstandRequest["ts"] = webRequest.Timestamp;
        mstandRequest["action_index"] = actionIndex++;
        mstandRequest["blocks"] = webRequest.GetBlocksSqueeze();
        mstandRequest["is_match"] = true;
        mstandRequest["mousetrack"] = webRequest.GetMousetrackSqueeze();
        mstandRequest["type"] = "request-extended";

        TString serpType = ToString(webRequest.SerpType);
        if (serpType == "touch" || serpType == "mobile" || serpType == "mobileapp") {
            mstandRequest["servicetype"] = "web-touch-extended";
        } else {
            mstandRequest["servicetype"] = "web-desktop-extended";
        }

        WebExtendedSqueeze.push_back(mstandRequest);
    }

    std::sort(WebExtendedSqueeze.begin(), WebExtendedSqueeze.end(), SqueezeCmp);
    return WebExtendedSqueeze;
}

const TVector<NYT::TNode>& TWebRequestsContainer::GetVideoSqueeze() {
    if (!VideoSqueeze.empty()) {
        return VideoSqueeze;
    }

    size_t actionIndex = 0;
    for (const auto& videoRequest : RequestsContainer) {
        if (!videoRequest.IsVideoAcceptableRequest) {
            continue;
        }

        for (auto& data : videoRequest.GetVideoRequestData()) {
            data["action_index"] = actionIndex++;
            VideoSqueeze.push_back(data);
        }
    }

    std::sort(VideoSqueeze.begin(), VideoSqueeze.end(), SqueezeCmp);
    return VideoSqueeze;
}

}; //namespace NMstand
