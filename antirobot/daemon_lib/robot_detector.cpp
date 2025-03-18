#include "robot_detector.h"

#include "antiddos.h"
#include "blocker.h"
#include "cacher_factors.h"
#include "cbb.h"
#include "classificator.h"
#include "classify_experiments.h"
#include "environment.h"
#include "eventlog_err.h"
#include "exp_bin.h"
#include "factors.h"
#include "fullreq_info.h"
#include "night_check.h"
#include "req_types.h"
#include "request_context.h"
#include "robot_set.h"
#include "search_engine_recognizer.h"
#include "stat.h"
#include "threat_level.h"
#include "training_set_generator.h"
#include "unified_agent_log.h"
#include "user_base.h"
#include "wizards.h"
#include "yql_rule_set.h"

#include <antirobot/idl/antirobot.ev.pb.h>
#include <antirobot/idl/daemon_log.pb.h>
#include <antirobot/lib/ar_utils.h>
#include <antirobot/lib/bad_user_agents.h>
#include <antirobot/lib/dynamic_thread_pool.h>
#include <antirobot/lib/ip_list.h>

#include <library/cpp/charset/ci_string.h>
#include <library/cpp/http/cookies/cookies.h>
#include <library/cpp/iterator/enumerate.h>
#include <library/cpp/iterator/mapped.h>
#include <library/cpp/protobuf/json/proto2json.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/random/random.h>
#include <util/stream/format.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/printf.h>
#include <util/system/tls.h>

#include <algorithm>
#include <array>
#include <atomic>

using namespace NAntirobotEvClass;

namespace NAntiRobot {
    TDetectedRobotInfo GetRobotInfo(
        const TRequestContext& rc,
        const TRequestFeatures& requestFeatures,
        bool matchedRules,
        bool matchedMatrixnet,
        bool matchedCbb,
        size_t requestsFromUid,
        bool isAlreadyRobot
    ) {
        static TDetectedRobotInfo robot = {.IsRobot = true, .IsForcedRobot = false};
        static TDetectedRobotInfo forcedRobot = {.IsRobot = true, .IsForcedRobot = true};
        static TDetectedRobotInfo notRobot = {.IsRobot = false, .IsForcedRobot = false};

        if (rc.CbbWhitelist) {
            return notRobot;
        }

        const TRequest* req = rc.Req.Get();
        bool mayBan = !req->UserAddr.IsWhitelisted() && req->MayBanFor() && !rc.CatboostWhitelist && !req->MarketExpSkipCaptcha;
        if (!mayBan) {
            return notRobot;
        }

        const bool forceBan = IsIn(TEST_REQUESTS_FOR_CAPTCHA, requestFeatures.ReqText);
        if (Y_UNLIKELY(forceBan)) {
            return forcedRobot;
        }

        if (rc.SuspiciousBan) {
            return robot;
        }

        if (isAlreadyRobot) {
            return robot;
        }

        const auto spravkaRequestsLimit = ANTIROBOT_DAEMON_CONFIG.SpravkaRequestsLimit;
        if (req->Uid.Ns == TUid::SPRAVKA && spravkaRequestsLimit > 0 && requestsFromUid > spravkaRequestsLimit) {
            return forcedRobot;
        }

        if (rc.CatboostBan || rc.ApiAutoVersionBan) {
            return robot;
        }

        if (
            (!YqlBansEnabled() || !matchedRules) &&
            (ANTIROBOT_DAEMON_CONFIG.DisableBansByFactors || !matchedMatrixnet) &&
            !matchedCbb
        ) {
            return notRobot;
        }

        if (req->Uid.Ns != TUid::SPRAVKA) {
            return robot;
        }

        const bool isSpravkaIgnored = !rc.MatchedRules->IgnoreSpravka.empty();
        const size_t minRequests = ANTIROBOT_DAEMON_CONFIG.JsonConfig[req->HostType].MinRequestsWithSpravka;
        return requestsFromUid > minRequests || isSpravkaIgnored ? robot : notRobot;
    }

    class TRobotDetector : public IRobotDetector {
    public:
        struct TMatrixNetResult {
            float Value = 0.0f;
            float Threshold = 0.0f;

            bool Matched() const {
                return Value >= Threshold;
            }
        };

        explicit TRobotDetector(
            const TEnv& env,
            const std::array<THolder<TIsBlocked>, EHostType::HOST_NUMTYPES>& isBlocked,
            TBlocker& blocker,
            ICbbIO* cbbIO
        )
            : Stats(env.ReqGroupClassifier.GetServiceToNumGroup())
            , ReloadableData(env.ReloadableData)
            , RequestLogJson(ANTIROBOT_DAEMON_CONFIG.UnifiedAgentUri)
            , CbbIO(cbbIO)
            , IsBlocked(isBlocked)
            , Blocker(blocker)
            , AntiDDoS(blocker, cbbIO)
            , ProcessorExperiments(ANTIROBOT_DAEMON_CONFIG.ProcessorExpDescriptions)
            , CacherExperiments(ANTIROBOT_DAEMON_CONFIG.CacherExpDescriptions)
        {
            InitZoneData();
            InitReqWizard();
        }

        void InitZoneData() {
            TAntirobotDaemonConfig::TZoneConfNames confNames = ANTIROBOT_DAEMON_CONFIG.GetZoneConfNames();

            for (size_t i = 0; i < confNames.size(); i++) {
                const TString& tld = confNames[i];
                const TAntirobotDaemonConfig::TZoneConf& conf = ANTIROBOT_DAEMON_CONFIG.ConfByTld(tld);

                ZoneData[tld] = new TZoneData(
                    ANTIROBOT_DAEMON_CONFIG.JsonConfig.GetFormulaFilesArrayByTld(tld),
                    ANTIROBOT_DAEMON_CONFIG.JsonConfig.GetFallbackFormulaFilesArrayByTld(tld),
                    conf.TrainingSetGenSchemes
                );
            }
        }

        void InitReqWizard() {
            try {
                ReqWizard_.Reset(new TWizardFactorsCalculator());
            } catch (const TNetworkResolutionError& e) {
                const TString message = TString{"Error in wizard hostname resolution. Going to retry with IPs only. "} + e.what();
                Cerr << message << Endl;
                EVLOG_MSG << EVLOG_ERROR << message;
                ReqWizard_.Reset(new TWizardFactorsCalculator(true));
            }
        }

        void ProcessInTrainingSetGenerators(TRequestContext& rc, const TRequest* req) {
            TZoneData& zoneData = GetZoneData(req->Tld);

            switch (zoneData.GetTrainSetGen().ProcessRequest(rc)) {
                case ITrainingSetGenerator::AddedRandomUserRequest:
                    Stats.IncUniqueRandomFactors(*req);
                    rc.IsTraining = true;
                    break;
                case ITrainingSetGenerator::RequestNotUsed:
                    break;
                default:
                    Y_ASSERT(false);
                    break;
            }
        }

        TRobotStatus GetRobotStatus(TRequestContext& rc) override {
            const TRequest* req = rc.Req.Get();

            ProcessInTrainingSetGenerators(rc, req);

            if (!req->CanShowCaptcha()) {
                return TRobotStatus::Normal();
            }

            if (req->ForceShowCaptcha) {
                return {.ForceCaptcha = true};
            }

            if (req->UserAddr.IsWhitelisted()) {
                return TRobotStatus::Normal();
            }

            if (rc.Suspiciousness > 0 && !req->HasValidSpravka && AtomicGet(rc.Env.SuspiciousFlags.Ban.GetByService(req->HostType))) {
                rc.SuspiciousBan = true;
                return {.SuspiciousnessCaptcha = true};
            }

            bool isAlreadyRobot = rc.Env.Robots->Contains(req->HostType, req->Uid);
            return isAlreadyRobot ? TRobotStatus{.RobotsetCaptcha = true} : TRobotStatus::Normal();
        }

        bool ShouldPrintFactors(bool isTraining, bool justBecomeRobot, bool isBlocked, ECaptchaRedirectType redirectType, const TRequest* req) {
            if ((isTraining || justBecomeRobot) && !isBlocked) {
                return true;
            }

            const float regularCaptchaProbabilityToPrint = ANTIROBOT_DAEMON_CONFIG.PartOfRegularCaptchaRedirectFactorsToPrint;

            if (redirectType == ECaptchaRedirectType::RegularCaptcha && RandomNumber<float>() < regularCaptchaProbabilityToPrint) {
                return true;
            }

            if (RandomNumber<float>() < ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(req->HostType, req->Tld).AdditionalFactorsProbability) {
                return true;
            }
            return false;
        }

        bool SoonWillBeUnbanned(const TRequestContext& rc, const TRequest* req, bool isAlreadyRobot) {
            if (!isAlreadyRobot) {
                return false;
            }

            const auto maxExpiration = rc.Env.Robots->GetExpirationTime(req->HostType, req->Uid);

            return
                maxExpiration == TInstant::Zero() ||
                TInstant::Now() > maxExpiration - ANTIROBOT_DAEMON_CONFIG.ReBanRobotsPeriod;
        }

        void ProcessRequest(const TRequestContext& rc, bool onCacher) override {
            const TRequest* req = rc.Req.Get();
            TRequestFeatures::TContext ctx = {req, ReqWizard_.Get(), &ReloadableData, rc.Env.Detector.Get(), rc.Env.AutoruOfferDetector.Get(), rc.Env.RpsFilter };
            const TRequestFeatures requestFeatures(ctx, Stats.GetWizardCalcTimeStats(), Stats.GetFactorsCalcTimeStats());

            rc.Env.UserBase.Get(req->Uid)->TouchWithAggregation(req->ArrivalTime);
            rc.Env.SearchEngineRecognizer.ProcessRequest(*req);

            const bool wizardError = requestFeatures.WizardFeatures.WizardError;

            if (wizardError) {
                LogWizardError(req);
            }

            const TProcessorLinearizedFactors linearizedFactors = GetFactors(rc, req, requestFeatures);
            const TUid& uid = req->Uid;
            const bool isAlreadyRobot = rc.Env.Robots->Contains(req->HostType, uid);
            const bool soonWillBeUnbanned = SoonWillBeUnbanned(rc, req, isAlreadyRobot);

            AntiDDoS.ProcessRequest(*req, rc.Env.UserBase, isAlreadyRobot);

            TIsBlocked::TBlockState blockState = IsBlocked[req->HostType]->GetBlockState(rc);

            TVector<TCbbRuleId> matchedRulesList;
            TVector<TCbbRuleId> markingRulesList;
            if (!wizardError) {
                // YQL правила могут использовать wizard-факторы
                GetYqlRulesResult(rc, linearizedFactors, &matchedRulesList, &markingRulesList);
            }

            const TZoneData& zoneData = GetZoneData(req->Tld);
            const auto classifier = zoneData.GetClassifierForHost(req->HostType);
            const auto fallbackClassifier = zoneData.GetFallbackClassifierForHost(req->HostType);
            const bool useFallbackClassifier = fallbackClassifier != nullptr
                && (wizardError || RandomNumber<float>() < ANTIROBOT_DAEMON_CONFIG.MatrixnetFallbackProbability);

            TMatrixNetResult matrixNetResult;
            if (useFallbackClassifier) {
                matrixNetResult = GetMatrixNetResult(req, *fallbackClassifier, linearizedFactors, isAlreadyRobot, soonWillBeUnbanned);
            } else if (!wizardError) {
                matrixNetResult = GetMatrixNetResult(req, *classifier, linearizedFactors, isAlreadyRobot, soonWillBeUnbanned);
            } else {
                matrixNetResult.Threshold = GetEnemyThreshold(req);
            }

            TFormulasResults expFormulasResults;
            TFormulasResults cacherExpFormulasResults;
            if (!wizardError) {
                tie(expFormulasResults, cacherExpFormulasResults) =
                    GetExperimentalFormulasResults(req, linearizedFactors, rc.CacherFactors.Factors);
            }

            const size_t requestsFromUid = GetRequestsCountFromUid(rc, requestFeatures, matrixNetResult);

            TVector<TBinnedCbbRuleKey> markingCbbRuleList = rc.MatchedRules->Mark;
            Copy(
                rc.MatchedRules->MarkLogOnly.begin(), rc.MatchedRules->MarkLogOnly.end(),
                std::back_inserter(markingCbbRuleList)
            );

            const bool matchedMatrixnet = matrixNetResult.Matched();
            const bool matchedRules = !matchedRulesList.empty();
            const bool matchedCbb = !rc.MatchedRules->Captcha.empty();

            const auto robotInfo = GetRobotInfo(
                rc, requestFeatures,
                matchedRules, matchedMatrixnet, matchedCbb,
                requestsFromUid, isAlreadyRobot
            );

            const bool isRobot = robotInfo.IsRobot;
            const bool forceBan = robotInfo.IsForcedRobot;
            const bool justBecomeRobot = isRobot && !isAlreadyRobot;

            if ((justBecomeRobot || (isRobot && soonWillBeUnbanned)) && (!matchedCbb || !req->Uid.IpBased())) {
                auto captchaRuleKeys = NFuncTools::Map(
                    [] (const auto& binnedKey) { return binnedKey.Key; },
                    rc.MatchedRules->Captcha
                );
                rc.Env.Robots->Add(
                    *req,
                    (!ANTIROBOT_DAEMON_CONFIG.DisableBansByFactors && matchedMatrixnet) || forceBan,
                    YqlBansEnabled() ? matchedRulesList : TVector<TCbbRuleId>{},
                    {captchaRuleKeys.begin(), captchaRuleKeys.end()}
                );
            }

            Stats.Update(
                isRobot, *req, matrixNetResult,
                onCacher, !markingRulesList.empty()
            );

            PrintFactorsToEventLogIfNecessary(rc, req, isRobot, justBecomeRobot, blockState, linearizedFactors);


            EMissedRobotStatus missed = GetMissedRobotStatus(rc, isRobot, justBecomeRobot);
            if (missed != EMissedRobotStatus::NotMissed) {
                Stats.IncMissedRobots(*req, missed);
            }

            NDaemonLog::TRecord rec = CreateDaemonLogRecord(
                rc, req, requestFeatures, forceBan, matrixNetResult, useFallbackClassifier,
                expFormulasResults, cacherExpFormulasResults,
                requestsFromUid, isRobot, blockState, missed, matchedRulesList, markingRulesList,
                rc.MatchedRules->Captcha, markingCbbRuleList, rc.MatchedRules->CheckboxBlacklist,
                rc.MatchedRules->CanShowCaptcha
            );

            WriteDaemonLogRecord(rec);

            NDaemonLog::TProcessorRecord processor_rec =  CreateDaemonLogProcessorRecord(
                rc, requestFeatures, forceBan, matrixNetResult, useFallbackClassifier,
                expFormulasResults, cacherExpFormulasResults,
                requestsFromUid, missed, matchedRulesList, markingRulesList
            );
            ANTIROBOT_DAEMON_CONFIG.ProcessorDaemonLog->WriteLogRecord(processor_rec);

            ProcessForceBlock(req, requestFeatures);
        }

        void ProcessForceBlock(const TRequest* req, const TRequestFeatures& rf) const {
            if (req->UserAddr.IsYandex() && rf.ReqText == TEST_REQUEST_FOR_BLOCK) {
                TBlockRecord blockRecord = {
                    GetIpUid(req->UserAddr),
                    BC_UNDEFINED,
                    req->UserAddr,
                    TString{req->YandexUid},
                    EBlockStatus::Block,
                    TInstant::Now() + FORCED_BLOCK_PERIOD,
                    "Force block",
                };

                for (const auto category : AllBlockCategories()) {
                    blockRecord.Category = category;
                    Blocker.AddForcedBlock(blockRecord);
                }
            }
        }

        size_t GetRequestsCountFromUid(const TRequestContext& rc, const TRequestFeatures& requestFeatures,
                                       const TMatrixNetResult& matrixNetResult) {
            if (!rc.Req->UserAddr.IsPrivileged()) {
                TUserBase::TValue tlUser = rc.Env.UserBase.Get(rc.Req->Uid);

                size_t requestsFromUid = tlUser->GetNumImportantReqs() + GetPenalty(requestFeatures, matrixNetResult);
                tlUser->UpdateNumImportantReqs(requestsFromUid);
                return requestsFromUid;
            }
            return 0;
        }

        std::pair<TFormulasResults, TFormulasResults> GetExperimentalFormulasResults(
            const TRequest* req,
            const TProcessorLinearizedFactors& lf,
            const TCacherLinearizedFactors& clf)
        {
            TFormulasResults expFormulasResults;
            TFormulasResults cacherExpFormulasResults;
            if (!req->UserAddr.IsPrivileged()) {
                {
                    TMeasureDuration measureDuration{Stats.GetExpFormulasCalcTimeStats()};
                    expFormulasResults = ProcessorExperiments.Apply(*req, lf);
                }
                {
                    TMeasureDuration cacherMeasureDuration{Stats.GetCacherExpFormulasCalcTimeStats()};
                    cacherExpFormulasResults = CacherExperiments.Apply(*req, clf);
                }
            }
            return {expFormulasResults, cacherExpFormulasResults};
        }

        TMatrixNetResult GetMatrixNetResult(
            const TRequest* req,
            const TClassificator<TProcessorLinearizedFactors>& classify,
            const TProcessorLinearizedFactors& lf,
            bool isAlreadyRobot,
            bool soonWillBeUnbanned
        ) {
            TMatrixNetResult result;
            result.Threshold = GetEnemyThreshold(req);

            if (req->UserAddr.IsPrivileged()) {
                result.Value = ANTIROBOT_DAEMON_CONFIG.MatrixnetResultForPrivileged;
            } else if (isAlreadyRobot && !soonWillBeUnbanned) {
                result.Value = ANTIROBOT_DAEMON_CONFIG.MatrixnetResultForAlreadyRobot;
            } else if (ANTIROBOT_DAEMON_CONFIG.NoMatrixnetReqTypesSet.contains(req->ReqType) && !req->MayBanFor()) {
                result.Value = ANTIROBOT_DAEMON_CONFIG.MatrixnetResultForNoMatrixnetReqTypes;
            } else {
                TMeasureDuration measureDuration{Stats.GetFormulaCalcTimeStats()};
                result.Value = classify(lf);
            }
            return result;
        }

        void GetYqlRulesResult(
            const TRequestContext& rc,
            const TProcessorLinearizedFactors& linearizedFactors,
            TVector<TCbbRuleId>* matchedRules,
            TVector<TCbbRuleId>* markingRules
        ) {
            static thread_local TYqlRuleSet* yqlRules = nullptr;

            if (!yqlRules) {
                yqlRules = new TYqlRuleSet(GetAllYqlRules());

                auto* const dtpThread = TDynamicThreadPool::TThread::Current();
                Y_ENSURE(
                    dtpThread,
                    "ProcessRequest must be executed by TDynamicThreadPool"
                );

                dtpThread->DeleteOnExit(yqlRules);
            }

            const auto& req = *rc.Req;

            if (
                !linearizedFactors.empty() &&
                !rc.Env.DisablingFlags.IsStopYqlForService(req.HostType)
            ) {
                const size_t numBanRules = ANTIROBOT_DAEMON_CONFIG.GlobalJsonConfig.Rules.size();

                const auto matchedYqlRules = yqlRules->Match(linearizedFactors);

                THashMap<ui32, std::pair<bool, bool>> ruleIndexToMatchedWhitelisted;
                ruleIndexToMatchedWhitelisted.reserve(matchedYqlRules.size());

                for (const auto ruleIndex : matchedYqlRules) {
                    ruleIndexToMatchedWhitelisted[ruleIndex] =
                        {!rc.Env.YqlHasBlockRules[ruleIndex], false};
                }

                const auto matchedCbbRules = rc.Env.YqlCbbRuleSet.Match(req, &rc.CacherFactors);

                for (const auto ruleKey : matchedCbbRules) {
                    const auto ruleIndex = static_cast<ui32>(ruleKey.Key.Group);

                    const auto matchedWhitelisted = ruleIndexToMatchedWhitelisted.FindPtr(ruleIndex);
                    if (!matchedWhitelisted) {
                        continue;
                    }

                    if (ruleKey.Whitelisting) {
                        matchedWhitelisted->second = true;
                    } else {
                        matchedWhitelisted->first = true;
                    }
                }

                for (const auto& [ruleIndex, matchedWhitelisted] : ruleIndexToMatchedWhitelisted) {
                    if (matchedWhitelisted.first && !matchedWhitelisted.second) {
                        (ruleIndex >= numBanRules ? markingRules : matchedRules)
                            ->push_back(rc.Env.YqlRuleIds[ruleIndex]);
                    }
                }
            }
        }

        TProcessorLinearizedFactors GetFactors(const TRequestContext& rc, const TRequest* req, const TRequestFeatures& rf) {
            TProcessorLinearizedFactors lf;
            if (!req->UserAddr.IsPrivileged()) {
                TMeasureDuration measureDuration{Stats.GetFactorsProcessingTimeStats()};
                lf = ApplyRequestToTls(rc, rf);
            }
            return lf;
        }

        EMissedRobotStatus GetMissedRobotStatus(const TRequestContext& rc, bool isRobot, bool justBecomeRobot) const {
            bool wasPassed = !rc.WasBlocked && rc.RedirectType == ECaptchaRedirectType::NoRedirect;

            if (justBecomeRobot && wasPassed) {
                return EMissedRobotStatus::MissedOnce;
            } else if (isRobot && wasPassed) {
                return EMissedRobotStatus::MissedMultiple;
            }

            return EMissedRobotStatus::NotMissed;
        }

        void PrintFactorsToEventLogIfNecessary(
            const TRequestContext& rc, const TRequest* req, bool isRobot, bool justBecomeRobot,
            const TIsBlocked::TBlockState& blockState, const TProcessorLinearizedFactors& lf) {
            if (!lf.empty()) {
                const bool forceToPrintFactors = ShouldPrintFactors(rc.IsTraining, justBecomeRobot,
                                                                    blockState.IsBlocked, rc.RedirectType, req);
                PrintFactorsToFactorsLog(req, isRobot, lf, forceToPrintFactors);
                if (rc.IsTraining
                   || justBecomeRobot
                   || RandomNumber<float>() < ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(req->HostType, req->Tld).RandomEventsProbability) {
                    req->LogRequestData(*ANTIROBOT_DAEMON_CONFIG.EventLog);
                }
            }
        }

        NDaemonLog::TRecord CreateDaemonLogRecord(
            const TRequestContext& rc, const TRequest* req, const TRequestFeatures& rf,
            bool forceBan, const TMatrixNetResult& matrixNetResult, bool useFallbackClassifier,
            const TFormulasResults& expFormulasResults,
            const TFormulasResults& cacherExpFormulasResults,
            size_t requestsFromUid, bool isRobot, const TIsBlocked::TBlockState& blockState,
            const EMissedRobotStatus& missed, const TVector<TCbbRuleId>& matchedRules,
            const TVector<TCbbRuleId>& markingRules, const TVector<TBinnedCbbRuleKey>& matchedCbbRuleList,
            const TVector<TBinnedCbbRuleKey>& markingCbbRuleList,
            const TVector<TBinnedCbbRuleKey>& checkboxBlacklistCbbRuleList,
            const TVector<TBinnedCbbRuleKey>& canShowCaptchaCbbRuleList
        ) const {
            static const TString DASH = "-";

            const bool isBannedByCbb = !rc.Env.CbbBannedIpHolder.GetBanReason(*req).empty();
            TStringBuf severity = isRobot || isBannedByCbb ? TStringBuf("ENEMY") : TStringBuf("NEUTRAL");

            if (blockState.IsBlocked) {
                severity = TStringBuf("BLOCKED");
            }

            const TInstant startTime = req->ArrivalTime;
            const TStringBuf& reqId = ReqId(req->ServiceReqId);
            const auto& ua = req->UserAgent();

            NDaemonLog::TRecord rec;

            rec.set_verdict(TString(severity));
            rec.set_force_ban(forceBan);
            rec.set_matrixnet(matrixNetResult.Value);
            rec.set_matrixnet_fallback(useFallbackClassifier);
            rec.set_req_type(ToString(req->HostType) + '-' + ToString(req->ReqType));
            rec.set_balancer_ip(NiceAddrStr(req->RequesterAddr));
            rec.set_spravka_ip(ToString(req->SpravkaAddr));
            rec.set_ident_type(ToString(req->Uid));
            rec.set_user_ip(ToString(req->UserAddr));
            rec.set_event_unixtime(Sprintf("%06f", static_cast<double>(startTime.MicroSeconds()) / 1000000));
            rec.set_web_req_count(requestsFromUid);
            rec.set_req_id(TString(reqId));
            rec.set_random_captcha(rc.IsTraining);
            rec.set_partner_ip(req->IsPartnerRequest() ? req->PartnerAddr.ToString() : DASH);
            rec.set_wizard_error(rf.WizardFeatures.WizardError);
            rec.set_yandexuid(!req->YandexUid.empty() ? TString(req->YandexUid) : DASH);
            rec.set_cacher_host(rc.CacherHost);
            rec.set_processor_host(ShortHostName());
            rec.set_missed(static_cast<unsigned int>(missed));
            rec.set_icookie(!req->ICookie.empty() ? ToString(req->ICookie) : DASH);
            rec.set_service(ToString(req->HostType));
            rec.set_host({req->Host.data(), req->Host.size()});
            rec.set_user_agent({ua.data(), ua.size()});
            rec.set_cbb_whitelist(rc.CbbWhitelist);
            rec.set_catboost_whitelist(rc.CatboostWhitelist);
            rec.set_lcookie(req->LCookieUid);

            TStringBuf reqUrl;
            TStringBuilder maskedReqUrl;

            TStringBuf referer;
            TStringBuilder maskedReferer;

            if (ANTIROBOT_DAEMON_CONFIG.JsonConfig[req->HostType].CgiSecrets.empty()) {
                reqUrl = req->RequestString;
                referer = req->Referer();
            } else {
                maskedReqUrl.reserve(req->RequestString.size());
                WriteMaskedUrl(req->HostType, req->RequestString, maskedReqUrl.Out);
                reqUrl = maskedReqUrl;

                const TStringBuf unmaskedReferer = req->Referer();
                maskedReferer.reserve(unmaskedReferer.size());
                WriteMaskedUrl(req->HostType, unmaskedReferer, maskedReferer.Out);
                referer = maskedReferer;
            }

            reqUrl.Trunc(
                reqUrl.StartsWith("/clck/") ?
                    MAX_REQ_URL_CLCK_SIZE :
                    ANTIROBOT_DAEMON_CONFIG.JsonConfig[req->HostType].MaxReqUrlSize
            );

            rec.set_req_url(reqUrl.data(), reqUrl.size());

            referer.Trunc(MAX_REFERER_URL_SIZE);
            rec.set_referer(referer.data(), referer.size());

            rec.set_may_ban(req->MayBanFor());
            rec.set_can_show_captcha(req->CanShowCaptcha());
            rec.set_threshold(matrixNetResult.Threshold);
            rec.set_ip_list(JoinSeq(",", req->UserAddr.IpList()));
            rec.set_mini_geobase_mask(req->UserAddr.MiniGeobaseMask());
            rec.set_block_reason(blockState.Reason);
            rec.set_cacher_blocked(rc.WasBlocked);
            rec.set_cacher_block_reason(rc.BlockReason);

            for (const auto& expRes : expFormulasResults) {
                (*rec.mutable_exp_formulas())[expRes.first] = expRes.second;
            }

            for (const auto& expRes : cacherExpFormulasResults) {
                (*rec.mutable_cacher_exp_formulas())[expRes.first] = expRes.second;
            }

            if (const auto ja3 = req->Ja3(); !ja3.empty()) {
                rec.set_ja3(ja3.Data(), ja3.Size());
            }

            if (const auto p0f = req->P0f(); !p0f.empty()) {
                rec.set_p0f(p0f.Data(), p0f.Size());
            }

            if (const auto hodor = req->Hodor; !hodor.empty()) {
                rec.set_hodor(hodor.Data(), hodor.Size());
            }

            if (const auto hodorHash = req->HodorHash; !hodorHash.empty()) {
                rec.set_hodor_hash(hodorHash.Data(), hodorHash.Size());
            }

            if (const auto ja4 = req->Ja4(); !ja4.empty()) {
                rec.set_ja4(ja4.Data(), ja4.Size());
            }

            rec.set_degradation(rc.Degradation);

            rec.set_robotness(rc.RobotnessHeaderValue);
            rec.set_suspiciousness(rc.Suspiciousness);
            rec.set_spravka_ignored(rc.Req->SpravkaIgnored);

            *rec.mutable_rules() = ConvertToProtoStrings(matchedRules);
            *rec.mutable_mark_rules() = ConvertToProtoStrings(markingRules);

            const auto& groupName = rc.Env.ReqGroupClassifier.GroupName(req->HostType, req->ReqGroup);
            rec.set_req_group(groupName.Data(), groupName.Size());

            const auto [prevRules, prevCbbBanRules] = rc.Env.Robots->GetRules(req->HostType, req->Uid);
            *rec.mutable_prev_rules() = ConvertToProtoStrings(prevRules);
            *rec.mutable_prev_cbb_ban_rules() = ConvertToProtoStrings(prevCbbBanRules);

            *rec.mutable_cbb_ban_rules() = ConvertToProtoStrings(matchedCbbRuleList);
            *rec.mutable_cbb_mark_rules() = ConvertToProtoStrings(markingCbbRuleList);
            *rec.mutable_cbb_checkbox_blacklist_rules() = ConvertToProtoStrings(checkboxBlacklistCbbRuleList);
            *rec.mutable_cbb_can_show_captcha_rules() = ConvertToProtoStrings(canShowCaptchaCbbRuleList);

            rec.set_unique_key(req->UniqueKey);
            *rec.mutable_experiments_test_id() = ConvertExpHeaderToProto(req->ExperimentsHeader);
            rec.set_jws_state(ToString(req->JwsState));
            rec.set_yandex_trust_state(ToString(req->YandexTrustState));
            rec.set_cacher_formula_result(rc.CacherFormulaResult);
            rec.set_ban_source_ip(rc.BanSourceIp);
            rec.set_valid_spravka_hash(req->HasValidSpravkaHash);
            rec.set_jws_hash(HexEncode(TStringBuf(req->JwsHash.begin(), req->JwsHash.end())));
            rec.set_valid_autoru_tamper(req->HasValidAutoRuTamper);
            rec.set_ban_fw_source_ip(rc.BanFWSourceIp);

            return rec;
        }

        void WriteDaemonLogRecord(const NDaemonLog::TRecord& rec) {
            if (ANTIROBOT_DAEMON_CONFIG.DaemonLogFormatJson) {
                RequestLogJson << NProtobufJson::Proto2Json(rec) << Endl;
            }
        }

        NDaemonLog::TProcessorRecord CreateDaemonLogProcessorRecord(
            const TRequestContext& rc, const TRequestFeatures& rf,
            bool forceBan, const TMatrixNetResult& matrixNetResult, bool useFallbackClassifier,
            const TFormulasResults& expFormulasResults,
            const TFormulasResults& cacherExpFormulasResults, size_t requestsFromUid,
            const EMissedRobotStatus& missed, const TVector<TCbbRuleId>& matchedRules,
            const TVector<TCbbRuleId>& markingRules
        ) const {
            const TInstant startTime = rc.FirstArrivalTime;

            NDaemonLog::TProcessorRecord rec;

            rec.set_unique_key(rc.Req->UniqueKey);
            rec.set_timestamp(startTime.MicroSeconds());
            rec.set_matrixnet(matrixNetResult.Value);
            rec.set_web_req_count(requestsFromUid);
            rec.set_wizard_error(rf.WizardFeatures.WizardError);
            rec.set_processor_host(ShortHostName());
            rec.set_missed(static_cast<unsigned int>(missed));
            rec.set_threshold(matrixNetResult.Threshold);

            for (const auto& expRes : expFormulasResults) {
                (*rec.mutable_exp_formulas())[expRes.first] = expRes.second;
            }

            *rec.mutable_rules() = ConvertToProtoStrings(matchedRules);
            *rec.mutable_mark_rules() = ConvertToProtoStrings(markingRules);

            const auto [prevRules, prevCbbBanRules] = rc.Env.Robots->GetRules(rc.Req->HostType, rc.Req->Uid);
            *rec.mutable_prev_rules() = ConvertToProtoStrings(prevRules);
            *rec.mutable_prev_cbb_ban_rules() = ConvertToProtoStrings(prevCbbBanRules);

            rec.set_force_ban(forceBan);

            for (const auto& expRes : cacherExpFormulasResults) {
                (*rec.mutable_cacher_exp_formulas())[expRes.first] = expRes.second;
            }

            rec.set_matrixnet_fallback(useFallbackClassifier);

            return rec;
        }

        void LogWizardError(const TRequest* req) {
            if (ANTIROBOT_DAEMON_CONFIG.DebugOutput) {
                Cerr << "Wizard Error!" << Endl;
            }
            Stats.IncWizardErrors(*req);
            NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(TWizardError(req->MakeLogHeader()));
            ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);
            req->LogRequestData(*ANTIROBOT_DAEMON_CONFIG.EventLog);
        }

        void ProcessCaptchaInput(const TRequestContext& rc, bool correctInput) override {
            const TRequest& req = *rc.Req;

            for (size_t level = 0; level <= TAllFactors::MAX_IP_AGGREGATION_LEVEL; ++level) {
                TUserBase::TValue tlAggr = rc.Env.UserBase.Get(TUid::FromIpAggregation(req.UserAddr, level));
                tlAggr->ProcessCaptchaInput(req.ArrivalTime.MicroSeconds(), correctInput);
            }

            if (TStringBuf ja3 = req.Ja3(); !ja3.empty()) {
                TUserBase::TValue tlAggr = rc.Env.UserBase.Get(TUid::FromJa3Aggregation(req.Ja3()));
                tlAggr->ProcessCaptchaInput(req.ArrivalTime.MicroSeconds(), correctInput);
            }

            if (correctInput && ANTIROBOT_DAEMON_CONFIG.ConfByTld(req.Tld).AllowBlock) {
                TUid uid = GetIpUid(req.UserAddr);
                TUserBase::TValue tlAggr = rc.Env.UserBase.Get(uid);
                bool wasBlocked = false;
                bool blocked = tlAggr->UpdateCaptchaInputFreq(req.ArrivalTime.MicroSeconds(), wasBlocked);
                if (blocked && !wasBlocked) {
                    const TInstant expireTime = req.ArrivalTime + ANTIROBOT_DAEMON_CONFIG.CaptchaFreqBanTime;
                    static const TString description = "Too many captcha inputs";

                    if (CbbIO) {
                        TAddr rangeStart, rangeEnd;

                        if (req.UserAddr.GetFamily() == AF_INET6) {
                            req.UserAddr.GetIntervalForMask(TUid::Ip6Bits, rangeStart, rangeEnd);
                        } else {
                            rangeStart = rangeEnd = req.UserAddr;
                        }
                        CbbIO->AddRangeBlock(ANTIROBOT_DAEMON_CONFIG.CbbFlag, rangeStart, rangeEnd, expireTime, description);
                    }
                }
            }
        }

        void PrintStats(const TRequestGroupClassifier& groupClassifier, TStatsWriter& out) const override {
            Stats.Print(groupClassifier, out);
            ProcessorExperiments.PrintStats(out);
            CacherExperiments.PrintStats(out);
            AntiDDoS.PrintStats(out);
        }

        void PrintStatsLW(
            TStatsWriter& out,
            EStatType service
        ) const override {
            Stats.PrintLW(out, service);
        }

        ITrainingSetGenerator& GetTrainSetGenerator(const TStringBuf& tld) override {
            return GetZoneData(tld).GetTrainSetGen();
        }

        static TStringBuf ReqId(const TStringBuf& reqid) {
            if (reqid.empty()) {
                return "-"sv;
            }

            return reqid;
        }

        static inline TUid GetIpUid(const TAddr& addr) {
            return TUid::FromAddrOrSubnet(addr);
        }

        void PrintFactorsToFactorsLog(const TRequest* req, bool isRobot, const TProcessorLinearizedFactors& lf, bool force) {
            if (!force && RandomNumber<float>() > ANTIROBOT_DAEMON_CONFIG.PartOfAllFactorsToPrint)
                return;

            TAntirobotFactors ev;
            ev.MutableHeader()->CopyFrom(req->MakeLogHeader());
            ev.SetIsRobot(isRobot);
            ev.MutableFactors()->Reserve(lf.size());
            ev.SetInitiallyWasXmlsearch(req->InitiallyWasXmlsearch);
            ev.SetFactorsVersion(FACTORS_VERSION);

            for (size_t i = 0; i < lf.size(); ++i) {
                ev.AddFactors(lf[i]);
            }

            NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(ev);
            ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);
        }

        static bool IsSeoQuery(const TRequestFeatures& rf) {
            static const size_t CGI_NUMDOC_INDEX = FindArrayIndex(CgiParamName, "numdoc"_sb);
            const TWizardFactorsCalculator::TSyntaxFactors& sf = rf.WizardFeatures;

            return sf.UrlRestr || sf.InUrlRestr || sf.SiteRestr || sf.HostRestr || (!rf.AdvancedSearch && rf.CgiParamPresent[CGI_NUMDOC_INDEX]);
        }

        TProcessorLinearizedFactors ApplyRequestToTls(const TRequestContext& rc, const TRequestFeatures& rf) {
            TAllFactors factors;

            {
                TUserBase::TValue tlUser = rc.Env.UserBase.Get(rf.Request->Uid);
                tlUser->ProcessRequest(rf, factors.FactorsWithAggregation[0]);
            }

            for (size_t level = 0; level <= TAllFactors::MAX_IP_AGGREGATION_LEVEL; ++level) {
                TUserBase::TValue tlAggr = rc.Env.UserBase.Get(TUid::FromIpAggregation(rf.Request->UserAddr, level));
                tlAggr->ProcessRequest(rf, factors.FactorsWithAggregation[level + 1]);
            }

            if (TStringBuf ja3 = rf.Request->Ja3(); !ja3.empty()) {
                TUserBase::TValue tlAggr = rc.Env.UserBase.Get(TUid::FromJa3Aggregation(ja3));
                tlAggr->ProcessRequest(rf, factors.FactorsWithAggregation[TAllFactors::JA3_AGGREGATION_LEVEL + 1]);
            } else {
                factors.FactorsWithAggregation[TAllFactors::JA3_AGGREGATION_LEVEL + 1].FillNaN();
            }

            rc.Env.UserBase.DeleteOldUsers();

            return factors.GetLinearizedFactors(&rc.Req->AntirobotCookie);
        }

        size_t GetPenalty(const TRequestFeatures& rf, const TMatrixNetResult& matrixNetResult) {
            const TRequest* req = rf.Request;

            if (req->Uid.Ns != TUid::SPRAVKA || !req->MayBanFor()) {
                return 0;
            }

            if (matrixNetResult.Value >= ANTIROBOT_DAEMON_CONFIG.JsonConfig[req->HostType].ProcessorThresholdForSpravkaPenalty || IsSeoQuery(rf)) {
                return ANTIROBOT_DAEMON_CONFIG.ConfByTld(rf.Request->Tld).SpravkaPenalty;
            }

            return 0;
        }

        class TStats {
        public:
            explicit TStats(std::array<size_t, HOST_NUMTYPES> groupCounts)
                : ServiceNsGroupCounters(groupCounts)
                , ServiceNsExpBinCounters()
            {}

            void Update(
                bool isRobot, const TRequest& req,
                const TMatrixNetResult& matrixNetResult, bool onCacher,
                bool marked
            ) {
                const bool highMn = matrixNetResult.Matched();

                if (req.TrustedUser) {
                    ++ServiceNsGroupCounters.Get(req, ERobotDetectorServiceNsGroupCounter::QueriesTrustedUsers);
                }

                ++ServiceNsExpBinCounters.Get(req, ERobotDetectorServiceNsExpBinCounter::Queries);
                ++ServiceNsGroupCounters.Get(req, ERobotDetectorServiceNsGroupCounter::Queries);
                if (req.CanShowCaptcha()) {
                    if (req.TrustedUser) {
                        ++ServiceNsGroupCounters.Get(req, ERobotDetectorServiceNsGroupCounter::QueriesWithCaptchaTrustedUsers);
                    }
                    ++ServiceNsGroupCounters.Get(req, ERobotDetectorServiceNsGroupCounter::QueriesWithCaptcha);
                }

                if (isRobot) {
                    ++ServiceNsGroupCounters.Get(req, ERobotDetectorServiceNsGroupCounter::Robots);

                    if (req.CanShowCaptcha()) {
                        ++ServiceCounters.Get(req, ERobotDetectorServiceCounter::RobotsWithCaptcha);
                    }
                }

                if (marked) {
                    ++ServiceCounters.Get(req, ERobotDetectorServiceCounter::MarkedByYql);
                }

                if (req.UserAddr.IsWhitelisted() && req.IsSearch) {
                    ++ServiceCounters.Get(req, ERobotDetectorServiceCounter::WhitelistQueries);

                    if (highMn) {
                        ++ServiceCounters.Get(req, ERobotDetectorServiceCounter::WhitelistQueriesSusp);
                    }
                }

                if (onCacher) {
                    ++Counters.Get(ERobotDetectorCounter::ProcessingsOnCacher);
                }
            }

            void IncUniqueRandomFactors(const TRequest& req) {
                ++ServiceCounters.Get(req, ERobotDetectorServiceCounter::UniqueRandomFactors);
            }

            void IncWizardErrors(const TRequest& req) {
                ++ServiceCounters.Get(req, ERobotDetectorServiceCounter::WizardFactors);
            }

            void IncMissedRobots(const TRequest& req, EMissedRobotStatus status) {
                switch (status) {
                case EMissedRobotStatus::MissedOnce:
                    ++ServiceCounters.Get(req, ERobotDetectorServiceCounter::MissedOnceRobots);
                    break;
                case EMissedRobotStatus::MissedMultiple:
                    ++ServiceCounters.Get(req, ERobotDetectorServiceCounter::MissedMultipleRobots);
                    break;
                default:
                    break;
                }
            }

            void Print(const TRequestGroupClassifier& groupClassifier, TStatsWriter& out) const {
                CacherExpFormulasCalcTimeStats.PrintStats(out);
                ExpFormulasCalcTimeStats.PrintStats(out);
                FactorsCalcTimeStats.PrintStats(out);
                FactorsProcessingTimeStats.PrintStats(out);
                FormulaCalcTimeStats.PrintStats(out);
                WizardCalcTimeStats.PrintStats(out);

                Counters.Print(out);
                ServiceCounters.Print(out);
                ServiceNsGroupCounters.Print(groupClassifier, out);
                ServiceNsExpBinCounters.Print(out);
            }

            void PrintLW(
                TStatsWriter& out,
                EStatType service
            ) const {
                auto service_as_host = StatTypeToHostType(service);
                if (service != EStatType::STAT_TOTAL) {
                    size_t totalCounter = 0;

                    for (size_t uidTypeIdx = 0; uidTypeIdx < static_cast<size_t>(ESimpleUidType::Count); ++uidTypeIdx) {
                        const auto uidType = static_cast<ESimpleUidType>(uidTypeIdx);
                        for (size_t expBinIdx = 0; expBinIdx < EnumValue(EExpBin::Count); ++expBinIdx) {
                            const auto expBin = static_cast<EExpBin>(expBinIdx);
                            totalCounter += ServiceNsExpBinCounters.Get(
                                service_as_host, uidType, expBin,
                                ERobotDetectorServiceNsExpBinCounter::Queries
                            );
                        }
                    }

                    out.WriteScalar(
                        ToString(ERobotDetectorServiceNsExpBinCounter::Queries),
                        totalCounter
                    );
                }
            }

            TTimeStats& GetWizardCalcTimeStats() {
                return WizardCalcTimeStats;
            }

            TTimeStats& GetFactorsCalcTimeStats() {
                return FactorsCalcTimeStats;
            }

            TTimeStats& GetFactorsProcessingTimeStats() {
                return FactorsProcessingTimeStats;
            }

            TTimeStats& GetFormulaCalcTimeStats() {
                return FormulaCalcTimeStats;
            }

            TTimeStats& GetExpFormulasCalcTimeStats() {
                return ExpFormulasCalcTimeStats;
            }

            TTimeStats& GetCacherExpFormulasCalcTimeStats() {
                return CacherExpFormulasCalcTimeStats;
            }

        private:
            TTimeStats CacherExpFormulasCalcTimeStats{TIME_STATS_VEC, "cacher_exp_formulas_calc_"};
            TTimeStats ExpFormulasCalcTimeStats{TIME_STATS_VEC, "exp_formulas_calc_"};
            TTimeStats FactorsCalcTimeStats{TIME_STATS_VEC, "factors_calc_"};
            TTimeStats FactorsProcessingTimeStats{TIME_STATS_VEC, "factors_processing_"};
            TTimeStats FormulaCalcTimeStats{TIME_STATS_VEC, "formula_calc_"};
            TTimeStats WizardCalcTimeStats{TIME_STATS_VEC, "wizard_calc_"};

            TCategorizedStats<
                std::atomic<size_t>, ERobotDetectorCounter
            > Counters;

            TCategorizedStats<
                std::atomic<size_t>, ERobotDetectorServiceCounter,
                EHostType
            > ServiceCounters;

            TCategorizedStats<
                std::atomic<size_t>, ERobotDetectorServiceNsGroupCounter,
                EHostType, ESimpleUidType, EReqGroup
            > ServiceNsGroupCounters;

            TCategorizedStats<
                std::atomic<size_t>, ERobotDetectorServiceNsExpBinCounter,
                EHostType, ESimpleUidType, EExpBin
            > ServiceNsExpBinCounters;
        } Stats;

        class TTrainingSetGeneratorAdapter : public ITrainingSetGenerator {
        public:
            TTrainingSetGeneratorAdapter(const TString& schemeConfigString) {
                THashMap<TStringBuf, ITrainingSetGenerator*> name2scheme;
                ITrainingSetGenerator* const GENS[] = {RandomUserTrainingSetGenerator(),
                                                       NullTrainingSetGenerator()};
                for (auto generator : GENS) {
                    name2scheme[generator->Name()] = generator;
                }

                THttpCookies schemes(schemeConfigString);
                ITrainingSetGenerator* defaultScheme = RandomUserTrainingSetGenerator();
                if (schemes.Has("default")) {
                    defaultScheme = name2scheme[schemes.Get("default")];
                }
                for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
                    TString hostType = ToString(static_cast<EHostType>(i));
                    if (schemes.Has(hostType)) {
                        TrainingSetGenerators[i] = name2scheme[schemes.Get(hostType)];
                    } else {
                        TrainingSetGenerators[i] = defaultScheme;
                    }
                }
            }

            void ProcessCaptchaGenerationError(const TRequestContext& rc) override {
                return GetGen(*rc.Req)->ProcessCaptchaGenerationError(rc);
            }

            EResult ProcessRequest(const TRequestContext& rc) override {
                return GetGen(*rc.Req)->ProcessRequest(rc);
            }
            TStringBuf Name() override {
                // Just to avoid compile errors.
                return TStringBuf();
            }

        private:
            ITrainingSetGenerator* GetGen(const TRequest& req) {
                /* Для XML-запросов, присылаемых партнёрами, у нас HostType == HOST_WEB
                 * (см. TRequest::ApplyHackForXmlPartners()). Если для HOST_WEB
                 * установлена схема генерации обучающей выборки, основанная на
                 * счётчике SERP'а, она неверно будет работать для партнёрского XML-поиска.
                 * Поэтому для партнёрских XML-запросов здесь жёстко задана схема со
                 * случайной капчей.
                 * */
                if (req.ClientType == CLIENT_XML_PARTNER) {
                    return RandomUserTrainingSetGenerator();
                }
                return GetGen(req.HostType);
            }
            ITrainingSetGenerator* GetGen(EHostType hostType) {
                return TrainingSetGenerators[hostType];
            }

        private:
            ITrainingSetGenerator* TrainingSetGenerators[HOST_NUMTYPES];
        };

        class TZoneData : public TNonCopyable {
            using TClassifierPtr = TSimpleSharedPtr<TClassificator<TProcessorLinearizedFactors>>;

            std::array<TClassifierPtr, HOST_NUMTYPES> Classifies;
            std::array<TClassifierPtr, HOST_NUMTYPES> FallbackClassifies;
            TTrainingSetGeneratorAdapter TrainSetGen;
            THashMap<TString, TClassifierPtr> ClassifiersCache;

            TClassifierPtr GetClassifier(const TString& fileName) {
                if (!ClassifiersCache.contains(fileName)) {
                    ClassifiersCache[fileName] = CreateProcessorClassificator(fileName);
                }
                return ClassifiersCache[fileName];
            }

        public:
            TZoneData(
                const std::array<TString, HOST_NUMTYPES>& formulaFiles,
                const std::array<TString, HOST_NUMTYPES>& fallbackFormulaFiles,
                const TString& schemeConfigString
            )
                : TrainSetGen(schemeConfigString)
            {
                for (size_t i = 0; i < HOST_NUMTYPES; ++i) {
                    EHostType hostType = static_cast<EHostType>(i);
                    Classifies[hostType] = GetClassifier(formulaFiles[i]);
                    if (fallbackFormulaFiles[i]) {
                        FallbackClassifies[hostType] = GetClassifier(fallbackFormulaFiles[i]);
                    }
                }
            }

            const TClassificator<TProcessorLinearizedFactors>* GetClassifierForHost(EHostType hostType) const {
                return Classifies[hostType].Get();
            }

            const TClassificator<TProcessorLinearizedFactors>* GetFallbackClassifierForHost(EHostType hostType) const {
                return FallbackClassifies[hostType].Get();
            }

            TTrainingSetGeneratorAdapter& GetTrainSetGen() {
                return TrainSetGen;
            }
        };

        inline TZoneData& GetZoneData(const TStringBuf& tld) {
            return *ZoneData[ANTIROBOT_DAEMON_CONFIG.ConfByTld(tld).Tld];
        }

        const TReloadableData& ReloadableData;

        TUnifiedAgentDaemonLog RequestLogJson;

        ICbbIO* CbbIO;
        const std::array<THolder<TIsBlocked>, EHostType::HOST_NUMTYPES>& IsBlocked;
        TBlocker& Blocker;
        TAntiDDoS AntiDDoS;

        THolder<TWizardFactorsCalculator> ReqWizard_;

        THashMap<TCiString, TAtomicSharedPtr<TZoneData>> ZoneData;
        TClassifyExperiments<TProcessorLinearizedFactors> ProcessorExperiments;
        TClassifyExperiments<TCacherLinearizedFactors> CacherExperiments;
    };

    IRobotDetector* CreateRobotDetector(const TEnv& env, const std::array<THolder<TIsBlocked>, EHostType::HOST_NUMTYPES>& isBlocked, TBlocker& blocker, ICbbIO* cbbIO) {
        if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService) {
            return new TDummyRobotDetector();
        }

        return new TRobotDetector(env, isBlocked, blocker, cbbIO);
    }
}
