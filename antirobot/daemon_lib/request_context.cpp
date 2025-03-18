#include "request_context.h"

#include "environment.h"
#include "fullreq_info.h"

#include <library/cpp/http/io/stream.h>

#include <util/stream/str.h>


namespace NAntiRobot {


TRequestContext ExtractWrappedRequest(const TRequestContext& rc) {
    TStringInput si(rc.Req->ContentData);
    THttpInput httpInput(&si);
    THolder<TFullReqInfo> reqInfo(new TFullReqInfo(
        httpInput,
        rc.Req->ContentData,
        rc.Req->RequesterAddr,
        rc.Env.ReloadableData,
        rc.Env.PanicFlags,
        GetSpravkaIgnorePredicate(rc.Env),
        &rc.Env.ReqGroupClassifier,
        &rc.Env
    ));

    return { rc.Env, reqInfo.Release() };
}

bool GetDegradation(const TRequestContext& rc) {
    return !rc.MatchedRules->Degradation.empty();
}

void TRequestContext::Init(TAtomicSharedPtr<TRequest> req) {
    if (req) {
        const auto reqPtr = req.Get();
        Req = std::move(req);
        InitCacherFactors();
        MatchRules(reqPtr); // Modifies req.
    }
}

void TRequestContext::InitCacherFactors() {
    TCacherRequestFeatures::TContext cacherCtx = {
        .Request = Req.Get(),
        .ReloadableData = &Env.ReloadableData,
        .Robots = Env.Robots.Get(),
        .Detector = Env.Detector.Get()
    };

    CacherFeatures = MakeAtomicShared<TCacherRequestFeatures>(
        cacherCtx,
        Env.CRD->GetFactorsCalcTimeStats()
    );

    CacherFactors.FillCacherRequestFeatures(*CacherFeatures);
}

void TRequestContext::MatchRules(TRequest* req) {
    MatchedRules = MakeAtomicShared<TMatchedRules>();

    const auto mayBanRules = Env.ServiceToMayBanFastRuleSet
        .GetByService(Req->HostType).Match(*Req, &CacherFactors);
    bool mayBanWhitelisted = false;

    for (const auto& entry : mayBanRules) {
        mayBanWhitelisted |= entry.Whitelisting;
    }

    if (mayBanWhitelisted) {
        for (const auto& entry : mayBanRules) {
            MatchedRules->Whitelisted.push_back(entry.ToBinnedKey());
        }
    } else {
        req->ForceCanShowCaptcha = !mayBanRules.empty();

        for (const auto& entry : mayBanRules) {
            MatchedRules->All.push_back(entry.ToBinnedKey());
            MatchedRules->CanShowCaptcha.push_back(entry.ToBinnedKey());
        }
    }

    const auto matchedRules = Env.FastRuleSet.Match(*Req, &CacherFactors);
    const auto matchedServiceRules = Env.ServiceToFastRuleSet
        .GetByService(Req->HostType).Match(*Req, &CacherFactors);
    const auto matchedNonBlockServiceRules = Env.ServiceToFastRuleSetNonBlock
        .GetByService(Req->HostType).Match(*Req, &CacherFactors);

    THashSet<TVector<TBinnedCbbRuleKey> TMatchedRules::*> whitelisted;

    const auto propertyMaps = {
        &Env.CbbGroupIdToProperties,
        &Env.ServiceToCbbGroupIdToProperties.GetByService(Req->HostType)
    };

    for (const auto& entry : Concatenate(
        matchedRules, matchedServiceRules, matchedNonBlockServiceRules
    )) {
        for (const auto idToProperties : propertyMaps) {
            const auto properties = idToProperties->FindPtr(entry.Key.Group);
            if (!properties) {
                continue;
            }

            for (const auto property : *properties) {
                if (entry.Whitelisting) {
                    whitelisted.insert(property);
                }
            }
        }
    }

    for (const auto& entry : Concatenate(
        matchedRules, matchedServiceRules, matchedNonBlockServiceRules
    )) {
        for (const auto idToProperties : propertyMaps) {
            const auto properties = idToProperties->FindPtr(entry.Key.Group);
            if (!properties) {
                continue;
            }

            for (const auto property : *properties) {
                if (entry.Whitelisting || whitelisted.contains(property)) {
                    MatchedRules->Whitelisted.push_back(entry.ToBinnedKey());
                } else {
                    ((*MatchedRules).*property).push_back(entry.ToBinnedKey());
                    MatchedRules->All.push_back(entry.ToBinnedKey());
                }
            }
        }
    }
}


} // namespace NAntiRobot
