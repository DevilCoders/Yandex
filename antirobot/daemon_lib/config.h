#pragma once

#include "cbb_id.h"
#include "config_helpers.h"
#include "json_config.h"
#include "req_types.h"
#include "work_mode.h"

#include <antirobot/lib/host_addr.h>

#include <kernel/geo/utils.h>

#include <library/cpp/charset/ci_string.h>
#include <library/cpp/eventlog/eventlog.h>
#include <library/cpp/http/cookies/cookies.h>
#include <library/cpp/http/cookies/lctable.h>
#include <library/cpp/tvmauth/type.h>

#include <search/wizard/config/config.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/random/random.h>
#include <util/string/vector.h>

#include <array>

namespace NAntiRobot {
    class TStochasticSelector {
    public:
        explicit TStochasticSelector(const TString& captchaType)
            : Probability2CaptchaType{{1.0, captchaType}}
            , Bound2CaptchaType{{1.0, captchaType}} {
        }

        explicit TStochasticSelector(const TMultiMap<float, TString>& probability2CaptchaType)
            : Probability2CaptchaType(probability2CaptchaType)
        {
            float current = 0.0;
            for (const auto& probVal : probability2CaptchaType) {
                current += probVal.first;
                Bound2CaptchaType[current] = probVal.second;
            }

            if (current != 1.0) {
                ythrow yexception() << "Probabilities of captcha type for service does not sum into 1.0";
            }
        }

        const TString& Choose() const {
            float randValue = RandomNumber<float>();
            return Bound2CaptchaType.upper_bound(randValue)->second;
        }

        const TMultiMap<float, TString>& GetProbabilityToCaptchaType() const {
            return Probability2CaptchaType;
        }

    private:
        TMultiMap<float, TString> Probability2CaptchaType;
        TMap<float, TString> Bound2CaptchaType;
    };

    const TString DEFAULT_CAPTCHA_TYPE_SERVICE = "default";
    const TString NON_BRANDED_PARTNER_CAPTCHA_TYPE_SERVICE = "non_branded_partners";
    constexpr size_t MAX_FORMULAS_NUMBER = 16;

    class TUnifiedAgentLogBackend;
    enum class EBrowserJsPrintFeature : ui32;

    class TAntirobotDaemonConfig {
    public:
        typedef TVector<TString> TZoneConfNames;

        struct TMtpQueueParams {
            TMtpQueueParams();
            static TMtpQueueParams FromString(const TString& values);
            TString ToString() const;

            size_t ThreadsMin;
            size_t ThreadsMax;
            size_t QueueSize;
        };

        struct TZoneConf {
            TString Tld;
            size_t SpravkaPenalty;

            THashMap<TString, TStochasticSelector> CaptchaTypes;

            bool PartnerCaptchaType;

            bool AllowBlock;

            bool DDosFlag1BlockEnabled;
            bool DDosFlag2BlockEnabled;

            TString TrainingSetGenSchemes;
        };

        struct TEndpointSetDescription {
            TString Cluster;
            TString Id;
            TMaybe<ui16> Port;

            TEndpointSetDescription() = default;

            explicit TEndpointSetDescription(
                TString cluster,
                TString id,
                TMaybe<ui16> port = Nothing()
            )
                : Cluster(std::move(cluster))
                , Id(std::move(id))
                , Port(port)
            {}
        };

        /* general config params */
        TString BaseDir;
        TString LogsDir;
        TString RuntimeDataDir;
        TString TVMClientCacheDir;
        TString TVMClientsList;

        TString WhiteList;
        TString UaProxyList;
        TString WhiteListsDir;

        ui16 Port;
        ui16 ProcessServerPort;
        ui16 AdminServerPort;
        ui16 UnistatServerPort;
        size_t MaxConnections;
        size_t ProcessServerMaxConnections;
        size_t AdminServerMaxConnections;
        size_t UnistatServerMaxConnections;
        size_t AdminServerThreads;
        TMtpQueueParams ServerQueueParams;
        TMtpQueueParams ProcessServerQueueParams;
        TMtpQueueParams ProcessingQueueParams;
        TMtpQueueParams ProcessorResponseApplyQueueParams;

        EWorkMode WorkMode;

        TString UnifiedAgentUri;
        THolder<TUnifiedAgentLogBackend> EventLog;
        THolder<TUnifiedAgentLogBackend> CacherDaemonLog;
        THolder<TUnifiedAgentLogBackend> ProcessorDaemonLog;

        TString DefaultHost;

        TDuration MinFuidAge;
        TDuration MinICookieAge;
        size_t MinRequestsWithSpravka;
        TDuration SpravkaApiExpireInterval;
        TDuration SpravkaExpireInterval;
        bool SpravkaIgnoreIfInRobotSet;
        size_t SpravkaRequestsLimit;
        TString KeysFile;

        TDuration CaptchaFreqBanTime;
        size_t CaptchaFreqMaxInputs;
        TDuration CaptchaFreqMaxInterval;

        TDuration CaptchaCheckTimeout;
        TDuration CaptchaGenTimeout;
        THostAddr CaptchaApiHost;
        TString CaptchaApiProtocol;
        int CaptchaGenTryCount;
        TDuration CaptchaRedirectTimeOut;
        bool CaptchaRedirectToSameHost;
        bool ProxyCaptchaUrls;
        TDuration CaptchaRequestKeyLifetime;

        bool FuryEnabled;
        THostAddr FuryHost;
        bool FuryPreprodEnabled;
        THostAddr FuryPreprodHost;
        TString FuryProtocol;
        TDuration FuryBaseTimeout;
        NTvmAuth::TTvmId FuryTVMClientId;
        NTvmAuth::TTvmId FuryPreprodTVMClientId;

        size_t RpsFilterSize;
        TDuration RpsFilterMinSafeInterval;
        TDuration RpsFilterRememberForInterval;

        TString VerochkaLogFile;
        bool VerochkaForcedWrite;
        size_t VerochkaQueueSizeLimit;

        size_t TimeDeltaMinDeltas;
        float TimeDeltaMaxDeviation;
        float PartOfAllFactorsToPrint;
        float PartOfRegularCaptchaRedirectFactorsToPrint;

        TString PrivilegedIpsFile;
        bool AllowBannedIpsAtNight;
        bool DebugOutput;
        bool OutputWizardFactors;
        bool IgnoreYandexIps;
        bool UseBdb;
        TDuration DbSyncInterval;
        size_t MaxItemsToSync;
        size_t MaxDbSize;
        size_t DbUsersToDelete;
        size_t NumWriteErrorsToResetDb;

        TString DictionariesDir;
        TString GeodataBinPath;
        TString LCookieKeysPath;
        TString YascKeyPath;
        TString SpravkaDataKeyPath;
        TString AutoruOfferSaltPath;
        TString BalancerJwsKeyPath;
        TString NarwhalJwsKeyPath;
        TDuration NarwhalJwsLeeway;
        TString MarketJwsKeyPath;
        TDuration MarketJwsLeeway;
        TString AutoRuTamperSaltPath;

        bool HypocrisyInject;
        TString HypocrisyBundlePath;
        TDuration HypocrisyInstanceLeeway;
        TDuration HypocrisyFingerprintLifetime;
        size_t HypocrisyCacheControlMaxAge;

        ui32 LogLevel;

        size_t MaxSafeUsers;
        size_t UsersToDelete;

        TString FormulasDir;

        TVector<TString> ProcessorExpFormulas;
        TVector<TString> CacherExpFormulas;
        TVector<float> ProcessorExpThresholds;
        TVector<float> CacherExpThresholds;
        TVector<std::pair<TString, float>> ProcessorExpDescriptions;
        TVector<std::pair<TString, float>> CacherExpDescriptions;
        std::array<std::array<float, EHostType::HOST_NUMTYPES>, MAX_FORMULAS_NUMBER> ProcessorExpFormulasProbability;
        std::array<std::array<float, EHostType::HOST_NUMTYPES>, MAX_FORMULAS_NUMBER> CacherExpFormulasProbability;

        TDuration AmnestyICookieInterval;
        TDuration AmnestyIpInterval;
        TDuration AmnestyIpV6Interval;
        TDuration AmnestyFuidInterval;
        TDuration AmnestyLCookieInterval;

        bool AuthorizeByFuid;
        bool AuthorizeByLCookie;
        bool AuthorizeByICookie;

        TDuration RemoveExpiredPeriod;
        TDuration DDosAmnestyPeriod;

        THostAddr CbbApiHost;
        TDuration CbbApiTimeout;
        NTvmAuth::TTvmId CbbTVMClientId;
        bool CbbEnabled;
        TCbbGroupId CbbFlag;
        TCbbGroupId CbbFlagNonblocking;
        TDuration CbbSyncPeriod;
        TDuration CbbMergePeriod;
        TCbbGroupId CbbFlagDDos1;
        TCbbGroupId CbbFlagDDos2;
        TCbbGroupId CbbFlagDDos3;
        TCbbGroupId CbbFlagIpBasedIdentificationsBan;
        TCbbGroupId CbbFlagIgnoreSpravka;
        TCbbGroupId CbbFlagMaxRobotness;
        TCbbGroupId CbbFlagSuspiciousness;
        TCbbGroupId CbbFlagCutRequests;
        TCbbGroupId CbbFlagNotBanned;
        TCbbGroupId CbbFlagDegradation;
        TCbbGroupId CbbFlagBanSourceIp;
        TCbbGroupId CbbFlagBanFWSourceIp;
        TCbbGroupId CbbFlagBanFWSourceIpsContainer;
        TString CbbCacheFile;
        TDuration CbbCachePeriod;
        TDuration CbbAddBanFWPeriod;

        TString YandexIpsFile;
        TString YandexIpsDir;
        TString SpecialIpsFile;

        TString RobotUidsDumpFile;
        TString BlocksDumpFile;
        TDuration RobotUidsDumpPeriod;

        TString ThreadPoolParams;

        TString TurboProxyIps;

        size_t MaxRequestLength;
        TString BadUserAgentsFile;
        TString BadUserAgentsNewFile;

        float DDosRpsThreshold;
        float DDosSmoothFactor;
        TDuration DDosFlag1BlockPeriod;
        TDuration DDosFlag2BlockPeriod;

        TString SearchBotsFile;
        TDuration SearchBotsLiveTime;
        size_t MaxSearchBotsCandidates;

        TDuration ProcessServerReadRequestTimeout;
        TDuration ServerReadRequestTimeout;
        TDuration AdminServerReadRequestTimeout;
        TDuration UnistatServerReadRequestTimeout;
        bool ServerFailOnReadRequestTimeout;
        TDuration ForwardRequestTimeout;
        TString RemoteWizards;
        TString DiscoveryFrequency;
        bool DisableBansByFactors;
        bool DisableBansByYql;
        TString DiscoveryCacheDir;
        TString DiscoveryHost;
        ui16 DiscoveryPort;
        TDuration DiscoveryInitTimeout;
        TString AllDaemons;
        TVector<TEndpointSetDescription> BackendEndpointSets;
        TVector<std::pair<TString, THostAddr>> ExplicitBackends;

        bool InitialChinaRedirectEnabled;
        bool InitialChinaUnauthorizedEnabled;
        TString ChinaUrlNotLoginnedRedirect;
        TString CaptchaInputForwardLocation;
        TString RequestForwardLocation;
        double DeadBackendSkipperMaxProbability;

        TDuration PanicModeDefaultTimeout;

        ui8 NightStartHour;
        ui8 NightEndHour;

        bool DaemonLogFormatJson;

        size_t IpV4SubnetBitsSizeForHashing;
        size_t IpV6SubnetBitsSizeForHashing;

        bool UseTVMClient;
        NTvmAuth::TTvmId  AntirobotTVMClientId;

        float MatrixnetResultForPrivileged;
        float MatrixnetResultForAlreadyRobot;
        float MatrixnetResultForNoMatrixnetReqTypes;
        float MatrixnetFallbackProbability;

        TString GlobalJsonConfFilePath;
        TString JsonConfFilePath;
        TString JsonServiceRegExpFilePath;
        TString ExperimentsConfigFilePath;

        TString CustomHashingRules;

        TGlobalJsonConfig GlobalJsonConfig;
        TJsonConfig JsonConfig;
        TExperimentsConfig ExperimentsConfig;

        THashSet<EReqType> NoMatrixnetReqTypesSet;
        THashSet<TString> SuppressedCookiesInLogsSet;
        size_t CharactersFromPrivateCookieToHide;

        size_t MaxNehQueueSize;

        TDuration ReBanRobotsPeriod;
        TDuration YandexTrustTokenExpireInterval;

        TDuration HandleWatcherPollInterval;
        TString HandleStopBlockFilePath;
        TString HandleStopBanFilePath;
        TString HandleStopBlockForAllFilePath;
        TString HandleStopBanForAllFilePath;
        TString HandleAllowMainBanFilePath;
        TString HandleAllowDzenSearchBanFilePath;
        TString HandleAllowBanAllFilePath;
        TString HandleAllowShowCaptchaAllFilePath;
        TString HandlePreviewIdentTypeEnabledFilePath;
        TString HandleAmnestyFilePath;
        TString HandleAmnestyForAllFilePath;
        TString HandleStopFuryForAllFilePath;
        TString HandleStopFuryPreprodForAllFilePath;
        TString HandleStopYqlFilePath;
        TString HandleStopYqlForAllFilePath;
        TString HandleStopDiscoveryForAllFilePath;
        TString HandleServerErrorEnablePath;
        TString HandleServerErrorDisableServicePath;
        TString HandleCbbPanicModePath;
        TString HandleManyRequestsEnableServicePath;
        TString HandleManyRequestsMobileEnableServicePath;
        TString HandleSuspiciousBanServicePath;
        TString HandleSuspiciousBlockServicePath;
        TString HandleCatboostWhitelistAllFilePath;
        TString HandleCatboostWhitelistServicePath;
        TString HandleAntirobotDisableExperimentsPath;
        TString HandlePingControlFilePath;

        size_t NehOutputConnectionsSoftLimit;
        size_t NehOutputConnectionsHardLimit;

        float CacherRandomFactorsProbability;

        THashMap<EBrowserJsPrintFeature, TString> BrowserJsPrintFeaturesMapping;

        bool Local;
        bool LockMemory;
        bool AsCaptchaApiService;
        TString YdbEndpoint;
        TString YdbDatabase;
        size_t YdbMaxActiveSessions;
        TDuration YdbSessionReadTimeout;
        TDuration YdbSessionWriteTimeout;
        TString CloudCaptchaApiEndpoint;
        TDuration CloudCaptchaApiKeepAliveTime;
        TDuration CloudCaptchaApiKeepAliveTimeout;
        TDuration CloudCaptchaApiGetClientKeyTimeout;
        TDuration CloudCaptchaApiGetServerKeyTimeout;
        int StaticFilesVersion;

        float ProcessorThresholdForSpravkaPenalty;

    public:
        TAntirobotDaemonConfig();
        ~TAntirobotDaemonConfig();

        void SetBaseDir(const TString& baseDir);
        void SetLogsDir(const TString& logsDir);
        void LoadFromString(const TString& str);
        void Dump(IOutputStream& out, EHostType service) const;
        void DumpZoneConf(IOutputStream& out, const TZoneConf& conf) const;

        void DumpNumUnresolvedDaemons(IOutputStream& out) const {
            out << "Number of unresolved daemons = " << NumUnresolvedDaemons << "\n";
        }

        const TZoneConf& ConfByTld(const TStringBuf& tld) const;
        TAutoPtr<TWizardConfig> GetWizardConfig(IOutputStream& out, bool useOnlyIps) const;
        TZoneConfNames GetZoneConfNames() const;

    private:
        typedef THashMap<TCiString, TZoneConf> TConfMap;

    private:
        void DoLoad(TAntirobotConfStrings& cs);
        void LoadGeneralConf(const TYandexConfig::Directives& directives);
        TString GetSubDirFullPath(TString subDir) const;
        void LoadZoneConf(TZoneConf& conf, TYandexConfig::Directives& directives, bool useDefault);
        void LoadZoneConfigs(TYandexConfig::Section* section);

    private:
        THolder<TAntirobotConfStrings> Config;
        TZoneConf DefaultZoneConf;
        TConfMap ZoneConfigs;
        TYandexConfig::Section* RemoteWizardConfig;
        size_t NumUnresolvedDaemons;
    };

    /// throw if param not defined
    template <typename T>
    static T FromCookie(const THttpCookies& params, const char* paramName) {
        if (!params.Has(paramName))
            ythrow yexception() << "param \"" << paramName << "\" not initialized";
        try {
            return FromString<T>(params.Get(paramName));
        } catch (const TFromStringException& e) {
            ythrow yexception() << "param \"" << paramName << "\" exception: " << e.what();
        }
    }
}

IOutputStream& operator<<(IOutputStream& out, const NAntiRobot::TAntirobotDaemonConfig::TMtpQueueParams& mtpQueueParams);
