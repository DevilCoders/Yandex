#include "config.h"

#include "eventlog_err.h"
#include "host_ops.h"
#include "unified_agent_log.h"
#include "localized_data.h"

#include <antirobot/lib/ar_utils.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/yconf/conf.h>

#include <util/folder/dirut.h>
#include <util/stream/file.h>
#include <util/stream/str.h>
#include <util/string/cast.h>
#include <util/string/join.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/string/vector.h>

#include <tuple>

namespace NAntiRobot {
    namespace {
        const TString DEFAULT_TLD = "ru";
        const TDuration DEFAULT_MIN_FUID_AGE = TDuration::Days(1);

        const TString DEFAULT_CAPTCHA_API_HOST = "new.captcha.yandex.net";
        const TDuration DEFAULT_CAPTCHA_API_TIMEOUT = TDuration::MilliSeconds(7);
        const TDuration DEFAULT_CAPTCHA_REDIRECT_TIMEOUT = TDuration::Seconds(3600);
        const TDuration DEFAULT_FURY_TIMEOUT = TDuration::MilliSeconds(100);
        const TDuration DEFAULT_DB_SYNC_INTERVAL = TDuration::Seconds(5 * 60);
        const size_t DEFAULT_MAX_ITEMS_TO_SYNC = 1000;

        const size_t DEFAULT_MAX_DB_SIZE = 5000000;
        const size_t DEFAULT_DB_USERS_TO_DELETE = 500000;

        const TString DEFAULT_LKEYS_PATH = "lkeys.txt";
        const TString DEFAULT_SPRAVKA_DATA_KEY_PATH = "data/spravka_data_key.txt";
        const TString DEFAULT_AUTORU_OFFER_SALT_PATH = "data/autoru_offer_salt.txt";
        const TString DEFAULT_BALANCER_JWS_KEY_PATH = "";
        const TString DEFAULT_NARWHAL_JWS_KEY_PATH = "data/narwhal_jws_key";
        const TString DEFAULT_MARKET_JWS_KEY_PATH = "data/market_jws_key";
        const TString DEFAULT_AUTO_RU_TAMPER_SALT_PATH = "data/autoru_tamper_salt";
        const TString DEFAULT_HYPOCRISY_BUNDLE_PATH = "data/hypocrisy";
        const size_t DEFAULT_HYPOCRISY_CACHE_CONTROL_MAX_AGE = 9 * 60 * 60;

        const size_t DEFAULT_MAX_SAFE_USERS = 200000;
        const size_t DEFAULT_USERS_TO_DELETE = 50000;

        const size_t DEFAULT_RPS_FILTER_SIZE = 2000;
        const TDuration DEFAULT_RPS_FILTER_MIN_SAFE_INTERVAL = TDuration::Seconds(60);
        const TDuration DEFAULT_RPS_FILTER_REMEMBER_FOR_INTERVAL = TDuration::Seconds(60);

        const TDuration DEFAULT_AMNESTY_INTERVAL = TDuration::Hours(1);
        const TDuration DEFAULT_SPRAVKA_EXPIRE_INTERVAL = TDuration::Days(365 * 100);
        const TDuration DEFAULT_NARWHAL_JWS_LEEWAY = TDuration::Seconds(300);
        const TDuration DEFAULT_MARKET_JWS_LEEWAY = TDuration::Seconds(300);
        const TDuration DEFAULT_HYPOCRISY_INSTANCE_LEEWAY = TDuration::Hours(2);
        const TDuration DEFAULT_HYPOCRISY_FINGERPRINT_LIFETIME = TDuration::Minutes(15);

        const TDuration DEFAULT_CAPTCHA_FREQ_BAN_TIME = TDuration::Hours(3);
        const size_t DEFAULT_CAPTCHA_FREQ_MAX_INPUTS = 1000;
        const TDuration DEFAULT_CAPTCHA_FREQ_MAX_INTERVAL = TDuration::Hours(24);
        const TString DEFAULT_CAPTCHA_TYPES = DEFAULT_CAPTCHA_TYPE_SERVICE + "=estd";

        const bool DEFAULT_INITIAL_CHINA_REDIRECT_ENABLED = true;
        const bool DEFAULT_INITIAL_CHINA_UNAUTHORIZED_ENABLED = true;

        void AddBase(const TString& base, TString& path) {
            if (!base.empty() && !path.empty() && path[0] != '/')
                path = base + "/" + path;
        }

        const TString DEFAULT_CBB_API_HOST = "cbb.yandex.net";
        const TDuration DEFAULT_CBB_API_TIMEOUT = TDuration::Seconds(5);
        const TCbbGroupId DEFAULT_CBB_FLAG{159}; // this is test flag number; should be changed in production
        const TCbbGroupId DEFAULT_CBB_FLAG_NONBLOCKING{160};
        const TCbbGroupId DEFAULT_CBB_FLAG_DDOS1{159}; // this is test flag number; should be changed in production
        const TCbbGroupId DEFAULT_CBB_FLAG_DDOS2{159};
        const TCbbGroupId DEFAULT_CBB_FLAG_DDOS3{159};
        const TCbbGroupId DEFAULT_CBB_FLAG_IP_BASED_IDENTIFICATIONS_BAN{225};
        const TCbbGroupId DEFAULT_CBB_FLAG_IGNORE_SPRAVKA{328};
        const TCbbGroupId DEFAULT_CBB_FLAG_MAX_ROBOTNESS{329};
        const TCbbGroupId DEFAULT_CBB_FLAG_SUSPICIOUSNESS{511};
        const TCbbGroupId DEFAULT_CBB_FLAG_CUT_REQUESTS{423};
        const TCbbGroupId DEFAULT_CBB_FLAG_NOT_BANNED{499};
        const TCbbGroupId DEFAULT_CBB_FLAG_DEGRADATION{661};
        const TCbbGroupId DEFAULT_CBB_FLAG_BAN_SOURCE_IP{808};
        const TCbbGroupId DEFAULT_CBB_FLAG_BAN_FW_SOURCE_IP{974};
        const TCbbGroupId DEFAULT_CBB_FLAG_BAN_FW_SOURCE_IPS_CONTAINTER{824};


        const TDuration DEFAULT_CBB_SYNC_PERIOD = TDuration::Minutes(10);
        const TDuration DEFAULT_CBB_MERGE_PERIOD = TDuration::Seconds(30);

        const TString YANDEX_IPS_FILE = "yandex_ips";
        const TString SPECIAL_IPS_FILE = "special_ips";

        const TString DEFAULT_ROBOT_UIDS_DUMP_FILE = "robot_uids_dump";
        const TDuration DEFAULT_ROBOT_UIDS_DUMP_PERIOD = TDuration::Hours(1);
        const TString DEFAULT_TURBO_PROXY_IPS_FILE = "data/trbosrvnets";
        const TString DEFAULT_EXP_FORMULAS = "";
        const TString DEFAULT_EXP_THRESHOLDS = "";
        const TString DEFAULT_SERVER_QUEUE_PARAMS = "threads_min=6; threads_max=16;  queue_size=2000";
        const TString DEFAULT_PROCESSING_QUEUE_PARAMS = "threads_min=6; threads_max=56; queue_size=10000";
        const TString DEFAULT_PROCESSOR_RESPONSE_APPLY_QUEUE_PARAMS = "threads_min=2; threads_max=10; queue_size=10000";
        const TString DEFAULT_THREAD_POOL_PARAMS = "free_min=4; free_max=300; total_max=1000; increase=2";

        const size_t DEFAULT_MAX_REQUEST_LENGTH = 1024;
        const TString DEFAULT_BAD_USER_AGENTS_FILE = "data/bad_user_agents.lst";
        const TString DEFAULT_BAD_USER_AGENTS_NEW_FILE = "data/bad_user_agents.lst";
        // TODO: Own file for new badua?

        const TDuration BLOCKER_AMNESTY_PERIOD_DEFAULT = TDuration::Seconds(30);

        const TString DEFAULT_KEYS_FILE = "data/keys";

        const TString DEFAULT_SUPPRESSED_COOKIES_IN_LOGS = "Session_id,sessionid2,ya_sess_id,sessguard";
        const size_t DEFAULT_CHARACTERS_FROM_PRIVATE_COOKIE_TO_HIDE = 30;

        THashMap<TString, TStochasticSelector> LoadCaptchaTypes(const TString& conf) {
            THashMap<TString, TStochasticSelector> result;
            THttpCookies cookies(conf);
            for (const auto& cook : cookies) {
                TStringBuf value = cook.second;
                Y_ENSURE(!value.empty(), "Empty captcha type for host " << cook.first);
                if (value.SkipPrefix("{")) {
                    Y_ENSURE(value.ChopSuffix("}"), "Unable to parse key-value captcha type pair " << cook.first << " " << cook.second);
                    TMultiMap<float, TString> probs;
                    for (const auto& it : StringSplitter(StripString(value)).Split(',').SkipEmpty()) {
                        TStringBuf captchaType;
                        TStringBuf probability;
                        Split(it.Token(), ":", probability, captchaType);
                        probs.emplace(FromString<float>(StripString(probability)), StripString(captchaType));
                    }
                    result.emplace(cook.first, probs);
                } else {
                    result.emplace(cook.first, ToString(cook.second));
                }
            }

            return result;
        }

        TAntirobotDaemonConfig::TEndpointSetDescription ParseEndpointSetDescription(TStringBuf str) {
            TAntirobotDaemonConfig::TEndpointSetDescription ret;

            const auto tokenRange = StringSplitter(str).Split(':');
            auto tokenIt = tokenRange.begin();
            ++tokenIt;

            Y_ENSURE(tokenIt != tokenRange.end(), "missing cluster name");
            ret.Cluster = tokenIt->Token();
            ++tokenIt;

            Y_ENSURE(tokenIt != tokenRange.end(), "missing endpoint set name");
            ret.Id = tokenIt->Token();
            ++tokenIt;

            if (tokenIt != tokenRange.end()) {
                ret.Port = FromString<ui16>(tokenIt->Token());
            }

            Y_ENSURE(tokenIt == tokenRange.end(), "expected end of string");

            return ret;
        }

        THostAddr ParseHostAddrOptions(TStringBuf addrOptions, bool skipHostNames = false) {
            // we assume that first element is a name of the host and it's not empty
            TString hostName = "";
            for (const auto& iter : StringSplitter(addrOptions).Split(';').SkipEmpty()) {
                TString host = ToString(iter.Token());
                if (hostName.empty()) {
                    hostName = host;
                    if (skipHostNames) {
                        continue;
                    }
                }

                try {
                    auto addr = CreateNetworkAddress(host);
                    return THostAddr(std::get<0>(addr), std::get<1>(addr), std::get<2>(addr), hostName);
                } catch (...) {
                }
            }
            ythrow yexception() << "Failed to resolve host from " << addrOptions;
        }

        std::pair<TString, THostAddr> ParseIdHostAddrOptions(TStringBuf str) {
            TStringBuf id, rest;
            Y_ENSURE(
                str.TrySplit(';', id, rest),
                "Expected ';' after id: " << str
            );

            id.SkipPrefix("id=");

            return {TString(id), ParseHostAddrOptions(rest)};
        }

        void FillBrowserJsPrintFeaturesMapping(THashMap<EBrowserJsPrintFeature, TString>& mapping) {
            TString configString;
            Y_ENSURE(NResource::FindExact("js_print_mapping.json", &configString));

            NJson::TJsonValue json;
            Y_ENSURE(NJson::ReadJsonTree(configString, &json));

            for (const auto& [longName, shortName] : json.GetMap()) {
                EBrowserJsPrintFeature feature;
                Y_ENSURE(TryFromString(longName, feature), "Feature '" << longName << "' is not present in EBrowserJsPrintFeature");
                mapping[feature] = shortName.GetStringSafe();
            }
            for (ui32 i = 0; i < static_cast<ui32>(EBrowserJsPrintFeature::Count); i++) {
                EBrowserJsPrintFeature feature = static_cast<EBrowserJsPrintFeature>(i);
                if (EqualToOneOf(feature, EBrowserJsPrintFeature::ValidJson, EBrowserJsPrintFeature::HasData)) {
                    continue;
                }

                Y_ENSURE(!!mapping[feature], "Has no mapping for '" << ToString(feature) << "' feature");
            }
        }
    } // anonymous namespace

    TAntirobotDaemonConfig::TAntirobotDaemonConfig()
        : RemoteWizardConfig(nullptr)
        , NumUnresolvedDaemons(0)
    {
        DefaultZoneConf.Tld = DEFAULT_TLD;

        // Initialize with default values
        LoadFromString("<Daemon>\n"
                       "CaptchaApiHost = ::\n"
                       "CbbApiHost = ::\n"
                       "</Daemon>");
    }

    TAntirobotDaemonConfig::~TAntirobotDaemonConfig() {
    }

    void TAntirobotDaemonConfig::LoadZoneConfigs(TYandexConfig::Section* section) {
        if (!DefaultHost.empty())
            DefaultZoneConf.Tld = GetTldFromHost(DefaultHost);

        LoadZoneConf(DefaultZoneConf, section->GetDirectives(), true);

        TYandexConfig::Section* child = section->Child;
        while (child) {
            const TString& tld = child->Name;
            TZoneConf& conf = ZoneConfigs[tld];
            conf = DefaultZoneConf;
            conf.Tld = tld;
            LoadZoneConf(conf, child->GetDirectives(), false);

            child = child->Next;
        }
    }

    TAntirobotDaemonConfig::TZoneConfNames TAntirobotDaemonConfig::GetZoneConfNames() const {
        TZoneConfNames res;

        res.push_back(DefaultZoneConf.Tld);
        for (TConfMap::const_iterator it = ZoneConfigs.begin(); it != ZoneConfigs.end(); it++)
            res.push_back(it->first);

        return res;
    }

    void TAntirobotDaemonConfig::DoLoad(TAntirobotConfStrings& cs) {
        TYandexConfig::Section* curSection = cs.GetRootSection()->Child;
        while (curSection) {
            if (!curSection->Parsed()) {
                TString errStr;
                cs.PrintErrors(errStr);
                Trace("Error parsing config: '%s'\n", errStr.c_str());
                continue;
            }

            if (curSection->Name == "Daemon"sv) {
                LoadGeneralConf(curSection->GetDirectives());
            } else if (curSection->Name == "Zone"sv) {
                LoadZoneConfigs(curSection);
            } else if (curSection->Name == "WizardsRemote"sv) {
                RemoteWizardConfig = curSection;
            } else {
                Trace("Warning: section '%s' will be ignored\n", curSection->Name);
            }

            curSection = curSection->Next;
        }
    }

    void TAntirobotDaemonConfig::DumpZoneConf(IOutputStream& out, const TZoneConf& conf) const {
        const TStringBuf ident = "    ";
#define DUMP_PROP(prop) out << "    " << #prop << " = " << conf.prop << Endl

        auto dumpFunc = [&](const THashMap<TString, TStochasticSelector>& field, TStringBuf name) {
            out << ident << name << " = ";
            for (const auto& pair : field) {
                out << pair.first << "={";
                for (const auto& it : pair.second.GetProbabilityToCaptchaType()) {
                    out << it.first << ": " << it.second << ", ";
                }
                out << "}; ";
            }
            out << Endl;
        };

        DUMP_PROP(SpravkaPenalty);

        DUMP_PROP(DDosFlag1BlockEnabled);
        DUMP_PROP(DDosFlag2BlockEnabled);

        DUMP_PROP(PartnerCaptchaType);

        dumpFunc(conf.CaptchaTypes, "CaptchaTypes"_sb);

        DUMP_PROP(AllowBlock);
        DUMP_PROP(TrainingSetGenSchemes);
#undef DUMP_PROP
    }

    void TAntirobotDaemonConfig::Dump(IOutputStream& out, EHostType service) const {
#define DUMP_PROP(prop) out << #prop << " = " << prop << Endl
        DUMP_PROP(BaseDir);
        DUMP_PROP(LogsDir);
        DUMP_PROP(RuntimeDataDir);
        DUMP_PROP(TVMClientCacheDir);
        DUMP_PROP(TVMClientsList);
        DUMP_PROP(WorkMode);
        DUMP_PROP(DiscoveryFrequency);
        DUMP_PROP(DisableBansByFactors);
        DUMP_PROP(DisableBansByYql);
        DUMP_PROP(DiscoveryCacheDir);
        DUMP_PROP(DiscoveryHost);
        DUMP_PROP(DiscoveryPort);
        DUMP_PROP(DiscoveryInitTimeout);
        DUMP_PROP(AllDaemons);
        DUMP_PROP(Port);
        DUMP_PROP(ProcessServerPort);
        DUMP_PROP(AdminServerPort);
        DUMP_PROP(AdminServerThreads);
        DUMP_PROP(UnistatServerPort);
        DUMP_PROP(MaxConnections);
        DUMP_PROP(ProcessServerMaxConnections);
        DUMP_PROP(AdminServerMaxConnections);
        DUMP_PROP(UnistatServerMaxConnections);
        DUMP_PROP(ServerQueueParams);
        DUMP_PROP(ProcessServerQueueParams);
        DUMP_PROP(ProcessingQueueParams);
        DUMP_PROP(ProcessorResponseApplyQueueParams);
        DUMP_PROP(AuthorizeByFuid);
        DUMP_PROP(AuthorizeByLCookie);
        DUMP_PROP(AuthorizeByICookie);
        DUMP_PROP(MinFuidAge);
        DUMP_PROP(MinICookieAge);
        DUMP_PROP(MinRequestsWithSpravka);
        DUMP_PROP(TimeDeltaMinDeltas);
        DUMP_PROP(TimeDeltaMaxDeviation);
        DUMP_PROP(PartOfAllFactorsToPrint);
        DUMP_PROP(PartOfRegularCaptchaRedirectFactorsToPrint);
        DUMP_PROP(UnifiedAgentUri);
        DUMP_PROP(DDosRpsThreshold);
        DUMP_PROP(DDosAmnestyPeriod);
        DUMP_PROP(DDosFlag1BlockPeriod);
        DUMP_PROP(DDosSmoothFactor);
        DUMP_PROP(DDosFlag2BlockPeriod);

        DUMP_PROP(FormulasDir);
        out << "ProcessorExpFormulas = "
            << JoinStrings(ProcessorExpFormulas.begin(), ProcessorExpFormulas.end(), TStringBuf(", "))
            << Endl;

        out << "ProcessorExpThresholds = "
            << JoinStrings(ProcessorExpThresholds.begin(), ProcessorExpThresholds.end(), TStringBuf(", "))
            << Endl;

        DUMP_PROP(ChinaUrlNotLoginnedRedirect);
        DUMP_PROP(InitialChinaRedirectEnabled);
        DUMP_PROP(InitialChinaUnauthorizedEnabled);
        DUMP_PROP(CaptchaRedirectToSameHost);
        DUMP_PROP(CaptchaApiHost);
        DUMP_PROP(CaptchaApiProtocol);
        DUMP_PROP(CaptchaGenTryCount);
        DUMP_PROP(CaptchaGenTimeout);
        DUMP_PROP(CaptchaCheckTimeout);
        DUMP_PROP(CaptchaRedirectTimeOut);
        DUMP_PROP(ProxyCaptchaUrls);
        DUMP_PROP(CaptchaRequestKeyLifetime);

        DUMP_PROP(FuryEnabled);
        DUMP_PROP(FuryHost);
        DUMP_PROP(FuryPreprodEnabled);
        DUMP_PROP(FuryPreprodHost);
        DUMP_PROP(FuryProtocol);
        DUMP_PROP(FuryBaseTimeout);
        DUMP_PROP(FuryTVMClientId);
        DUMP_PROP(FuryPreprodTVMClientId);

        DUMP_PROP(WhiteList);
        DUMP_PROP(UaProxyList);
        DUMP_PROP(WhiteListsDir);
        DUMP_PROP(YandexIpsDir);
        DUMP_PROP(PrivilegedIpsFile);
        DUMP_PROP(AllowBannedIpsAtNight);
        DUMP_PROP(DebugOutput);
        DUMP_PROP(OutputWizardFactors);
        DUMP_PROP(IgnoreYandexIps);
        DUMP_PROP(SpravkaApiExpireInterval);
        DUMP_PROP(SpravkaExpireInterval);
        DUMP_PROP(SpravkaIgnoreIfInRobotSet);
        DUMP_PROP(SpravkaRequestsLimit);
        DUMP_PROP(KeysFile);

        DUMP_PROP(UseBdb);
        DUMP_PROP(DbSyncInterval);
        DUMP_PROP(MaxItemsToSync);
        DUMP_PROP(MaxDbSize);
        DUMP_PROP(DbUsersToDelete);
        DUMP_PROP(NumWriteErrorsToResetDb);
        DUMP_PROP(GeodataBinPath);
        DUMP_PROP(DictionariesDir);
        DUMP_PROP(LCookieKeysPath);
        DUMP_PROP(YascKeyPath);
        DUMP_PROP(SpravkaDataKeyPath);
        DUMP_PROP(AutoruOfferSaltPath);
        DUMP_PROP(BalancerJwsKeyPath);
        DUMP_PROP(NarwhalJwsKeyPath);
        DUMP_PROP(NarwhalJwsLeeway);
        DUMP_PROP(MarketJwsKeyPath);
        DUMP_PROP(MarketJwsLeeway);
        DUMP_PROP(AutoRuTamperSaltPath);
        DUMP_PROP(HypocrisyInject);
        DUMP_PROP(HypocrisyBundlePath);
        DUMP_PROP(HypocrisyInstanceLeeway);
        DUMP_PROP(HypocrisyFingerprintLifetime);
        DUMP_PROP(HypocrisyCacheControlMaxAge);
        DUMP_PROP(LogLevel);
        DUMP_PROP(MaxSafeUsers);
        DUMP_PROP(UsersToDelete);
        DUMP_PROP(RpsFilterSize);
        DUMP_PROP(RpsFilterMinSafeInterval);
        DUMP_PROP(RpsFilterRememberForInterval);

        DUMP_PROP(RemoveExpiredPeriod);
        DUMP_PROP(AmnestyICookieInterval);
        DUMP_PROP(AmnestyIpInterval);
        DUMP_PROP(AmnestyIpV6Interval);
        DUMP_PROP(AmnestyFuidInterval);
        DUMP_PROP(AmnestyLCookieInterval);

        DUMP_PROP(CaptchaFreqMaxInputs);
        DUMP_PROP(CaptchaFreqMaxInterval);
        DUMP_PROP(CaptchaFreqBanTime);

        DUMP_PROP(CbbApiHost);
        DUMP_PROP(CbbApiTimeout);
        DUMP_PROP(CbbFlag);
        DUMP_PROP(CbbFlagNonblocking);
        DUMP_PROP(CbbFlagDDos1);
        DUMP_PROP(CbbFlagDDos2);
        DUMP_PROP(CbbFlagDDos3);
        DUMP_PROP(CbbFlagIpBasedIdentificationsBan);
        DUMP_PROP(CbbFlagIgnoreSpravka);
        DUMP_PROP(CbbFlagMaxRobotness);
        DUMP_PROP(CbbFlagSuspiciousness);
        DUMP_PROP(CbbFlagCutRequests);
        DUMP_PROP(CbbFlagNotBanned);
        DUMP_PROP(CbbFlagDegradation);
        DUMP_PROP(CbbFlagBanSourceIp);
        DUMP_PROP(CbbFlagBanFWSourceIp);
        DUMP_PROP(CbbFlagBanFWSourceIpsContainer);
        DUMP_PROP(CbbEnabled);
        DUMP_PROP(CbbSyncPeriod);
        DUMP_PROP(CbbMergePeriod);
        DUMP_PROP(CbbTVMClientId);
        DUMP_PROP(CbbCacheFile);
        DUMP_PROP(CbbCachePeriod);
        DUMP_PROP(CbbAddBanFWPeriod);

        DUMP_PROP(SpecialIpsFile);

        DUMP_PROP(BlocksDumpFile);
        DUMP_PROP(RobotUidsDumpFile);
        DUMP_PROP(RobotUidsDumpPeriod);

        DUMP_PROP(TurboProxyIps);
        DUMP_PROP(ThreadPoolParams);
        DUMP_PROP(MaxRequestLength);
        DUMP_PROP(BadUserAgentsFile);
        DUMP_PROP(BadUserAgentsNewFile);
        DUMP_PROP(SearchBotsFile);
        DUMP_PROP(SearchBotsLiveTime);
        DUMP_PROP(MaxSearchBotsCandidates);
        DUMP_PROP(DefaultHost);
        DUMP_PROP(ProcessServerReadRequestTimeout);
        DUMP_PROP(ServerReadRequestTimeout);
        DUMP_PROP(AdminServerReadRequestTimeout);
        DUMP_PROP(UnistatServerReadRequestTimeout);
        DUMP_PROP(ServerFailOnReadRequestTimeout);
        DUMP_PROP(ForwardRequestTimeout);
        DUMP_PROP(CaptchaInputForwardLocation);
        DUMP_PROP(RequestForwardLocation);
        DUMP_PROP(DeadBackendSkipperMaxProbability);
        DUMP_PROP(NightStartHour);
        DUMP_PROP(NightEndHour);
        DUMP_PROP(DaemonLogFormatJson);
        DUMP_PROP(UseTVMClient);
        DUMP_PROP(AntirobotTVMClientId);
        DUMP_PROP(MatrixnetResultForPrivileged);
        DUMP_PROP(MatrixnetResultForAlreadyRobot);
        DUMP_PROP(MatrixnetResultForNoMatrixnetReqTypes);
        DUMP_PROP(MatrixnetFallbackProbability);
        DUMP_PROP(IpV4SubnetBitsSizeForHashing);
        DUMP_PROP(IpV6SubnetBitsSizeForHashing);
        DUMP_PROP(CustomHashingRules);
        DUMP_PROP(ReBanRobotsPeriod);
        DUMP_PROP(YandexTrustTokenExpireInterval);

        DUMP_PROP(HandleStopBlockFilePath);
        DUMP_PROP(HandleStopBanFilePath);
        DUMP_PROP(HandleStopBlockForAllFilePath);
        DUMP_PROP(HandleStopBanForAllFilePath);
        DUMP_PROP(HandleStopFuryForAllFilePath);
        DUMP_PROP(HandleStopFuryPreprodForAllFilePath);
        DUMP_PROP(HandleStopYqlFilePath);
        DUMP_PROP(HandleStopYqlForAllFilePath);
        DUMP_PROP(HandleStopDiscoveryForAllFilePath);
        DUMP_PROP(HandleServerErrorEnablePath);
        DUMP_PROP(HandleServerErrorDisableServicePath);
        DUMP_PROP(HandleCbbPanicModePath);
        DUMP_PROP(HandleManyRequestsEnableServicePath);
        DUMP_PROP(HandleManyRequestsMobileEnableServicePath);
        DUMP_PROP(HandleSuspiciousBanServicePath);
        DUMP_PROP(HandleSuspiciousBlockServicePath);
        DUMP_PROP(HandleAmnestyFilePath);
        DUMP_PROP(HandleAmnestyForAllFilePath);
        DUMP_PROP(HandleCatboostWhitelistAllFilePath);
        DUMP_PROP(HandleCatboostWhitelistServicePath);
        DUMP_PROP(HandleAntirobotDisableExperimentsPath);
        DUMP_PROP(HandleWatcherPollInterval);
        DUMP_PROP(CacherRandomFactorsProbability);
        DUMP_PROP(CharactersFromPrivateCookieToHide);
        DUMP_PROP(GlobalJsonConfFilePath);
        DUMP_PROP(JsonConfFilePath);
        DUMP_PROP(ExperimentsConfigFilePath);

        DUMP_PROP(Local);
        DUMP_PROP(LockMemory);
        DUMP_PROP(AsCaptchaApiService);
        DUMP_PROP(YdbEndpoint);
        DUMP_PROP(YdbDatabase);
        DUMP_PROP(YdbMaxActiveSessions);
        DUMP_PROP(YdbSessionReadTimeout);
        DUMP_PROP(YdbSessionWriteTimeout);
        DUMP_PROP(CloudCaptchaApiEndpoint);
        DUMP_PROP(CloudCaptchaApiKeepAliveTime);
        DUMP_PROP(CloudCaptchaApiKeepAliveTimeout);
        DUMP_PROP(CloudCaptchaApiGetClientKeyTimeout);
        DUMP_PROP(CloudCaptchaApiGetServerKeyTimeout);
        DUMP_PROP(StaticFilesVersion);

        DUMP_PROP(ProcessorThresholdForSpravkaPenalty);

        DUMP_PROP(HandlePingControlFilePath);

        JsonConfig.Dump(out, service);
#undef DUMP_PROP

        Cerr << DefaultZoneConf.Tld << ':' << Endl;
        DumpZoneConf(out, DefaultZoneConf);
        for (TConfMap::const_iterator it = ZoneConfigs.begin(); it != ZoneConfigs.end(); it++) {
            Cerr << it->first << ":" << Endl;
            DumpZoneConf(out, it->second);
        }

        out.Flush();
    }

    void TAntirobotDaemonConfig::SetBaseDir(const TString& baseDir) {
        BaseDir = baseDir;
    }

    void TAntirobotDaemonConfig::SetLogsDir(const TString& logsDir) {
        LogsDir = logsDir;
    }

    TString TAntirobotDaemonConfig::GetSubDirFullPath(TString subDir) const {
        AddBase(BaseDir, subDir);

        if (subDir.empty())
            subDir = BaseDir;

        return subDir;
    }

    void TAntirobotDaemonConfig::LoadGeneralConf(const TYandexConfig::Directives& directives) {
#define RELOAD_PROP(prop, def) prop = directives.Value(#prop, def)

        auto reloadHost = [&directives](THostAddr& value, const TString& name, const TString& defaultValue) {
            value = ParseHostAddrOptions(directives.Value(name, defaultValue));
        };

        if (BaseDir.empty()) {
            RELOAD_PROP(BaseDir, TString("."));
        }
        if (LogsDir.empty()) {
            RELOAD_PROP(LogsDir, TString());
        }
        RELOAD_PROP(RuntimeDataDir, TString());
        RELOAD_PROP(WorkMode, WORK_ENABLED);

        RELOAD_PROP(Port, 13512);
        RELOAD_PROP(ProcessServerPort, Port + 1);
        RELOAD_PROP(AdminServerPort, Port + 2);
        RELOAD_PROP(UnistatServerPort, Port + 3);
        RELOAD_PROP(AdminServerThreads, 3);
        RELOAD_PROP(MaxConnections, 2000);
        RELOAD_PROP(ProcessServerMaxConnections, 2000);
        RELOAD_PROP(AdminServerMaxConnections, 2000);
        RELOAD_PROP(UnistatServerMaxConnections, 2000);
        RELOAD_PROP(AuthorizeByFuid, false);
        RELOAD_PROP(AuthorizeByLCookie, false);
        RELOAD_PROP(AuthorizeByICookie, false);
        RELOAD_PROP(MinFuidAge, DEFAULT_MIN_FUID_AGE);
        RELOAD_PROP(MinICookieAge, DEFAULT_MIN_FUID_AGE); // same as default fuid age
        RELOAD_PROP(MinRequestsWithSpravka, 0);

        RELOAD_PROP(TimeDeltaMinDeltas, 5);
        RELOAD_PROP(TimeDeltaMaxDeviation, 0.1f);
        RELOAD_PROP(PartOfAllFactorsToPrint, 1.0f);
        RELOAD_PROP(PartOfRegularCaptchaRedirectFactorsToPrint, 0.01f);

        RELOAD_PROP(UnifiedAgentUri, TString());
        RELOAD_PROP(DDosRpsThreshold, 10.0);
        RELOAD_PROP(DDosAmnestyPeriod, TDuration::Minutes(30));
        RELOAD_PROP(DDosFlag1BlockPeriod, TDuration::Minutes(5));
        RELOAD_PROP(DDosSmoothFactor, 0.01);
        RELOAD_PROP(DDosFlag2BlockPeriod, TDuration::Minutes(5));

        RELOAD_PROP(FormulasDir, TString("formulas"));
        AddBase(BaseDir, FormulasDir);

        ServerQueueParams = TMtpQueueParams::FromString(directives.Value("ServerQueueParams", DEFAULT_SERVER_QUEUE_PARAMS));
        ProcessServerQueueParams = TMtpQueueParams::FromString(directives.Value("ProcessServerQueueParams", DEFAULT_SERVER_QUEUE_PARAMS));
        ProcessingQueueParams = TMtpQueueParams::FromString(directives.Value("ProcessingQueueParams", DEFAULT_PROCESSING_QUEUE_PARAMS));
        ProcessorResponseApplyQueueParams = TMtpQueueParams::FromString(directives.Value("ProcessorResponseApplyQueueParams", DEFAULT_PROCESSOR_RESPONSE_APPLY_QUEUE_PARAMS));

        RELOAD_PROP(WhiteList, TString());
        RELOAD_PROP(UaProxyList, TString());
        RELOAD_PROP(WhiteListsDir, TString());
        RELOAD_PROP(YandexIpsDir, TString());
        RELOAD_PROP(PrivilegedIpsFile, TString());
        RELOAD_PROP(AllowBannedIpsAtNight, false);

        RELOAD_PROP(ChinaUrlNotLoginnedRedirect, TString());
        RELOAD_PROP(InitialChinaRedirectEnabled, DEFAULT_INITIAL_CHINA_REDIRECT_ENABLED);
        RELOAD_PROP(InitialChinaUnauthorizedEnabled, DEFAULT_INITIAL_CHINA_UNAUTHORIZED_ENABLED);
        RELOAD_PROP(CaptchaRedirectToSameHost, false);
        reloadHost(CaptchaApiHost, "CaptchaApiHost", DEFAULT_CAPTCHA_API_HOST);
        RELOAD_PROP(CaptchaApiProtocol, TString("http"));
        RELOAD_PROP(CaptchaGenTryCount, 3);
        if (CaptchaGenTryCount <= 0) {
            ythrow yexception() << "CaptchaGenTryCount must be positive";
        }
        RELOAD_PROP(CaptchaCheckTimeout, TDuration::MilliSeconds(20));
        RELOAD_PROP(CaptchaGenTimeout, DEFAULT_CAPTCHA_API_TIMEOUT);
        RELOAD_PROP(CaptchaRedirectTimeOut, DEFAULT_CAPTCHA_REDIRECT_TIMEOUT);
        RELOAD_PROP(ProxyCaptchaUrls, false);
        RELOAD_PROP(CaptchaRequestKeyLifetime, TDuration::Minutes(5));

        RELOAD_PROP(FuryEnabled, false);
        if (FuryEnabled) {
            reloadHost(FuryHost, "FuryHost", "");
        }
        RELOAD_PROP(FuryPreprodEnabled, false);
        if (FuryPreprodEnabled) {
            reloadHost(FuryPreprodHost, "FuryPreprodHost", "");
        }
        RELOAD_PROP(FuryProtocol, TString("http"));
        RELOAD_PROP(FuryBaseTimeout, DEFAULT_FURY_TIMEOUT);
        RELOAD_PROP(FuryTVMClientId, 2020887); // https://a.yandex-team.ru/arc/trunk/arcadia/quality/antifraud/xurma/configs/xurma.json (see "TvmSelfId" for "captcha")
        RELOAD_PROP(FuryPreprodTVMClientId, 2021085);

        RELOAD_PROP(DebugOutput, false);
        RELOAD_PROP(OutputWizardFactors, false);
        RELOAD_PROP(IgnoreYandexIps, true);
        RELOAD_PROP(SpravkaApiExpireInterval, TDuration::Hours(1));
        RELOAD_PROP(SpravkaExpireInterval, DEFAULT_SPRAVKA_EXPIRE_INTERVAL);
        RELOAD_PROP(SpravkaIgnoreIfInRobotSet, true);
        RELOAD_PROP(SpravkaRequestsLimit, 0);
        RELOAD_PROP(KeysFile, DEFAULT_KEYS_FILE);

        RELOAD_PROP(UseBdb, false);
        RELOAD_PROP(DbSyncInterval, DEFAULT_DB_SYNC_INTERVAL);
        RELOAD_PROP(MaxItemsToSync, DEFAULT_MAX_ITEMS_TO_SYNC);
        RELOAD_PROP(MaxDbSize, DEFAULT_MAX_DB_SIZE);
        RELOAD_PROP(DbUsersToDelete, DEFAULT_DB_USERS_TO_DELETE);
        RELOAD_PROP(NumWriteErrorsToResetDb, 0);
        RELOAD_PROP(GeodataBinPath, TString("data/geodata6-xurma.bin"));
        RELOAD_PROP(DictionariesDir, TString("data/dictionaries"));
        RELOAD_PROP(GlobalJsonConfFilePath, TString());
        RELOAD_PROP(JsonConfFilePath, TString());
        RELOAD_PROP(ExperimentsConfigFilePath, TString());
        RELOAD_PROP(JsonServiceRegExpFilePath, TString());
        RELOAD_PROP(LCookieKeysPath, DEFAULT_LKEYS_PATH);
        RELOAD_PROP(YascKeyPath, TString("data/yasc_key"));
        RELOAD_PROP(SpravkaDataKeyPath, DEFAULT_SPRAVKA_DATA_KEY_PATH);
        RELOAD_PROP(AutoruOfferSaltPath, DEFAULT_AUTORU_OFFER_SALT_PATH);
        RELOAD_PROP(BalancerJwsKeyPath, DEFAULT_BALANCER_JWS_KEY_PATH);
        RELOAD_PROP(NarwhalJwsKeyPath, DEFAULT_NARWHAL_JWS_KEY_PATH);
        RELOAD_PROP(NarwhalJwsLeeway, DEFAULT_NARWHAL_JWS_LEEWAY);
        RELOAD_PROP(MarketJwsKeyPath, DEFAULT_MARKET_JWS_KEY_PATH);
        RELOAD_PROP(MarketJwsLeeway, DEFAULT_MARKET_JWS_LEEWAY);
        RELOAD_PROP(AutoRuTamperSaltPath, DEFAULT_AUTO_RU_TAMPER_SALT_PATH);
        RELOAD_PROP(HypocrisyInject, false);
        RELOAD_PROP(HypocrisyBundlePath, DEFAULT_HYPOCRISY_BUNDLE_PATH);
        RELOAD_PROP(HypocrisyInstanceLeeway, DEFAULT_HYPOCRISY_INSTANCE_LEEWAY);
        RELOAD_PROP(HypocrisyFingerprintLifetime, DEFAULT_HYPOCRISY_FINGERPRINT_LIFETIME);
        RELOAD_PROP(HypocrisyCacheControlMaxAge, DEFAULT_HYPOCRISY_CACHE_CONTROL_MAX_AGE);
        RELOAD_PROP(LogLevel, (ui32)EVLOG_ERROR);
        RELOAD_PROP(MaxSafeUsers, DEFAULT_MAX_SAFE_USERS);
        RELOAD_PROP(UsersToDelete, DEFAULT_USERS_TO_DELETE);
        RELOAD_PROP(RpsFilterSize, DEFAULT_RPS_FILTER_SIZE);
        RELOAD_PROP(RpsFilterMinSafeInterval, DEFAULT_RPS_FILTER_MIN_SAFE_INTERVAL);
        RELOAD_PROP(RpsFilterRememberForInterval, DEFAULT_RPS_FILTER_REMEMBER_FOR_INTERVAL);

        RELOAD_PROP(AmnestyICookieInterval, DEFAULT_AMNESTY_INTERVAL);
        RELOAD_PROP(AmnestyIpInterval, DEFAULT_AMNESTY_INTERVAL);
        RELOAD_PROP(AmnestyIpV6Interval, DEFAULT_AMNESTY_INTERVAL);
        RELOAD_PROP(AmnestyFuidInterval, DEFAULT_AMNESTY_INTERVAL);
        RELOAD_PROP(AmnestyLCookieInterval, DEFAULT_AMNESTY_INTERVAL);

        RELOAD_PROP(CaptchaFreqMaxInputs, DEFAULT_CAPTCHA_FREQ_MAX_INPUTS);
        RELOAD_PROP(CaptchaFreqMaxInterval, DEFAULT_CAPTCHA_FREQ_MAX_INTERVAL);
        RELOAD_PROP(CaptchaFreqBanTime, DEFAULT_CAPTCHA_FREQ_BAN_TIME);

        RELOAD_PROP(RemoveExpiredPeriod, BLOCKER_AMNESTY_PERIOD_DEFAULT);

        reloadHost(CbbApiHost, "CbbApiHost", DEFAULT_CBB_API_HOST);
        RELOAD_PROP(CbbApiTimeout, DEFAULT_CBB_API_TIMEOUT);
        RELOAD_PROP(CbbTVMClientId, 2000300); // See https://abc.yandex-team.ru/services/ipfilter/resources/

        RELOAD_PROP(CbbFlag, DEFAULT_CBB_FLAG);
        RELOAD_PROP(CbbFlagNonblocking, DEFAULT_CBB_FLAG_NONBLOCKING);
        RELOAD_PROP(CbbFlagDDos1, DEFAULT_CBB_FLAG_DDOS1);
        RELOAD_PROP(CbbFlagDDos2, DEFAULT_CBB_FLAG_DDOS2);
        RELOAD_PROP(CbbFlagDDos3, DEFAULT_CBB_FLAG_DDOS3);
        RELOAD_PROP(CbbFlagIpBasedIdentificationsBan, DEFAULT_CBB_FLAG_IP_BASED_IDENTIFICATIONS_BAN);
        RELOAD_PROP(CbbFlagIgnoreSpravka, DEFAULT_CBB_FLAG_IGNORE_SPRAVKA);
        RELOAD_PROP(CbbFlagMaxRobotness, DEFAULT_CBB_FLAG_MAX_ROBOTNESS);
        RELOAD_PROP(CbbFlagSuspiciousness, DEFAULT_CBB_FLAG_SUSPICIOUSNESS);
        RELOAD_PROP(CbbFlagCutRequests, DEFAULT_CBB_FLAG_CUT_REQUESTS);
        RELOAD_PROP(CbbFlagNotBanned, DEFAULT_CBB_FLAG_NOT_BANNED);
        RELOAD_PROP(CbbFlagDegradation, DEFAULT_CBB_FLAG_DEGRADATION);
        RELOAD_PROP(CbbFlagBanSourceIp, DEFAULT_CBB_FLAG_BAN_SOURCE_IP);
        RELOAD_PROP(CbbFlagBanFWSourceIp, DEFAULT_CBB_FLAG_BAN_FW_SOURCE_IP);
        RELOAD_PROP(CbbFlagBanFWSourceIpsContainer, DEFAULT_CBB_FLAG_BAN_FW_SOURCE_IPS_CONTAINTER);
        RELOAD_PROP(CbbEnabled, true);
        RELOAD_PROP(CbbSyncPeriod, DEFAULT_CBB_SYNC_PERIOD);
        RELOAD_PROP(CbbMergePeriod, DEFAULT_CBB_MERGE_PERIOD);
        RELOAD_PROP(CbbCacheFile, TString("cbb_cache"));
        RELOAD_PROP(CbbCachePeriod, TDuration::Minutes(30));
        RELOAD_PROP(CbbAddBanFWPeriod, TDuration::Seconds(2));
        RELOAD_PROP(YandexIpsFile, YANDEX_IPS_FILE);
        RELOAD_PROP(SpecialIpsFile, SPECIAL_IPS_FILE);

        RELOAD_PROP(BlocksDumpFile, TString("blocks_dump"));
        RELOAD_PROP(RobotUidsDumpFile, DEFAULT_ROBOT_UIDS_DUMP_FILE);
        RELOAD_PROP(RobotUidsDumpPeriod, DEFAULT_ROBOT_UIDS_DUMP_PERIOD);
        RELOAD_PROP(TurboProxyIps, DEFAULT_TURBO_PROXY_IPS_FILE);
        RELOAD_PROP(ThreadPoolParams, DEFAULT_THREAD_POOL_PARAMS);

        RELOAD_PROP(MaxRequestLength, DEFAULT_MAX_REQUEST_LENGTH);
        RELOAD_PROP(BadUserAgentsFile, DEFAULT_BAD_USER_AGENTS_FILE);
        RELOAD_PROP(BadUserAgentsNewFile, DEFAULT_BAD_USER_AGENTS_NEW_FILE);
        RELOAD_PROP(SearchBotsFile, TString("search_engine_bots"));
        RELOAD_PROP(SearchBotsLiveTime, TDuration::Days(1));
        RELOAD_PROP(MaxSearchBotsCandidates, 10000);

        RELOAD_PROP(RemoteWizards, TString());

        RELOAD_PROP(DefaultHost, TString("yandex.ru"));
        if (DefaultHost.empty()) {
            ythrow yexception() << "Parameter \"DefaultHost\" shouldn't be empty";
        }
        RELOAD_PROP(ProcessServerReadRequestTimeout, TDuration::Seconds(1));
        RELOAD_PROP(ServerReadRequestTimeout, TDuration::Seconds(1));
        RELOAD_PROP(AdminServerReadRequestTimeout, TDuration::Seconds(1));
        RELOAD_PROP(UnistatServerReadRequestTimeout, TDuration::Seconds(1));
        RELOAD_PROP(ServerFailOnReadRequestTimeout, true);
        RELOAD_PROP(ForwardRequestTimeout, TDuration::MilliSeconds(30));

        RELOAD_PROP(DisableBansByFactors, false);
        RELOAD_PROP(DisableBansByYql, false);

        RELOAD_PROP(DiscoveryFrequency, TString());
        RELOAD_PROP(DiscoveryCacheDir, TString("discovery_cache"));
        RELOAD_PROP(DiscoveryHost, TString());
        RELOAD_PROP(DiscoveryPort, 0);
        RELOAD_PROP(DiscoveryInitTimeout, TDuration::Seconds(1));
        RELOAD_PROP(AllDaemons, TString::Join("localhost:", ToString(ProcessServerPort)));

        BackendEndpointSets.clear();
        ExplicitBackends.clear();

        for (const auto& it : StringSplitter(AllDaemons).Split(' ').SkipEmpty()) {
            try {
                const auto entry = it.Token();

                if (entry.StartsWith("yp:")) {
                    BackendEndpointSets.push_back(ParseEndpointSetDescription(entry));
                } else if (entry.StartsWith("id=")) {
                    ExplicitBackends.push_back(ParseIdHostAddrOptions(entry));
                } else {
                    const auto hostAddr = ParseHostAddrOptions(entry);
                    ExplicitBackends.push_back({hostAddr.HostName, hostAddr});
                }
            } catch (...) {
                ++NumUnresolvedDaemons;
            }
        }

        RELOAD_PROP(CaptchaInputForwardLocation, TString("/captchainput"));
        RELOAD_PROP(RequestForwardLocation, TString("/process"));
        RELOAD_PROP(DeadBackendSkipperMaxProbability, 0.95);

        RELOAD_PROP(NightStartHour, 1);
        Y_ENSURE(0 <= NightStartHour && NightStartHour <= 23, "NightStartHour should be between 0 and 23");
        RELOAD_PROP(NightEndHour, 7);
        Y_ENSURE(0 <= NightEndHour && NightEndHour <= 23, "NightEndHour should be between 0 and 23");
        Y_ENSURE(NightStartHour <= NightEndHour, "NightStartHour should be less than or equal to NightEndHour");

        RELOAD_PROP(DaemonLogFormatJson, false);

        RELOAD_PROP(UseTVMClient, true);
        RELOAD_PROP(AntirobotTVMClientId, 2002152); // See https://abc.yandex-team.ru/services/robotolovilka/resources/
        RELOAD_PROP(TVMClientCacheDir, TString("tvm_cache"));
        RELOAD_PROP(TVMClientsList, TString(""));
        RELOAD_PROP(MatrixnetResultForPrivileged, 0.0f);
        RELOAD_PROP(MatrixnetResultForAlreadyRobot, 10.0f);
        RELOAD_PROP(MatrixnetResultForNoMatrixnetReqTypes, 0.0f);
        RELOAD_PROP(MatrixnetFallbackProbability, 0.0f);
        RELOAD_PROP(IpV4SubnetBitsSizeForHashing, 16);
        RELOAD_PROP(IpV6SubnetBitsSizeForHashing, 48);
        RELOAD_PROP(CustomHashingRules, TString(""));
        RELOAD_PROP(MaxNehQueueSize, 10000);
        RELOAD_PROP(ReBanRobotsPeriod, TDuration::Minutes(1));
        RELOAD_PROP(YandexTrustTokenExpireInterval, TDuration::Minutes(1));
        RELOAD_PROP(CacherRandomFactorsProbability, 0.1);
        RELOAD_PROP(HandleStopBanFilePath, TString("/controls/stop_ban"));
        RELOAD_PROP(HandleStopBlockFilePath, TString("/controls/stop_block"));
        RELOAD_PROP(HandleStopBlockForAllFilePath, TString("/controls/stop_block_for_all"));
        RELOAD_PROP(HandleStopBanForAllFilePath, TString("/controls/stop_ban_for_all"));
        RELOAD_PROP(HandleStopFuryForAllFilePath, TString("/controls/stop_fury_for_all"));
        RELOAD_PROP(HandleStopFuryPreprodForAllFilePath, TString("/controls/stop_fury_preprod_for_all"));
        RELOAD_PROP(HandleStopYqlFilePath, TString("/controls/stop_yql"));
        RELOAD_PROP(HandleStopYqlForAllFilePath, TString("/controls/stop_yql_for_all"));
        RELOAD_PROP(HandleStopDiscoveryForAllFilePath, TString("/controls/stop_discovery_for_all"));
        RELOAD_PROP(HandleServerErrorEnablePath, TString("/controls/500_enable"));
        RELOAD_PROP(HandleServerErrorDisableServicePath, TString("/controls/500_disable_service"));
        RELOAD_PROP(HandleCbbPanicModePath, TString("/controls/cbb_panic_mode"));
        RELOAD_PROP(HandleManyRequestsEnableServicePath, TString("/controls/suspicious_429"));
        RELOAD_PROP(HandleManyRequestsMobileEnableServicePath, TString("/controls/suspicious_mobile_429"));
        RELOAD_PROP(HandleSuspiciousBanServicePath, TString("/controls/suspicious_ban"));
        RELOAD_PROP(HandleSuspiciousBlockServicePath, TString("/controls/suspicious_block"));
        RELOAD_PROP(HandleAntirobotDisableExperimentsPath, TString("/controls/disable_experiments"));
        RELOAD_PROP(HandleAllowMainBanFilePath, TString("/controls/allow_main_ban"));
        RELOAD_PROP(HandleAllowDzenSearchBanFilePath, TString("/controls/allow_dzensearch_ban"));
        RELOAD_PROP(HandleAllowBanAllFilePath, TString("/controls/allow_ban_all"));
        RELOAD_PROP(HandleAllowShowCaptchaAllFilePath, TString("/controls/allow_show_captcha_all"));
        RELOAD_PROP(HandlePreviewIdentTypeEnabledFilePath, TString("/controls/preview_ident_type_enabled"));
        RELOAD_PROP(HandleAmnestyFilePath, TString("/controls/amnesty"));
        RELOAD_PROP(HandleAmnestyForAllFilePath, TString("/controls/all_amnesty"));
        RELOAD_PROP(HandleCatboostWhitelistAllFilePath, TString("/controls/disable_catboost_whitelist_all"));
        RELOAD_PROP(HandleCatboostWhitelistServicePath, TString("/controls/disable_catboost_whitelist"));
        RELOAD_PROP(HandlePingControlFilePath, TString("/controls/weight"));

        RELOAD_PROP(HandleWatcherPollInterval, TDuration::Seconds(1));

        RELOAD_PROP(NehOutputConnectionsSoftLimit, 10000);
        RELOAD_PROP(NehOutputConnectionsHardLimit, 30000);

        RELOAD_PROP(Local, false);
        RELOAD_PROP(LockMemory, false);
        RELOAD_PROP(AsCaptchaApiService, false);
        RELOAD_PROP(YdbEndpoint, TString(""));
        RELOAD_PROP(YdbDatabase, TString(""));
        RELOAD_PROP(YdbMaxActiveSessions, 500);
        RELOAD_PROP(YdbSessionReadTimeout, TDuration::MilliSeconds(100));
        RELOAD_PROP(YdbSessionWriteTimeout, TDuration::MilliSeconds(100));
        RELOAD_PROP(CloudCaptchaApiEndpoint, TString(""));
        RELOAD_PROP(CloudCaptchaApiKeepAliveTime, TDuration::MilliSeconds(100));
        RELOAD_PROP(CloudCaptchaApiKeepAliveTimeout, TDuration::MilliSeconds(100));
        RELOAD_PROP(CloudCaptchaApiGetClientKeyTimeout, TDuration::MilliSeconds(100));
        RELOAD_PROP(CloudCaptchaApiGetServerKeyTimeout, TDuration::MilliSeconds(100));
        RELOAD_PROP(StaticFilesVersion, 0);
        if (AsCaptchaApiService) {
            Y_ENSURE(
                StaticFilesVersion == 0 || IsIn(TLocalizedData::Instance().GetExternalVersionedFilesVersions(), StaticFilesVersion),
                "StaticFilesVersion=" << StaticFilesVersion << " is not correct value"
            );
        } else {
            Y_ENSURE(
                StaticFilesVersion == 0 || IsIn(TLocalizedData::Instance().GetAntirobotVersionedFilesVersions(), StaticFilesVersion),
                "StaticFilesVersion=" << StaticFilesVersion << " is not correct value"
            );
        }

        RELOAD_PROP(ProcessorThresholdForSpravkaPenalty, 2.0);

        Y_ENSURE(
            !BackendEndpointSets.empty() || !ExplicitBackends.empty(),
            "Empty list of Antirobot instances"
        );

        if (!IsDir(BaseDir))
            ythrow yexception() << "Invalid BaseDir: " << BaseDir.Quote();

        LogsDir = GetSubDirFullPath(LogsDir);
        RuntimeDataDir = GetSubDirFullPath(RuntimeDataDir);

        AddBase(BaseDir, WhiteListsDir);
        AddBase(BaseDir, YandexIpsDir);
        AddBase(BaseDir, PrivilegedIpsFile);
        AddBase(BaseDir, UaProxyList);
        AddBase(BaseDir, GeodataBinPath);
        AddBase(BaseDir, DictionariesDir);
        AddBase(BaseDir, GlobalJsonConfFilePath);
        AddBase(BaseDir, JsonConfFilePath);
        AddBase(BaseDir, ExperimentsConfigFilePath);
        AddBase(BaseDir, JsonServiceRegExpFilePath);
        AddBase(BaseDir, LCookieKeysPath);
        AddBase(BaseDir, YascKeyPath);
        AddBase(BaseDir, SpravkaDataKeyPath);
        AddBase(BaseDir, AutoruOfferSaltPath);
        AddBase(BaseDir, BalancerJwsKeyPath);
        AddBase(BaseDir, NarwhalJwsKeyPath);
        AddBase(BaseDir, MarketJwsKeyPath);
        AddBase(BaseDir, AutoRuTamperSaltPath);
        AddBase(BaseDir, HypocrisyBundlePath);
        AddBase(BaseDir, SpecialIpsFile);
        AddBase(BaseDir, TurboProxyIps);
        AddBase(BaseDir, BadUserAgentsFile);
        AddBase(BaseDir, BadUserAgentsNewFile);
        AddBase(BaseDir, KeysFile);
        AddBase(BaseDir, HandleAllowBanAllFilePath);
        AddBase(BaseDir, HandleAllowMainBanFilePath);
        AddBase(BaseDir, HandleAllowDzenSearchBanFilePath);
        AddBase(BaseDir, HandleAllowShowCaptchaAllFilePath);
        AddBase(BaseDir, HandleAmnestyFilePath);
        AddBase(BaseDir, HandleAmnestyForAllFilePath);
        AddBase(BaseDir, HandlePreviewIdentTypeEnabledFilePath);
        AddBase(BaseDir, HandleStopBanFilePath);
        AddBase(BaseDir, HandleStopBanForAllFilePath);
        AddBase(BaseDir, HandleStopBlockFilePath);
        AddBase(BaseDir, HandleStopBlockForAllFilePath);
        AddBase(BaseDir, HandleStopYqlFilePath);
        AddBase(BaseDir, HandleStopYqlForAllFilePath);
        AddBase(BaseDir, HandleStopDiscoveryForAllFilePath);
        AddBase(BaseDir, HandleServerErrorEnablePath);
        AddBase(BaseDir, HandleServerErrorDisableServicePath);
        AddBase(BaseDir, HandleCbbPanicModePath);
        AddBase(BaseDir, HandleManyRequestsEnableServicePath);
        AddBase(BaseDir, HandleManyRequestsMobileEnableServicePath);
        AddBase(BaseDir, HandleSuspiciousBanServicePath);
        AddBase(BaseDir, HandleSuspiciousBlockServicePath);
        AddBase(BaseDir, HandleCatboostWhitelistAllFilePath);
        AddBase(BaseDir, HandleCatboostWhitelistServicePath);
        AddBase(BaseDir, HandleAntirobotDisableExperimentsPath);
        AddBase(BaseDir, HandlePingControlFilePath);
        AddBase(RuntimeDataDir, BlocksDumpFile);
        AddBase(RuntimeDataDir, RobotUidsDumpFile);
        AddBase(RuntimeDataDir, SearchBotsFile);
        AddBase(RuntimeDataDir, CbbCacheFile);
        AddBase(RuntimeDataDir, TVMClientCacheDir);
        AddBase(RuntimeDataDir, DiscoveryCacheDir);

        // TOOD(rzhikharevich): it's not quite great that we create stuff on disk during config
        // *parsing*.
        // Passing an empty path is OK – it will create a null evlog backend.

        EventLog.Reset(new TUnifiedAgentLogBackend(UnifiedAgentUri, "eventlog"));
        CacherDaemonLog.Reset(new TUnifiedAgentLogBackend(UnifiedAgentUri, "cacher_daemonlog"));
        ProcessorDaemonLog.Reset(new TUnifiedAgentLogBackend(UnifiedAgentUri, "processor_daemonlog"));

        if (UsersToDelete > MaxSafeUsers)
            ythrow yexception() << "UsersToDelete must not be greater than MaxSafeUsers";

        TString NoMatrixnetReqTypes;
        RELOAD_PROP(NoMatrixnetReqTypes, TString());

        NoMatrixnetReqTypesSet.clear();
        for (const auto& it : StringSplitter(NoMatrixnetReqTypes).Split(',').SkipEmpty()) {
            NoMatrixnetReqTypesSet.insert(FromString<EReqType>(it.Token()));
        }

        TString SuppressedCookiesInLogs;
        RELOAD_PROP(SuppressedCookiesInLogs, DEFAULT_SUPPRESSED_COOKIES_IN_LOGS);
        SuppressedCookiesInLogsSet = StringSplitter(SuppressedCookiesInLogs).Split(',').SkipEmpty();
        RELOAD_PROP(CharactersFromPrivateCookieToHide, DEFAULT_CHARACTERS_FROM_PRIVATE_COOKIE_TO_HIDE);

        FillBrowserJsPrintFeaturesMapping(BrowserJsPrintFeaturesMapping);

#undef RELOAD_PROP
    }

    void TAntirobotDaemonConfig::LoadZoneConf(TZoneConf& conf, TYandexConfig::Directives& directives, bool useDefault) {
#define RELOAD_PROP(prop, def) conf.prop = directives.Value(#prop, useDefault ? def : conf.prop)

        RELOAD_PROP(SpravkaPenalty, 0);
        RELOAD_PROP(DDosFlag1BlockEnabled, false);
        RELOAD_PROP(DDosFlag2BlockEnabled, false);
        RELOAD_PROP(PartnerCaptchaType, false);

        const TString& configCaptchaTypes = directives.Value("CaptchaTypes", useDefault ? DEFAULT_CAPTCHA_TYPES : "");
        const auto& currentZoneCaptchaTypes = LoadCaptchaTypes(configCaptchaTypes);
        MergeHashMaps(conf.CaptchaTypes, currentZoneCaptchaTypes);

        if (!conf.CaptchaTypes.contains(DEFAULT_CAPTCHA_TYPE_SERVICE)) {
            ythrow yexception() << "\"CaptchaType\" should have '"
                                << DEFAULT_CAPTCHA_TYPE_SERVICE << "' key=value pair";
        }

        RELOAD_PROP(AllowBlock, false);

        RELOAD_PROP(TrainingSetGenSchemes, TString());
    }

    void TAntirobotDaemonConfig::LoadFromString(const TString& str) {
        Config.Reset(new TAntirobotConfStrings());

        TAntirobotConfStrings& cs = *Config;
        TString errStr;

        if (!cs.ParseMemory(str.c_str())) {
            cs.PrintErrors(errStr);

            ythrow yexception() << "can not parse config string " << str.Quote() << "(" << errStr << ")";
        }

        if (!cs.GetRootSection()) {
            ythrow yexception() << "can not parse config string " << str.Quote() << "(no root section)";
        }

        DoLoad(cs);
    }

    TAntirobotDaemonConfig::TMtpQueueParams::TMtpQueueParams()
        : ThreadsMin(0)
        , ThreadsMax(0)
        , QueueSize(0)
    {
    }

    TAntirobotDaemonConfig::TMtpQueueParams TAntirobotDaemonConfig::TMtpQueueParams::FromString(const TString& values) {
        TMtpQueueParams result;
        try {
            const THttpCookies params(values);
            result.ThreadsMin = FromCookie<size_t>(params, "threads_min");
            result.ThreadsMax = FromCookie<size_t>(params, "threads_max");
            result.QueueSize = FromCookie<size_t>(params, "queue_size");
        } catch (const yexception& e) {
            ythrow yexception() << "MtpQueueParams parse exception: " << e.what();
        }
        return result;
    }

    TString TAntirobotDaemonConfig::TMtpQueueParams::ToString() const {
        TStringStream res;
        res << "threads_min=" << ThreadsMin << "; "
            << "threads_max=" << ThreadsMax << "; "
            << "queue_size=" << QueueSize;
        return res.Str();
    }

    const TAntirobotDaemonConfig::TZoneConf& TAntirobotDaemonConfig::ConfByTld(const TStringBuf& tld) const {
        TConfMap::const_iterator it = ZoneConfigs.find(tld);
        if (it != ZoneConfigs.end())
            return it->second;

        return DefaultZoneConf;
    }

    TAutoPtr<TWizardConfig> TAntirobotDaemonConfig::GetWizardConfig(IOutputStream& out, bool useOnlyIps) const {
        /* Make a config for wizard */
        TStringStream stream;
        stream << "<RemoteWizard>" << Endl;
        Y_ENSURE(RemoteWizardConfig != nullptr, "Section WizardsRemote is not present in config");
        const TYandexConfig::Directives& directives = RemoteWizardConfig->GetDirectives();
        for (const auto& directive : directives) {
            if (directive.first != "RemoteWizards") {
                stream << directive.first << " " << directive.second << Endl;
                continue;
            }

            stream << directive.first << " ";
            size_t NumUnresolvedWizards = 0;
            for (const auto& hostIter : StringSplitter(directive.second).Split(' ').SkipEmpty()) {
                try {
                    auto addr = ParseHostAddrOptions(hostIter.Token(), useOnlyIps);
                    stream << addr.HostOrIp << ":" << addr.Port << " ";
                } catch (...) {
                    ++NumUnresolvedWizards;
                }
            }
            out << "Number of unresolved wizards = " << NumUnresolvedWizards << "\n";
            stream << Endl;
        }
        stream << "</RemoteWizard>" << Endl;

        TAntirobotConfStrings wizYaConfig;
        if (!wizYaConfig.ParseMemory(stream.Str().c_str()))
            ythrow yexception() << "Could not parse specialized wizard config";

        TYandexConfig::Section* wizSection = wizYaConfig.GetRootSection()->Child;

        return TWizardConfig::CreateWizardConfig(wizYaConfig, wizSection);
    }
}

IOutputStream& operator<<(IOutputStream& out, const NAntiRobot::TAntirobotDaemonConfig::TMtpQueueParams& mtpQueueParams) {
    out << mtpQueueParams.ToString();
    return out;
}
