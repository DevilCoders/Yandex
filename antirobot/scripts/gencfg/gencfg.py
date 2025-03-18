import sys
import socket
from time import sleep
from optparse import OptionParser

from infra.yp_service_discovery.python.resolver.resolver import Resolver
from infra.yp_service_discovery.api import api_pb2
from google.protobuf.json_format import MessageToDict


# Please keep the parameters in alphabetical order!
#
CONF_TEMPLATE = """instance = {
    conf = [[
<Daemon>
    # SvnUrl: UNDEFINED_URL
    # SvnRevision: UNDEFINED_REVISION

    AdminServerMaxConnections = 100
    AdminServerPort = %(AdminServerPort)d
    AdminServerReadRequestTimeout = 100ms
    AdminServerThreads = 3
    AllDaemons = %(AllDaemons)s
    AllowBannedIpsAtNight = 0
    AmnestyFuidInterval = 4d
    AmnestyICookieInterval = 4d
    AmnestyIpInterval = 40m
    AmnestyIpV6Interval = 15m
    AmnestyLCookieInterval = 1h
    AsCaptchaApiService = %(AsCaptchaApiService)d
    AuthorizeByFuid = 1
    AuthorizeByICookie = 1
    AuthorizeByLCookie = 1
    AutoRuTamperSalt = %(DataDir)s/autoru_tamper_salt

    BadUserAgentsFile = %(DataDir)s/bad_user_agents.lst
    BadUserAgentsNewFile = %(DataDir)s/bad_user_agents.lst
    BalancerJwsKeyPath =
    BaseDir = /hol/antirobot
    BlockerAmnestyPeriod = 30s
    BlocksDumpFile = blocks_dump

    CacherRandomFactorsProbability = 0.2
    CaptchaApiHost = %(CaptchaApiHost)s
    CaptchaApiProtocol = http
    CaptchaCheckTimeout = 0.160s
    CaptchaFreqBanTime = 8640000s
    CaptchaFreqMaxInputs = 50000
    CaptchaFreqMaxInterval = 86400s
    CaptchaGenTimeout = 0.080s
    CaptchaGenTryCount = 1
    CaptchaInputForwardLocation = /captchainput
    CaptchaRedirectTimeOut = 3600s
    CaptchaRedirectToSameHost = 1
    CbbCacheFile = cbb_cache
    CbbCachePeriod = 30m
    CbbFlagDDos1 = 166
    CbbFlagDDos2 = 167
    CbbFlagDDos3 = 168
    CbbApiHost = %(CbbApiHost)s
    CbbApiTimeout = 5s
    CbbFlag = 154
    CbbFlagIpBasedIdentificationsBan = 225
    CbbFlagNonblocking = 9
    CbbFlagIgnoreSpravka = 328
    CbbFlagMaxRobotness = 329
    CbbFlagSuspiciousness = 511
    CbbFlagDegradation = 661
    CbbEnabled = 1
    CbbSyncPeriod = 1s
    CbbMergePeriod = 30s
    CharactersFromPrivateCookieToHide = 30
    ChinaUrlNotLoginnedRedirect = https://passport.yandex.%%s/auth/?origin=china&retpath=%%s
    CloudCaptchaApiEndpoint = %(CloudCaptchaApiEndpoint)s
    CloudCaptchaApiKeepAliveTime = 0.5s
    CloudCaptchaApiKeepAliveTimeout = 0.1s
    CloudCaptchaApiGetClientKeyTimeout = 0.2s
    CloudCaptchaApiGetServerKeyTimeout = 0.2s
    CustomHashingRules = 176.59.0.0/16=23,213.87.128.0/19=21,217.118.64.0/19=22,31.173.64.0/19=23,188.170.0.0/16=22,85.140.0.0/19=23,109.252.0.0/17=21,85.249.0.0/16=22,94.25.0.0/16=21,89.113.0.0/16=21,178.176.0.0/16=23,85.26.0.0/16=21,83.220.0.0/16=23,83.149.0.0/16=21,188.162.0.0/16=21,91.193.0.0/16=21

    DaemonLogFormatJson = 1
    DbMinAggregationLevel = -1
    DbSyncInterval = 30s
    DbUsersToDelete = 300000
    DDosAmnestyPeriod = 300s
    DDosFlag1BlockPeriod = 300s
    DDosFlag2BlockPeriod = 300s
    DDosRpsThreshold = 2500.0
    DDosSmoothFactor = 0.6
    DeadBackendSkipperMaxProbability = 0.99
    DebugOutput = 0
    DefaultHost = yandex.ru
    DictionariesDir = %(DataDir)s/dictionaries
    DisableBansByFactors = 0
    DisableBansByYql = 0
    DiscoveryFrequency = 60s
    DiscoveryCacheDir = discovery_cache
    DiscoveryPort = %(DiscoveryPort)s
    DiscoveryHost = %(DiscoveryHost)s
    DiscoveryInitTimeout = 1s

    UnifiedAgentUri = %(UnifiedAgentUri)s

    FormulasDir = %(FormulasDir)s
    ForwardRequestTimeout = 0.1s
    FuryBaseTimeout = 150ms
    FuryEnabled = 1
    FuryHost = %(FuryHost)s
    FuryPreprodEnabled = 1
    FuryPreprodHost = %(FuryPreprodHost)s
    FuryProtocol = http

    GeodataBinPath = %(DataDir)s/geodata6-xurma.bin
    GlobalJsonConfFilePath = %(DataDir)s/global_config.json

    HandleAllowBanAllFilePath = controls/allow_ban_all
    HandleAllowDzenSearchBanFilePath = controls/allow_dzensearch_ban
    HandleAllowMainBanFilePath = controls/allow_main_ban
    HandleAllowShowCaptchaAllFilePath = controls/allow_show_captcha_all
    HandleAmnestyFilePath = controls/amnesty
    HandleAmnestyForAllFilePath = controls/all_amnesty
    HandleCatboostWhitelistAllFilePath = controls/disable_catboost_whitelist_all
    HandleCatboostWhitelistServicePath = controls/disable_catboost_whitelist
    HandleManyRequestsEnableServicePath = controls/suspicious_429
    HandleManyRequestsMobileEnableServicePath = controls/suspicious_mobile_429
    HandlePreviewIdentTypeEnabledFilePath = controls/preview_ident_type_enabled
    HandleServerErrorDisableServicePath = controls/500_disable_service
    HandleCbbPanicModePath = controls/cbb_panic_mode
    HandleServerErrorEnablePath = controls/500_enable
    HandleStopBanFilePath = controls/stop_ban
    HandleStopBanForAllFilePath = controls/stop_ban_for_all
    HandleStopBlockFilePath = controls/stop_block
    HandleStopBlockForAllFilePath = controls/stop_block_for_all
    HandleStopDiscoveryForAllFilePath = controls/stop_discovery_for_all
    HandleStopFuryForAllFilePath = controls/stop_fury_for_all
    HandleStopFuryPreprodForAllFilePath = controls/stop_fury_preprod_for_all
    HandleStopYqlFilePath = controls/stop_yql
    HandleStopYqlForAllFilePath = controls/stop_yql_for_all
    HandleSuspiciousBanServicePath = controls/suspicious_ban
    HandleSuspiciousBlockServicePath = controls/suspicious_block
    HandleAntirobotDisableExperimentsPath = controls/disable_experiments
    HandlePingControlFilePath = controls/weight
    HandleWatcherPollInterval = 5s

    HypocrisyCacheControlMaxAge = 32400
    HypocrisyInject = 0
    HypocrisyBundlePath = %(DataDir)s/hypocrisy
    HypocrisyFingerprintLifetime = 15m
    HypocrisyInstanceLeeway = 2h

    IgnoreYandexIps = 1
    InitialChinaRedirectEnabled = true
    IpV4SubnetBitsSizeForHashing = 24
    IpV6SubnetBitsSizeForHashing = 56

    JsonConfFilePath = %(DataDir)s/service_config.json
    JsonServiceRegExpFilePath = %(DataDir)s/service_identifier.json
    ExperimentsConfigFilePath = %(DataDir)s/experiments_config.json

    KeysFile = %(DataDir)s/keys

    Local = 0
    LockMemory = 0
    LogsDir = logs
    LogLevel = 1
    LCookieKeysPath = %(DataDir)s/L-cookie-keys.txt

    MarketJwsKeyPath = data/market_jws_key
    MarketJwsLeeway = 300s
    MatrixnetResultForAlreadyRobot = 10.0
    MatrixnetResultForNoMatrixnetReqTypes = 0.0
    MatrixnetResultForPrivileged = 0.0
    MatrixnetFallbackProbability = 0.01
    MaxConnections = 10000
    MaxDbSize = 2000000
    MaxItemsToSync = 1000
    MaxNehQueueSize = 10000
    MaxRequestLength = 1024
    MaxSafeUsers = 700000
    MaxSearchBotsCandidates = 10000
    MinFuidAge = 86400s
    MinICookieAge = 86400s
    MinRequestsWithSpravka = 20

    NarwhalJwsKeyPath = data/narwhal_jws_key
    NarwhalJwsLeeway = 300s
    NehOutputConnectionsHardLimit = 500000
    NehOutputConnectionsSoftLimit = 10000
    NightEndHour = 7
    NightStartHour = 1
    NoMatrixnetReqTypes = other,redir
    NumWriteErrorsToResetDb = 1000

    OutputWizardFactors = 0

    PartOfAllFactorsToPrint = 0.0001
    PartOfRegularCaptchaRedirectFactorsToPrint = 0.01
    Port = %(Port)d
    PrivilegedIpsFile = %(DataDir)s/privileged_ips
    ProcessingQueueParams = threads_min=6; threads_max=56; queue_size=10000
    ProcessorResponseApplyQueueParams = threads_min=2; threads_max=10; queue_size=10000
    ProcessServerPort = %(ProcessPort)d
    ProcessServerQueueParams = threads_min=6; threads_max=16; queue_size=2000
    ProcessServerReadRequestTimeout = 1s
    ProxyCaptchaUrls = 1

    ReBanRobotsPeriod = 1m
    RemoveExpiredPeriod = 30s
    RequestForwardLocation = /process
    RobotUidsDumpFile = robot_uids_dump
    RobotUidsDumpPeriod = 3600s
    RpsFilterMinSafeInterval = 60s
    RpsFilterRememberForInterval = 60s
    RpsFilterSize = 2000
    RuntimeDataDir = %(RuntimeDir)s

    ProcessServerMaxConnections = 2500

    SearchBotsFile = search_engine_bots
    SearchBotsLiveTime = 604800s
    ServerReadRequestTimeout = 20ms
    ServerQueueParams = threads_min=6; threads_max=32; queue_size=2000
    ServerFailOnReadRequestTimeout = 0
    SpecialIpsFile = %(DataDir)s/special_ips
    SpravkaApiExpireInterval = 10m
    SpravkaDataKeyFile = %(DataDir)s/spravka_data_key
    SpravkaExpireInterval = 4d
    SpravkaIgnoreIfInRobotSet = 1
    SpravkaRequestsLimit = 300
    StaticFilesVersion = %(StaticFilesVersion)d
    SuppressedCookiesInLogs = Session_id,sessionid2,ya_sess_id,sessguard

    ThreadPoolParams = free_min=4; free_max=300; total_max=1000; increase=2
    TimeDeltaMinDeltas = 7
    TimeDeltaMaxDeviation = 0.07
    TurboProxyIps = %(DataDir)s/trbosrvnets
    TvmClientCacheDir = tvm_cache
    TvmClientsList = tvm_clients.json

    UaProxyList = %(DataDir)s/ua_proxy_ips
    UnistatServerMaxConnections = 100
    UnistatServerPort = %(UnistatServerPort)d
    UnistatServerReadRequestTimeout = 100ms
    UseBdb = 1
    UseRemoteWizard = 1
    UsersToDelete = 210000

    WhiteList = whitelist_ips
    WhitelistsDir = %(DataDir)s
    WorkMode = enabled

    YandexIpsDir = %(DataDir)s
    YandexIpsFile = yandex_ips
    YandexTrustTokenExpireInterval = 10m
    YascKeyPath = %(DataDir)s/yasc_key

    YdbEndpoint = %(YdbEndpoint)s
    YdbDatabase = %(YdbDatabase)s
    YdbMaxActiveSessions = 500
    YdbSessionReadTimeout = 100ms
    YdbSessionWriteTimeout = 100ms

    ProcessorThresholdForSpravkaPenalty = 2.0
</Daemon>

<Zone>
    AllowBlock = 1

    CaptchaTypes = default=txt_v1_en; default:xml=estd; default:ajax=estd; eda=txt_v1

    DDosFlag1BlockEnabled = 0
    DDosFlag2BlockEnabled = 0

    PartnerCaptchaType = 1

    RemoteWizards = %(RemoteWizards)s

    SpravkaPenalty = 5

    TrainingSetGenSchemes = default=random_factors; video=none;

    <ru>
        CaptchaTypes = default=txt_v1; default:ajax=ocr; default:xml=ocr; non_branded_partners=txt_v1; webmaster=txt_v1; autoru=txt_v1; autoru:ajax=ocr; kinopoisk=txt_v1; kinopoisk:ajax=ocr
    </ru>

    <ua>
        CaptchaTypes = default=txt_v1; default:ajax=ocr; default:xml=ocr; non_branded_partners=txt_v1; webmaster=txt_v1; autoru=txt_v1; autoru:ajax=ocr; kinopoisk=txt_v1; kinopoisk:ajax=ocr
    </ua>

    <by>
        CaptchaTypes = default=txt_v1; default:ajax=ocr; default:xml=ocr; non_branded_partners=txt_v1; webmaster=txt_v1; autoru=txt_v1; autoru:ajax=ocr; kinopoisk=txt_v1; kinopoisk:ajax=ocr
    </by>

    <kz>
        CaptchaTypes = default=txt_v1; default:ajax=ocr; default:xml=ocr; non_branded_partners=txt_v1; webmaster=txt_v1; autoru=txt_v1; autoru:ajax=ocr; kinopoisk=txt_v1; kinopoisk:ajax=ocr
    </kz>

    <uz>
        CaptchaTypes = default=txt_v1; default:ajax=ocr; default:xml=ocr; non_branded_partners=txt_v1; webmaster=txt_v1; autoru=txt_v1; autoru:ajax=ocr; kinopoisk=txt_v1; kinopoisk:ajax=ocr
    </uz>

    <tr>
        SpravkaPenalty = 0

        TrainingSetGenSchemes = default=random_factors; web=none; video=none; img=none
    </tr>

    <com>
    </com>

    <eu>
    </eu>
</Zone>

<WizardsRemote>
    LocalWizard = localhost:%(WizardPort)d
    RemoteWizards = %(RemoteWizards)s
    RemoteWizardAttempts = 1
    RemoteWizardSimultaneousRequests = 3
    ReqWizardRules = PopularityRequest Commercial PornoQuery IsNav Url QueryLang PersonData Classification Serialization Text BinaryTree
    SelectWizardHostsPolicy = %(WizardPolicy)s
    WizardPort = %(WizardPort)d
    WizardTimeout = 1000
    RemoteWizardsEnableIpV6 = 1
</WizardsRemote>
    ]];

    non_branded_partners = [[
rambler-p
rambler-xml
    ]];
};
"""  # noqa: E501

WIZARD_GENCFG_PORT = 8891
WIZARD_YP_PORT = 80
DEFAULT_PORT = 80

CAPTCHA_API_HOST = 'new.captcha.yandex.net'
CAPTCHA_API_TESTING_HOST = 'apicaptcha-test.yandex-team.ru'

FURY_HOST = 'fury-captcha-responder-prod.search.yandex.net'
FURY_PREPROD_HOST = 'fury-captcha-responder-preprod.search.yandex.net'
FURY_PORT = 14000

CBB_API_HOST = 'cbb-ext.yandex-team.ru'
CBB_API_TESTING_HOST = 'cbb-testing.yandex-team.ru'

YDB_ENDPOINT = 'ydb-ru.yandex.net:2135'
YDB_DATABASE = '/ru/captcha/prod/captcha-sessions'

YDB_TESTING_ENDPOINT = 'ydb-ru-prestable.yandex.net:2135'
YDB_TESTING_DATABASE = '/ru-prestable/captcha/test/captcha'

CLOUD_CAPTCHA_API_ENDPOINT = 'captcha-cloud-api.yandex.net:80'
CLOUD_CAPTCHA_API_TESTING_ENDPOINT = 'captcha-cloud-api-preprod.yandex.net:80'


def ParseArgs():
    parser = OptionParser('Usage: %prog [options]\nSee https://wiki.yandex-team.ru/jandekspoisk/sepe/antirobotonline/docs/config')
    parser.add_option('-p', '--port', dest='port', action='store', type='int',
                      help='port for common requests (default: %default)', default=13512)
    parser.add_option('', '--geo', dest='geo', action='store', type='string',
                      help='geo tag, i.e. man, sas, vla')
    parser.add_option('-d', '--data', dest='data', action='store', type='string',
                      help='path to data dir (default: %default)', default='data')
    parser.add_option('-f', '--formulas', dest='formulas', action='store', type='string',
                      help='path to formulas dir (default: %default)', default='formulas')
    parser.add_option('-r', '--runtime', dest='runtime', action='store', type='string',
                      help='path where runtime data is stored (robot set dump, recognised search engines, etc., default: %default)', default='logs')
    parser.add_option('', '--process-port', dest='processPort', action='store', type='int',
                      help='Port for server that handles /process requests')
    parser.add_option('', '--admin-port', dest='adminPort', action='store', type='int',
                      help='Port for server that handles /admin requests')
    parser.add_option('', '--unistat-port', dest='unistatPort', action='store', type='int',
                      help='Port for server that handles /unistat requests')
    parser.add_option('', '--current-yp-wizard-pods', dest='ypWizardPods', action='append', type='string',
                      help='Yp wizard pods')
    parser.add_option('', '--current-yp-wizard-pods-prefix', dest='ypWizardPodsPrefix', action='store', type='string',
                      help='Yp wizard pods prefix')
    parser.add_option('', '--current-yp-pods', dest='ypPods', action='append', type='string',
                      help='Yp pods')
    parser.add_option('', '--current-yp-process-pods', dest='ypProcessPods', action='append', type='string',
                      help='Yp process pods')
    parser.add_option('', '--explicit-backends', dest='explicitBackends', action='append', type='string',
                      help='backend with format id=XXX;host:port;[ip]:port')
    parser.add_option('', '--only-remote-wizards', dest='remoteWizards', action='store_true',
                      help='use only remote wizards')
    parser.add_option('', '--as-captcha-api-service', dest='asCaptchaApiService', action='store_true',
                      help='captcha api mode', default=False)
    parser.add_option('', '--local', dest='local', action='store_true',
                      help='Do not use any production instances (for testing purposes).')
    parser.add_option('', '--testing', dest='testing', action='store_true',
                      help='Use testing services (cbb, fury, ydb, captcha, etc).')
    parser.add_option('', '--unified-agent-port', dest='unifiedAgentPort', action='store', type='int')
    parser.add_option('', '--static-files-version', dest='staticFilesVersion', action='store', type='int', default=0)

    (opts, args) = parser.parse_args()

    if opts.local:
        assert not opts.ypPods
        assert not opts.ypProcessPods
        assert not opts.explicitBackends
    else:
        assert opts.geo, 'You must specify geo tag\nSee https://wiki.yandex-team.ru/jandekspoisk/sepe/antirobotonline/docs/config'
    return opts


def TryToResolveHosts(hosts, port):
    resolvedHosts = []
    unresolvedHosts = []
    for h in hosts:
        try:
            if isinstance(h, tuple):
                ipAddr = socket.getaddrinfo(h[1], port)[0][4][0]
                entry = (h[0], h[1], ipAddr)
            else:
                ipAddr = socket.getaddrinfo(h, port)[0][4][0]
                entry = (h, ipAddr)

            resolvedHosts.append(entry)
        except:
            unresolvedHosts.append(h)

    return resolvedHosts, unresolvedHosts


def GetResolvedHosts(hosts, port, errorStr):
    resolvedHosts, unresolvedHosts = TryToResolveHosts(hosts, port)

    tries = 0
    maxTries = 50

    while len(unresolvedHosts) and tries < maxTries:
        tries += 1
        sleep(0.1)

        print("Unresolved hosts: " + ','.join(unresolvedHosts), file=sys.stderr)

        newResolvedHosts, unresolvedHosts = TryToResolveHosts(unresolvedHosts, port)
        resolvedHosts += newResolvedHosts

    if len(unresolvedHosts):
        print(errorStr, ", Unresolved hosts:", ','.join(unresolvedHosts), file=sys.stderr)
        raise Exception("Could not resolve some hosts")

    return resolvedHosts


def PrintHosts(hosts, port=None):
    portSuffix = '' if port is None else ':{}'.format(port)

    def printHost(host):
        if len(host) == 2:
            return '{0}{2};[{1}]{2}'.format(host[0], host[1], portSuffix)
        elif len(host) == 3:
            return 'id={0};{1}{3};[{2}]{3}'.format(host[0], host[1], host[2], portSuffix)
        else:
            raise Exception("expected a pair or a triplet")

    return '   '.join(printHost(host) for host in hosts)


def GetYpEndpoints(cluster, endpoint_set_id):
    resolver = Resolver(client_name='gencfg-antirobot:{}'.format(socket.gethostname()), timeout=5)

    request = api_pb2.TReqResolveEndpoints()
    request.cluster_name = cluster
    request.endpoint_set_id = endpoint_set_id

    response = resolver.resolve_endpoints(request)
    result = MessageToDict(response)
    if 'endpointSet' in result:
        endpoint_set = result['endpointSet'].get('endpoints', [])
    else:
        endpoint_set = []
    return [(e.get('id'), e.get('fqdn')) for e in endpoint_set]


def main():
    opts = ParseArgs()

    processPort = opts.processPort if opts.processPort else opts.port + 1
    adminPort = opts.adminPort if opts.adminPort else opts.port + 2
    unistatPort = opts.unistatPort if opts.unistatPort else opts.port + 3

    discovery_port = 0
    discovery_host = ''
    if opts.local:
        WIZARD_PORT = WIZARD_GENCFG_PORT
        RemoteWizardsWithAddrs = [("localhost", "::1")]
    else:
        discovery_port = 8080
        discovery_host = opts.geo + '.sd.yandex.net'

        if opts.ypWizardPods:
            RemoteWizards = []
            for pods in opts.ypWizardPods:
                endpoint, cluster = pods.split(',')
                RemoteWizards += [e[1] for e in GetYpEndpoints(cluster, endpoint)]
                WIZARD_PORT = WIZARD_YP_PORT
        elif opts.ypWizardPodsPrefix:
            cluster = opts.geo
            endpoint = opts.ypWizardPodsPrefix + opts.geo
            RemoteWizards = [e[1] for e in GetYpEndpoints(cluster, endpoint)]
            WIZARD_PORT = WIZARD_YP_PORT

        if not RemoteWizards:
            print("Fatal: got empty list of wizards", file=sys.stderr)
            sys.exit(1)

        RemoteWizardsWithAddrs = GetResolvedHosts(RemoteWizards, WIZARD_PORT, 'Fatal: wizards DNS resolving error')

    wizardPolicy = 'hhh' if opts.remoteWizards else 'lhh'

    allDaemons = []
    allProcessEndpoints = []
    explicitBackends = []

    if opts.ypPods:
        for pods in opts.ypPods:
            endpoint, cluster = pods.split(',')
            allDaemons += GetYpEndpoints(cluster, endpoint)

    if opts.ypProcessPods:
        for pods in opts.ypProcessPods:
            endpoint, cluster = pods.split(',')
            allProcessEndpoints.append(':'.join(['yp', cluster, endpoint]))

    if opts.explicitBackends:
        explicitBackends = opts.explicitBackends

    allDaemons = sorted(set(allDaemons))
    if (opts.ypPods or opts.ypProcessPods) and not allDaemons and not allProcessEndpoints and not explicitBackends:
        print("Fatal: got empty list of Antirobot instances", file=sys.stderr)
        sys.exit(2)

    allDaemonsWithAddrs = GetResolvedHosts(allDaemons, processPort, 'Fatal: Antirobot instances DNS resolving error')
    CaptchaApiHost = GetResolvedHosts([CAPTCHA_API_TESTING_HOST if opts.testing else CAPTCHA_API_HOST], DEFAULT_PORT, 'Fatal: captcha API host DNS resolving error')
    FuryHost = GetResolvedHosts([FURY_HOST], FURY_PORT, 'Fatal: fury API host DNS resolving error')
    FuryPreprodHost = GetResolvedHosts([FURY_PREPROD_HOST], FURY_PORT, 'Fatal: fury preprod API host DNS resolving error')
    CbbApiHost = GetResolvedHosts([CBB_API_TESTING_HOST if opts.testing else CBB_API_HOST], DEFAULT_PORT, 'Fatal: ebb API host DNS resolving error')
    if opts.asCaptchaApiService:
        YdbDatabase = YDB_TESTING_DATABASE if opts.testing else YDB_DATABASE
        YdbEndpoint = YDB_TESTING_ENDPOINT if opts.testing else YDB_ENDPOINT
        CloudCaptchaApiEndpoint = CLOUD_CAPTCHA_API_TESTING_ENDPOINT if opts.testing else CLOUD_CAPTCHA_API_ENDPOINT
    else:
        YdbDatabase = YdbEndpoint = CloudCaptchaApiEndpoint = ''

    print(CONF_TEMPLATE % {
        'AdminServerPort'        : adminPort,
        'UnistatServerPort'      : unistatPort,
        'Port'                   : opts.port,
        'RemoteWizards'          : PrintHosts(RemoteWizardsWithAddrs, WIZARD_PORT),
        'DataDir'                : opts.data,
        'FormulasDir'            : opts.formulas,
        'RuntimeDir'             : opts.runtime,
        'WizardPort'             : WIZARD_PORT,
        'AllDaemons'             : PrintHosts(allDaemonsWithAddrs, processPort) + '   ' + '   '.join(allProcessEndpoints) + '   ' + '   '.join(explicitBackends),
        'ProcessPort'            : processPort,
        'CaptchaApiHost'         : PrintHosts(CaptchaApiHost),
        'FuryHost'               : PrintHosts(FuryHost, FURY_PORT),
        'FuryPreprodHost'        : PrintHosts(FuryPreprodHost, FURY_PORT),
        'CbbApiHost'             : PrintHosts(CbbApiHost),
        'WizardPolicy'           : wizardPolicy,
        'DiscoveryPort'          : discovery_port,
        'DiscoveryHost'          : discovery_host,
        'AsCaptchaApiService'    : opts.asCaptchaApiService,
        'YdbDatabase'            : YdbDatabase,
        'YdbEndpoint'            : YdbEndpoint,
        'CloudCaptchaApiEndpoint': CloudCaptchaApiEndpoint,
        'UnifiedAgentUri'        : 'localhost:' + str(opts.unifiedAgentPort),
        'StaticFilesVersion'     : opts.staticFilesVersion,
    })


if __name__ == '__main__':
    main()
