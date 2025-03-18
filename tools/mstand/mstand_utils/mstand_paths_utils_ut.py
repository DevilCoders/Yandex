import datetime

import pytest

import mstand_utils.mstand_paths_utils as mstand_upaths
import yaqutils.time_helpers as utime

from mstand_enums.mstand_online_enums import ServiceSourceEnum, ServiceEnum, TableGroupEnum


class TestDailyPathsGetter:
    def test_regular(self):
        day = datetime.date(2021, 6, 27)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day, source=ServiceSourceEnum.SEARCH)
        expected_paths = [
            "//home/abt/yuid_testids/20210627",
            "//user_sessions/pub/bs_chevent_log/daily/2021-06-27/clean",
            "//user_sessions/pub/search/daily/2021-06-27/clean",
            "//user_sessions/pub/search/daily/2021-06-27/tech",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_market(self):
        day = datetime.date(2021, 4, 7)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day, source=ServiceSourceEnum.MARKET)
        expected_paths = [
            "//home/market-search/yuid-testids/20210407",
            "//user_sessions/pub/market/daily/2021-04-07/clean",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_zen_search(self):
        day = datetime.date(2021, 4, 7)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day, source=ServiceSourceEnum.ZEN_SEARCH)
        expected_paths = [
            "//home/abt/zen/2021-04-07",
            "//user_sessions/pub/search/daily/2021-04-07/clean",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_zen(self):
        day = datetime.date(2021, 4, 7)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day, source=ServiceSourceEnum.ZEN)
        expected_paths = [
            "//logs/zen-events-checked-log/1d/2021-04-07",
            "//home/recommender/zen/sessions_aggregates/background/2021-04-07",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_zen_without_sessions(self):
        day = datetime.date(2021, 4, 7)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.ZEN,
                                                                        no_zen_sessions=True)
        expected_paths = [
            "//logs/zen-events-checked-log/1d/2021-04-07",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_video_with_true_checker(self):
        day = datetime.date(2021, 3, 16)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.VIDEO,
                                                                        path_checker=lambda path: True)
        expected_paths = [
            "//home/abt/yuid_testids/20210316",
            "//user_sessions/pub/video/daily/2021-03-16/clean",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_video_with_false_checker(self):
        day = datetime.date(2021, 3, 16)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.VIDEO,
                                                                        path_checker=lambda path: False)
        expected_paths = [
            "//home/abt/yuid_testids/20210316",
            "//user_sessions/pub/video/daily/2021-03-16/clean",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_video_before_2020_03_04(self):
        day = datetime.date(2020, 3, 1)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.VIDEO,
                                                                        path_checker=lambda path: True)
        expected_paths = [
            "//home/abt/yuid_testids/20200301",
            "//user_sessions/pub/search/daily/2020-03-01/clean",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_images_with_disabled_tech_paths(self):
        day = datetime.date(2021, 3, 16)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.IMAGES,
                                                                        enable_tech=False)
        expected_paths = [
            "//home/abt/yuid_testids/20210316",
            "//user_sessions/pub/images/daily/2021-03-16/clean",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_images_before_2020_03_04(self):
        day = datetime.date(2020, 3, 1)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.IMAGES,
                                                                        path_checker=lambda path: True)
        expected_paths = [
            "//home/abt/yuid_testids/20200301",
            "//user_sessions/pub/search/daily/2020-03-01/clean",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_cache_news(self):
        day = datetime.date(2021, 3, 16)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.CACHE_NEWS)
        expected_paths = [
            "//home/mstand/squeeze/testids/yuid-reqid-testid-filter/0/20210316",
            "//home/ynews/squeeze/testids/news/0/20210316",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_search_with_bro_yuid_tables(self):
        day = datetime.date(2021, 6, 27)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.SEARCH,
                                                                        use_bro_yuids_tables=True)
        expected_paths = [
            "//home/browser-user-sessions/desktop/prod/desktop-yandexuids-abt/2021-06-27",
            "//home/browser-user-sessions/mobile/v2/mobile-yandexuids-abt/2021-06-27",
            "//user_sessions/pub/bs_chevent_log/daily/2021-06-27/clean",
            "//user_sessions/pub/search/daily/2021-06-27/clean",
            "//user_sessions/pub/search/daily/2021-06-27/tech",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_broken_logs_no_services_exception(self):
        day = datetime.date(2016, 4, 1)
        with pytest.raises(Exception, match=r"Can't use broken user sessions"):
            mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                            source=ServiceSourceEnum.SEARCH)

    def test_broken_logs(self):
        day = datetime.date(2016, 4, 1)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.SEARCH,
                                                                        services={ServiceEnum.IMAGES})
        expected_paths = [
            "//home/abt/yuid_testids/20160401",
            "//user_sessions/pub/search/daily/2016-04-01.broken/clean",
            "//user_sessions/pub/search/daily/2016-04-01.broken/tech",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_watchlog_yuids_tables_accumulation(self):
        date_from = datetime.date(2021, 4, 20)
        date_to = datetime.date(2021, 4, 24)
        day = datetime.date(2021, 4, 22)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.WATCHLOG,
                                                                        dates=utime.DateRange(date_from, date_to))
        expected_paths = [
            "//home/abt/yuid_testids/20210420",
            "//home/abt/yuid_testids/20210421",
            "//home/abt/yuid_testids/20210422",
            "//user_sessions/pub/watch_log_tskv/daily/2021-04-22/clean",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_not_watchlog_yuids_tables_accumulation(self):
        date_from = datetime.date(2021, 4, 20)
        date_to = datetime.date(2021, 4, 24)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=date_to,
                                                                        source=ServiceSourceEnum.MARKET_WEB,
                                                                        dates=utime.DateRange(date_from, date_to))
        expected_paths = [
            "//home/abt/yuid_testids/20210424",
            "//user_sessions/pub/market/daily/2021-04-24/clean",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_exp_dates_for_history(self):
        history_range = utime.DateRange(datetime.date(2021, 4, 20), datetime.date(2021, 4, 24))
        day = datetime.date(2021, 4, 27)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.MARKET_WEB,
                                                                        exp_dates_for_history=history_range)

        expected_paths = [
            "//home/abt/yuid_testids/20210420",
            "//home/abt/yuid_testids/20210421",
            "//home/abt/yuid_testids/20210422",
            "//home/abt/yuid_testids/20210423",
            "//home/abt/yuid_testids/20210424",
            "//user_sessions/pub/market/daily/2021-04-27/clean",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_surveys_source_path(self):
        day = datetime.date(2021, 9, 1)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.SURVEYS)
        expected_paths = [
            "//home/mstand/squeeze/surveys_yuid_testids/20210901",
            "//home/goda/prod/surveys/pythia_answers_enrich",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_surveys_future_source_path(self):
        day = datetime.date(2021, 11, 23)
        history_range = utime.DateRange.deserialize("20211119:20211121")
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.SURVEYS,
                                                                        exp_dates_for_history=history_range)
        expected_paths = [
            "//home/mstand/squeeze/surveys_yuid_testids/20211119",
            "//home/mstand/squeeze/surveys_yuid_testids/20211120",
            "//home/mstand/squeeze/surveys_yuid_testids/20211121",
            "//home/goda/prod/surveys/pythia_answers_enrich",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_zen_surveys_source_path(self):
        day = datetime.date(2021, 11, 19)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.ZEN_SURVEYS)
        expected_paths = [
            "//home/mstand/squeeze/surveys_strongest_testids/20211119",
            "//home/recommender/zen/sessions_aggregates/background/2021-11-19",
            "//home/goda/prod/surveys/zen_answers_enrich",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_vh_source_path(self):
        day = datetime.date(2021, 12, 1)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.VH)
        expected_paths = [
            "//home/videoquality/vh_analytics/strm_cube_2/2021-12-01/sessions",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_search_staff_source_path(self):
        day = datetime.date(2021, 12, 10)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.SEARCH_STAFF)
        expected_paths = [
            "//user_sessions/pub/search/daily/2021-12-10/yandex_staff",
        ]
        assert squeeze_paths.tables == expected_paths


class TestSqueezePathsGetter:
    def test_regular(self):
        first_date = datetime.date(2021, 5, 1)
        last_date = datetime.date(2021, 5, 5)
        squeeze_paths = mstand_upaths.get_squeeze_paths(dates=utime.DateRange(first_date, last_date),
                                                        testid="0",
                                                        service=ServiceEnum.IMAGES,
                                                        filter_hash=None)
        expected_paths = [
            "//home/mstand/squeeze/testids/images/0/20210501",
            "//home/mstand/squeeze/testids/images/0/20210502",
            "//home/mstand/squeeze/testids/images/0/20210503",
            "//home/mstand/squeeze/testids/images/0/20210504",
            "//home/mstand/squeeze/testids/images/0/20210505",
        ]
        assert squeeze_paths == expected_paths

    def test_watchlog_squeeze_path(self):
        first_date = datetime.date(2021, 3, 1)
        last_date = datetime.date(2021, 3, 3)
        squeeze_paths = mstand_upaths.get_squeeze_paths(dates=utime.DateRange(first_date, last_date),
                                                        testid="12345",
                                                        service=ServiceEnum.WATCHLOG,
                                                        filter_hash="98765")
        expected_paths = [
            "//home/mstand/squeeze/testids/watchlog/12345.98765:20210301/20210301",
            "//home/mstand/squeeze/testids/watchlog/12345.98765:20210301/20210302",
            "//home/mstand/squeeze/testids/watchlog/12345.98765:20210301/20210303",
        ]
        assert squeeze_paths == expected_paths


class TestHistorySqueezePathsGetter:
    def test_regular(self):
        first_date = datetime.date(2021, 5, 1)
        last_date = datetime.date(2021, 5, 5)
        first_testid_date = datetime.date(2021, 4, 30)
        last_testid_date = datetime.date(2021, 5, 31)
        squeeze_paths = mstand_upaths.get_squeeze_history_paths(
            dates=utime.DateRange(first_date, last_date),
            testid="0",
            service=ServiceEnum.IMAGES,
            filter_hash=None,
            testid_dates=utime.DateRange(first_testid_date, last_testid_date)
        )
        expected_paths = [
            "//home/mstand/squeeze/history/images/0/20210430_20210531/20210501",
            "//home/mstand/squeeze/history/images/0/20210430_20210531/20210502",
            "//home/mstand/squeeze/history/images/0/20210430_20210531/20210503",
            "//home/mstand/squeeze/history/images/0/20210430_20210531/20210504",
            "//home/mstand/squeeze/history/images/0/20210430_20210531/20210505",
        ]
        assert squeeze_paths == expected_paths


# noinspection PyClassHasNoInit
class TestTableGroupPathsGetter:
    STAFF_ONLY = [TableGroupEnum.STAFF]
    ROBOTS_ONLY = [TableGroupEnum.ROBOTS]
    ALL = TableGroupEnum.ALL

    def test_search_all(self):
        day = datetime.date(2021, 8, 10)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.SEARCH,
                                                                        table_groups=self.ALL)
        expected_paths = [
            "//home/abt/yuid_testids/20210810",
            "//user_sessions/pub/bs_chevent_log/daily/2021-08-10/clean",
            "//user_sessions/pub/search/daily/2021-08-10/clean",
            "//user_sessions/pub/search/daily/2021-08-10/tech",
            "//user_sessions/pub/search/daily/2021-08-10/yandex_staff",
            "//user_sessions/pub/search/daily/2021-08-10/robots",
            "//user_sessions/pub/search/daily/2021-08-10/robots.tech",
            "//user_sessions/pub/search/daily/2021-08-10/frauds",
            "//user_sessions/pub/search/daily/2021-08-10/frauds.tech",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_search_all_using_money_event(self):
        day = datetime.date(2021, 12, 10)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.SEARCH,
                                                                        table_groups=self.ALL,
                                                                        path_checker=lambda x: True)
        expected_paths = [
            "//home/abt/yuid_testids/20211210",
            "//statbox/cube/daily/product_money/v1/2021-12-10",
            "//user_sessions/pub/search/daily/2021-12-10/clean",
            "//user_sessions/pub/search/daily/2021-12-10/tech",
            "//user_sessions/pub/search/daily/2021-12-10/yandex_staff",
            "//user_sessions/pub/search/daily/2021-12-10/robots",
            "//user_sessions/pub/search/daily/2021-12-10/robots.tech",
            "//user_sessions/pub/search/daily/2021-12-10/frauds",
            "//user_sessions/pub/search/daily/2021-12-10/frauds.tech",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_search_staff_only(self):
        day = datetime.date(2021, 12, 10)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.SEARCH,
                                                                        table_groups=self.STAFF_ONLY)
        expected_paths = [
            "//home/abt/yuid_testids/20211210",
            "//user_sessions/pub/search/daily/2021-12-10/yandex_staff",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_search_robots_only(self):
        day = datetime.date(2021, 12, 10)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.SEARCH,
                                                                        table_groups=self.ROBOTS_ONLY)
        expected_paths = [
            "//home/abt/yuid_testids/20211210",
            "//user_sessions/pub/search/daily/2021-12-10/robots",
            "//user_sessions/pub/search/daily/2021-12-10/robots.tech",
            "//user_sessions/pub/search/daily/2021-12-10/frauds",
            "//user_sessions/pub/search/daily/2021-12-10/frauds.tech",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_news_all(self):
        day = datetime.date(2021, 12, 10)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.NEWS,
                                                                        table_groups=self.ALL)
        expected_paths = [
            "//home/abt/yuid_testids/20211210",
            "//user_sessions/pub/search/daily/2021-12-10/clean",
            "//user_sessions/pub/search/daily/2021-12-10/yandex_staff",
            "//user_sessions/pub/search/daily/2021-12-10/robots",
            "//user_sessions/pub/search/daily/2021-12-10/frauds",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_news_staff_only(self):
        day = datetime.date(2021, 12, 10)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.NEWS,
                                                                        table_groups=self.STAFF_ONLY)
        expected_paths = [
            "//home/abt/yuid_testids/20211210",
            "//user_sessions/pub/search/daily/2021-12-10/yandex_staff",
        ]
        assert squeeze_paths.tables == expected_paths

    def test_news_robots_only(self):
        day = datetime.date(2021, 12, 10)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(day=day,
                                                                        source=ServiceSourceEnum.NEWS,
                                                                        table_groups=self.ROBOTS_ONLY)
        expected_paths = [
            "//home/abt/yuid_testids/20211210",
            "//user_sessions/pub/search/daily/2021-12-10/robots",
            "//user_sessions/pub/search/daily/2021-12-10/frauds",
        ]
        assert squeeze_paths.tables == expected_paths
