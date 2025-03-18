from enum import IntEnum
from typing import List
from typing import NewType
from typing import Set
from typing import Union

from yaqutils import YaqEnum


class ScriptEnum(YaqEnum):
    MSTAND_SQUEEZE = "mstand-squeeze"


class RectypeEnum(YaqEnum):
    START = "start"
    FINISH = "finish"
    TECH = "tech"
    OPERATION_START = "operation-start"
    OPERATION_FINISH = "operation-finish"
    YT_MAPREDUCE_START = "yt-mapreduce-start"
    YT_MAPREDUCE_FINISH = "yt-mapreduce-finish"
    YT_REDUCE_START = "yt-reduce-start"
    YT_REDUCE_FINISH = "yt-reduce-finish"
    YT_SORT_START = "yt-sort-start"
    YT_SORT_FINISH = "yt-sort-finish"
    NILE_REDUCE_START = "nile-reduce-start"
    NILE_REDUCE_FINISH = "nile-reduce-finish"
    BIN_REDUCE_START = "bin-reduce-start"
    BIN_REDUCE_FINISH = "bin-reduce-finish"
    YQL_REDUCE_START = "yql-reduce-start"
    YQL_REDUCE_FINISH = "yql-reduce-finish"


class FiltersEnum(YaqEnum):
    NONE = "none"
    USER = "user"
    DAY = "day"
    FROM_FIRST_DAY = "from_first_day"
    FROM_FIRST_TS = "from_first_ts"


# Uses numbers instead of strings to shape table indexes for YT Reduce operation
class TableTypeEnum(IntEnum):
    YUID = 0
    SOURCE = 1
    SQUEEZE = 2


class TableGroupEnum(YaqEnum):
    T = NewType("TableGroupEnum", str)

    CLEAN = T("clean")
    STAFF = T("staff")
    ROBOTS = T("robots")

    ALL = {CLEAN, STAFF, ROBOTS}


class ServiceSourceEnum(YaqEnum):
    # classic user_sessions tables (for example //user_sessions/pub/spy_log/daily/2011-06-08/clean)
    SEARCH = "search"
    VIDEO = "video"
    IMAGES = "images"
    NEWS = "news"
    SEARCH_STAFF = "search + yandex_staff"
    WATCHLOG = "watch_log_tskv"
    MARKET = "market"
    MARKET_BIN = "market_bin"
    MARKET_WEB = "market_web"
    RECOMMENDER = "recommender-reqans-log"
    ZEN_SEARCH = "zen-search"

    USER_SESSIONS_LOGS = {
        SEARCH,
        VIDEO,
        IMAGES,
        NEWS,
        SEARCH_STAFF,
        WATCHLOG,
        MARKET,
        MARKET_WEB,
        RECOMMENDER,
        ZEN_SEARCH,
    }

    USE_LIBRA = {
        SEARCH,
        VIDEO,
        IMAGES,
        NEWS,
        SEARCH_STAFF,
        RECOMMENDER,
        ZEN_SEARCH,
    }

    # special (not from user_sessions or redir)
    TOLOKA = "special:toloka"
    ALICE = "special:alice"
    APP_METRICS = "special:app-metrics"
    YA_METRICS = "special:ya-metrics"
    ZEN = "special:zen"
    TURBO = "special:turbo"
    ETHER = "special:ether"
    YA_VIDEO = "special:ya-video"
    ECOM = "special:ecom"
    PRISM = "special:prism"
    YQL_AB = "special:yql-ab"
    CACHE_DESKTOP = "special:cache-desktop"
    CACHE_TOUCH = "special:cache-touch"
    CACHE_DESKTOP_EXTENDED = "special:cache-desktop-extended"
    CACHE_TOUCH_EXTENDED = "special:cache-touch-extended-extended"
    CACHE_NEWS = "special:cache-news"
    INTRASEARCH_METRIKA = "special:intrasearch-metrika"
    OTT_IMPRESSIONS = "special:ott-impressions"
    OTT_SESSIONS = "special:ott-sessions"
    OBJECT_ANSWER = "special:object-answer"
    PP = "special:pp"
    SURVEYS = "special:surveys"
    ZEN_SURVEYS = "special:zen-surveys"
    VH = "special:vh"

    # how to use yuid_testids tables
    YUIDS_STRICT = {ECOM, MARKET_WEB, PRISM, WATCHLOG, ZEN_SEARCH, SURVEYS}
    YUIDS_ZEN = {
        ZEN_SEARCH,
    }

    SKIP_TIMESTAMP_CHECK = {ALICE, APP_METRICS, ZEN, OTT_IMPRESSIONS, OTT_SESSIONS, MARKET, PP}
    IGNORE_INVALID_TIMESTAMPS = {ALICE, ZEN, PP}
    YUID_PREFIXES_IGNORE = {
        APP_METRICS, ZEN, YA_METRICS, TURBO, ETHER, YA_VIDEO, YQL_AB,
        INTRASEARCH_METRIKA, OTT_IMPRESSIONS, OTT_SESSIONS, OBJECT_ANSWER, ZEN_SURVEYS,
    }
    CACHE_SOURCES = {CACHE_DESKTOP, CACHE_TOUCH, CACHE_DESKTOP_EXTENDED, CACHE_TOUCH_EXTENDED, CACHE_NEWS}
    SQUEEZE_BIN_SOURCES = {MARKET_BIN}
    SQUEEZE_YQL_SOURCES = {ZEN}

    ALL = {
        SEARCH,
        IMAGES,
        VIDEO,
        NEWS,
        WATCHLOG,
        MARKET,
        MARKET_BIN,
        RECOMMENDER,
        SEARCH_STAFF,
        TOLOKA,
        ALICE,
        APP_METRICS,
        ZEN,
        YA_METRICS,
        MARKET_WEB,
        TURBO,
        ETHER,
        ZEN_SEARCH,
        YA_VIDEO,
        ECOM,
        PRISM,
        YQL_AB,
        CACHE_DESKTOP,
        CACHE_TOUCH,
        CACHE_DESKTOP_EXTENDED,
        CACHE_TOUCH_EXTENDED,
        CACHE_NEWS,
        INTRASEARCH_METRIKA,
        OTT_IMPRESSIONS,
        OTT_SESSIONS,
        OBJECT_ANSWER,
        PP,
        SURVEYS,
        ZEN_SURVEYS,
        VH,
    }


class ServiceEnum(YaqEnum):
    WEB_AUTO = "web-auto"
    WEB_AUTO_EXTENDED = "web-auto-extended"

    AUTO = {
        WEB_AUTO,
        WEB_AUTO_EXTENDED,
    }

    WEB_DESKTOP = "web"
    WEB_DESKTOP_EXTENDED = "web-desktop-extended"
    WEB_TOUCH = "touch"
    WEB_TOUCH_EXTENDED = "web-touch-extended"

    WEB = {
        WEB_DESKTOP,
        WEB_TOUCH
    }

    WEB_EXTENDED = {
        WEB_DESKTOP_EXTENDED,
        WEB_TOUCH_EXTENDED,
    }

    INTRASEARCH = "intrasearch"
    VIDEO = "video"
    IMAGES = "images"
    CV = "cv"
    MORDA = "morda"
    NEWS = "news"
    YUID_REQID_TESTID_FILTER = "yuid-reqid-testid-filter"
    WEB_SURVEYS = "web-surveys"

    WATCHLOG = "watchlog"
    MARKET_SESSIONS_STAT = "market-sessions-stat"
    MARKET_SEARCH_SESSIONS = "market-search-sessions"
    MARKET_WEB_REQID = "market-web-reqid"

    RECOMMENDER = "recommender"
    TOLOKA = "toloka"
    ALICE = "alice"
    APP_METRICS_TOLOKA = "app-metrics-toloka"
    ZEN = "zen"
    YA_METRICS_TOLOKA = "ya-metrics-toloka"
    TURBO = "turbo"
    ETHER = "ether"
    ZEN_MORDA = "zen-morda"
    YA_VIDEO = "ya-video"
    ECOM = "ecom"
    PRISM = "prism"
    YQL_AB = "yql-ab"
    INTRASEARCH_METRIKA = "intrasearch-metrika"
    OTT_IMPRESSIONS = "ott-impressions"
    OTT_SESSIONS = "ott-sessions"
    OBJECT_ANSWER = "object-answer"
    PP = "pp"
    SURVEYS = "surveys"
    ZEN_SURVEYS = "zen-surveys"
    VH = "vh"

    CACHE_DESKTOP = "cache-desktop"
    CACHE_TOUCH = "cache-touch"
    CACHE_DESKTOP_EXTENDED = "cache-desktop-extended"
    CACHE_TOUCH_EXTENDED = "cache-touch-extended"
    CACHE_NEWS = "cache-news"

    WEB_ALL = {
        WEB_DESKTOP, WEB_DESKTOP_EXTENDED,
        WEB_TOUCH, WEB_TOUCH_EXTENDED,
    }

    ALL = {
        WEB_DESKTOP, WEB_DESKTOP_EXTENDED,
        WEB_TOUCH, WEB_TOUCH_EXTENDED,
        INTRASEARCH,
        VIDEO,
        IMAGES,
        CV,
        MORDA,
        NEWS,
        WATCHLOG,
        WEB_SURVEYS,
        MARKET_SESSIONS_STAT,
        MARKET_SEARCH_SESSIONS,
        MARKET_WEB_REQID,
        RECOMMENDER,
        TOLOKA,
        ALICE,
        APP_METRICS_TOLOKA,
        ZEN,
        YA_METRICS_TOLOKA,
        TURBO,
        ETHER,
        ZEN_MORDA,
        YA_VIDEO,
        ECOM,
        PRISM,
        YQL_AB,
        YUID_REQID_TESTID_FILTER,
        CACHE_DESKTOP,
        CACHE_TOUCH,
        CACHE_DESKTOP_EXTENDED,
        CACHE_TOUCH_EXTENDED,
        CACHE_NEWS,
        INTRASEARCH_METRIKA,
        OTT_IMPRESSIONS,
        OTT_SESSIONS,
        OBJECT_ANSWER,
        PP,
        SURVEYS,
        ZEN_SURVEYS,
        VH,
    }

    SKIP_UNICODE_DECODE_ERROR = {
        ECOM,
        IMAGES,
        VIDEO,
        CV,
        MORDA,
        WEB_DESKTOP,
        WEB_TOUCH,
        WEB_DESKTOP_EXTENDED,
        WEB_TOUCH_EXTENDED,
        MARKET_SESSIONS_STAT,
        WEB_SURVEYS,
    }

    CACHE_SUPPORTED = {
        WEB_DESKTOP,
        WEB_TOUCH,
        WEB_DESKTOP_EXTENDED,
        WEB_TOUCH_EXTENDED,
        NEWS,
        YUID_REQID_TESTID_FILTER,
        WEB_SURVEYS,
    }

    SERVICE_TO_CACHE = {
        WEB_DESKTOP: CACHE_DESKTOP,
        WEB_TOUCH: CACHE_TOUCH,
        WEB_DESKTOP_EXTENDED: CACHE_DESKTOP_EXTENDED,
        WEB_TOUCH_EXTENDED: CACHE_TOUCH_EXTENDED,
        NEWS: CACHE_NEWS,
    }

    SKIP_TIMESTAMP_CHECK = {WEB_SURVEYS, ZEN_SURVEYS, SURVEYS}
    SKIP_REQUEST_CHECK = {WEB_SURVEYS}

    SQUEEZE_BIN_SUPPORTED = WEB_ALL.union({YUID_REQID_TESTID_FILTER, WEB_SURVEYS, VIDEO})

    ALIASES = {
        "web": WEB_DESKTOP,
        "web-desktop": WEB_DESKTOP,
        "touch": WEB_TOUCH,
        "web-touch": WEB_TOUCH,
    }

    # services mapping for libra.ParseSession and libra.Parse. related to MSTAND-1513
    # read more: https://wiki.yandex-team.ru/logs/man/libra/#parsesession
    LIBRA = {
        WEB_DESKTOP: "web",
        WEB_DESKTOP_EXTENDED: "web",
        WEB_TOUCH: "web",
        WEB_TOUCH_EXTENDED: "web",
        VIDEO: "vid",
        IMAGES: "img",
        RECOMMENDER: "recommendations",
        MARKET_SEARCH_SESSIONS: "market",
    }

    SOURCES = {
        WEB_DESKTOP: ServiceSourceEnum.SEARCH,
        WEB_DESKTOP_EXTENDED: ServiceSourceEnum.SEARCH,
        WEB_TOUCH: ServiceSourceEnum.SEARCH,
        WEB_TOUCH_EXTENDED: ServiceSourceEnum.SEARCH,

        INTRASEARCH: ServiceSourceEnum.SEARCH_STAFF,
        IMAGES: ServiceSourceEnum.IMAGES,
        CV: ServiceSourceEnum.IMAGES,
        VIDEO: ServiceSourceEnum.VIDEO,
        MORDA: ServiceSourceEnum.SEARCH,
        NEWS: ServiceSourceEnum.NEWS,
        MARKET_SESSIONS_STAT: ServiceSourceEnum.MARKET,
        MARKET_SEARCH_SESSIONS: ServiceSourceEnum.MARKET_BIN,
        MARKET_WEB_REQID: ServiceSourceEnum.MARKET_WEB,
        YUID_REQID_TESTID_FILTER: ServiceSourceEnum.SEARCH,
        WEB_SURVEYS: ServiceSourceEnum.SEARCH,

        WATCHLOG: ServiceSourceEnum.WATCHLOG,

        RECOMMENDER: ServiceSourceEnum.RECOMMENDER,
        TOLOKA: ServiceSourceEnum.TOLOKA,
        ALICE: ServiceSourceEnum.ALICE,
        APP_METRICS_TOLOKA: ServiceSourceEnum.APP_METRICS,
        ZEN: ServiceSourceEnum.ZEN,
        YA_METRICS_TOLOKA: ServiceSourceEnum.YA_METRICS,
        TURBO: ServiceSourceEnum.TURBO,
        ETHER: ServiceSourceEnum.ETHER,
        ZEN_MORDA: ServiceSourceEnum.ZEN_SEARCH,
        YA_VIDEO: ServiceSourceEnum.YA_VIDEO,
        ECOM: ServiceSourceEnum.ECOM,
        PRISM: ServiceSourceEnum.PRISM,
        YQL_AB: ServiceSourceEnum.YQL_AB,
        CACHE_DESKTOP: ServiceSourceEnum.CACHE_DESKTOP,
        CACHE_TOUCH: ServiceSourceEnum.CACHE_TOUCH,
        CACHE_DESKTOP_EXTENDED: ServiceSourceEnum.CACHE_DESKTOP_EXTENDED,
        CACHE_TOUCH_EXTENDED: ServiceSourceEnum.CACHE_TOUCH_EXTENDED,
        CACHE_NEWS: ServiceSourceEnum.CACHE_NEWS,
        INTRASEARCH_METRIKA: ServiceSourceEnum.INTRASEARCH_METRIKA,
        OTT_IMPRESSIONS: ServiceSourceEnum.OTT_IMPRESSIONS,
        OTT_SESSIONS: ServiceSourceEnum.OTT_SESSIONS,
        OBJECT_ANSWER: ServiceSourceEnum.OBJECT_ANSWER,
        PP: ServiceSourceEnum.PP,
        SURVEYS: ServiceSourceEnum.SURVEYS,
        ZEN_SURVEYS: ServiceSourceEnum.ZEN_SURVEYS,
        VH: ServiceSourceEnum.VH,
    }

    CACHE_SERVICE_BY_SOURCE = {
        YUID_REQID_TESTID_FILTER: YUID_REQID_TESTID_FILTER,
        ServiceSourceEnum.CACHE_DESKTOP: WEB_DESKTOP,
        ServiceSourceEnum.CACHE_TOUCH: WEB_TOUCH,
        ServiceSourceEnum.CACHE_DESKTOP_EXTENDED: WEB_DESKTOP_EXTENDED,
        ServiceSourceEnum.CACHE_TOUCH_EXTENDED: WEB_TOUCH_EXTENDED,
        ServiceSourceEnum.CACHE_NEWS: NEWS,
    }

    @staticmethod
    def extend_services(services: List[str]) -> List[str]:
        services_new = []
        for service in services:
            if service == ServiceEnum.WEB_AUTO:
                services_new.extend([ServiceEnum.WEB_DESKTOP, ServiceEnum.WEB_TOUCH])
            elif service == ServiceEnum.WEB_AUTO_EXTENDED:
                services_new.extend([ServiceEnum.WEB_DESKTOP_EXTENDED, ServiceEnum.WEB_TOUCH_EXTENDED])
            else:
                services_new.append(service)
        return services_new

    @staticmethod
    def convert_aliases(services: Union[List[str], Set[str]]) -> List[str]:
        return sorted({ServiceEnum.ALIASES.get(s, s) for s in services})

    @staticmethod
    def get_all_services() -> List[str]:
        result = set(ServiceEnum.ALIASES.keys())
        result.update(ServiceEnum.ALL)
        result.update(ServiceEnum.AUTO)
        return sorted(result)

    @staticmethod
    def get_cache_service(service: str) -> str:
        if service in ServiceEnum.SERVICE_TO_CACHE:
            return ServiceEnum.SERVICE_TO_CACHE[service]
        return service

    @staticmethod
    def get_cache_source(service: str) -> str:
        return ServiceEnum.SOURCES[ServiceEnum.get_cache_service(service)]

    @staticmethod
    def check_cache_services(services: List[str]) -> bool:
        if ServiceEnum.YUID_REQID_TESTID_FILTER in services:
            assert all(s in ServiceEnum.CACHE_SUPPORTED for s in ServiceEnum.extend_services(services))
            return True
        return False


class SqueezeResultEnum(YaqEnum):
    SKIPPED = "skipped"
    SKIPPED_AFTER_LOCK = "skipped-after-lock"
    SQUEEZED = "squeezed"

    ALL = {
        SKIPPED,
        SKIPPED_AFTER_LOCK,
        SQUEEZED,
    }
