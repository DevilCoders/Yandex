from lua_globals import LuaGlobal
from collections import OrderedDict

# regexp for stats and expressions
TOUCHLOGS_URI = '/mobilesearch/touchlogs(.*)?'

# Main weights file
L7_DEFAULT_WEIGHTS_DIR = LuaGlobal('WeightsDir', './controls/')
L7_WEIGHT_SWITCH_FILE = L7_DEFAULT_WEIGHTS_DIR + '/production.weights'

# time-related statistics data
DEFAULT_TIMELINES = '300ms,500ms,700ms,1s,3s,10s'
PERIODS_10ms_10s = '10ms,20ms,50ms,100ms,200ms,300ms,400ms,500ms,600ms,700ms,800ms,1s,3s,10s'

CLCK_TIMELINES = '5ms,10ms,50ms,100ms,1s,10s'
SUGGEST_TIMELINES = '5ms,10ms,50ms,100ms,1s,10s'
ADDRS_TIMELINES = '5ms,10ms,20ms,30ms,50ms,100ms,150ms,200ms,300ms,500ms,1s,3s,5s,10s'
MEDIA_BDI_TIMELINES = '5ms,10ms,15ms,30ms,50ms,100ms,120ms,150ms,200ms,300ms,600ms,1000ms,1400ms,1500ms'
IMPROXY_TIMELINES = '1ms,10ms,20ms,30ms,50ms,100ms,200ms,500ms,1s,5s,10s'
MAPSUGGEST_TIMELINES = '1ms,2ms,4ms,7ms,10ms,1s'
# SEPE-10070
WEBSUGGEST_TIMELINES = '3ms,5ms,7ms,10ms,40ms,70ms,100ms,150ms,200ms,500ms,1s'
SITESUGGEST_TIMELINES = '1ms,2ms,3ms,4ms,10ms,50ms,100ms,150ms,200ms,500ms,1s'
# End if SEPE-10070
FAVICONS_TIMELINES = '1ms,10ms,20ms,30ms,50ms,200ms,500ms,10s'
BETAWEBMASTER_TIMELINES = '1ms,10ms,20ms,30ms,50ms,200ms,500ms,10s,100s'
PORTAL_TIMELINES = '10ms,50ms,100ms,200ms,500ms,1s,10s'
GEOBASE_TIMELINES = '1ms,2ms,3ms,4ms,10ms,20ms,30ms,50ms,100ms,150ms,200ms,500ms,1s'
SUGGEST_TIMELINES_NEW = '10ms,30ms,50ms,100ms,200ms,300ms,500ms,1s'
YASM_TIMELINES = '100ms,200ms,300ms,500ms,1s,2s,5s,10s'
ALL_REPORT_TIMELINES = '7ms,11ms,17ms,26ms,39ms,58ms,87ms,131ms,197ms,296ms,444ms,666ms,1000ms,1500ms,2250ms,3375ms,5062ms,7593ms,11390ms,17085ms'  # noqa
NORMAL_REPORT_TIMELINES = '7ms,17ms,39ms,87ms,197ms,444ms,1000ms,2250ms,5062ms,11390ms'

SOURCES_NAMES = [
    {'stats_attr': 'cloudflare', 'match_fsm':
        {'header': {'name': 'CF-Connecting-IP', 'value': '.*'}}}
]

NOC_STATIC_CHECK = '/static_check\\\\.html'
BALANCER_HEALTH_CHECK = '/balancer-health-check'
BALANCER_HEALTH_CHECK_REQUEST = r'GET /balancer-health-check HTTP/1.1\r\n\r\n'
AWACS_BALANCER_HEALTH_CHECK_REQUEST = r'GET /awacs-balancer-health-check HTTP/1.1\r\n\r\n'
APPHOST_HEALTH_CHECK_REQUEST = r'GET /admin?action=ping HTTP/1.1\r\nHost: localhost\r\n\r\n'

# URI matching without parameters.  Options surround=true enables substing match
# web balancer locations
# video removed and presents like dedicated section
YANDSEARCH_REQUEST_URI_WEIGHTED = '/(yandsearch|yandpage|search|msearch|telsearch|schoolsearch|largesearch|familysearch|touchsearch|padsearch|msearchpart|jsonsearch|yca).*'   # noqa

CAPTCHASEARCH_URI = '/x?(show|check)?captcha.*'
CLCKSEARCH_URI = '/clck/.*'
SUGGEST_URI = '/(suggest|suggest-mobile).*|/jquery\\\\.crossframeajax\\\\.html'
SUGGEST_COMMON_URI = '/(suggest|suggest-delete-text|suggest-dict-info|suggest-endings|suggest-experiments|suggest-explain|suggest-fact|suggest-fact-two|suggest-filters|suggest-first-char|suggest-ie|suggest-kinopoiskbro|suggest-mobile|suggest-news-by|suggest-news-com|suggest-news-kz|suggest-news-ru|suggest-news-sources|suggest-news-ua|suggest-opera|suggest-quality-test|suggest-relq-serp|suggest-rich|suggest-search-lib-examples|suggest-sl|suggest-trendy-queries)(/.*)?|/(suggest-ff\\\\.cgi|suggest-sd\\\\.cgi|suggest-ya\\\\.cgi)'  # noqa
# TODO(mvel): check this
SUGGEST_MARKET_URI = '/(suggest-market|suggest-internal)(/.*)?'
SUGGEST_SPOK_URI = '/suggest-spok(/.*)?|/suggest-collections(/.*)?'
SUGGEST_HISTORY_URI = '/suggest-history(/.*)?'
SUGGEST_EXPRT_URI = '/suggest-exprt(/.*)?'

# used only in example
# NEWS_NGINX_URI = '(.*\\\\.js|.*\\\\.rss|/rss/.*|/api/.*|/live(/.*)?|/cl2picture.*|/crossdomain.xml.*|/export/.*|/favicon\\\\.ico|/lasttimeout|/opensearch\\\\.xml.*|/robots\\\\.txt|/apple-touch-icon(|-precomposed)\\\\.png)'   # noqa
SERPAPI_URI = '/serpapi(/)?.*'
PROMO_URI = '/promo/.+'  # SEPE-7155
PROMO_PRJ = ['promo', 'oda', 'yac2014']
FAVICON_URI = '(/favicon/).*'
IMAGES_TODAY_URI = '/(images|gorsel)/today.*'
IMAGES_URI = '/(images|gorsel)(/.*)?'
IMAGES_HOSTS = '.*(images|gorsel)(\\\\..*)?(\\\\.yandex).*'  # SEPE-10517


# # Blacklists
BLACKLISTED_HEADERS = [
    'X-Yandex-Internal-Request',
    'X-Forwarded-Proto',
    'X-Forwarded-Host',
    'Proxy',
    'X-Yandex-ExpConfigVersion',
    'X-Yandex-ExpBoxes',
    'X-Yandex-ExpFlags',
    'X-L7-Exp',
    'X-Yandex-ExpConfigVersion-Pre',
    'X-Yandex-ExpBoxes-Pre',
    'X-Yandex-ExpFlags-Pre',
    'X-L7-Exp-Testing',
    'X-Forwarded-For',
    'Y-Balancer-Experiments',
    'X-Yandex-IP',
]

BLACKLISTED_HEADERS_HTTP3 = [
    'X-Yandex-Internal-Request',
    'X-Forwarded-Host',
    'Proxy',
    'X-Yandex-ExpConfigVersion',
    'X-Yandex-ExpBoxes',
    'X-Yandex-ExpFlags',
    'X-L7-Exp',
    'X-Yandex-ExpConfigVersion-Pre',
    'X-Yandex-ExpBoxes-Pre',
    'X-Yandex-ExpFlags-Pre',
    'X-L7-Exp-Testing',
    'Y-Balancer-Experiments',
]

SHADOW_HEADERS = {
    'X-Forwarded-For': 'Shadow-X-Forwarded-For',
}


BALANCER_EXP_HEADER = "Y-Balancer-Experiments"
AB_EXP_HEADER = "X-Yandex-ExpBoxes"

# yandex domain names
YANDEX_HOST = '(.*\\\\.)?(xn----7sbhgfw0a0bcg8l1a\\\\.xn--p1ai|xn--80aefebu0a0bbh8l\\\\.xn--p1ai|xn--d1acpjx3f\\\\.xn--p1ai|2yandex\\\\.ru|jandeks\\\\.com\\\\.tr|jandex\\\\.com\\\\.tr|kremlyandex\\\\.ru|video-yandex\\\\.ru|videoyandex\\\\.ru|wwwyandex\\\\.ru|xyyandex\\\\.net|yanclex\\\\.ru|yandeks\\\\.com|yandeks\\\\.com\\\\.tr|yandes\\\\.ru|yandesk\\\\.com|yandesk\\\\.org|yandesk\\\\.ru|yandex-plus-plus\\\\.ru|yandex-plusplus\\\\.ru|yandex-rambler\\\\.ru|yandex-video\\\\.ru|yandex\\\\.asia|yandex\\\\.az|yandex\\\\.biz\\\\.tr|yandex\\\\.by|yandex\\\\.co\\\\.il|yandex\\\\.co\\\\.no|yandex\\\\.com|yandex\\\\.com\\\\.de|yandex\\\\.com\\\\.kz|yandex\\\\.com\\\\.tr|yandex\\\\.com\\\\.ua|yandex\\\\.de|yandex\\\\.dk|yandex\\\\.do|yandex\\\\.ee|yandex\\\\.es|yandex\\\\.ie|yandex\\\\.in|yandex\\\\.info\\\\.tr|yandex\\\\.it|yandex\\\\.jobs|yandex\\\\.jp\\\\.net|yandex\\\\.kg|yandex\\\\.kz|yandex\\\\.lt|yandex\\\\.lu|yandex\\\\.lv|yandex\\\\.md|yandex\\\\.mobi|yandex\\\\.mx|yandex\\\\.name|yandex\\\\.net|yandex\\\\.net\\\\.ru|yandex\\\\.no|yandex\\\\.nu|yandex\\\\.org|yandex\\\\.eu|yandex\\\\.pl|yandex\\\\.fi|yandex\\\\.pt|yandex\\\\.qa|yandex\\\\.ro|yandex\\\\.rs|yandex\\\\.ru|yandex\\\\.sk|yandex\\\\.st|yandex\\\\.sx|yandex\\\\.tj|yandex\\\\.tm|yandex\\\\.ua|yandex\\\\.uz|yandex\\\\.web\\\\.tr|yandex\\\\.xxx|yandexbox\\\\.ru|yandexmedia\\\\.ru|yandexplusplus\\\\.ru|yandexvideo\\\\.ru|yandfex\\\\.ru|yandx\\\\.ru|yaplusplus\\\\.ru|yandex\\\\.com\\\\.ge|yandex\\\\.fr|yandex\\\\.az|yandex\\\\.uz|yandex\\\\.com\\\\.am|yandex\\\\.co\\\\.il|yandex\\\\.kg|yandex\\\\.lt|yandex\\\\.lv|yandex\\\\.md|yandex\\\\.tj|yandex\\\\.tm|yandex\\\\.ee)(:\\\\d+|\\\\.)?'  # noqa

# yaru domain names
YARU_HOST = '(.*\\\\.)?(ya\\\\.ru)(:\\\\d+|\\\\.)?'  # noqa

# MINOTAUR-1581
YANDEX_GEO_ONLY_HOST = '(sas|man|vla)\\\\.yandex\\\\.(.*)'  # noqa

# some instances not presented in generator
MORDA_INSTANCES_COM = '213.180.193.3:80,213.180.204.3:80,87.250.250.3:80,93.158.134.3:80'.split(
    ','
)  # ips of slb www.yandex.ru
MORDA_INSTANCES_BETA = []
MORDA_INSTANCES_BETA.extend(["i%d.wfront.yandex.net:80" % num for num in range(1, 31)])
MORDA_INSTANCES_BETA.extend(["m%d.wfront.yandex.net:80" % num for num in range(1, 31)])
MORDA_INSTANCES_BETA.extend(["n%d.wfront.yandex.net:80" % num for num in range(1, 31) + range(51, 81)])
MORDA_INSTANCES_BETA.extend(["s%d.wfront.yandex.net:80" % num for num in range(1, 31) + range(51, 81)])
# MORDA_INSTANCES_BETA.extend(["u%d.wfront.yandex.net:80" % num for num in range(51, 81)]) portal backends from UGR3
MORDA_INSTANCES_RU = MORDA_INSTANCES_BETA
MORDA_INSTANCES_RU_IPV6 = [s + '(resolve=6)' for s in MORDA_INSTANCES_RU]
MORDA_INSTANCES_WEIGHTS_INSTANCE_TEST = ["n%02d.wfront.yandex.net:80(resolve=6)" % i for i in range(51, 81)]

STATIC_YANDEX_NET_MSK = []
STATIC_YANDEX_NET_MSK.extend(["pstatic-n%d.yandex.net:80" % num for num in range(1, 8) + range(11, 18)])
STATIC_YANDEX_NET_MSK.extend(["pstatic-i%d.yandex.net:80" % num for num in range(1, 8) + range(11, 18)])
STATIC_YANDEX_NET_MSK.extend(["pstatic-m%d.yandex.net:80" % num for num in range(1, 8) + range(11, 18)])
STATIC_YANDEX_NET_MSK.extend(["pstatic-u%d.yandex.net:80" % num for num in range(11, 18)])
STATIC_YANDEX_NET_MSK_IPV6 = [s + '(resolve=6)' for s in STATIC_YANDEX_NET_MSK]
STATIC_YANDEX_NET_SAS = []
STATIC_YANDEX_NET_SAS.extend(["pstatic-s%d.yandex.net:80" % num for num in range(1, 8) + range(11, 18) + range(21, 24)])
STATIC_YANDEX_NET_SAS_IPV6 = [s + '(resolve=6)' for s in STATIC_YANDEX_NET_SAS]
STATIC_YANDEX_NET_MAN = []
STATIC_YANDEX_NET_MAN.extend(["pstatic-f%d.yandex.net:80" % num for num in range(1, 16)])
STATIC_YANDEX_NET_MAN_IPV6 = [s + '(resolve=6)' for s in STATIC_YANDEX_NET_MAN]

FAKE_INSTANCES = '127.0.0.1:4'.split(',')
WMC_NTF_INSTANCES = [
    "wmc-back%02d%s.search.yandex.net:33560" % (num, char) for char in ['e', 'g'] for num in range(1, 5)
]
WMC_BACK_STABLE_INSTANCES = [
    "wmc-back%02d%s.search.yandex.net:33545" % (num, char) for char in ['e', 'g'] for num in range(1, 5)
]
BETA_SITE_INSTANCES = [
    "wmc-nfront%02d%s.search.yandex.net:80" % (num, char) for char in ['e', 'g'] for num in range(1, 5)
]
SEPE6130_XML_INSTANCES = [
    "wmc-nfront%02d%s.search.yandex.net:80" % (num, char) for char in ['e', 'g'] for num in range(1, 5)
]
BETA_WEBMASTER_INSTANCES = [
    "wmc-xfront%02d%s.search.yandex.net:80" % (num, char) for char in ['e', 'g'] for num in range(1, 5)
]
CONTENT_MAPS_BACKENDS = [
    '01.pvec.maps.yandex.net:80',
    '02.pvec.maps.yandex.net:80',
    '03.pvec.maps.yandex.net:80',
    '04.pvec.maps.yandex.net:80',
    'sat01.maps.yandex.net:80',
    'sat02.maps.yandex.net:80',
    'sat03.maps.yandex.net:80',
    'sat04.maps.yandex.net:80',
    'vec01.maps.yandex.net:80',
    'vec02.maps.yandex.net:80',
    'vec03.maps.yandex.net:80',
    'vec04.maps.yandex.net:80',
    'jgo.maps.yandex.net:80',
]

PROMO_INSTANCES = 'nodejs.spec.yandex.net:80'.split(',')

# SSL_CIPHERS_SUITES = 'ECDHE-RSA-AES128-GCM-SHA256:kEECDH:RC4:kRSA+AES128:kRSA:+3DES:!aNULL:!eNULL:!MD5:!EXPORT:!LOW:!SEED:!CAMELLIA:!IDEA:!PSK:!SRP:!SSLv2'  # noqa
# SEPE-11622
SSL_CIPHERS_SUITES = 'kEECDH+AESGCM+AES128:kEECDH+AES128:kEECDH+AESGCM+AES256:kRSA+AESGCM+AES128:kRSA+AES128:RC4-SHA:DES-CBC3-SHA:!aNULL:!eNULL:!MD5:!EXPORT:!LOW:!SEED:!CAMELLIA:!IDEA:!PSK:!SRP:!SSLv2'  # noqa
SSL_CIPHERS_SUITES_SHA2 = 'ECDHE-ECDSA-AES128-GCM-SHA256:kEECDH+AESGCM+AES128:kEECDH+AES128:kEECDH+AESGCM+AES256:kRSA+AESGCM+AES128:kRSA+AES128:RC4-SHA:DES-CBC3-SHA:!aNULL:!eNULL:!MD5:!EXPORT:!LOW:!SEED:!CAMELLIA:!IDEA:!PSK:!SRP:!SSLv2'   # noqa
SSL_CIPHERS_SUITES_SHA2_2 = 'ECDHE-ECDSA-AES128-GCM-SHA256:kEECDH+AESGCM+AES128:kEECDH+AES128:kEECDH+AESGCM+AES256:kRSA+AESGCM+AES128:kRSA+AES128:RC4-SHA:DES-CBC3-SHA:!ECDHE-ECDSA-AES128-SHA:!aNULL:!eNULL:!MD5:!EXPORT:!LOW:!SEED:!CAMELLIA:!IDEA:!PSK:!SRP:!SSLv2'  # noqa
SSL_CIPHERS_SUITES_CHACHA_SHA2 = 'ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:kEECDH+AESGCM+AES128:kEECDH+AES128:kEECDH+AESGCM+AES256:kRSA+AESGCM+AES128:kRSA+AES128:RC4-SHA:DES-CBC3-SHA:!ECDHE-ECDSA-AES128-SHA:!aNULL:!eNULL:!MD5:!EXPORT:!LOW:!SEED:!CAMELLIA:!IDEA:!PSK:!SRP:!SSLv2:!RC4' # noqa

SEARCHSOURCE_HOSTS = [
    'bar.blogs.yandex.net:80',
    'advquick.yandex.ru:80',
    'apps-searcher.http.yandex.net:80',
    'auto2-wsearch-tr.http.yandex.net:80',
    'auto2-wsearch.yandex.net:80',
    'back.video.yandex.net:80',
    'cs-bu-wizard.http.yandex.ru:80',
    'rabota-search.http.yandex.net:80',
    'rabota-zarplatomer.http.yandex.net:80',
    'realty2-wizard.http.yandex.net:80',
    's.feeds.yandex.net:80'
]

L7_SEARCH_JUNK_URI = '/(v|vl|versions|viewconfig|cgi-bin|redir_warning|redir|storeclick)(/.*)?|/(yandcache\\\\.js|tail-log|advanced\\\\.html)(.*)?'  # noqa

WEBMASTER_INSTANCES = [
    "wmc-nfront%02d%s.search.yandex.net:80" % (num, char) for char in ['e', 'g'] for num in range(1, 5)
]
WEBMASTER_INSTANCES_IPV6 = [
    "wmc-nfront%02d%s.search.yandex.net:80(resolve=6)" % (num, char) for char in ['e', 'g'] for num in range(1, 5)
]

getIpByHostName = """function getIpByHostName(data)
    local handler = io.popen("dig AAAA $(hostname) +short")
    local_ip = handler:read("*l")
    handler:close()
    return local_ip
end"""

GetIpByIproute = """function GetIpByIproute(b)
    for _, value in pairs(b) do
        local ipcmd
        if value == "v4" then
            ipcmd = [[ip route get 77.88.8.8 2>/dev/null | awk 'BEGIN{RS="src"} {if (NR!=1) print $1}']]
        elseif value == "v6" then
            ipcmd = "ip route get 2a00:1450:4010:c05::65 2>/dev/null | grep -oE '2a02[:0-9a-f]+' | tail -1"
        else
            error("invalid parameter")
        end
        local handler = io.popen(ipcmd)
        local ip = handler:read("*l")
        handler:close()
        if ip == nil or ip == "proto" then
            return "127.0.0.2"
        else
            return ip
        end
    end
end"""

regproxy_prefix = """function getipbyhost(b)
 local cmd = "dig +short $(hostname) "..b
 local handler = io.popen(cmd)
 local ip = handler:read("*l")
 handler:close()
 return ip
end
local_ip = table.concat({getipbyhost('A'),getipbyhost('AAAA')}, ",")
"""

exp_testing_matcher = OrderedDict([
    ('CGI', '(exp-testing=da|exp_confs=testing)'),
    ('surround', 'true')
])

GetPort = '''function GetPort(port_offset)
    port = _G["Port"]
    if port == nil then
        print("Please specify \\"Port\\" value")
        os.exit(1)
    end
    return port + port_offset['port_offset']
end'''
