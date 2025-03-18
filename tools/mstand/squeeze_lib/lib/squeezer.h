#pragma once

#include "squeezer_market_search_sessions.h"
#include "squeezer_web_extended.h"
#include "squeezer_web_surveys.h"
#include "squeezer_web.h"
#include "squeezer_video.h"
#include "squeezer_yuid_reqid_testid_filter.h"

#include <quality/user_sessions/request_aggregate_lib/all.h>


namespace NMstand {

class TUserSessionsSqueezer
{
public:
    TUserSessionsSqueezer() = default;
    explicit TUserSessionsSqueezer(const TVector<TExperimentForSqueeze>& experimentIndexes);
    TVector<TResultAction> SqueezeSessions(
        const NRA::TRequestsContainer& container,
        const TVector<NYT::TNode>& rows,
        const THashMap<TString, NRA::TFilterPack>& filters,
        const THashSet<TString>& availableTestids
    );
    void Init();
    const THashSet<TString>& GetTestids();

private:
    THashMap<TString, TVector<TExperimentForSqueeze>> Service2Experiments;
    bool HasFilters = false;
    THashMap<TString, NRA::TFilterPack> Hash2Filters;
    THashSet<TString> Testids;

public:
    Y_SAVELOAD_DEFINE(Service2Experiments, HasFilters, Testids);
};

class TSqueezerFactory
{
public:
    static std::shared_ptr<TActionsSqueezer> GetSqueezer(const TString& service)
    {
        if (service == NServiceType::WEB) {
            return std::make_shared<TWebDesktopActionsSqueezer>();
        }
        if (service == NServiceType::TOUCH) {
            return std::make_shared<TWebTouchActionsSqueezer>();
        }
        if (service == NServiceType::WEB_DESKTOP_EXTENDED) {
            return std::make_shared<TWebDesktopExtendedActionsSqueezer>();
        }
        if (service == NServiceType::WEB_TOUCH_EXTENDED) {
            return std::make_shared<TWebTouchExtendedActionsSqueezer>();
        }
        if (service == NServiceType::MARKET_SEARCH_SESSIONS) {
            return std::make_shared<TMarketSearchSessionsActionsSqueezer>();
        }
        if (service == NServiceType::YUID_REQID_TESTID_FILTER) {
            return std::make_shared<TYuidReqidTestidFilterActionsSqueezer>();
        }
        if (service == NServiceType::WEB_SURVEYS) {
            return std::make_shared<TWebSurveysActionsSqueezer>();
        }
        if (service == NServiceType::VIDEO) {
            return std::make_shared<TVideoActionsSqueezer>();
        }
        ythrow yexception() << "Squeezer for " << service << " does not exist!";
    }
};

};
