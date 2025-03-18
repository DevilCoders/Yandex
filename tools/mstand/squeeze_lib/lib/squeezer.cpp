#include "squeezer.h"
#include <library/cpp/user_agent/browser_detector.h>


namespace NMstand {

TUserSessionsSqueezer::TUserSessionsSqueezer(const TVector<TExperimentForSqueeze>& experiments) {
    for(const auto& exp : experiments) {
        Service2Experiments[exp.Service].push_back(exp);
        HasFilters |= exp.Filters.size() > 0;
        Testids.insert(exp.Testid);
    }
}

TVector<TResultAction> TUserSessionsSqueezer::SqueezeSessions(
    const NRA::TRequestsContainer& container,
    const TVector<NYT::TNode>& rows,
    const THashMap<TString, NRA::TFilterPack>& filters,
    const THashSet<TString>& availableTestids
) {
    TVector<TResultAction> result;
    for (const auto& it : Service2Experiments) {
        TVector<TExperimentForSqueeze> possibleExperiments;
        for (const auto& exp : it.second) {
            if (availableTestids.contains(exp.Testid)) {
                possibleExperiments.push_back(exp);
            }
        }
        if (possibleExperiments.empty()) {
            continue;
        }

        auto squeezer = TSqueezerFactory::GetSqueezer(it.first);
        TActionSqueezerArguments argument(container, possibleExperiments, Hash2Filters, rows, filters);
        squeezer->GetActions(argument);

        ui32 action_index = 0;
        for (const auto& action : argument.ResultActions) {
            for (const auto& exp : argument.ResultExperiments) {
                auto isMatch = action.ExpBucketInfo.Matched.contains(exp);
                auto bucket = action.ExpBucketInfo.GetBucket(exp);
                auto extendedAction = action.Action;

                extendedAction["servicetype"] = exp.Service;
                extendedAction["testid"] = exp.Testid;
                extendedAction["is_match"] = isMatch;
                extendedAction["action_index"] = action_index;
                if (bucket >= 0) {
                    extendedAction["bucket"] = bucket;
                }

                result.push_back(TResultAction(exp, extendedAction));
            }
            action_index++;
        }
    }

    return result;
}

NRA::TFilterPack BuildFilter(const TExperimentForSqueeze& exp) {
    NRA::TFilterPack filterPack;
    for (const auto& fltr : exp.Filters) {
        auto filter = NRA::TRequestFilterFactory::Instance().Create(fltr.Name, fltr.Value);
        if (filter == nullptr) {
            ythrow yexception() << exp.ToString() << " has invalid " << fltr.ToString();
        }
        filterPack.Add(filter);
    }
    filterPack.Init();
    return filterPack;
}

void TUserSessionsSqueezer::Init() {
    NUserAgent::TBrowserDetector::Instance().InitializeUatraits("browser.xml");

    if (!HasFilters) {
        return;
    }

    NRA::TRequestFilterFactory::Instance().InitRegionsDB("geodata4.bin");

    for (const auto& it : Service2Experiments) {
        for (const auto& exp : it.second) {
            if (!Hash2Filters.contains(exp.FilterHash)) {
                Hash2Filters[exp.FilterHash] = BuildFilter(exp);
            }
        }
    }
}

const THashSet<TString>& TUserSessionsSqueezer::GetTestids() {
    return Testids;
}

};
