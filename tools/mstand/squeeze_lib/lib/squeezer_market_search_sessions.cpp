#include "squeezer_market_search_sessions.h"

#include "tools/mstand/squeeze_lib/requests/market/market_request.h"


namespace NMstand {

using namespace NYT;

TTableSchema TMarketSearchSessionsActionsSqueezer::GetSchema() const
{
    TTableSchema schema;
    schema.AddColumn(TColumnSchema().Name("yuid").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("ts").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("action_index").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("bucket").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("is_match").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("testid").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("servicetype").Type(NYT::VT_STRING));

    schema.AddColumn(TColumnSchema().Name("browser").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("reqid").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("page").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("ui").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("url").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("type").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("is_text").Type(NYT::VT_BOOLEAN));

    schema.AddColumn(TColumnSchema().Name("show_uid").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("pos").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("clicked").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("ovl_clicked").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("cpa_order").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("cpc_click").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("has_cpa_click").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("has_cpa_click_without_properties").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("has_cpa_model_click").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("has_cpa_search_click").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("inclid").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("is_cpa").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("dwelltime").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("clicks").Type(NYT::VT_INT64));

    schema.AddColumn(TColumnSchema().Name("req_cpc_clicks").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("req_cpc_orders").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("req_cpa_clicks").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("req_cpa_orders").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("block_cpc_clicks").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("block_cpc_orders").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("block_cpa_clicks").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("block_cpa_orders").Type(NYT::VT_INT64));

    schema.AddColumn(TColumnSchema().Name("block_type").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("viewtype").Type(NYT::VT_STRING));
    return schema.Strict(true);
}

ui32 TMarketSearchSessionsActionsSqueezer::GetVersion() const
{
    return 1;
}

bool TMarketSearchSessionsActionsSqueezer::CheckRequest(const NRA::TRequest* request) const {
    return dynamic_cast<const NRA::TMarketSearchxmlProperties*>(request) != nullptr;
};

void TMarketSearchSessionsActionsSqueezer::GetActions(TActionSqueezerArguments& args) const
{
    TRequestsSettings requestsSettings(ERequestsContainerType::SRCT_MARKET);

    TMarketEventsDict reqToShowuidToEvent;
    for (const auto& row : args.Rows) {
        TMarketRequestsContainer::TryAddEvent(reqToShowuidToEvent, row["value"].AsString());
    }

    TMarketRequestsContainer marketRequestsContainer(args.Container, requestsSettings, reqToShowuidToEvent);

    THashMap<TString, TExpBucketInfo> reqid2BucketInfo;
    for (const NRA::TRequest* request : args.Container.GetRequests()) {
        reqid2BucketInfo[request->GetReqID()] = CheckExperiments(args, request);
    }

    for (const auto& squeeze : marketRequestsContainer.GetMarketSqueeze()) {
        const auto& reqid = squeeze["reqid"].AsString();
        args.ResultActions.push_back(TResultAction(squeeze, reqid2BucketInfo[reqid]));
    }
}

};
