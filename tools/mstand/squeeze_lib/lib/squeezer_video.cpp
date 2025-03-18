#include "squeezer_video.h"

#include "tools/mstand/squeeze_lib/requests/web/web_request.h"


namespace NMstand {

using namespace NYT;

TTableSchema TVideoActionsSqueezer::GetSchema() const {
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
    schema.AddColumn(TColumnSchema().Name("correctedquery").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("dir_show").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("hasmisspell").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("maxrelevpredict").Type(NYT::VT_DOUBLE));
    schema.AddColumn(TColumnSchema().Name("minrelevpredict").Type(NYT::VT_DOUBLE));
    schema.AddColumn(TColumnSchema().Name("query").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("referer").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("sources").Type(NYT::VT_ANY));
    schema.AddColumn(TColumnSchema().Name("suggest").Type(NYT::VT_ANY));
    schema.AddColumn(TColumnSchema().Name("userpersonalization").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("userregion").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("wiz_show").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("wizard").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("queryregion").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("pay_detector_predict").Type(NYT::VT_DOUBLE));
    schema.AddColumn(TColumnSchema().Name("query_about_one_product").Type(NYT::VT_DOUBLE));
    schema.AddColumn(TColumnSchema().Name("query_about_many_products").Type(NYT::VT_DOUBLE));
    schema.AddColumn(TColumnSchema().Name("ecom_classifier_prob").Type(NYT::VT_DOUBLE));
    schema.AddColumn(TColumnSchema().Name("ecom_classifier_result").Type(NYT::VT_BOOLEAN));

     // Click
    schema.AddColumn(TColumnSchema().Name("baobab_attrs").Type(NYT::VT_ANY)); // MSTAND-1410
    schema.AddColumn(TColumnSchema().Name("baobab_path").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("block_id").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("dwelltime_on_service").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("fraud_bits").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("is_dynamic_click").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("is_misc_click").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("original_url").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("path").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("placement").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("pos").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("restype").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("suggestion").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("target").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("url").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("visual_pos_ad").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("visual_pos").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("wizard_name").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("is_scroll").Type(NYT::VT_BOOLEAN));

    // Techevent
    schema.AddColumn(TColumnSchema().Name("from_block").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("vars").Type(NYT::VT_STRING));

    // Other
    schema.AddColumn(TColumnSchema().Name("duration").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("playing_duration").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("ticks").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("videodata").Type(NYT::VT_ANY));

    return schema.Strict(true);
}

ui32 TVideoActionsSqueezer::GetVersion() const
{
    return 14;
}

bool TVideoActionsSqueezer::CheckRequest(const NRA::TRequest* request) const
{
    return dynamic_cast<const NRA::TVideoRequestProperties*>(request) != nullptr;
}

void TVideoActionsSqueezer::GetActions(TActionSqueezerArguments& args) const
{
    TRequestsSettings requestsSettings(ERequestsContainerType::SRCT_WEB);
    TWebRequestsContainer requestsContainer(args.Container, requestsSettings);
    
    THashMap<TString, TExpBucketInfo> reqid2BucketInfo;
    for (const NRA::TRequest* request : args.Container.GetRequests()) {
        if (CheckRequest(request)) {
            reqid2BucketInfo[request->GetReqID()] = CheckExperiments(args, request);
        }
    }
    
    for (const auto& squeeze : requestsContainer.GetVideoSqueeze()) {
        const auto& reqid = squeeze["reqid"].AsString();
        auto bucketInfoPtr = reqid2BucketInfo.FindPtr(reqid);
        if (bucketInfoPtr != nullptr) {
            args.ResultActions.push_back(TResultAction(squeeze, *bucketInfoPtr));
        }
    }
}

}; // NMstand
