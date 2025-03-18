#pragma once

#include "amnesty_flags.h"
#include "antirobot_cookie.h"
#include "antirobot_experiment.h"
#include "autoru_offer.h"
#include "backend_sender.h"
#include "blocker.h"
#include "cacher_robot_detector.h"
#include "captcha_stat.h"
#include "cbb.h"
#include "cbb_banned_ip_holder.h"
#include "cbb_cache.h"
#include "cbb_iplist_manager.h"
#include "cbb_list_watcher.h"
#include "cbb_relist_manager.h"
#include "cbb_textlist_manager.h"
#include "china_redirect_stats.h"
#include "cloud_grpc_client.h"
#include "cpu_usage.h"
#include "disabling_flags.h"
#include "disabling_stat.h"
#include "exp_bin.h"
#include "forwarding_stats.h"
#include "geo_checker.h"
#include "many_requests.h"
#include "partners_captcha_stat.h"
#include "reloadable_data.h"
#include "request_context.h"
#include "robot_detector.h"
#include "robot_set.h"
#include "rule_set.h"
#include "search_engine_recognizer.h"
#include "server_error_stats.h"
#include "server_exceptions_stat.h"
#include "server_time_stat.h"
#include "service_param_holder.h"
#include "stat.h"
#include "time_stats.h"
#include "tvm.h"
#include "unified_agent_log.h"
#include "user_base.h"
#include "user_metric.h"
#include "user_name_list.h"
#include "verochka_stats.h"
#include "ydb_session_storage.h"

#include <antirobot/lib/alarmer.h>
#include <antirobot/lib/data_process_queue.h>
#include <antirobot/lib/ip_list.h>
#include <antirobot/lib/ip_map.h>
#include <antirobot/lib/mtp_queue_decorators.h>
#include <antirobot/lib/preemptive_mtp_queue.h>
#include <antirobot/lib/regex_detector.h>
#include <antirobot/lib/stats_writer.h>
#include <antirobot/lib/hypocrisy/hypocrisy.h>

#include <metrika/uatraits/include/uatraits/detector.hpp>

#include <library/cpp/http/misc/httpcodes.h>
#include <library/cpp/http/server/http.h>
#include <library/cpp/lcookie/lcookie.h>
#include <library/cpp/threading/synchronized/synchronized.h>

#include <util/generic/ptr.h>
#include <util/system/rwlock.h>
#include <util/thread/lfqueue.h>

#include <atomic>

namespace NLCookie {
struct IKeychain;
}

namespace NAntiRobot {
class IRobotDetector;
class TIsBlocked;
class TBlockResponsesStats;
class TRpsFilter;
class TUnifiedAgentEventLog;

using TStatPrinter = std::function<void(TStatsWriter&)>;
using TStatPrinters = TVector<TStatPrinter>;

struct TCaptchaInputInfo {
    TRequestContext Context;
    bool IsCorrect;
};

struct TEnv {
    TAlarmer Alarmer;
    TTimeStats TimeStats;
    TTimeStats TimeStatsCaptcha;
    TTimeStats TimeStatsHandle;
    TServerTimeStat TimeStatsHandleByService;
    TTimeStats TimeStatsRead;
    TTimeStats TimeStatsWait;
    TTimeStats ProcessServerTimeStats;
    TTimeStats ProcessServerTimeStatsHandle;
    TTimeStats ProcessServerTimeStatsRead;
    TTimeStats ProcessServerTimeStatsWait;
    TTimeStats ProcessServerTimeStatsProcessing;
    TRequestGroupClassifier ReqGroupClassifier;
    THolder<TBlockResponsesStats> BlockResponsesStats;
    TVector<TSimpleSharedPtr<THttpServer>> Servers;
    TUserBase UserBase;
    TRpsFilter RpsFilter;
    TVerochkaStats VerochkaStats;
    TChinaRedirectStats ChinaRedirectStats;
    TServerExceptionsStats ServerExceptionsStats;
    TServerErrorStats ServerErrorStats;
    TServerErrorFlags ServerErrorFlags;
    TAntirobotDisableExperimentsFlag AntirobotDisableExperimentsFlag;
    TManyRequestsStats ManyRequestsStats;
    TSuspiciousFlags SuspiciousFlags;
    TUserNameList NonBrandedPartners;
    THolder<TCaptchaStat> CaptchaStat;
    TPartnersCaptchaStat PartnersCaptchaStat;
    TSearchEngineRecognizer SearchEngineRecognizer;

    TAtomicSharedPtr<TPreemptiveMtpQueue> ProcessingQueue;
    TAtomicSharedPtr<TThreadPool> InternalTasksQueue;

    TReloadableData ReloadableData;
    TAtomicSharedPtr<TRobotSet> Robots;
    TAtomicSharedPtr<TRobotSet> UniqueLcookies;

    NThreading::TSynchronized<const TString> RobotUidsDumpFile;
    NThreading::TSynchronized<const TString> BlocksDumpFile;

    TAtomicSharedPtr<TBlocker> Blocker;
    TAlarmTaskHolder RemoveExpired;
    TAlarmTaskHolder SaveCheckPoint;

    TServiceParamHolder<THolder<TIsBlocked>> IsBlocked;
    THolder<IRobotDetector> RD;
    THolder<TCacherRobotDetector> CRD;
    THolder<TAutoruOfferDetector> AutoruOfferDetector;

    THashMap<
        TCbbGroupId,
        TVector<TVector<TBinnedCbbRuleKey> TRequestContext::TMatchedRules::*>
    > CbbGroupIdToProperties;
    TServiceParamHolder<
        THashMap<
            TCbbGroupId,
            TVector<TVector<TBinnedCbbRuleKey> TRequestContext::TMatchedRules::*>
        >
    > ServiceToCbbGroupIdToProperties;
    TIncrementalRuleSet FastRuleSet;
    TServiceParamHolder<TIncrementalRuleSet> ServiceToMayBanFastRuleSet;
    TServiceParamHolder<TIncrementalRuleSet> ServiceToFastRuleSet;
    TServiceParamHolder<TIncrementalRuleSet> ServiceToFastRuleSetNonBlock;

    TLockFreeQueue<TAddr> BanFWAddrs;
    TAlarmTaskHolder CbbAddBanFWSourceIp;

    TVector<bool> YqlHasBlockRules;
    TVector<TCbbRuleId> YqlRuleIds;
    TRuleSet YqlCbbRuleSet;

    THolder<TAntirobotTvm> Tvm;

    TRefreshableAddrSet NonBlockingAddrs;
    TCbbIpListManager CbbIpListManager;
    TCbbReListManager CbbReListManager;
    TCbbTextListManager CbbTextListManager;
    TCbbListWatcher CbbListWatcher;
    TAtomicSharedPtr<TCbbCache> CbbCache;
    THolder<ICbbIO> CbbIO;
    THolder<ICbbIO> CbbCacheIO;
    TAlarmer RuleSetMerger;

    TAlarmTaskHolder SaveCbbCacheAlarmer;
    TAlarmTaskHolder CbbListWatcherPoller;

    TCbbBannedIpHolder CbbBannedIpHolder;
    std::function<bool(const TUid&)> NonBlockingChecker;

    TAtomicSharedPtr<TFailCountingQueue> ProcessorResponseApplyQueue;

    /* ProcessingQueue must be destroyed before all the data to ensure that remaining
     * request are processed properly. That's why it is placed the last in the list
     * of fields. */
    THolder<TDataProcessQueue<TRequestContext>> RequestProcessingQueue;
    THolder<TDataProcessQueue<TCaptchaInputInfo>> CaptchaInputProcessingQueue;

    TDisablingFlags DisablingFlags;
    TDisablingStat DisablingStat;

    TIpRangeMap<size_t> CustomHashingMap;
    TBackendSender BackendSender;
    TAlarmTaskHolder DiscoveryStartTask;

    TCpuUsage CpuUsage;
    TAlarmTaskHolder CpuAlarmer;

    TPanicFlags PanicFlags;

    TAmnestyFlags AmnestyFlags;

    enum class ECounter {
        Page400s                /* "page_400" */,
        Page404s                /* "page_404" */,
        ReadReqTimeouts         /* "read_req_timeouts" */,
        ReplyFails              /* "reply_fails" */,
        UnknownServiceHeaders   /* "unknown_service_headers" */,
        ValidAutoRuTampers      /* "valid_autoru_tampers" */,
        CaptchaCheckErrors      /* "captcha_check_errors" */,
        CaptchaCheckBadRequests /* "captcha_check_bad_requests" */,
        FuryCheckErrors         /* "fury_check_errors" */,
        FuryPreprodCheckErrors  /* "fury_preprod_check_errors" */,
        Count
    };

    TCategorizedStats<std::atomic<size_t>, ECounter> Counters;

    enum class EServiceCounter {
        CrawlerRequests  /* "crawler_requests" */,
        YandexRequests   /* "yandex_requests" */,
        Count
    };

    TCategorizedStats<
        std::atomic<size_t>, EServiceCounter,
        EHostType
    > ServiceCounters;

    enum class EServiceExpBinCounter {
        BanSourceIp  /* "ban_source_ip" */,
        CutRequests  /* "cut_requests_by_cbb" */,
        BanFWSourceIp  /* "ban_fw_source_ip" */,
        Count
    };

    TCategorizedStats<
        std::atomic<size_t>, EServiceExpBinCounter,
        EHostType, EExpBin
    > ServiceExpBinCounters;

    enum class EServiceNsGroupCounter {
        WithSuspiciousnessTrustedUsersRequests  /* "with_suspiciousness_requests_trusted_users" */,
        RequestsPassedToService                 /* "requests_passed_to_service_req" */,
        WithDegradationRequests                 /* "with_degradation_requests_req" */,
        WithSuspiciousnessRequests              /* "with_suspiciousness_requests_req" */,
        Count
    };

    TCategorizedStats<
        std::atomic<size_t>, EServiceNsGroupCounter,
        EHostType, ESimpleUidType, EReqGroup
    > ServiceNsGroupCounters;

    enum class EServiceNsExpBinCounter {
        RequestsPassedToService                 /* "requests_passed_to_service" */,
        WithDegradationRequests                 /* "with_degradation_requests" */,
        WithSuspiciousnessRequests              /* "with_suspiciousness_requests" */,
        Count
    };

    TCategorizedStats<
        std::atomic<size_t>, EServiceNsExpBinCounter,
        EHostType, ESimpleUidType, EExpBin
    > ServiceNsExpBinCounters;

    TAtomic IsChinaRedirectEnabled;
    TAtomic IsChinaUnauthorizedEnabled;

    THolder<uatraits::detector> Detector;
    THolder<ISpravkaSessionsStorage> SpravkaSessionsStorage;
    THolder<ICloudApiClient> CloudApiClient;

    TString YascKey;
    TRuleSet LastVisitsRuleSet;
    THashSet<TLastVisitsCookie::TRuleId> LastVisitsIds;

    TString BalancerJwsKeyId;
    TString BalancerJwsKey;
    TString NarwhalJwsKeyId;
    TString NarwhalJwsKey;
    TString MarketJwsKey;
    TString AutoruOfferSalt;
    TString AutoRuTamperSalt;

    std::atomic<size_t> RSWeight;

    TUnifiedAgentBillingLog BillingLogJson;
    TUnifiedAgentResourceMetrics ResourceMetricsLog;

    enum class EJwsCounter {
        Invalid              /* "invalid_jws" */,
        UnknownValid         /* "unknown_valid_jws" */,
        AndroidValid         /* "android_valid_jws" */,
        IosValid             /* "ios_valid_jws" */,
        AndroidValidExpired  /* "android_valid_expired_jws" */,
        IosValidExpired      /* "ios_valid_expired_jws" */,
        Default              /* "default_jws" */,
        DefaultExpired       /* "default_expired_jws" */,
        AndroidSusp          /* "android_susp_jws" */,
        IosSusp              /* "ios_susp_jws" */,
        AndroidSuspExpired   /* "android_susp_expired_jws" */,
        IosSuspExpired       /* "ios_susp_expired_jws" */,
        Count
    };

    enum class EYandexTrustCounter {
        Invalid              /* "invalid_yandex_trust" */,
        Valid                /* "valid_yandex_trust" */,
        ValidExpired         /* "valid_expired_yandex_trust" */,
        Count
    };

    TCategorizedStats<std::atomic<size_t>, EJwsCounter, EHostType> JwsCounters;
    TCategorizedStats<std::atomic<size_t>, EYandexTrustCounter, EHostType> YandexTrustCounters;

    NHypocrisy::TBundle HypocrisyBundle;
    TVector<TString> BrotliHypocrisyInstances;
    TVector<TString> GzipHypocrisyInstances;

    TEnv();
    ~TEnv();

    void ForwardRequest(const TRequestContext& rc);
    void ForwardCaptchaInput(const TRequestContext& rc, bool isCorrect);

    void PrintStats(TStatsWriter& out) const;
    void PrintStatsLW(TStatsWriter& out, EStatType service);
    void InitReloadable();
    void LoadBadUserAgents();
    void LoadLKeychain();
    void LoadIpLists();
    void LoadKeyRing();
    void LoadYandexTrustKeyRing();
    void LoadYascKey();
    void LoadSpravkaDataKey();
    void LoadAutoruOfferSalt();
    void LoadDictionaries();
    void LoadClassifierRules(const TAntirobotDaemonConfig& config);
    void IncreaseTimeouts();
    void IncreaseUnknownServiceHeaders();
    void IncreaseReplyFails();
    void Log400Event(const TRequest& req, HttpCodes code, const TString& reason);
    void EnableChinaRedirect();
    void DisableChinaRedirect();
    void EnableChinaUnauthorized();
    void DisableChinaUnauthorized();

    void AddStatPrinter(TStatPrinter statPrinter) {
        StatPrinters.push_back(std::move(statPrinter));
    }

private:
    void UpdateListManagers();
    void StartDiscovery();

private:
    TStatPrinters StatPrinters;
};

static_assert(sizeof(TEnv) < 800000, "TEnv is too large. It may cause stack overflow errors");

} /* namespace NAntiRobot */
