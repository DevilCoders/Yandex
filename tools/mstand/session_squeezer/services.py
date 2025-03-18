from typing import Dict
from typing import List
from typing import Set
from typing import Union

from mstand_enums.mstand_online_enums import ServiceEnum
from mstand_structs.squeeze_versions import SqueezeVersions
from session_squeezer.squeezer_alice import ActionsSqueezerAlice
from session_squeezer.squeezer_app_metrics_toloka import ActionsSqueezerAppMetricsToloka
from session_squeezer.squeezer_cv import ActionsSqueezerCv
from session_squeezer.squeezer_ecom import ActionsSqueezerEcom
from session_squeezer.squeezer_ether import ActionsSqueezerEther
from session_squeezer.squeezer_images import ActionsSqueezerImages
from session_squeezer.squeezer_intrasearch import ActionsSqueezerIntra
from session_squeezer.squeezer_intrasearch_metrika import ActionsSqueezerIntrasearchMetrika
from session_squeezer.squeezer_market_sessions_stat import ActionsSqueezerMarketSessionsStat
from session_squeezer.squeezer_market_search_sessions import ActionsSqueezerMarketSearchSessions
from session_squeezer.squeezer_market_web_reqid import ActionsSqueezerMarketWebReqid
from session_squeezer.squeezer_morda import ActionsSqueezerMorda, ActionsSqueezerZenMorda
from session_squeezer.squeezer_news import ActionsSqueezerNews
from session_squeezer.squeezer_object_answer import ActionsSqueezerObjectAnswer
from session_squeezer.squeezer_ott_impressions import ActionsSqueezerOttImpressions
from session_squeezer.squeezer_ott_sessions import ActionsSqueezerOttSessions
from session_squeezer.squeezer_pp import ActionsSqueezerPp
from session_squeezer.squeezer_prism import ActionsSqueezerPrism
from session_squeezer.squeezer_recommender import ActionsSqueezerRecommender
from session_squeezer.squeezer_surveys import ActionsSqueezerSurveys
from session_squeezer.squeezer_toloka import ActionsSqueezerToloka
from session_squeezer.squeezer_turbo import ActionsSqueezerTurbo
from session_squeezer.squeezer_yuid_reqid_testid_filter import ActionsSqueezerYuidReqidTestidFilter
from session_squeezer.squeezer_vh import ActionsSqueezerVH
from session_squeezer.squeezer_video import ActionsSqueezerVideo
from session_squeezer.squeezer_watchlog import ActionsSqueezerWatchlog
from session_squeezer.squeezer_web import ActionsSqueezerWebDesktop
from session_squeezer.squeezer_web import ActionsSqueezerWebTouch
from session_squeezer.squeezer_web_cache import (
    ActionsSqueezerCacheDesktop,
    ActionsSqueezerCacheTouch,
    ActionsSqueezerCacheDesktopExtended,
    ActionsSqueezerCacheTouchExtended,
    ActionsSqueezerCacheNews,
)
from session_squeezer.squeezer_web_extended import ActionsSqueezerWebDesktopExtended
from session_squeezer.squeezer_web_extended import ActionsSqueezerWebTouchExtended
from session_squeezer.squeezer_web_surveys import ActionsSqueezerWebSurveys
from session_squeezer.squeezer_ya_metrics import ActionsSqueezerYaMetricsToloka
from session_squeezer.squeezer_ya_video import ActionsSqueezerYaVideo
from session_squeezer.squeezer_yql_ab import ActionsSqueezerYqlAb
from session_squeezer.squeezer_zen import ActionsSqueezerZen
from session_squeezer.squeezer_zen_surveys import ActionsSqueezerZenSurveys


SQUEEZERS = {
    ServiceEnum.WEB_DESKTOP: ActionsSqueezerWebDesktop,
    ServiceEnum.WEB_DESKTOP_EXTENDED: ActionsSqueezerWebDesktopExtended,
    ServiceEnum.WEB_TOUCH: ActionsSqueezerWebTouch,
    ServiceEnum.WEB_TOUCH_EXTENDED: ActionsSqueezerWebTouchExtended,

    ServiceEnum.INTRASEARCH: ActionsSqueezerIntra,
    ServiceEnum.IMAGES: ActionsSqueezerImages,
    ServiceEnum.VIDEO: ActionsSqueezerVideo,
    ServiceEnum.CV: ActionsSqueezerCv,
    ServiceEnum.MORDA: ActionsSqueezerMorda,
    ServiceEnum.NEWS: ActionsSqueezerNews,  # TODO: seems to be unused, consider removing it, no yt tests
    ServiceEnum.YUID_REQID_TESTID_FILTER: ActionsSqueezerYuidReqidTestidFilter,
    ServiceEnum.WEB_SURVEYS: ActionsSqueezerWebSurveys,

    ServiceEnum.WATCHLOG: ActionsSqueezerWatchlog,
    ServiceEnum.MARKET_SESSIONS_STAT: ActionsSqueezerMarketSessionsStat,
    ServiceEnum.MARKET_SEARCH_SESSIONS: ActionsSqueezerMarketSearchSessions,
    ServiceEnum.MARKET_WEB_REQID: ActionsSqueezerMarketWebReqid,

    # TODO: no yt tests due to QB2 import problems, requires solving
    ServiceEnum.RECOMMENDER: ActionsSqueezerRecommender,
    ServiceEnum.TOLOKA: ActionsSqueezerToloka,
    ServiceEnum.ALICE: ActionsSqueezerAlice,
    ServiceEnum.APP_METRICS_TOLOKA: ActionsSqueezerAppMetricsToloka,
    ServiceEnum.ZEN: ActionsSqueezerZen,  # TODO: seems to be incomplete and lacks yt tests, should be completed
    ServiceEnum.YA_METRICS_TOLOKA: ActionsSqueezerYaMetricsToloka,
    # TODO: seems to be incomplete and lacks yt tests, should be completed
    ServiceEnum.TURBO: ActionsSqueezerTurbo,
    ServiceEnum.ETHER: ActionsSqueezerEther,
    ServiceEnum.YA_VIDEO: ActionsSqueezerYaVideo,
    ServiceEnum.ECOM: ActionsSqueezerEcom,
    ServiceEnum.PRISM: ActionsSqueezerPrism,
    ServiceEnum.YQL_AB: ActionsSqueezerYqlAb,
    ServiceEnum.INTRASEARCH_METRIKA: ActionsSqueezerIntrasearchMetrika,
    ServiceEnum.OTT_IMPRESSIONS: ActionsSqueezerOttImpressions,
    ServiceEnum.OTT_SESSIONS: ActionsSqueezerOttSessions,
    ServiceEnum.OBJECT_ANSWER: ActionsSqueezerObjectAnswer,
    ServiceEnum.PP: ActionsSqueezerPp,
    ServiceEnum.SURVEYS: ActionsSqueezerSurveys,
    ServiceEnum.ZEN_SURVEYS: ActionsSqueezerZenSurveys,
    ServiceEnum.VH: ActionsSqueezerVH,

    ServiceEnum.ZEN_MORDA: ActionsSqueezerZenMorda,

    ServiceEnum.CACHE_DESKTOP: ActionsSqueezerCacheDesktop,
    ServiceEnum.CACHE_TOUCH: ActionsSqueezerCacheTouch,
    ServiceEnum.CACHE_DESKTOP_EXTENDED: ActionsSqueezerCacheDesktopExtended,
    ServiceEnum.CACHE_TOUCH_EXTENDED: ActionsSqueezerCacheTouchExtended,
    ServiceEnum.CACHE_NEWS: ActionsSqueezerCacheNews,
}


def assert_services(services: List[str]) -> None:
    assert services
    for service in ServiceEnum.extend_services(services):
        assert service in SQUEEZERS


def create_service_squeezers(services: Union[List[str], Set[str]], enable_cache: bool = False) -> Dict[str, str]:
    if enable_cache:
        return {
            service: SQUEEZERS[ServiceEnum.get_cache_service(service)]()
            for service in services
        }
    return {service: SQUEEZERS[service]() for service in services}


def get_squeezer_versions(services: List[str]) -> SqueezeVersions:
    service_versions = {}
    for service in ServiceEnum.extend_services(services):
        service_versions[service] = SQUEEZERS[service].VERSION
    return SqueezeVersions(
        service_versions=service_versions,
        common=SQUEEZE_COMMON_VERSION,
        history=SQUEEZE_HISTORY_VERSION,
        filters=SQUEEZE_FILTERS_VERSION,
    )


def has_filter_support(service: str) -> bool:
    return service in SQUEEZERS and (SQUEEZERS[service].USE_LIBRA or
                                     SQUEEZERS[service].USE_ANY_FILTER or
                                     service in ServiceEnum.CACHE_SUPPORTED)


def get_columns_description_by_service(service: str) -> List[Dict[str, str]]:
    service_squeezer = SQUEEZERS[service]
    return service_squeezer.YT_SCHEMA


def check_allow_any_filters(services: List[str]) -> bool:
    return all(SQUEEZERS[s].USE_ANY_FILTER for s in ServiceEnum.extend_services(services))


"""
Common versions
1: initial version
2: fix bug with is_match field MSTAND-1045
"""
SQUEEZE_COMMON_VERSION = 2

"""
History versions
1: initial version
"""
SQUEEZE_HISTORY_VERSION = 1

"""
Filters versions
1: initial version
2: all-user filters
"""
SQUEEZE_FILTERS_VERSION = 2
