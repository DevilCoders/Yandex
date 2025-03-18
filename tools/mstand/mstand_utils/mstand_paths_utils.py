import argparse
import datetime

from typing import Callable
from typing import Dict
from typing import List
from typing import Optional
from typing import Set

import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.mstand_tables as mstand_tables
import yaqutils.time_helpers as utime

from mstand_enums.mstand_online_enums import ServiceEnum, ServiceSourceEnum, TableTypeEnum, TableGroupEnum


class SqueezePaths:
    def __init__(self, tables: List[mstand_tables.BaseTable]) -> None:
        self.yuids: List[str] = _get_paths_from_tables(tables=tables, table_type=TableTypeEnum.YUID)
        self.sources: List[str] = _get_paths_from_tables(tables=tables, table_type=TableTypeEnum.SOURCE)

    @property
    def tables(self) -> List[str]:
        tables: List[str] = []

        tables.extend(self.yuids)
        tables.extend(self.sources)

        return tables

    @property
    def types(self) -> List[int]:
        types: List[int] = []

        types.extend([TableTypeEnum.YUID] * len(self.yuids))
        types.extend([TableTypeEnum.SOURCE] * len(self.sources))

        return types

    def __str__(self) -> str:
        return "\n".join(self.tables)


class PathsParams:
    def __init__(self,
                 user_sessions_path: str = mstand_tables.DEFAULT_USER_SESSIONS_PATH,
                 zen_path: str = mstand_tables.DEFAULT_ZEN_PATH,
                 zen_sessions_path: str = mstand_tables.DEFAULT_ZEN_SESSIONS_PATH,
                 yuids_path: str = mstand_tables.DEFAULT_YUIDS_PATH,
                 yuids_market_path: str = mstand_tables.DEFAULT_YUID_MARKET_PATH,
                 squeeze_path: str = mstand_tables.DEFAULT_SQUEEZE_PATH) -> None:
        self.user_sessions_path = user_sessions_path
        self.zen_path = zen_path
        self.zen_sessions_path = zen_sessions_path
        self.yuids_path = yuids_path
        self.yuids_market_path = yuids_market_path
        self.squeeze_path = squeeze_path

    @staticmethod
    def from_cli_args(cli_args: argparse.Namespace) -> "PathsParams":
        return PathsParams(
            user_sessions_path=cli_args.sessions_path,
            yuids_path=cli_args.yuids_path,
            yuids_market_path=cli_args.yuids_market_path,
            squeeze_path=cli_args.squeeze_path,
            zen_path=cli_args.zen_path,
            zen_sessions_path=cli_args.zen_sessions_path,
        )

    @staticmethod
    def add_cli_args(parser: argparse.ArgumentParser,
                     default_sessions: str = mstand_tables.DEFAULT_USER_SESSIONS_PATH,
                     default_squeeze: str = mstand_tables.DEFAULT_SQUEEZE_PATH,
                     default_yuid: str = mstand_tables.DEFAULT_YUIDS_PATH,
                     default_yuid_market: str = mstand_tables.DEFAULT_YUID_MARKET_PATH,
                     default_zen: str = mstand_tables.DEFAULT_ZEN_PATH,
                     default_zen_sessions: str = mstand_tables.DEFAULT_ZEN_SESSIONS_PATH) -> None:
        mstand_uargs.add_sessions_path(parser, default_sessions)
        mstand_uargs.add_yuids_path(parser, default_yuid, default_yuid_market)
        mstand_uargs.add_squeeze_path(parser, default_squeeze)
        mstand_uargs.add_zen_path(parser, default_zen)
        mstand_uargs.add_zen_sessions_path(parser, default_zen_sessions)


def get_daily_squeeze_paths_by_source(day: datetime.date,
                                      source: str,
                                      paths_params: PathsParams = PathsParams(),
                                      services: Optional[Set[str]] = None,
                                      enable_tech: bool = True,
                                      use_bro_yuids_tables: bool = False,
                                      no_zen_sessions: bool = False,
                                      path_checker: Optional[Callable[[str], bool]] = None,
                                      exp_dates_for_history: Optional[utime.DateRange] = None,
                                      dates: Optional[utime.DateRange] = None,
                                      table_groups: Optional[List[str]] = None) -> SqueezePaths:
    tables_by_source = get_tables_by_source(day=day,
                                            paths_params=paths_params,
                                            services=services,
                                            use_bro_yuids_tables=use_bro_yuids_tables,
                                            enable_tech=enable_tech,
                                            no_zen_sessions=no_zen_sessions,
                                            path_checker=path_checker,
                                            exp_dates_for_history=exp_dates_for_history,
                                            dates=dates,
                                            table_groups=table_groups)

    source_tables = tables_by_source.get(source)
    if not source_tables:
        raise Exception("Unable to map task.source [{}] to the source table ".format(source))

    return SqueezePaths(tables=source_tables)


def get_tables_by_source(day: datetime.date,
                         paths_params: PathsParams = PathsParams(),
                         use_bro_yuids_tables: bool = False,
                         enable_tech: bool = True,
                         services: Optional[Set[str]] = None,
                         no_zen_sessions: bool = False,
                         path_checker: Optional[Callable[[str], bool]] = None,
                         exp_dates_for_history: Optional[utime.DateRange] = None,
                         dates: Optional[utime.DateRange] = None,
                         table_groups: Optional[List[str]] = None) -> Dict[str, List[mstand_tables.BaseTable]]:
    source_to_tables: Dict[str, List[mstand_tables.BaseTable]] = {
        ServiceSourceEnum.IMAGES: [
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.IMAGES,
                                            services=services,
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.IMAGES,
                                            table_name="yandex_staff",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.IMAGES,
                                            table_name="robots",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.IMAGES,
                                            table_name="frauds",
                                            table_groups=table_groups),
            *_get_yuids_tables(day=day,
                               history=exp_dates_for_history,
                               yuids_dir=paths_params.yuids_path,
                               use_bro_yuids_tables=use_bro_yuids_tables),
        ],
        ServiceSourceEnum.VIDEO: [
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.VIDEO,
                                            services=services,
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.VIDEO,
                                            table_name="yandex_staff",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.VIDEO,
                                            table_name="robots",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.VIDEO,
                                            table_name="frauds",
                                            table_groups=table_groups),
            *_get_yuids_tables(day=day,
                               history=exp_dates_for_history,
                               yuids_dir=paths_params.yuids_path,
                               use_bro_yuids_tables=use_bro_yuids_tables),
        ],
        ServiceSourceEnum.NEWS: [
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.SEARCH,
                                            services=services,
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.SEARCH,
                                            table_name="yandex_staff",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.SEARCH,
                                            table_name="robots",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.SEARCH,
                                            table_name="frauds",
                                            table_groups=table_groups),
            *_get_yuids_tables(day=day,
                               history=exp_dates_for_history,
                               yuids_dir=paths_params.yuids_path,
                               use_bro_yuids_tables=use_bro_yuids_tables),
        ],
        ServiceSourceEnum.SEARCH: [
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source="bs_chevent_log",
                                            yt_path_checker=path_checker,
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.SEARCH,
                                            services=services,
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.SEARCH,
                                            services=services,
                                            table_name="tech",
                                            enable_tech=enable_tech,
                                            yt_path_checker=path_checker,
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.SEARCH,
                                            table_name="yandex_staff",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.SEARCH,
                                            table_name="robots",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.SEARCH,
                                            services=services,
                                            table_name="robots.tech",
                                            enable_tech=enable_tech,
                                            yt_path_checker=path_checker,
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.SEARCH,
                                            table_name="frauds",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.SEARCH,
                                            services=services,
                                            table_name="frauds.tech",
                                            enable_tech=enable_tech,
                                            yt_path_checker=path_checker,
                                            table_groups=table_groups),
            *_get_yuids_tables(day=day,
                               history=exp_dates_for_history,
                               yuids_dir=paths_params.yuids_path,
                               use_bro_yuids_tables=use_bro_yuids_tables),
        ],
        ServiceSourceEnum.SEARCH_STAFF: [
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.SEARCH,
                                            services=services,
                                            table_name="yandex_staff",
                                            table_groups=[TableGroupEnum.STAFF]),
        ],
        ServiceSourceEnum.RECOMMENDER: [
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source="bs_chevent_log",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.SEARCH,
                                            services=services,
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.SEARCH,
                                            table_name="yandex_staff",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.SEARCH,
                                            table_name="robots",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.SEARCH,
                                            table_name="frauds",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.RECOMMENDER,
                                            services=services,
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.RECOMMENDER,
                                            table_name="yandex_staff",
                                            table_groups=table_groups),
        ],
        ServiceSourceEnum.MARKET_WEB: [
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.MARKET,
                                            services=services,
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.MARKET,
                                            table_name="yandex_staff",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.MARKET,
                                            table_name="robots",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.MARKET,
                                            table_name="frauds",
                                            table_groups=table_groups),
            *_get_yuids_tables(day=day,
                               history=exp_dates_for_history,
                               yuids_dir=paths_params.yuids_path,
                               use_bro_yuids_tables=use_bro_yuids_tables),
        ],
        ServiceSourceEnum.MARKET: [
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.MARKET,
                                            services=services,
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.MARKET,
                                            table_name="yandex_staff",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.MARKET,
                                            table_name="robots",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.MARKET,
                                            table_name="frauds",
                                            table_groups=table_groups),
            *_get_yuids_tables(day=day,
                               history=exp_dates_for_history,
                               yuids_dir=paths_params.yuids_market_path,
                               use_bro_yuids_tables=use_bro_yuids_tables),
        ],
        ServiceSourceEnum.MARKET_BIN: [
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.MARKET,
                                            services=services,
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.MARKET,
                                            table_name="yandex_staff",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.MARKET,
                                            table_name="robots",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.MARKET,
                                            table_name="frauds",
                                            table_groups=table_groups),
            *_get_yuids_tables(day=day,
                               history=exp_dates_for_history,
                               yuids_dir=paths_params.yuids_market_path,
                               use_bro_yuids_tables=use_bro_yuids_tables),
        ],
        ServiceSourceEnum.WATCHLOG: [
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.WATCHLOG,
                                            services=services,
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.WATCHLOG,
                                            table_name="yandex_staff",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.WATCHLOG,
                                            table_name="robots",
                                            table_groups=table_groups),
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.WATCHLOG,
                                            table_name="frauds",
                                            table_groups=table_groups),
            *_get_yuid_tables_accumulation(day=day,
                                           dates=dates,
                                           exp_dates_for_history=exp_dates_for_history,
                                           yuids_dir=paths_params.yuids_path,
                                           use_bro_yuids_tables=use_bro_yuids_tables),
        ],
        ServiceSourceEnum.ZEN_SEARCH: [
            mstand_tables.UserSessionsTable(day=day,
                                            user_sessions=paths_params.user_sessions_path,
                                            source=ServiceSourceEnum.ZEN_SEARCH,
                                            services=services,
                                            table_groups=table_groups),
            mstand_tables.ZenYuidsTable(day=day),
        ],

        ServiceSourceEnum.ALICE: [
            mstand_tables.AliceTable(day=day),
            mstand_tables.VoiceHeartbeatsTable(day=day)
        ],
        ServiceSourceEnum.TOLOKA: [
            mstand_tables.TolokaEligiblePoolsTable(day=day),
            mstand_tables.TolokaWorkerPoolSelectionTable(day=day),
            mstand_tables.TolokaResultsV56Table(day=day),
            mstand_tables.TolokaWorkerSourceTable(),
            mstand_tables.TolokaWorkerDailyStatisticsTable(day=day),
        ],
        ServiceSourceEnum.APP_METRICS: [
            mstand_tables.AppMetricsTable(day=day),
        ],
        ServiceSourceEnum.ZEN: [
            mstand_tables.ZenTable(day=day, zen_path=paths_params.zen_path),
            mstand_tables.ZenSessionsTable(day=day,
                                           zen_sessions_path=paths_params.zen_sessions_path,
                                           no_zen_sessions=no_zen_sessions),
        ],
        ServiceSourceEnum.YA_METRICS: [
            mstand_tables.YaMetricsTable(day=day),
        ],
        ServiceSourceEnum.TURBO: [
            mstand_tables.TurboTable(day=day),
        ],
        ServiceSourceEnum.ETHER: [
            mstand_tables.VideoStrmSessionsTable(day=day),
        ],
        ServiceSourceEnum.YA_VIDEO: [
            mstand_tables.VideoStrmSessionsTable(day=day),
        ],
        ServiceSourceEnum.ECOM: [
            mstand_tables.EcomTable(day=day),
            *_get_yuids_tables(day=day,
                               history=exp_dates_for_history,
                               yuids_dir=paths_params.yuids_path,
                               use_bro_yuids_tables=use_bro_yuids_tables),
        ],
        ServiceSourceEnum.PRISM: [
            mstand_tables.PrismTable(day=day),
            *_get_yuids_tables(day=day,
                               history=exp_dates_for_history,
                               yuids_dir=paths_params.yuids_path,
                               use_bro_yuids_tables=use_bro_yuids_tables),
        ],
        ServiceSourceEnum.YQL_AB: [
            mstand_tables.YqlAbTable(day=day),
        ],
        ServiceSourceEnum.INTRASEARCH_METRIKA: [
            mstand_tables.IntrasearchMetrikaTable(day=day),
        ],
        ServiceSourceEnum.OTT_SESSIONS: [
            mstand_tables.OttRecommendationTable(day=day),
            mstand_tables.OttSessionsTable(day=day),
        ],
        ServiceSourceEnum.OTT_IMPRESSIONS: [
            mstand_tables.OttRecommendationTable(day=day),
            mstand_tables.OttImpressionsTable(day=day),
        ],
        ServiceSourceEnum.OBJECT_ANSWER: [
            mstand_tables.ObjectAnswerTable(day=day),
        ],
        ServiceSourceEnum.PP: [
            mstand_tables.PpTable(day=day),
        ],
        ServiceSourceEnum.SURVEYS: [
            *_get_yuids_tables(day=day,
                               history=exp_dates_for_history,
                               yuids_dir="//home/mstand/squeeze/surveys_yuid_testids",
                               use_bro_yuids_tables=use_bro_yuids_tables),
            mstand_tables.SurveysTable(day=day),
        ],
        ServiceSourceEnum.ZEN_SURVEYS: [
            *_get_yuids_tables(day=day,
                               history=exp_dates_for_history,
                               yuids_dir="//home/mstand/squeeze/surveys_strongest_testids",
                               use_bro_yuids_tables=use_bro_yuids_tables),
            mstand_tables.ZenSessionsTable(day=day),
            mstand_tables.ZenSurveysTable(day=day),
        ],
        ServiceSourceEnum.VH: [
            mstand_tables.VideoSessionsTable(day=day),
        ],
        ServiceSourceEnum.CACHE_DESKTOP: [
            mstand_tables.CacheTable(day=day, source=ServiceEnum.YUID_REQID_TESTID_FILTER),
            mstand_tables.CacheTable(day=day, source=ServiceSourceEnum.CACHE_DESKTOP),
        ],
        ServiceSourceEnum.CACHE_TOUCH: [
            mstand_tables.CacheTable(day=day, source=ServiceEnum.YUID_REQID_TESTID_FILTER),
            mstand_tables.CacheTable(day=day, source=ServiceSourceEnum.CACHE_TOUCH),
        ],
        ServiceSourceEnum.CACHE_DESKTOP_EXTENDED: [
            mstand_tables.CacheTable(day=day, source=ServiceEnum.YUID_REQID_TESTID_FILTER),
            mstand_tables.CacheTable(day=day, source=ServiceSourceEnum.CACHE_DESKTOP_EXTENDED),
        ],
        ServiceSourceEnum.CACHE_TOUCH_EXTENDED: [
            mstand_tables.CacheTable(day=day, source=ServiceEnum.YUID_REQID_TESTID_FILTER),
            mstand_tables.CacheTable(day=day, source=ServiceSourceEnum.CACHE_TOUCH_EXTENDED),
        ],
        ServiceSourceEnum.CACHE_NEWS: [
            mstand_tables.CacheTable(day=day, source=ServiceEnum.YUID_REQID_TESTID_FILTER),
            mstand_tables.CacheTable(day=day, source=ServiceSourceEnum.CACHE_NEWS),
        ],
    }

    return source_to_tables


def get_squeeze_paths(dates: utime.DateRange,
                      testid: str,
                      filter_hash: Optional[str],
                      service: str,
                      squeeze_path: str = mstand_tables.DEFAULT_SQUEEZE_PATH) -> List[str]:
    squeeze_tables: List[mstand_tables.BaseTable] = [
        mstand_tables.MstandDailyTable(
            day=day,
            squeeze_path=squeeze_path,
            testid=testid,
            filter_hash=filter_hash,
            dates=dates,
            service=service) for day in dates
    ]
    return _get_paths_from_tables(tables=squeeze_tables)


def get_squeeze_history_paths(dates: utime.DateRange,
                              testid: str,
                              filter_hash: Optional[str],
                              testid_dates: utime.DateRange,
                              service: str,
                              squeeze_path: str = mstand_tables.DEFAULT_SQUEEZE_PATH) -> List[str]:
    squeeze_tables: List[mstand_tables.BaseTable] = [
        mstand_tables.MstandHistoryTable(
            day=day,
            squeeze_path=squeeze_path,
            testid=testid,
            filter_hash=filter_hash,
            dates=testid_dates,
            service=service) for day in dates
    ]
    return _get_paths_from_tables(tables=squeeze_tables)


def _get_yuids_tables(day: datetime.date,
                      yuids_dir: str = mstand_tables.DEFAULT_YUIDS_PATH,
                      history: Optional[utime.DateRange] = None,
                      use_bro_yuids_tables: bool = False) -> List[mstand_tables.BaseTable]:
    if use_bro_yuids_tables:
        return [
            mstand_tables.DesktopYandexuidsTable(day=day),
            mstand_tables.MobileYandexuidsTable(day=day),
        ]

    if history is not None and yuids_dir:
        return [
            mstand_tables.MstandYuidsTable(yuids_dir=yuids_dir, day=day) for day in history
        ]
    elif yuids_dir:
        return [
            mstand_tables.MstandYuidsTable(day=day, yuids_dir=yuids_dir)
        ]
    return []


def _get_paths_from_tables(tables: List[mstand_tables.BaseTable],
                           table_type: Optional[TableTypeEnum] = None) -> List[str]:
    paths: List[str] = []
    for table in tables:
        if table_type is not None and table.type != table_type:
            continue
        path: Optional[str] = table.path
        if path:
            paths.append(path)
    return paths


def _get_yuid_tables_accumulation(day: datetime.date,
                                  dates: Optional[utime.DateRange] = None,
                                  exp_dates_for_history: Optional[utime.DateRange] = None,
                                  yuids_dir: str = mstand_tables.DEFAULT_YUIDS_PATH,
                                  use_bro_yuids_tables: bool = False) -> List[mstand_tables.BaseTable]:
    yuids_tables: List[mstand_tables.BaseTable] = []
    if dates and not exp_dates_for_history:
        for cur_day in utime.DateRange(dates.start, day):
            yuids_tables.extend(
                _get_yuids_tables(day=cur_day,
                                  yuids_dir=yuids_dir,
                                  use_bro_yuids_tables=use_bro_yuids_tables)
            )
    return yuids_tables
