#include "squeezer_web.h"

#include "tools/mstand/squeeze_lib/requests/web/web_request.h"


namespace NMstand {

using namespace NYT;

/////////////////////////////// TWebDesktopActionsSqueezer
TTableSchema TWebDesktopActionsSqueezer::GetSchema() const {
    TTableSchema schema;
    // System
    schema.AddColumn(TColumnSchema().Name("yuid").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("ts").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("action_index").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("bucket").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("is_match").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("testid").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("servicetype").Type(NYT::VT_STRING));

    // Base
    schema.AddColumn(TColumnSchema().Name("domregion").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("page").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("reqid").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("ui").Type(NYT::VT_STRING));

    // Common
    schema.AddColumn(TColumnSchema().Name("type").Type(NYT::VT_STRING));

    // Request
    schema.AddColumn(TColumnSchema().Name("browser").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("clid").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("cluster_3k").Type(NYT::VT_STRING)); // MSTAND-1963
    schema.AddColumn(TColumnSchema().Name("cluster").Type(NYT::VT_STRING)); // MSTAND-1708
    schema.AddColumn(TColumnSchema().Name("correctedquery").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("dir_show").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("ecom_classifier_prob").Type(NYT::VT_DOUBLE));
    schema.AddColumn(TColumnSchema().Name("ecom_classifier_result").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("filmlistprediction").Type(NYT::VT_DOUBLE));
    schema.AddColumn(TColumnSchema().Name("hasmisspell").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("hp_detector_predict").Type(NYT::VT_DOUBLE)); // MSTAND-1786
    schema.AddColumn(TColumnSchema().Name("is_permission_requested").Type(NYT::VT_BOOLEAN)); // MSTAND-1446
    schema.AddColumn(TColumnSchema().Name("is_visible").Type(NYT::VT_BOOLEAN)); // MSTAND-1983
    schema.AddColumn(TColumnSchema().Name("maxrelevpredict").Type(NYT::VT_DOUBLE));
    schema.AddColumn(TColumnSchema().Name("minrelevpredict").Type(NYT::VT_DOUBLE));
    schema.AddColumn(TColumnSchema().Name("msid").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("pay_detector_predict").Type(NYT::VT_DOUBLE)); // MSTAND-1880
    schema.AddColumn(TColumnSchema().Name("query_about_many_products").Type(NYT::VT_DOUBLE));
    schema.AddColumn(TColumnSchema().Name("query_about_one_product").Type(NYT::VT_DOUBLE));
    schema.AddColumn(TColumnSchema().Name("query").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("queryregion").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("rearr_values").Type(NYT::VT_ANY)); // MSTAND-1880
    schema.AddColumn(TColumnSchema().Name("relev_values").Type(NYT::VT_ANY)); // MSTAND-2118
    schema.AddColumn(TColumnSchema().Name("referer").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("request_params").Type(NYT::VT_ANY));
    schema.AddColumn(TColumnSchema().Name("sources").Type(NYT::VT_ANY));
    // schema.AddColumn(TColumnSchema().Name("suggest").Type(NYT::VT_ANY));
    schema.AddColumn(TColumnSchema().Name("user_ip").Type(NYT::VT_STRING)); // MSTAND-2157
    schema.AddColumn(TColumnSchema().Name("user_history").Type(NYT::VT_ANY)); // MSTAND-1867
    schema.AddColumn(TColumnSchema().Name("userpersonalization").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("userregion").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("wiz_show").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("wizard").Type(NYT::VT_BOOLEAN));

    // Click
    schema.AddColumn(TColumnSchema().Name("baobab_attrs").Type(NYT::VT_ANY)); // MSTAND-1410
    schema.AddColumn(TColumnSchema().Name("baobab_path").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("block_id").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("click_id").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("dwelltime_on_service").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("fraud_bits").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("is_dynamic_click").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("is_misc_click").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("original_url").Type(NYT::VT_STRING)); // MSTAND-1824
    schema.AddColumn(TColumnSchema().Name("path").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("placement").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("pos").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("restype").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("suggestion").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("target").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("url").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("visual_pos_ad").Type(NYT::VT_INT64)); // MSTAND-1355
    schema.AddColumn(TColumnSchema().Name("visual_pos").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("wizard_name").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("is_scroll").Type(NYT::VT_BOOLEAN)); // MSTAND-2148

    // Techevent
    schema.AddColumn(TColumnSchema().Name("from_block").Type(NYT::VT_STRING)); // MSTAND-1588
    schema.AddColumn(TColumnSchema().Name("vars").Type(NYT::VT_STRING));

    // Event
    schema.AddColumn(TColumnSchema().Name("channel_id").Type(NYT::VT_STRING)); // MSTAND-1653
    schema.AddColumn(TColumnSchema().Name("duration").Type(NYT::VT_INT32)); // MSTAND-1750
    schema.AddColumn(TColumnSchema().Name("full_screen").Type(NYT::VT_BOOLEAN)); // MSTAND-1653
    schema.AddColumn(TColumnSchema().Name("mute").Type(NYT::VT_BOOLEAN)); // MSTAND-1653

    // New
    schema.AddColumn(TColumnSchema().Name("watched_time").Type(NYT::VT_DOUBLE));

    //SURPLUSTOOLS-22
    schema.AddColumn(TColumnSchema().Name("search_props").Type(NYT::VT_ANY));

    //MSTAND-2210
    schema.AddColumn(TColumnSchema().Name("is_reload").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("is_reload_back_forward").Type(NYT::VT_BOOLEAN));

    return schema.Strict(true);
}

ui32 TWebDesktopActionsSqueezer::GetVersion() const
{
    return 1;
}

bool TWebDesktopActionsSqueezer::CheckRequest(const NRA::TRequest* request) const
{
    return dynamic_cast<const NRA::TYandexWebRequest*>(request) != nullptr;
}

void TWebDesktopActionsSqueezer::GetActions(TActionSqueezerArguments& args) const
{
    TRequestsSettings requestsSettings(ERequestsContainerType::SRCT_WEB);
    TWebRequestsContainer requestsContainer(args.Container, requestsSettings);

    THashMap<TString, TExpBucketInfo> reqid2BucketInfo;
    for (const NRA::TRequest* request : args.Container.GetRequests()) {
        if (CheckRequest(request)) {
            reqid2BucketInfo[request->GetReqID()] = CheckExperiments(args, request);
        }
    }

    for (const auto& squeeze : requestsContainer.GetWebSqueeze()) {
        if (squeeze.HasKey("reqid")) {
            auto bucketInfoPtr = reqid2BucketInfo.FindPtr(squeeze["reqid"].AsString());
            if (bucketInfoPtr != nullptr) {
                args.ResultActions.emplace_back(squeeze, *bucketInfoPtr);
            }
        }
    }

    if (!args.ResultActions.empty()) {
        for (const auto& squeeze : requestsContainer.GetWebSqueeze()) {
            if (!squeeze.HasKey("reqid")) {
                args.ResultActions.emplace_back(squeeze, TExpBucketInfo());
            }
        }
    }
}
/////////////////////////////// TWebDesktopActionsSqueezer End

/////////////////////////////// TWebTouchActionsSqueezer
bool TWebTouchActionsSqueezer::CheckRequest(const NRA::TRequest* request) const
{
    return dynamic_cast<const NRA::TTouchYandexWebRequest*>(request) != nullptr
        || dynamic_cast<const NRA::TPadYandexWebRequest*>(request) != nullptr
        || dynamic_cast<const NRA::TMobileAppYandexWebRequest*>(request) != nullptr;
}
/////////////////////////////// TWebTouchActionsSqueezer End

}; // NMstand
