#include "environment.h"

#include "blocker.h"
#include "config_global.h"
#include "dynamic_thread_pool.h"
#include "eventlog_err.h"
#include "forward_request.h"
#include "json_config.h"
#include "match_rule_parser.h"
#include "neh_requesters.h"
#include "request_context.h"
#include "rule_set.h"
#include "yql_rule_set.h"

#include <antirobot/idl/antirobot.ev.pb.h>
#include <antirobot/lib/http_request.h>
#include <antirobot/lib/keyring.h>
#include <antirobot/lib/mtp_queue_decorators.h>
#include <antirobot/lib/range.h>
#include <antirobot/lib/spravka_key.h>
#include <antirobot/lib/yandex_trust_keyring.h>

#include <library/cpp/blockcodecs/codecs.h>
#include <library/cpp/iterator/concatenate.h>
#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/iterator/mapped.h>
#include <library/cpp/iterator/zip.h>
#include <library/cpp/lcookie/lkey_reader.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/is_in.h>
#include <util/generic/maybe.h>
#include <util/generic/xrange.h>
#include <util/generic/yexception.h>
#include <util/stream/zlib.h>
#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/strip.h>

namespace NAntiRobot {

namespace {

struct TRpsAndFailCounter {
    TAtomicSharedPtr<TRpsCountingQueue> Rps;
    TAtomicSharedPtr<TFailCountingQueue> Fail;

    TRpsAndFailCounter(TAtomicSharedPtr<IThreadPool> slave)
        : Rps(new TRpsCountingQueue(std::move(slave)))
        , Fail(new TFailCountingQueue(Rps))
    {
    }
};

struct TRpsAndFailCounters {
    TRpsAndFailCounter ProcessingReqs;
    TRpsAndFailCounter ProcessingCaptchaInputs;
};

TAtomicSharedPtr<TPreemptiveMtpQueue> CreateQueue(const TAntirobotDaemonConfig::TMtpQueueParams& params)
{
    auto result = MakeAtomicShared<TPreemptiveMtpQueue>(GetAntiRobotDynamicThreadPool(), params.ThreadsMax);
    result->Start(params.ThreadsMin, params.QueueSize);

    return result;
}

THolder<TDataProcessQueue<TRequestContext>> CreateRequestProcessingQueue(TAtomicSharedPtr<IThreadPool> slave, TTimeStats& timeStats) {
    auto requestProcessor = [&timeStats](const TRequestContext& rc) {
        TMeasureDuration measureDuration{timeStats};
        rc.Env.RD->ProcessRequest(rc);
    };

    return MakeHolder<TDataProcessQueue<TRequestContext>>(std::move(slave), requestProcessor);
}

THolder<TDataProcessQueue<TCaptchaInputInfo>> CreateCaptchaInputProcessingQueue(TAtomicSharedPtr<IThreadPool> slave) {
    auto requestProcessor = [](const TCaptchaInputInfo& info) {
        info.Context.Env.RD->ProcessCaptchaInput(info.Context, info.IsCorrect);
    };

    return MakeHolder<TDataProcessQueue<TCaptchaInputInfo>>(std::move(slave), requestProcessor);
}

THolder<TBlocker> CreateBlocker(TBlocker::TNonBlockingChecker nonblockingChecker,
                                const TString& checkpointFilename,
                                TAtomicSharedPtr<IThreadPool> updateQueue)
{
    TBlockSet blockSet;
    try {
        TIFStream in(checkpointFilename);
        in >> blockSet;
    } catch (yexception&) {
        Cerr << "Failed to load blocks checkpoint: " << CurrentExceptionMessage() << '\n';
    }
    return MakeHolder<TBlocker>(nonblockingChecker, std::move(blockSet), std::move(updateQueue));
}

THolder<TRobotSet> CreateRobotSet(const TString& checkpointFilename) {
    THolder<TRobotSet> robotSet;

    if (checkpointFilename.empty()) {
        robotSet = MakeHolder<TRobotSet>();
    } else {
        try {
            TFileInput input(checkpointFilename);
            robotSet = MakeHolder<TRobotSet>(&input);
        } catch (yexception&) {
            Cerr << "Failed to load robots checkpoint: " << CurrentExceptionMessage() << '\n';
            robotSet = MakeHolder<TRobotSet>();
        }
    }

    for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
        robotSet->UnsafeInherit(
            static_cast<EHostType>(i),
            ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].InheritBans
        );
    }

    return robotSet;
}

void WorkQueueTasksStatPrinterImpl(TStatsWriter& stats, TRpsAndFailCounter rpsFailCounter) {
    auto rpsStat = rpsFailCounter.Rps->GetRps();
    float processedDiff = static_cast<float>(rpsStat.Processed - rpsStat.ProcessedPrev);
    float timeDiffMilliseconds = static_cast<float>(rpsStat.Time.MilliSeconds() - rpsStat.TimePrev.MilliSeconds());

    stats.WriteScalar("slow_processed", rpsStat.Processed)
         .WriteScalar("slow_sum_time", rpsStat.Time.Seconds())
         .WriteHistogram("slow_queue_rps", timeDiffMilliseconds > 0 ? processedDiff * 1000 / timeDiffMilliseconds : 0.0f)
         .WriteScalar("spilled_reqs", rpsFailCounter.Fail->GetFailedAdditionsCount())
         .WriteHistogram("num_tasks", rpsFailCounter.Rps->Size());
    rpsFailCounter.Rps->UpdatePrevData();
}

TStatPrinter CreateWorkQueueStatPrinter(TAtomicSharedPtr<TPreemptiveMtpQueue> processingQueue,
                                        TAtomicSharedPtr<TPreemptiveMtpQueue> processorResponseApplyQueue,
                                        TAtomicSharedPtr<TFailCountingQueue> failCountingResponseProcessingQueue,
                                        const TRpsAndFailCounters& rpsFailCounters) {
    return [=](TStatsWriter& out) {
        auto processingStats = out.WithPrefix("processing_queue.");
        processingQueue->PrintStatistics(processingStats);

        auto processorResponseApplyStats = out.WithPrefix("processor_response_apply_queue.");
        processorResponseApplyQueue->PrintStatistics(processorResponseApplyStats);

        const auto spilledApplies = failCountingResponseProcessingQueue->GetFailedAdditionsCount();
        processorResponseApplyStats.WriteScalar("spilled_applies", spilledApplies);

        auto processingReqStats = processingStats.WithPrefix("processing_reqs.");
        WorkQueueTasksStatPrinterImpl(processingReqStats, rpsFailCounters.ProcessingReqs);

        auto processingCaptchaInputStats = processingStats.WithPrefix("processing_captcha_inputs.");
        WorkQueueTasksStatPrinterImpl(processingCaptchaInputStats, rpsFailCounters.ProcessingCaptchaInputs);
    };
}

TString LoadPlainJwsKey(const TString& path) {
    TFileInput file(path);
    auto encodedKey = file.ReadAll();
    StripInPlace(encodedKey);
    return Base64StrictDecode(encodedKey);
}

std::pair<TString, TString> LoadParametrizedJwsKey(const TString& path) {
    TFileInput file(path);
    auto content = file.ReadAll();
    StripInPlace(content);

    TStringBuf id;
    TStringBuf algorithm;
    TStringBuf encodedKey;

    try {
        ParseSplitTokensInto(StringSplitter(content).Split(':'), &id, &algorithm, &encodedKey);
    } catch (const std::exception& exc) {
        ythrow yexception() << "Failed to parse JWS key: " << path << ": " << exc.what();
    }

    Y_ENSURE(
        algorithm == "HS256",
        "Bad JWS key algorithm: " << path << ": " << algorithm
    );

    try {
        return {TString(id), HexDecode(encodedKey)};
    } catch (const std::exception& exc) {
        ythrow yexception() << "Failed to parse JWS key: " << path << ": " << exc.what();
    }
}

} // anonymous namespace

TEnv::TEnv()
    : TimeStats(TIME_STATS_VEC_CACHER, TString())
    , TimeStatsCaptcha(TIME_STATS_VEC_CACHER, "captcha_")
    , TimeStatsHandle(TIME_STATS_VEC_CACHER, "handle_")
    , TimeStatsHandleByService(TIME_STATS_VEC_CACHER, "handle_")
    , TimeStatsRead(TIME_STATS_VEC_CACHER, "read_")
    , TimeStatsWait(TIME_STATS_VEC_CACHER, "wait_")
    , ProcessServerTimeStats(TIME_STATS_VEC, TString())
    , ProcessServerTimeStatsHandle(TIME_STATS_VEC, "handle_")
    , ProcessServerTimeStatsRead(TIME_STATS_VEC, "read_")
    , ProcessServerTimeStatsWait(TIME_STATS_VEC, "wait_")
    , ProcessServerTimeStatsProcessing(TIME_STATS_VEC, "processing_")
    , ReqGroupClassifier(ANTIROBOT_DAEMON_CONFIG.JsonConfig.GetReqGroupRegExps())
    , BlockResponsesStats(new TBlockResponsesStats(ReqGroupClassifier.GetServiceToNumGroup()))
    , UserBase(ANTIROBOT_DAEMON_CONFIG.RuntimeDataDir)
    , RpsFilter(ANTIROBOT_DAEMON_CONFIG.RpsFilterSize, ANTIROBOT_DAEMON_CONFIG.RpsFilterMinSafeInterval, ANTIROBOT_DAEMON_CONFIG.RpsFilterRememberForInterval)
    , CaptchaStat(MakeHolder<TCaptchaStat>(ReqGroupClassifier.GetServiceToNumGroup()))
    , RobotUidsDumpFile(ANTIROBOT_DAEMON_CONFIG.RobotUidsDumpFile)
    , BlocksDumpFile(ANTIROBOT_DAEMON_CONFIG.BlocksDumpFile)
    , FastRuleSet({}, nullptr, &CbbIpListManager, &CbbReListManager)
    , Tvm(new TAntirobotTvm(
        ANTIROBOT_DAEMON_CONFIG.UseTVMClient,
        ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService ? ANTIROBOT_DAEMON_CONFIG.TVMClientsList : ""
    ))
    , CbbCache(ANTIROBOT_DAEMON_CONFIG.CbbEnabled ? new TCbbCache(ANTIROBOT_DAEMON_CONFIG.CbbCacheFile) : nullptr)
    , CbbIO(ANTIROBOT_DAEMON_CONFIG.CbbEnabled ? new TCbbIO(TCbbIO::TOptions{
                                                                        ANTIROBOT_DAEMON_CONFIG.CbbApiHost,
                                                                        ANTIROBOT_DAEMON_CONFIG.CbbApiTimeout,
                                                                        CbbCache.Get()}, Tvm.Get())
                                               : nullptr)
    , CbbCacheIO(ANTIROBOT_DAEMON_CONFIG.CbbEnabled ? new TCbbCacheIO(CbbCache.Get()) : nullptr)
    , BackendSender(
        ANTIROBOT_DAEMON_CONFIG.DiscoveryFrequency,
        ANTIROBOT_DAEMON_CONFIG.DiscoveryCacheDir,
        ANTIROBOT_DAEMON_CONFIG.ForwardRequestTimeout,
        ANTIROBOT_DAEMON_CONFIG.DeadBackendSkipperMaxProbability,
        ANTIROBOT_DAEMON_CONFIG.DiscoveryHost,
        ANTIROBOT_DAEMON_CONFIG.DiscoveryPort,
        ANTIROBOT_DAEMON_CONFIG.BackendEndpointSets,
        ANTIROBOT_DAEMON_CONFIG.ExplicitBackends
    )
    , ServiceNsGroupCounters(ReqGroupClassifier.GetServiceToNumGroup())
    , SpravkaSessionsStorage(CreateSpravkaSessionsStorage())
    , CloudApiClient(CreateCloudApiClient())
    , RSWeight(10)
    , BillingLogJson(ANTIROBOT_DAEMON_CONFIG.UnifiedAgentUri)
    , ResourceMetricsLog(ANTIROBOT_DAEMON_CONFIG.UnifiedAgentUri)
{
    ProcessingQueue = CreateQueue(ANTIROBOT_DAEMON_CONFIG.ProcessingQueueParams);
    auto slaveProcessorResponseApplyQueue = CreateQueue(ANTIROBOT_DAEMON_CONFIG.ProcessorResponseApplyQueueParams);
    ProcessorResponseApplyQueue = MakeAtomicShared<TFailCountingQueue>(slaveProcessorResponseApplyQueue);
    InternalTasksQueue = MakeAtomicShared<TThreadPool>();
    InternalTasksQueue->Start(1);

    // https://rb.yandex-team.ru/arc/r/209575 (CAPTHA-876)
    const TRpsAndFailCounter processingQueueCounter(ProcessingQueue);
    const TRpsAndFailCounters rpsFailCounters{
        {processingQueueCounter.Fail},
        {processingQueueCounter.Fail},
    };

    StatPrinters.push_back(CreateWorkQueueStatPrinter(ProcessingQueue, slaveProcessorResponseApplyQueue, ProcessorResponseApplyQueue, rpsFailCounters));

    RequestProcessingQueue = CreateRequestProcessingQueue(rpsFailCounters.ProcessingReqs.Fail, ProcessServerTimeStatsProcessing);
    CustomHashingMap = ParseCustomHashingRules(ANTIROBOT_DAEMON_CONFIG.CustomHashingRules);

    CaptchaInputProcessingQueue = CreateCaptchaInputProcessingQueue(rpsFailCounters.ProcessingCaptchaInputs.Fail);

    auto GetAddrList = [this](const TVector<TCbbGroupId>& cbbGroup) {
        return CbbIO ? CbbIpListManager.Add(cbbGroup) : MakeRefreshableAddrSet();
    };

    NonBlockingAddrs = GetAddrList({ANTIROBOT_DAEMON_CONFIG.CbbFlagNonblocking});
    auto externalBlocked = GetAddrList({ANTIROBOT_DAEMON_CONFIG.CbbFlag});
    auto ipBasedIdentificationsBanList = GetAddrList({ANTIROBOT_DAEMON_CONFIG.CbbFlagIpBasedIdentificationsBan});
    std::array<TVector<TCbbGroupId>, HOST_NUMTYPES> farmableFlags;
    std::array<TRefreshableAddrSet, HOST_NUMTYPES> farmableIdentificationsBanListByHosts;
    std::array<TRefreshableAddrSet, HOST_NUMTYPES> manualBlockListByHosts;
    for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
        farmableFlags[i] = ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].CbbFlagsFarmableIdentificationsBan;

        farmableIdentificationsBanListByHosts[i] = GetAddrList(
            ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].CbbFlagsFarmableIdentificationsBan
        );

        manualBlockListByHosts[i] = GetAddrList(
            ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].CbbFlagsManual
        );
    }

    NonBlockingChecker = [&nonBlockingAddrs = NonBlockingAddrs](const TUid& uid){
        return uid.IpBased() && nonBlockingAddrs->Get()->ContainsActual(uid.ToAddr());
    };

    Blocker = CreateBlocker(
        NonBlockingChecker,
        ANTIROBOT_DAEMON_CONFIG.BlocksDumpFile, InternalTasksQueue
    );

    Robots = CreateRobotSet(ANTIROBOT_DAEMON_CONFIG.RobotUidsDumpFile);
    UniqueLcookies = CreateRobotSet("");

    for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
        IsBlocked.GetByService(i).Reset(new TIsBlocked(*Blocker, manualBlockListByHosts[i], NonBlockingChecker));
    }

    RD.Reset(CreateRobotDetector(*this, IsBlocked.GetArray(), *Blocker, CbbIO.Get()));
    CRD.Reset(new TCacherRobotDetector);

    CbbBannedIpHolder = TCbbBannedIpHolder(
        ANTIROBOT_DAEMON_CONFIG.CbbFlagIpBasedIdentificationsBan,
        farmableFlags,
        ipBasedIdentificationsBanList,
        farmableIdentificationsBanListByHosts
    );

    InitReloadable();
    LoadIpLists();

    AutoruOfferDetector.Reset(new TAutoruOfferDetector(AutoruOfferSalt));

    TStringInput si(ANTIROBOT_CONFIG.NonBrandedPartners);
    NonBrandedPartners.Load(si);

    if (CbbIO) {
        for (auto& ruleSet : Concatenate(
            ServiceToMayBanFastRuleSet.GetArray(),
            ServiceToFastRuleSet.GetArray()
        )) {
            ruleSet.Reset(nullptr, &CbbIpListManager, &CbbReListManager);
        }

        for (auto& ruleSet : ServiceToFastRuleSetNonBlock.GetArray()) {
            ruleSet.Reset(NonBlockingAddrs, &CbbIpListManager, &CbbReListManager);
        }

        for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
            const auto serviceStr = ToString(static_cast<EHostType>(i));

            CbbTextListManager.Add(
                ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].CbbFlagsCanShowCaptcha,
                "may ban rule set for " + serviceStr,
                &ServiceToMayBanFastRuleSet.GetByService(i)
            );

            auto ids = Concatenate(
                ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].CbbFlagsCaptchaByRegexp,
                ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].CbbFlagsCheckboxBlacklist
            );

            CbbTextListManager.Add(
                {ids.begin(), ids.end()},
                "rule set for " + serviceStr,
                &ServiceToFastRuleSet.GetByService(i)
            );

            auto nonBlockIds = Concatenate(
                ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].CbbFlagsDDosControl,
                ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].CbbFlagsDDosControlMarkOnly,
                ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].CbbFlagsDDosControlMarkAndLogOnly,
                ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].CbbFlagsDDosControlUserMark
            );

            CbbTextListManager.Add(
                {nonBlockIds.begin(), nonBlockIds.end()},
                "nonblock rule set for " + serviceStr,
                &ServiceToFastRuleSetNonBlock.GetByService(i)
            );

            const auto groupIdsProperties = {
                std::pair{
                    &ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].CbbFlagsCaptchaByRegexp,
                    &TRequestContext::TMatchedRules::Captcha
                },
                {
                    &ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].CbbFlagsCheckboxBlacklist,
                    &TRequestContext::TMatchedRules::CheckboxBlacklist
                },
                {
                    &ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].CbbFlagsDDosControl,
                    &TRequestContext::TMatchedRules::ManagedBlock
                },
                {
                    &ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].CbbFlagsDDosControlMarkOnly,
                    &TRequestContext::TMatchedRules::Mark
                },
                {
                    &ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].CbbFlagsDDosControlMarkAndLogOnly,
                    &TRequestContext::TMatchedRules::MarkLogOnly
                },
                {
                    &ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].CbbFlagsDDosControlUserMark,
                    &TRequestContext::TMatchedRules::UserMark
                }
            };

            for (const auto& [groupIds, property] : groupIdsProperties) {
                for (const auto groupId : *groupIds) {
                    ServiceToCbbGroupIdToProperties.GetByService(i)[groupId].push_back(property);
                }
            }
        }

        CbbTextListManager.Add(
            {
                ANTIROBOT_DAEMON_CONFIG.CbbFlagNotBanned,
                ANTIROBOT_DAEMON_CONFIG.CbbFlagCutRequests,
                ANTIROBOT_DAEMON_CONFIG.CbbFlagIgnoreSpravka,
                ANTIROBOT_DAEMON_CONFIG.CbbFlagMaxRobotness,
                ANTIROBOT_DAEMON_CONFIG.CbbFlagSuspiciousness,
                ANTIROBOT_DAEMON_CONFIG.CbbFlagDegradation,
                ANTIROBOT_DAEMON_CONFIG.CbbFlagBanSourceIp,
                ANTIROBOT_DAEMON_CONFIG.CbbFlagBanFWSourceIp,
            },
            "common rule set",
            &FastRuleSet
        );

        const auto groupIdsProperties = {
            std::pair{ANTIROBOT_DAEMON_CONFIG.CbbFlagNotBanned, &TRequestContext::TMatchedRules::NotBanned},
            {ANTIROBOT_DAEMON_CONFIG.CbbFlagCutRequests, &TRequestContext::TMatchedRules::CutRequest},
            {ANTIROBOT_DAEMON_CONFIG.CbbFlagIgnoreSpravka, &TRequestContext::TMatchedRules::IgnoreSpravka},
            {ANTIROBOT_DAEMON_CONFIG.CbbFlagMaxRobotness, &TRequestContext::TMatchedRules::MaxRobotness},
            {ANTIROBOT_DAEMON_CONFIG.CbbFlagSuspiciousness, &TRequestContext::TMatchedRules::Suspiciousness},
            {ANTIROBOT_DAEMON_CONFIG.CbbFlagDegradation, &TRequestContext::TMatchedRules::Degradation},
            {ANTIROBOT_DAEMON_CONFIG.CbbFlagBanSourceIp, &TRequestContext::TMatchedRules::BanSourceIp},
            {ANTIROBOT_DAEMON_CONFIG.CbbFlagBanFWSourceIp, &TRequestContext::TMatchedRules::BanFWSourceIp},
        };

        for (const auto& [groupId, property] : groupIdsProperties) {
            CbbGroupIdToProperties[groupId].push_back(property);
        }

        CbbListWatcher.Add(&CbbIpListManager);
        CbbListWatcher.Add(&CbbReListManager);
        CbbListWatcher.Add(&CbbTextListManager);

        CbbListWatcher.Poll(&*CbbCacheIO);

        CbbListWatcherPoller.Reset(
            Alarmer,
            ANTIROBOT_DAEMON_CONFIG.CbbSyncPeriod,
            0,
            true,
            [this] { CbbListWatcher.Poll(&*CbbIO); }
        );

        RuleSetMerger.Add(ANTIROBOT_DAEMON_CONFIG.CbbMergePeriod, 0, true, [this] () {
            CbbTextListManager.IterateCallbacks([] (auto& callback) {
                callback.Merge();
            });
        });
    }

    TVector<std::pair<TCbbGroupId, TVector<TPreparedRule>>> yqlCbbRules;

    for (const auto [i, rule] : Enumerate(Concatenate(
        ANTIROBOT_DAEMON_CONFIG.GlobalJsonConfig.Rules,
        ANTIROBOT_DAEMON_CONFIG.GlobalJsonConfig.MarkRules
    ))) {
        bool hasBlockRules = false;
        TVector<TPreparedRule> rules;

        for (const auto& cbbRule : rule.Cbb) {
            auto preparedRule = TPreparedRule::Parse(cbbRule);
            hasBlockRules = (hasBlockRules || !preparedRule.Nonblock);
            rules.push_back(std::move(preparedRule));
        }

        YqlHasBlockRules.push_back(hasBlockRules);
        YqlRuleIds.push_back(rule.Id);
        yqlCbbRules.push_back({static_cast<TCbbGroupId>(i), std::move(rules)});

        TYqlRuleSet yqlRuleCheck(rule.Yql);
    }

    YqlCbbRuleSet = TRuleSet(
        yqlCbbRules,
        NonBlockingAddrs,
        &CbbIpListManager,
        &CbbReListManager
    );

    RemoveExpired.Reset(Alarmer, ANTIROBOT_DAEMON_CONFIG.RemoveExpiredPeriod, 0, true, [this]() {
        Blocker->RemoveExpired();
        Robots->RemoveExpired();
        UniqueLcookies->RemoveExpired();
    });

    SaveCheckPoint.Reset(Alarmer, ANTIROBOT_DAEMON_CONFIG.RobotUidsDumpPeriod, 0, false, [this]() {
        SaveBlockSet(*Blocker->GetCopy(), *BlocksDumpFile.Access());
        Robots->LockedSave(*RobotUidsDumpFile.Access());
    });

    CbbAddBanFWSourceIp.Reset(Alarmer, ANTIROBOT_DAEMON_CONFIG.CbbAddBanFWPeriod, 0, false, [this]() {
        TVector<TAddr> addrs;
        BanFWAddrs.DequeueAll(&addrs);

        if (addrs.empty()) {
            return;
        }

        SortUnique(addrs);
        CbbIO->AddIps(ANTIROBOT_DAEMON_CONFIG.CbbFlagBanFWSourceIpsContainer, addrs);
    });

    if (CbbCache) {
        SaveCbbCacheAlarmer.Reset(Alarmer, ANTIROBOT_DAEMON_CONFIG.CbbCachePeriod, 0, false, [this]() {
            SaveCbbCache(CbbCache->GetCopy(), ANTIROBOT_DAEMON_CONFIG.CbbCacheFile);
        });
    }

    if (ANTIROBOT_DAEMON_CONFIG.InitialChinaRedirectEnabled) {
        EnableChinaRedirect();
    }

    if (ANTIROBOT_DAEMON_CONFIG.InitialChinaUnauthorizedEnabled) {
        EnableChinaUnauthorized();
    }

    AmnestyFlags.SetRobots(Robots);

    const TString uaData = NResource::Find("browser.xml");
    const TString uaProfiles = NResource::Find("profiles.xml");
    const TString uaExtra = NResource::Find("extra.xml");
    Detector = MakeHolder<uatraits::detector>(uaData.c_str(), uaData.length(), uaProfiles.c_str(), uaProfiles.length(), uaExtra.c_str(), uaExtra.length());

    TVector<std::pair<TCbbGroupId, TVector<NAntiRobot::TPreparedRule>>> lastVisitsRules;

    for (const auto& rule : ANTIROBOT_DAEMON_CONFIG.GlobalJsonConfig.LastVisitsRules) {
        lastVisitsRules.push_back({
            static_cast<TCbbGroupId>(rule.Id),
            {TPreparedRule::Parse(rule.Rule)}
        });

        LastVisitsIds.insert(rule.Id);
    }

    LastVisitsRuleSet = TRuleSet(
        lastVisitsRules,
        NonBlockingAddrs,
        &CbbIpListManager,
        &CbbReListManager
    );

    CpuAlarmer.Reset(Alarmer, TDuration::Seconds(1), 0, false, [this]() {
        CpuUsage.Update();
    });

    StartDiscovery();

    if (!ANTIROBOT_DAEMON_CONFIG.HypocrisyBundlePath.empty()) {
        HypocrisyBundle = NHypocrisy::TBundle::Load(ANTIROBOT_DAEMON_CONFIG.HypocrisyBundlePath);

        const auto brotli = NBlockCodecs::Codec("brotli_11");

        for (const auto& instance : HypocrisyBundle.Instances) {
            auto compressedInstance = brotli->Encode(instance);
            compressedInstance.erase(0, sizeof(ui64));
            BrotliHypocrisyInstances.push_back(std::move(compressedInstance));
        }

        for (const auto& instance : HypocrisyBundle.Instances) {
            TStringBuilder compressedInstance;
            TZLibCompress zlib(&compressedInstance.Out, ZLib::GZip, 9);
            zlib << instance;
            zlib.Finish();
            GzipHypocrisyInstances.push_back(std::move(compressedInstance));
        }
    }
}

TEnv::~TEnv() {
    ProcessingQueue->Stop();
    ProcessorResponseApplyQueue->Stop();
    InternalTasksQueue->Stop();

    SaveBlockSet(*Blocker->GetCopy(), *BlocksDumpFile.Access());
    Robots->LockedSave(*RobotUidsDumpFile.Access());
}

void TEnv::ForwardRequest(const TRequestContext& rc) {
    ForwardRequestAsync(rc, ANTIROBOT_DAEMON_CONFIG.RequestForwardLocation);
}

void TEnv::ForwardCaptchaInput(const TRequestContext& rc, bool isCorrect) {
    if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
        return;
    }
    ForwardCaptchaInputAsync({rc, isCorrect}, ANTIROBOT_DAEMON_CONFIG.CaptchaInputForwardLocation);
}

void TEnv::InitReloadable() {
    LoadKeyRing();
    LoadYandexTrustKeyRing();
    LoadYascKey();
    LoadSpravkaDataKey();
    std::tie(NarwhalJwsKeyId, NarwhalJwsKey) = LoadParametrizedJwsKey(ANTIROBOT_DAEMON_CONFIG.NarwhalJwsKeyPath);
    MarketJwsKey = LoadPlainJwsKey(ANTIROBOT_DAEMON_CONFIG.MarketJwsKeyPath);
    AutoRuTamperSalt = Strip(TFileInput(ANTIROBOT_DAEMON_CONFIG.AutoRuTamperSaltPath).ReadAll());
    LoadBadUserAgents();
    ReloadableData.GeoChecker.Set(TGeoChecker(ANTIROBOT_DAEMON_CONFIG.GeodataBinPath));
    LoadLKeychain();
    LoadClassifierRules(ANTIROBOT_DAEMON_CONFIG);
    LoadDictionaries();
    LoadAutoruOfferSalt();

    if (!ANTIROBOT_DAEMON_CONFIG.BalancerJwsKeyPath.empty()) {
        std::tie(BalancerJwsKeyId, BalancerJwsKey) =
            LoadParametrizedJwsKey(ANTIROBOT_DAEMON_CONFIG.BalancerJwsKeyPath);
    }
}

void TEnv::PrintStats(TStatsWriter& out) const {
    BackendSender.PrintStats(&out);
    BlockResponsesStats->PrintStats(ReqGroupClassifier, out);
    CaptchaStat->Print(ReqGroupClassifier, out);
    CbbBannedIpHolder.PrintStats(out);
    CbbIO->PrintStats(out);
    ChinaRedirectStats.PrintStats(out);
    Counters.Print(out);
    CpuUsage.PrintStats(out);
    CRD->PrintStats(out);
    DisablingStat.PrintStats(out);
    GetAntiRobotDynamicThreadPool()->PrintStatistics(out);
    JwsCounters.Print(out);
    YandexTrustCounters.Print(out);
    ManyRequestsStats.PrintStats(out);
    PartnersCaptchaStat.Print(out);
    ProcessServerTimeStatsProcessing.PrintStats(out);
    RD->PrintStats(ReqGroupClassifier, out);
    Robots->PrintStats(&out);
    ServerErrorStats.PrintStats(out);
    ServerExceptionsStats.Print(out);
    ServiceCounters.Print(out);
    ServiceExpBinCounters.Print(out);
    ServiceNsGroupCounters.Print(ReqGroupClassifier, out);
    ServiceNsExpBinCounters.Print(out);
    TimeStats.PrintStats(out);
    TimeStatsCaptcha.PrintStats(out);
    TimeStatsHandleByService.PrintStats(out);
    TimeStatsRead.PrintStats(out);
    TimeStatsWait.PrintStats(out);
    UserBase.PrintStats(out);
    VerochkaStats.PrintStats(out);
    ANTIROBOT_DAEMON_CONFIG.EventLog->PrintStats(out);
    ANTIROBOT_DAEMON_CONFIG.CacherDaemonLog->PrintStats(out);
    ANTIROBOT_DAEMON_CONFIG.ProcessorDaemonLog->PrintStats(out);

    out
        .WriteHistogram("internal_queue_size", InternalTasksQueue->Size())
        .WriteHistogram("neh_queue_size", TGeneralNehRequester::Instance().NehQueueSize())
        .WriteHistogram("captcha_neh_queue_size", TCaptchaNehRequester::Instance().NehQueueSize())
        .WriteHistogram("fury_neh_queue_size", TFuryNehRequester::Instance().NehQueueSize())
        .WriteHistogram("cbb_neh_queue_size", TCbbNehRequester::Instance().NehQueueSize())
        .WriteHistogram("block_responses_with_unique_Lcookies", UniqueLcookies->Size());

    for (auto& statPrinter : StatPrinters) {
        statPrinter(out);
    }
}

void TEnv::PrintStatsLW(
    TStatsWriter& out,
    EStatType service
) {
    // TODO (ashagarov@): rewrite it in TCategorizedStats 
    {
        const auto host = StatTypeToHostType(service);
        if (service != EStatType::STAT_TOTAL) {
            size_t totalCounter = 0;
            for (size_t uidTypeIdx = 0; uidTypeIdx < static_cast<size_t>(ESimpleUidType::Count); ++uidTypeIdx) {
                const auto uidType = static_cast<ESimpleUidType>(uidTypeIdx);
                for (size_t expBinIdx = 0; expBinIdx < EnumValue(EExpBin::Count); ++expBinIdx) {
                    const auto expBin = static_cast<EExpBin>(expBinIdx);
                    totalCounter += ServiceNsExpBinCounters.Get(
                            host, uidType, expBin,
                            EServiceNsExpBinCounter::WithSuspiciousnessRequests
                            );
                }
            }

            out.WriteScalar(
                    ToString(EServiceNsExpBinCounter::WithSuspiciousnessRequests),
                    totalCounter
                    );
        }
    }
    CRD->PrintStatsLW(out, service);
    RD->PrintStatsLW(out, service);
}

void TEnv::LoadLKeychain() {
    if (ANTIROBOT_DAEMON_CONFIG.AuthorizeByLCookie) {
        ReloadableData.LKeychain.Set(MakeHolder<NLCookie::TFileKeyReader>(ANTIROBOT_DAEMON_CONFIG.LCookieKeysPath));
    }
}

void TEnv::LoadBadUserAgents() {
    TFileInput fi(ANTIROBOT_DAEMON_CONFIG.BadUserAgentsFile);
    ReloadableData.BadUserAgents.Load(fi);
    TFileInput fin(ANTIROBOT_DAEMON_CONFIG.BadUserAgentsNewFile);
    ReloadableData.BadUserAgentsNew.Load(fin, false);
}

void TEnv::LoadIpLists() {
    const auto searchEngines = SearchEngineRecognizer.EvaluateBotsIps();

    TIpRangeMap<ECrawler> searchEnginesData;
    for (const auto& crawlerRange : TSearchEngineRecognizer::CandidatesToRanges(searchEngines)) {
        searchEnginesData.Insert(crawlerRange);
    }
    searchEnginesData.EnsureNoIntersections();
    ReloadableData.SearchEngines.Set(std::move(searchEnginesData));

    TVector<TAddr> searchEngineAddrs;
    searchEngineAddrs.reserve(searchEngines.size());
    for (const auto& se : searchEngines) {
        searchEngineAddrs.push_back(se.Address);
    }

    for (
        const auto& [dst, path] : (std::pair<
            NThreading::TRcuAccessor<TIpListProjId>&,
            const TString&
        >[]){
            {ReloadableData.SpecialIps, ANTIROBOT_DAEMON_CONFIG.SpecialIpsFile},
            {ReloadableData.TurboProxyIps, ANTIROBOT_DAEMON_CONFIG.TurboProxyIps},
            {ReloadableData.UaProxyIps, ANTIROBOT_DAEMON_CONFIG.UaProxyList}
        }
    ) {
        dst.Set(TIpListProjId(path));
    }

    for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
        for (
            const auto& [dst, listDir, list] : (std::tuple<
                NThreading::TRcuAccessor<TIpListProjId>&,
                TFsPath,
                const TVector<TString>&
            >[]){
                {ReloadableData.ServiceToWhitelist[i], ANTIROBOT_DAEMON_CONFIG.WhiteListsDir, ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].Whitelists},
                {ReloadableData.ServiceToYandexIps[i], ANTIROBOT_DAEMON_CONFIG.YandexIpsDir, ANTIROBOT_DAEMON_CONFIG.JsonConfig[i].YandexIps},
            }
        ) {
            TIpListProjId ipList;
            for (const auto& path : list) {
                ipList.Append(listDir / path, false);
            }

            try {
                ipList.EnsureNoIntersections();
            } catch (const std::exception& exc) {
                ythrow yexception() << "IpList " << JoinSeq(",", list) << " intersection found: " << exc.what();
            }
            dst.Set(std::move(ipList));
        }
    }

    TIpListProjId privilegedIps(ANTIROBOT_DAEMON_CONFIG.PrivilegedIpsFile);
    privilegedIps.AddAddresses(searchEngineAddrs);
    ReloadableData.PrivilegedIps.Set(std::move(privilegedIps));
}

void TEnv::LoadKeyRing() {
    TUnbufferedFileInput fi(ANTIROBOT_DAEMON_CONFIG.KeysFile);
    TKeyRing::SetInstance(TKeyRing(fi));
}

void TEnv::LoadYandexTrustKeyRing() {
    // TODO: ротация ключей будет сделана в рамках CAPTCHA-2678
    // а пока здесь https://bitbucket.browser.yandex-team.ru/projects/STARDUST/repos/browser-server-config/browse/common/ytrust.json?at=refs%2Fheads%2Fproduction
    TYandexTrustKeyRing::SetInstance(TYandexTrustKeyRing("189C09EB0B6BB890403547C7E87A0D4BCC67AA0B1B6C00E2806F7ECCAAE1156C"));
}

void TEnv::LoadYascKey() {
    TFileInput input(ANTIROBOT_DAEMON_CONFIG.YascKeyPath);
    YascKey = HexDecode(Strip(input.ReadAll()));
}

void TEnv::LoadSpravkaDataKey() {
    TFileInput input(ANTIROBOT_DAEMON_CONFIG.SpravkaDataKeyPath);
    TSpravkaKey::SetInstance(TSpravkaKey(input));
}

void TEnv::LoadAutoruOfferSalt() {
    TFileInput input(ANTIROBOT_DAEMON_CONFIG.AutoruOfferSaltPath);
    AutoruOfferSalt = Strip(input.ReadAll());
}

void TEnv::LoadClassifierRules(const TAntirobotDaemonConfig& config) {
    const auto rules = config.JsonConfig.GetReqTypeRegExps();
    ReloadableData.RequestClassifier.Set(
        TRequestClassifier::CreateFromArray(rules)
    );
}

void TEnv::LoadDictionaries() {
    if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
        return;
    }

    const auto& dictionariesList = ANTIROBOT_DAEMON_CONFIG.GlobalJsonConfig.Dictionaries;
    for (const auto& dictConfig : dictionariesList) {
        if (dictConfig.Name == "fraud_ja3" && dictConfig.Type == TGlobalJsonConfig::EDictionaryType::CityHash64) {
            THashDictionary dictionary;
            TIFStream in(ANTIROBOT_DAEMON_CONFIG.DictionariesDir + "/" + dictConfig.Name);
            in >> dictionary;
            ReloadableData.FraudJa3.Set(std::move(dictionary));
        } else if (dictConfig.Name == "fraud_subnet" && dictConfig.Type == TGlobalJsonConfig::EDictionaryType::Subnet) {
            TSubnetDictionary<24, 64> dictionary;
            TIFStream in(ANTIROBOT_DAEMON_CONFIG.DictionariesDir + "/" + dictConfig.Name);
            dictionary.Load(in);
            ReloadableData.FraudSubnet.Set(std::move(dictionary));
        } else if (dictConfig.Name == "fraud_subnet_new" && dictConfig.Type == TGlobalJsonConfig::EDictionaryType::Subnet) {
            TSubnetDictionary<24, 32> dictionary;
            TIFStream in(ANTIROBOT_DAEMON_CONFIG.DictionariesDir + "/" + dictConfig.Name);
            dictionary.Load(in);
            ReloadableData.FraudSubnetNew.Set(std::move(dictionary));
        } else if (dictConfig.Name == "trusted_users" && dictConfig.Type == TGlobalJsonConfig::EDictionaryType::Uid) {
            TTrustedUsers users;
            TIFStream in(ANTIROBOT_DAEMON_CONFIG.DictionariesDir + "/" + dictConfig.Name);
            users.Load(in);
            ReloadableData.TrustedUsers.Set(std::move(users));
        } else if (dictConfig.Name == "kinopoisk_films_honeypots") {
            Y_ENSURE(dictConfig.Type == TGlobalJsonConfig::EDictionaryType::CityHash64);
            THashDictionary dictionary;
            TIFStream in(ANTIROBOT_DAEMON_CONFIG.DictionariesDir + "/" + dictConfig.Name);
            in >> dictionary;
            ReloadableData.KinopoiskFilmsHoneypots.Set(std::move(dictionary));
        } else if (dictConfig.Name == "kinopoisk_names_honeypots") {
            Y_ENSURE(dictConfig.Type == TGlobalJsonConfig::EDictionaryType::CityHash64);
            THashDictionary dictionary;
            TIFStream in(ANTIROBOT_DAEMON_CONFIG.DictionariesDir + "/" + dictConfig.Name);
            in >> dictionary;
            ReloadableData.KinopoiskNamesHoneypots.Set(std::move(dictionary));
        } else if (dictConfig.Name == "mini_geobase" && dictConfig.Type == TGlobalJsonConfig::EDictionaryType::MiniGeobase) {
            TMiniGeobase miniGeobase;
            TIFStream in(ANTIROBOT_DAEMON_CONFIG.DictionariesDir + "/" + dictConfig.Name);
            miniGeobase.Load(in);
            ReloadableData.MiniGeobase.Set(std::move(miniGeobase));
        } else if (dictConfig.Name == "market_jws_states_stats" && dictConfig.Type == TGlobalJsonConfig::EDictionaryType::JwsStats) {
            TMarketJwsStatsDictionary stats;
            TIFStream in(ANTIROBOT_DAEMON_CONFIG.DictionariesDir + "/" + dictConfig.Name);
            in >> stats;
            ReloadableData.MarketJwsStatesStats.Set(std::move(stats));
        } else if (dictConfig.Name.StartsWith("market_stats_") && dictConfig.Type == TGlobalJsonConfig::EDictionaryType::MarketStats) {
            TMarketStatsDictionary stats;
            TIFStream in(ANTIROBOT_DAEMON_CONFIG.DictionariesDir + "/" + dictConfig.Name);
            in >> stats;
            if (dictConfig.Name == "market_stats_ja3") {
                ReloadableData.MarketJa3Stats.Set(std::move(stats));
            } else if (dictConfig.Name == "market_stats_subnet") {
                ReloadableData.MarketSubnetStats.Set(std::move(stats));
            } else if (dictConfig.Name == "market_stats_user_agent") {
                ReloadableData.MarketUAStats.Set(std::move(stats));
            } else {
                ythrow yexception() << "unknown dict: " << dictConfig.Name << " : " << dictConfig.Type;
            }
        } else if (dictConfig.Name == "autoru_ja3" && dictConfig.Type == TGlobalJsonConfig::EDictionaryType::CityHash64) {
            THashDictionary dictionary;
            TIFStream in(ANTIROBOT_DAEMON_CONFIG.DictionariesDir + "/" + dictConfig.Name);
            in >> dictionary;
            ReloadableData.AutoruJa3.Set(std::move(dictionary));
        } else if (dictConfig.Name == "autoru_subnet" && dictConfig.Type == TGlobalJsonConfig::EDictionaryType::Subnet) {
            TSubnetDictionary<24, 32> dictionary;
            TIFStream in(ANTIROBOT_DAEMON_CONFIG.DictionariesDir + "/" + dictConfig.Name);
            dictionary.Load(in);
            ReloadableData.AutoruSubnet.Set(std::move(dictionary));
        } else if (dictConfig.Name == "autoru_user_agent" && dictConfig.Type == TGlobalJsonConfig::EDictionaryType::String) {
            TEntityDict<float> dictionary;
            TIFStream in(ANTIROBOT_DAEMON_CONFIG.DictionariesDir + "/" + dictConfig.Name);
            in >> dictionary;
            ReloadableData.AutoruUA.Set(std::move(dictionary));
        } else {
            ythrow yexception() << "unknown dict: " << dictConfig.Name << " : " << dictConfig.Type;
        }
    }
}

void TEnv::IncreaseTimeouts() {
    Counters.Inc(ECounter::ReadReqTimeouts);
}

void TEnv::IncreaseUnknownServiceHeaders() {
    Counters.Inc(ECounter::UnknownServiceHeaders);
}

void TEnv::IncreaseReplyFails() {
    Counters.Inc(ECounter::ReplyFails);
}

void TEnv::Log400Event(const TRequest& req, HttpCodes httpCode, const TString& reason) {
    const auto header = req.MakeLogHeader();
    EVLOG_MSG << header << "Returned " <<  httpCode << " on request: " << req.RequestMethod << ' ' << req.Scheme << req.HostWithPort << req.RequestString
                                    << ", reason: " << reason;
    if (httpCode == HTTP_NOT_FOUND) {
        Counters.Inc(ECounter::Page404s);
    } else {
        Counters.Inc(ECounter::Page400s);
    }
}

void TEnv::EnableChinaRedirect() {
    AtomicSet(ChinaRedirectStats.IsChinaRedirectEnabled, 1);
    AtomicSet(IsChinaRedirectEnabled, 1);
}

void TEnv::DisableChinaRedirect() {
    AtomicSet(ChinaRedirectStats.IsChinaRedirectEnabled, 0);
    AtomicSet(IsChinaRedirectEnabled, 0);
}

void TEnv::EnableChinaUnauthorized() {
    AtomicSet(ChinaRedirectStats.IsChinaUnauthorizedEnabled, 1);
    AtomicSet(IsChinaUnauthorizedEnabled, 1);
}

void TEnv::DisableChinaUnauthorized() {
    AtomicSet(ChinaRedirectStats.IsChinaUnauthorizedEnabled, 0);
    AtomicSet(IsChinaUnauthorizedEnabled, 0);
}

void TEnv::StartDiscovery() {
    bool startedDiscovery = false;

    try {
        BackendSender.StartDiscovery();
        startedDiscovery = true;
    } catch (...) {
        const auto discoveryFrequency = TDuration::Parse(ANTIROBOT_DAEMON_CONFIG.DiscoveryFrequency);
        const auto discoveryStartTaskId =
            MakeAtomicShared<std::atomic<TAlarmer::TTaskId>>(TAlarmer::INVALID_ID);

        *discoveryStartTaskId = Alarmer.Add(
            discoveryFrequency, 0, false,
            [this, discoveryStartTaskId, finished = false] () mutable {
                if (finished) {
                    Alarmer.Remove(*discoveryStartTaskId);
                    return;
                }

                try {
                    BackendSender.StartDiscovery();
                    finished = true;
                } catch (...) {
                    return;
                }

                Alarmer.Remove(*discoveryStartTaskId);
            }
        );

        DiscoveryStartTask.Reset(Alarmer, *discoveryStartTaskId);
    }

    if (startedDiscovery) {
        BackendSender.GetUpdateEvent().WaitT(ANTIROBOT_DAEMON_CONFIG.DiscoveryInitTimeout);
    }
}

} /* namespace NAntiRobot */
