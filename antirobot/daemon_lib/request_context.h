#pragma once

#include "cacher_factors.h"
#include "cbb_id.h"
#include "cloud_grpc_client.h"
#include "request_features.h"
#include "request_params.h"

#include <antirobot/idl/cache_sync.pb.h>

#include <utility>


namespace NAntiRobot {
    struct TEnv;

    enum class ECaptchaRedirectType {
        RegularCaptcha,
        NoRedirect,
        RandomCaptchaDeprecated, // удалить после того, как такие значения перестанут посылаться с кешера на процессор
    };

    struct TRequestContext {
        TEnv& Env;
        TAtomicSharedPtr<const TRequest> Req;
        bool IsTraining = false;
        TString CacherHost = "-";
        ECaptchaRedirectType RedirectType = ECaptchaRedirectType::NoRedirect;
        TInstant FirstArrivalTime;
        bool WasBlocked = false;
        TString BlockReason = "";
        float RobotnessHeaderValue = 0.0f;
        float Suspiciousness = 0.0f;
        bool CbbWhitelist = false;
        bool SuspiciousBan = false;
        bool CatboostWhitelist = false;
        bool Degradation = false;
        bool BanSourceIp = false;
        bool BanFWSourceIp = false;
        float CacherFormulaResult = 0.0f;
        bool CatboostBan = false;
        bool ApiAutoVersionBan = false;

        TAtomicSharedPtr<TCacherRequestFeatures> CacherFeatures;
        TRawCacherFactors CacherFactors;

        struct TMatchedRules {
            TVector<TBinnedCbbRuleKey> All;
            TVector<TBinnedCbbRuleKey> Whitelisted;
            TVector<TBinnedCbbRuleKey> Captcha;
            TVector<TBinnedCbbRuleKey> CheckboxBlacklist;
            TVector<TBinnedCbbRuleKey> CanShowCaptcha;
            TVector<TBinnedCbbRuleKey> CutRequest;
            TVector<TBinnedCbbRuleKey> NotBanned;
            TVector<TBinnedCbbRuleKey> IgnoreSpravka;
            TVector<TBinnedCbbRuleKey> MaxRobotness;
            TVector<TBinnedCbbRuleKey> Suspiciousness;
            TVector<TBinnedCbbRuleKey> Degradation;
            TVector<TBinnedCbbRuleKey> BanSourceIp;
            TVector<TBinnedCbbRuleKey> BanFWSourceIp;
            TVector<TBinnedCbbRuleKey> Mark;
            TVector<TBinnedCbbRuleKey> MarkLogOnly;
            TVector<TBinnedCbbRuleKey> UserMark;
            TVector<TBinnedCbbRuleKey> ManagedBlock;
        };

        TAtomicSharedPtr<TMatchedRules> MatchedRules;

        TRequestContext() = delete;

        TRequestContext(TEnv& env, TAtomicSharedPtr<TRequest> request)
            : Env(env)
        {
            Init(std::move(request));
        }

        TRequestContext(TEnv& env,
                        TAtomicSharedPtr<TRequest> request,
                        bool isTraining,
                        TString cacherHost,
                        ECaptchaRedirectType redirectType,
                        TInstant firstArrivalTime,
                        bool wasBlocked,
                        TString blockReason,
                        float robotnessHeaderValue,
                        float suspiciousness,
                        bool cbbWhitelist,
                        bool suspiciousBan,
                        bool catboostWhitelist,
                        bool degradation,
                        bool banSourceIp,
                        bool banFWSourceIp,
                        float cacherFormulaResult,
                        bool catboostBan,
                        bool apiAutoVersionBan)
            : Env(env)
            , IsTraining(isTraining)
            , CacherHost(std::move(cacherHost))
            , RedirectType(redirectType)
            , FirstArrivalTime(firstArrivalTime)
            , WasBlocked(wasBlocked)
            , BlockReason(std::move(blockReason))
            , RobotnessHeaderValue(robotnessHeaderValue)
            , Suspiciousness(suspiciousness)
            , CbbWhitelist(cbbWhitelist)
            , SuspiciousBan(suspiciousBan)
            , CatboostWhitelist(catboostWhitelist)
            , Degradation(degradation)
            , BanSourceIp(banSourceIp)
            , BanFWSourceIp(banFWSourceIp)
            , CacherFormulaResult(cacherFormulaResult)
            , CatboostBan(catboostBan)
            , ApiAutoVersionBan(apiAutoVersionBan)
        {
            Init(std::move(request));
        }

    private:
        void Init(TAtomicSharedPtr<TRequest> req);
        void InitCacherFactors();
        void MatchRules(TRequest* req);
    };

    TRequestContext ExtractWrappedRequest(const TRequestContext& rc);
    bool GetDegradation(const TRequestContext& rc);
} // namespace NAntiRobot
