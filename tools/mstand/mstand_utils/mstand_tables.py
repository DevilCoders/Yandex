import datetime
import os

from typing import Callable
from typing import List
from typing import Optional
from typing import Set
from typing import Tuple
from typing import Union

import yaqutils.time_helpers as utime

from yaqtypes import JsonDict

from mstand_enums.mstand_online_enums import ServiceSourceEnum, ServiceEnum, TableTypeEnum, TableGroupEnum

DEFAULT_SQUEEZE_PATH = "//home/mstand/squeeze"
DEFAULT_USER_SESSIONS_PATH = "//user_sessions/pub"
DEFAULT_YUIDS_PATH = "//home/abt/yuid_testids"
DEFAULT_YUID_MARKET_PATH = "//home/market-search/yuid-testids"
DEFAULT_ZEN_PATH = "//logs/zen-stats-checked-log/1d"
DEFAULT_ZEN_SESSIONS_PATH = "//home/recommender/zen/sessions_aggregates/background"
DEFAULT_EVENT_MONEY_PATH = "//statbox/cube/daily/product_money/v1"
DEFAULT_FAST_EVENT_MONEY_PATH = "//statbox/cube/daily/event_money_direct/v2/1d"

TFilterKey = Optional[Union[str, int, Tuple[Union[str, int], ...]]]


class BaseTable:
    day: datetime.date
    path_template: str
    use_pretty_date_format: bool = True
    type: TableTypeEnum = TableTypeEnum.SOURCE
    filter_by: Optional[str] = None
    exact_key: Optional[str] = None
    ranges: Optional[List[JsonDict]] = None

    @property
    def path(self) -> Optional[str]:
        day_str = utime.format_date(date=self.day, pretty=self.use_pretty_date_format)
        return os.path.join(self.path_template, day_str)

    def get_lower_upper_filter_keys(self, lower_yuid: str, upper_yuid: str) -> Tuple[TFilterKey, TFilterKey]:
        return lower_yuid, upper_yuid


class UserSessionsTable(BaseTable):
    def __init__(self,
                 day: datetime.date,
                 source: str,
                 user_sessions: str = DEFAULT_USER_SESSIONS_PATH,
                 services: Optional[Set[str]] = None,
                 table_name: str = "clean",
                 enable_tech: bool = True,
                 yt_path_checker: Optional[Callable[[str], bool]] = None,
                 table_groups: Optional[List[str]] = None) -> None:
        self.day = day
        self.path_template = user_sessions
        self.source = source
        self.services = services
        self.table_name = table_name
        self.enable_tech = enable_tech
        self.yt_path_checker = yt_path_checker
        self.table_groups = [TableGroupEnum.CLEAN] if table_groups is None else table_groups

    @property
    def path(self) -> Optional[str]:
        day_str = self.day.strftime("%Y-%m-%d")

        if self.table_name.endswith("tech") and not self.enable_tech:
            return None

        if self.source == "bs_chevent_log":
            if self.day < datetime.date(2021, 1, 21):
                return None
            if self.day >= datetime.date(2021, 10, 1):  # https://st.yandex-team.ru/MSTAND-2108#62014f2f6e9da2476aa4ba76
                if TableGroupEnum.CLEAN in self.table_groups:
                    table_path = os.path.join(DEFAULT_EVENT_MONEY_PATH, day_str)
                    if self.yt_path_checker and self.yt_path_checker(table_path):
                        return table_path
                    else:
                        return os.path.join(DEFAULT_FAST_EVENT_MONEY_PATH, day_str)
                else:
                    return None

        if self.table_name.startswith("yandex_staff"):
            if TableGroupEnum.STAFF not in self.table_groups:
                return None
        elif self.table_name.startswith("robots") or self.table_name.startswith("frauds"):
            if TableGroupEnum.ROBOTS not in self.table_groups:
                return None
        else:
            if TableGroupEnum.CLEAN not in self.table_groups:
                return None

        real_source = self.source
        if self.source in {ServiceSourceEnum.VIDEO, ServiceSourceEnum.IMAGES} and self.day < datetime.date(2020, 3, 4):
            real_source = ServiceSourceEnum.SEARCH

        if real_source in ServiceSourceEnum.YUIDS_ZEN:
            real_source = real_source.lstrip("zen-")
        if real_source == "search" and datetime.date(2016, 4, 1) <= self.day <= datetime.date(2016, 4, 7):
            if not self.services:
                raise Exception("Can't use broken user sessions %s/%s", real_source, day_str)
            not_supported_services = self.services - {ServiceEnum.IMAGES, ServiceEnum.VIDEO}
            if not_supported_services:
                raise Exception("Can't use broken user sessions %s/%s for %s",
                                real_source,
                                day_str,
                                sorted(not_supported_services))
            day_str += ".broken"
        if real_source == ServiceSourceEnum.WATCHLOG and self.day <= datetime.date(2015, 5, 15):
            real_source = "watch_log"

        path = os.path.join(
            self.path_template,
            real_source,
            "daily",
            day_str,
            self.table_name,
        )

        if self.yt_path_checker is not None and self.table_name == "tech" and not self.yt_path_checker(path):
            return None
        return path


class CacheTable(BaseTable):
    def __init__(self, source: str, day: datetime.date) -> None:
        self.path_template = get_cache_dir(source=source)
        self.day = day
        self.use_pretty_date_format = False


class TolokaEligiblePoolsTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/toloka/prod/export/eligible_pools"
        self.day = day


class TolokaWorkerPoolSelectionTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/toloka/prod/export/worker_pool_selection"
        self.day = day


class TolokaResultsV56Table(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.day = day

    @property
    def path(self) -> Optional[str]:
        today = datetime.date.today()
        if (today - self.day).days < 90:
            return "//home/toloka-analytics/prod/export/results_v56/data"
        else:
            return "//home/toloka/prod/export/assignments/results_v56"


class TolokaWorkerSourceTable(BaseTable):
    def __init__(self) -> None:
        self.path_template = "//home/toloka-analytics/prod/export/workers/worker_source"

    @property
    def path(self) -> Optional[str]:
        return self.path_template


class TolokaWorkerDailyStatisticsTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/toloka/prod/done/rating/worker_daily_statistics"
        self.day = day


class AliceTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/alice/dialog/prepared_logs_expboxes"
        self.day = day
        self.filter_by = "uuid"


class VoiceHeartbeatsTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/voice/logs/heartbeats"
        self.day = day
        self.filter_by = "uuid"

    @property
    def path(self) -> Optional[str]:
        if self.day < datetime.date(2019, 12, 27):
            return None
        return super().path


class AppMetricsTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//statbox/metrika-mobile-log"
        self.day = day


class YaMetricsTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//logs/bs-watch-log/1d"
        self.day = day
        self.exact_key = "26573484"
        self.filter_by = "passportuid"


class ZenTable(BaseTable):
    def __init__(self, day: datetime.date, zen_path: str = DEFAULT_ZEN_PATH) -> None:
        self.day = day
        if zen_path == "//logs/zen-stats-checked-log/1d":
            if self.day <= datetime.date(2020, 12, 31):
                self.path_template = "//home/recommender/logs/zen-stats-log/1d"
                self.filter_by = "yandexid"
            else:
                self.path_template = "//logs/zen-events-checked-log/1d"
                self.filter_by = "strongest_id"
        else:
            self.path_template = zen_path


class ZenSessionsTable(BaseTable):
    def __init__(self,
                 day: datetime.date,
                 no_zen_sessions: bool = False,
                 zen_sessions_path: str = DEFAULT_ZEN_SESSIONS_PATH) -> None:
        self.day = day
        self.no_zen_sessions = no_zen_sessions
        self.path_template = zen_sessions_path

    @property
    def path(self) -> Optional[str]:
        if self.no_zen_sessions:
            return None
        return super().path


class TurboTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/search-functionality/mstand_squeeze"
        self.day = day
        self.filter_by = "yuid"

    def get_lower_upper_filter_keys(self, lower_yuid: str, upper_yuid: str) -> Tuple[TFilterKey, TFilterKey]:
        try:
            return int(lower_yuid), int(upper_yuid)
        except ValueError:
            raise Exception("Lower and upper keys must be numerical for turbo")


class VideoStrmSessionsTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//cubes/video-strm"
        self.day = day
        self.filter_by = "yandexuid"

    @property
    def path(self) -> Optional[str]:
        return os.path.join(super().path, "sessions")


class EcomTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/abt/ecom"
        self.day = day


class PrismTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/prism/user_weights"
        self.day = day
        self.filter_by = "yandexuid"


class YqlAbTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/portalytics/yql_ab/mstand_export"
        self.day = day
        self.filter_by = "uniqid"

    def get_lower_upper_filter_keys(self, lower_yuid: str, upper_yuid: str) -> Tuple[TFilterKey, TFilterKey]:
        try:
            return int(lower_yuid), int(upper_yuid)
        except ValueError:
            raise Exception("Lower and upper keys must be numerical for yql-ab")


class IntrasearchMetrikaTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/logfeller/logs/bs-watch-log/1d"
        self.day = day
        self.filter_by = "uniqid"


class OttRecommendationTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/ott-analytics/logs/recommendations/recommendations_date_link"
        self.day = day

    def get_lower_upper_filter_keys(self, lower_yuid: str, upper_yuid: str) -> Tuple[TFilterKey, TFilterKey]:
        day_str = self.day.strftime("%Y-%m-%d")
        return (day_str, lower_yuid), (day_str, upper_yuid)


class OttImpressionsTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/ott-analytics/logs/impressions/impressions_date_link"
        self.day = day

    def get_lower_upper_filter_keys(self, lower_yuid: str, upper_yuid: str) -> Tuple[TFilterKey, TFilterKey]:
        day_str = self.day.strftime("%Y-%m-%d")
        return (day_str, lower_yuid), (day_str, upper_yuid)


class OttSessionsTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/ott-analytics/kaminsky/prod/sessions_cube/sessions"
        self.day = day


class ObjectAnswerTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/dict/ontodb/squeezer"
        self.day = day
        self.filter_by = "UID"

    @property
    def path(self) -> Optional[str]:
        return os.path.join(super().path, "web")


class PpTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/mobilesearch/analytics/app_sessions/clean"
        self.day = day
        self.type = TableTypeEnum.SOURCE
        self.filter_by = "icookie"
        self.ranges = [
            {"exact": {"key": [app_id, "VERTICAL_SWITCH", "HEADER_CLICK"]}}
            for app_id in ("ru.yandex.searchplugin", "ru.yandex.searchplugin.beta")
        ]

    def get_lower_upper_filter_keys(self, lower_yuid: str, upper_yuid: str) -> Tuple[TFilterKey, TFilterKey]:
        return int(lower_yuid.lstrip("y")), int(upper_yuid.lstrip("y"))


class SurveysTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/goda/prod/surveys/pythia_answers_enrich"
        self.day = day
        self.type = TableTypeEnum.SOURCE
        self.filter_by = "yandexuid"

    @property
    def path(self) -> Optional[str]:
        return self.path_template

    def get_lower_upper_filter_keys(self, lower_yuid: str, upper_yuid: str) -> Tuple[TFilterKey, TFilterKey]:
        return int(lower_yuid.lstrip("y")), int(upper_yuid.lstrip("y"))


class ZenSurveysTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/goda/prod/surveys/zen_answers_enrich"
        self.day = day
        self.type = TableTypeEnum.SOURCE
        self.filter_by = "strongest_id"

    @property
    def path(self) -> Optional[str]:
        return self.path_template


class VideoSessionsTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/videoquality/vh_analytics/strm_cube_2"
        self.day = day
        self.type = TableTypeEnum.SOURCE
        self.filter_by = "yandexuid"

    @property
    def path(self) -> Optional[str]:
        return os.path.join(super().path, "sessions")

    def get_lower_upper_filter_keys(self, lower_yuid: str, upper_yuid: str) -> Tuple[TFilterKey, TFilterKey]:
        return lower_yuid.lstrip("y"), upper_yuid.lstrip("y")


class MstandYuidsTable(BaseTable):
    def __init__(self,
                 day: datetime.date,
                 yuids_dir: str = DEFAULT_YUIDS_PATH) -> None:
        self.path_template = yuids_dir
        self.day = day
        self.type = TableTypeEnum.YUID
        self.use_pretty_date_format = False

    @property
    def path(self) -> Optional[str]:
        # https://st.yandex-team.ru/MSTAND-2119#620a5385f243cb682f4ee4d9
        # https://st.yandex-team.ru/MSTAND-2214
        if self.day == datetime.date(2022, 1, 27) and "surveys_strongest_testids" not in self.path_template:
            return None
        return super().path


class DesktopYandexuidsTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/browser-user-sessions/desktop/prod/desktop-yandexuids-abt"
        self.day = day
        self.type = TableTypeEnum.YUID


class MobileYandexuidsTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/browser-user-sessions/mobile/v2/mobile-yandexuids-abt"
        self.day = day
        self.type = TableTypeEnum.YUID


class ZenYuidsTable(BaseTable):
    def __init__(self, day: datetime.date) -> None:
        self.path_template = "//home/abt/zen"
        self.day = day
        self.type = TableTypeEnum.YUID


class MstandDailyTable(BaseTable):
    def __init__(self,
                 day: datetime.date,
                 service: str,
                 testid: str,
                 dates: Optional[utime.DateRange] = None,
                 filter_hash: Optional[str] = None,
                 squeeze_path: str = DEFAULT_SQUEEZE_PATH) -> None:
        self.path_template = mstand_experiment_dir(squeeze_path=squeeze_path,
                                                   service=service,
                                                   testid=testid,
                                                   dates=dates,
                                                   filter_hash=filter_hash)
        self.day = day
        self.use_pretty_date_format = False
        self.type = TableTypeEnum.SQUEEZE


class MstandHistoryTable(BaseTable):
    def __init__(self,
                 day: datetime.date,
                 service: str,
                 testid: str,
                 dates: utime.DateRange,
                 filter_hash: Optional[str] = None,
                 squeeze_path: str = DEFAULT_SQUEEZE_PATH) -> None:
        self.path_template = _mstand_history_dir(squeeze_path=squeeze_path,
                                                 service=service,
                                                 testid=testid,
                                                 filter_hash=filter_hash,
                                                 dates=dates)
        self.day = day
        self.use_pretty_date_format = False
        self.type = TableTypeEnum.SQUEEZE


def get_cache_dir(source: Optional[str] = None, service: Optional[str] = None) -> str:
    if source is not None and service is None:
        service = ServiceEnum.CACHE_SERVICE_BY_SOURCE.get(source)

    if not service:
        raise Exception("Unsupported service {}".format(source))

    account = "ynews" if service == ServiceEnum.NEWS else "mstand"

    return os.path.join("//home", account, "squeeze/testids", service, "0")


def mstand_experiment_dir(service: str,
                          testid: str,
                          dates: Optional[utime.DateRange] = None,
                          filter_hash: Optional[str] = None,
                          history_dates: Optional[utime.DateRange] = None,
                          squeeze_path: str = DEFAULT_SQUEEZE_PATH) -> str:
    if history_dates is not None:
        return _mstand_history_dir(squeeze_path=squeeze_path,
                                   service=service,
                                   testid=testid,
                                   filter_hash=filter_hash,
                                   dates=history_dates)

    date_from = None
    if service == ServiceEnum.WATCHLOG:
        assert dates is not None
        date_from = dates.start

    return os.path.join(
        squeeze_path,
        "testids",
        service,
        _experiment_dir_name(testid, filter_hash, date_from),
    )


def _experiment_dir_name(testid: str,
                         filter_hash: Optional[str] = None,
                         date_from: Optional[datetime.date] = None) -> str:
    dir_name = testid
    if filter_hash:
        dir_name += "." + filter_hash
    if date_from:
        dir_name += ":" + utime.format_date(date_from)
    return dir_name


def _mstand_history_dir(service: str,
                        testid: str,
                        dates: utime.DateRange,
                        filter_hash: Optional[str] = None,
                        squeeze_path: str = DEFAULT_SQUEEZE_PATH) -> str:
    return os.path.join(
        squeeze_path,
        "history",
        service,
        _experiment_dir_name(testid, filter_hash),
        "{}_{}".format(utime.format_date(dates.start), utime.format_date(dates.end)),
    )
