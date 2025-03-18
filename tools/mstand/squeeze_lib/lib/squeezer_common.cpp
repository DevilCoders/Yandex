#include "squeezer_common.h"


namespace NMstand {

NYT::TNode BuildRequestData(const NRA::TRequest* request)
{
    auto request_data = NYT::TNode::CreateMap();
    request_data["yuid"] = request->GetUID();
    request_data["ts"] = request->GetTimestamp();
    request_data["reqid"] = request->GetReqID();
    request_data["type"] = NActionType::REQUEST;

    return request_data;
}

bool TActionsSqueezer::IsTestidMatch(const NRA::TRequest* request, const TExperimentForSqueeze& exp) const {
    if (exp.IsAllUsers() || exp.IsHistoryMode || exp.IsBro()) {
        return true;
    }
    if (const auto tst = dynamic_cast<const NRA::TTestRequestProperties*>(request)) {
        return tst->HasTestID(exp.Testid);
    }
    return false;
}

bool TActionsSqueezer::IsFilterMatch(const NRA::TRequest* request, const TExperimentForSqueeze& exp, const THashMap<TString, NRA::TFilterPack>& hash2Filters) const {
    if (exp.FilterHash.empty() || !hash2Filters.contains(exp.FilterHash)) {
        return true;
    }
    return hash2Filters.at(exp.FilterHash).Filter(*request);
}

i32 TActionsSqueezer::GetBucket(const NRA::TRequest* request, const TExperimentForSqueeze& exp) const {
    if (const auto tst = dynamic_cast<const NRA::TTestRequestProperties*>(request)) {
        return tst->GetBucketByTestID(exp.Testid);
    }
    return -1;
}

TExpBucketInfo TActionsSqueezer::CheckExperiments(TActionSqueezerArguments& args, const NRA::TRequest* request) const {
    TExpBucketInfo result;
    for (const auto& exp : args.Experiments) {
        result.Buckets[exp] = GetBucket(request, exp);
        if (IsTestidMatch(request, exp)) {
            args.ResultExperiments.insert(exp);
            if(IsFilterMatch(request, exp, args.Hash2Filters)) {
                result.Matched.insert(exp);
            }
        }
    }
    return result;
}

};
