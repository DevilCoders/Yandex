#include "squeezer_yuid_reqid_testid_filter.h"


namespace NMstand {

using namespace NYT;

/////////////////////////////// TYuidReqidTestidFilterActionsSqueezer
TTableSchema TYuidReqidTestidFilterActionsSqueezer::GetSchema() const {
    TTableSchema schema;
    schema.AddColumn(TColumnSchema().Name("yuid").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("ts").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("action_index").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("bucket").Type(NYT::VT_INT64));
    schema.AddColumn(TColumnSchema().Name("is_match").Type(NYT::VT_BOOLEAN));
    schema.AddColumn(TColumnSchema().Name("testid").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("servicetype").Type(NYT::VT_STRING));

    schema.AddColumn(TColumnSchema().Name("filters").Type(NYT::VT_ANY));  // TODO: type_v3 .TypeV3(NTi::List(NTi::String()))
    schema.AddColumn(TColumnSchema().Name("reqid").Type(NYT::VT_STRING));
    schema.AddColumn(TColumnSchema().Name("test_buckets").Type(NYT::VT_ANY));  // TODO: type_v3 .TypeV3(NTi::Dict(NTi::String(), NTi::Int16()))
    return schema.Strict(true);
}

ui32 TYuidReqidTestidFilterActionsSqueezer::GetVersion() const
{
    return 1;
}

bool TYuidReqidTestidFilterActionsSqueezer::CheckRequest(const NRA::TRequest* request) const
{
    return dynamic_cast<const NRA::TTouchYandexWebRequest*>(request) != nullptr
        || dynamic_cast<const NRA::TPadYandexWebRequest*>(request) != nullptr
        || dynamic_cast<const NRA::TMobileAppYandexWebRequest*>(request) != nullptr
        || dynamic_cast<const NRA::TYandexWebRequest*>(request) != nullptr;
}

void TYuidReqidTestidFilterActionsSqueezer::GetActions(TActionSqueezerArguments& args) const
{
    for (const NRA::TRequest* request : args.Container.GetRequests()) {
        if (!CheckRequest(request)) {
            continue;
        }
        try {
            NYT::TNode squeeze;
            squeeze["yuid"] = request->GetUID();
            squeeze["ts"] = request->GetTimestamp();
            squeeze["reqid"] = request->GetReqID();

            auto testBuckets = NYT::TNode::CreateMap();
            if (const auto testProps = dynamic_cast<const NRA::TTestRequestProperties*>(request)) {
                for (const auto& id : testProps->GetTestInfo()) {
                    testBuckets[id.TestID] = id.Bucket;
                }
            }
            if (testBuckets.Empty()){
                continue;
            }
            squeeze["test_buckets"] = testBuckets;

            auto filters = NYT::TNode::CreateList();
            for (const auto& item : args.Filters) {
                if (item.second.Filter(*request)) {
                    filters.Add(item.first);
                }
            }
            squeeze["filters"] = filters;

            args.ResultActions.emplace_back(squeeze, CheckExperiments(args, request));

        } catch (...) {
            Cerr << "Fail a request process: reqid: " << request->GetReqID() << Endl;
        }
    }
}
/////////////////////////////// TYuidReqidTestidFilterActionsSqueezer End

}; // NMstand
