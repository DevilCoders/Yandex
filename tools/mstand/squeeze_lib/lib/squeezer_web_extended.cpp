#include "squeezer_web_extended.h"

#include "tools/mstand/squeeze_lib/requests/web/web_request.h"


namespace NMstand {

using namespace NYT;

/////////////////////////////// TWebDesktopExtendedActionsSqueezer
TTableSchema TWebDesktopExtendedActionsSqueezer::GetSchema() const {
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

    // Other
    schema.AddColumn(TColumnSchema().Name("blocks").Type(NYT::VT_ANY));
    schema.AddColumn(TColumnSchema().Name("mousetrack").Type(NYT::VT_ANY));

    return schema.Strict(true);
}

ui32 TWebDesktopExtendedActionsSqueezer::GetVersion() const
{
    return 1;
}

bool TWebDesktopExtendedActionsSqueezer::CheckRequest(const NRA::TRequest* request) const
{
    return dynamic_cast<const NRA::TYandexWebRequest*>(request) != nullptr;
}

void TWebDesktopExtendedActionsSqueezer::GetActions(TActionSqueezerArguments& args) const
{
    TRequestsSettings requestsSettings(ERequestsContainerType::SRCT_WEB);
    TWebRequestsContainer requestsContainer(args.Container, requestsSettings);

    THashMap<TString, TExpBucketInfo> reqid2BucketInfo;
    for (const NRA::TRequest* request : args.Container.GetRequests()) {
        if (CheckRequest(request)) {
            reqid2BucketInfo[request->GetReqID()] = CheckExperiments(args, request);
        }
    }

    for (const auto& squeeze : requestsContainer.GetWebExtendedSqueeze()) {
        const auto& reqid = squeeze["reqid"].AsString();
        auto bucketInfoPtr = reqid2BucketInfo.FindPtr(reqid);
        if (bucketInfoPtr != nullptr) {
            args.ResultActions.push_back(TResultAction(squeeze, *bucketInfoPtr));
        }
    }
}
/////////////////////////////// TWebDesktopExtendedActionsSqueezer End

/////////////////////////////// TWebTouchExtendedActionsSqueezer
bool TWebTouchExtendedActionsSqueezer::CheckRequest(const NRA::TRequest* request) const
{
    return dynamic_cast<const NRA::TTouchYandexWebRequest*>(request) != nullptr
        || dynamic_cast<const NRA::TPadYandexWebRequest*>(request) != nullptr
        || dynamic_cast<const NRA::TMobileAppYandexWebRequest*>(request) != nullptr;
}
/////////////////////////////// TWebTouchExtendedActionsSqueezer End

}; // NMstand
