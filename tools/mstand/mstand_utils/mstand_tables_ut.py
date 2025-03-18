import datetime
import os

import pytest

import mstand_utils.mstand_tables as mstand_tables
import yaqutils.time_helpers as utime

from mstand_enums.mstand_online_enums import ServiceSourceEnum, ServiceEnum, TableTypeEnum, TableGroupEnum


# noinspection PyClassHasNoInit
class TestUserSessionsPaths:
    ROBOTS_ONLY = [TableGroupEnum.ROBOTS]

    def test_with_slash(self):
        day = datetime.date(2016, 6, 27)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub/",
                                                source=ServiceSourceEnum.SEARCH)
        path = table.path
        assert path == "//user_sessions/pub/search/daily/2016-06-27/clean"

    def test_without_slash(self):
        day = datetime.date(2016, 6, 27)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.SEARCH)
        path = table.path
        assert path == "//user_sessions/pub/search/daily/2016-06-27/clean"

    def test_old_video_logs(self):
        day = datetime.date(2016, 6, 27)
        table = mstand_tables.UserSessionsTable(day=day,
                                                source=ServiceSourceEnum.VIDEO)
        path = table.path
        assert path == "//user_sessions/pub/search/daily/2016-06-27/clean"

    def test_old_images_logs(self):
        day = datetime.date(2016, 6, 27)
        table = mstand_tables.UserSessionsTable(day=day,
                                                source=ServiceSourceEnum.IMAGES)
        path = table.path
        assert path == "//user_sessions/pub/search/daily/2016-06-27/clean"

    def test_staff(self):
        day = datetime.date(2016, 6, 27)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.SEARCH,
                                                table_name="yandex_staff",
                                                table_groups=[TableGroupEnum.STAFF])
        path = table.path
        assert path == "//user_sessions/pub/search/daily/2016-06-27/yandex_staff"

    def test_robots(self):
        day = datetime.date(2016, 6, 27)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.SEARCH,
                                                table_name="robots",
                                                table_groups=[TableGroupEnum.ROBOTS])
        path = table.path
        assert path == "//user_sessions/pub/search/daily/2016-06-27/robots"

    def test_watchlog(self):
        day = datetime.date(2016, 6, 27)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source="watch_log_tskv")
        path = table.path
        assert path == "//user_sessions/pub/watch_log_tskv/daily/2016-06-27/clean"

    def test_watchlog_old(self):
        day = datetime.date(2015, 5, 10)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.WATCHLOG)
        path = table.path
        assert path == "//user_sessions/pub/watch_log/daily/2015-05-10/clean"

    def test_broken(self):
        day = datetime.date(2016, 4, 3)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.SEARCH)
        with pytest.raises(Exception):
            table.path

    def test_broken_morda(self):
        day = datetime.date(2016, 4, 3)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.SEARCH,
                                                services={"morda", "video"})
        with pytest.raises(Exception):
            table.path

    def test_broken_images(self):
        day = datetime.date(2016, 4, 3)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.SEARCH,
                                                services={"images"})
        path = table.path
        assert path == "//user_sessions/pub/search/daily/2016-04-03.broken/clean"

    def test_bs_chevent_log(self):
        day = datetime.date(2021, 4, 3)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source="bs_chevent_log")
        path = table.path
        assert path == "//user_sessions/pub/bs_chevent_log/daily/2021-04-03/clean"

    def test_bs_chevent_log_absence(self):
        day = datetime.date(2020, 4, 3)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source="bs_chevent_log")
        path = table.path
        assert path is None

    def test_money_event(self):
        day = datetime.date(2021, 10, 1)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source="bs_chevent_log",
                                                yt_path_checker=lambda x: True)
        assert table.path == "//statbox/cube/daily/product_money/v1/2021-10-01"

    def test_disabled_tech_path(self):
        day = datetime.date(2020, 4, 3)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.IMAGES,
                                                table_name="tech",
                                                enable_tech=False)
        path = table.path
        assert path is None

        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.SEARCH,
                                                table_name="robots.tech",
                                                enable_tech=False,
                                                table_groups=self.ROBOTS_ONLY)
        path = table.path
        assert path is None

    def test_tech_path_false_checker(self):
        day = datetime.date(2020, 4, 3)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.IMAGES,
                                                table_name="tech",
                                                enable_tech=True,
                                                yt_path_checker=lambda path: False)
        path = table.path
        assert path is None

    def test_tech_path_true_checker(self):
        day = datetime.date(2020, 4, 3)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.IMAGES,
                                                table_name="tech",
                                                enable_tech=True,
                                                yt_path_checker=lambda path: True)
        path = table.path
        assert path == "//user_sessions/pub/images/daily/2020-04-03/tech"

    def test_tech_path_without_checker(self):
        day = datetime.date(2020, 4, 3)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.IMAGES,
                                                table_name="tech",
                                                enable_tech=True)
        path = table.path
        assert path == "//user_sessions/pub/images/daily/2020-04-03/tech"

        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.SEARCH,
                                                table_name="robots.tech",
                                                enable_tech=True,
                                                table_groups=self.ROBOTS_ONLY)
        path = table.path
        assert path == "//user_sessions/pub/search/daily/2020-04-03/robots.tech"

    def test_yuids_zen_sources(self):
        day = datetime.date(2020, 4, 3)
        for source in ServiceSourceEnum.YUIDS_ZEN:
            table = mstand_tables.UserSessionsTable(day=day,
                                                    user_sessions="//user_sessions/pub",
                                                    source=source)
            path = table.path
            corrected_source = source.lstrip("zen-")
            assert path == os.path.join("//user_sessions/pub/", corrected_source, "daily/2020-04-03/clean")


# noinspection PyClassHasNoInit
class TestTableGroup:
    STAFF_ONLY = [TableGroupEnum.STAFF]
    ROBOTS_ONLY = [TableGroupEnum.ROBOTS]
    ALL = TableGroupEnum.ALL

    def test_search_all(self):
        day = datetime.date(2021, 12, 10)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.SEARCH,
                                                table_groups=self.ALL)
        assert table.path == "//user_sessions/pub/search/daily/2021-12-10/clean"

        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.SEARCH,
                                                table_name="yandex_staff",
                                                table_groups=self.ALL)
        assert table.path == "//user_sessions/pub/search/daily/2021-12-10/yandex_staff"

    def test_search_staff_only(self):
        day = datetime.date(2021, 12, 10)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.SEARCH,
                                                table_groups=self.STAFF_ONLY)
        assert table.path is None

        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.SEARCH,
                                                table_name="yandex_staff",
                                                table_groups=self.STAFF_ONLY)
        assert table.path == "//user_sessions/pub/search/daily/2021-12-10/yandex_staff"

    def test_search_robots_only(self):
        day = datetime.date(2021, 12, 10)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.SEARCH,
                                                table_groups=self.ROBOTS_ONLY)
        assert table.path is None

        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.SEARCH,
                                                table_name="robots",
                                                table_groups=self.ROBOTS_ONLY)
        assert table.path == "//user_sessions/pub/search/daily/2021-12-10/robots"

        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.SEARCH,
                                                table_name="frauds",
                                                table_groups=self.ROBOTS_ONLY)
        assert table.path == "//user_sessions/pub/search/daily/2021-12-10/frauds"

    def test_news_all(self):
        day = datetime.date(2021, 12, 10)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.NEWS,
                                                table_groups=self.ALL)
        assert table.path == "//user_sessions/pub/news/daily/2021-12-10/clean"

        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.NEWS,
                                                table_name="yandex_staff",
                                                table_groups=self.ALL)
        assert table.path == "//user_sessions/pub/news/daily/2021-12-10/yandex_staff"

    def test_news_staff_only(self):
        day = datetime.date(2021, 12, 10)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.NEWS,
                                                table_groups=self.STAFF_ONLY)
        assert table.path is None

        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.NEWS,
                                                table_name="yandex_staff",
                                                table_groups=self.STAFF_ONLY)
        assert table.path == "//user_sessions/pub/news/daily/2021-12-10/yandex_staff"

    def test_news_robots_only(self):
        day = datetime.date(2021, 12, 10)
        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.NEWS,
                                                table_groups=self.ROBOTS_ONLY)
        assert table.path is None

        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.NEWS,
                                                table_name="robots",
                                                table_groups=self.ROBOTS_ONLY)
        assert table.path == "//user_sessions/pub/news/daily/2021-12-10/robots"

        table = mstand_tables.UserSessionsTable(day=day,
                                                user_sessions="//user_sessions/pub",
                                                source=ServiceSourceEnum.NEWS,
                                                table_name="frauds",
                                                table_groups=self.ROBOTS_ONLY)
        assert table.path == "//user_sessions/pub/news/daily/2021-12-10/frauds"

    def test_bs_chevent_log_all(self):
        table = mstand_tables.UserSessionsTable(day=datetime.date(2021, 9, 1),
                                                user_sessions="//user_sessions/pub",
                                                source="bs_chevent_log",
                                                table_groups=self.ALL)
        assert table.path == "//user_sessions/pub/bs_chevent_log/daily/2021-09-01/clean"

        table = mstand_tables.UserSessionsTable(day=datetime.date(2021, 10, 1),
                                                user_sessions="//user_sessions/pub",
                                                source="bs_chevent_log",
                                                yt_path_checker=lambda x: True,
                                                table_groups=self.ALL)
        assert table.path == "//statbox/cube/daily/product_money/v1/2021-10-01"

        table = mstand_tables.UserSessionsTable(day=datetime.date(2021, 10, 1),
                                                user_sessions="//user_sessions/pub",
                                                source="bs_chevent_log",
                                                table_groups=self.ALL)
        assert table.path == "//statbox/cube/daily/event_money_direct/v2/1d/2021-10-01"

    def test_bs_chevent_log_staff_only(self):
        table = mstand_tables.UserSessionsTable(day=datetime.date(2021, 9, 1),
                                                user_sessions="//user_sessions/pub",
                                                source="bs_chevent_log",
                                                table_name="yandex_staff",
                                                table_groups=self.STAFF_ONLY)
        assert table.path == "//user_sessions/pub/bs_chevent_log/daily/2021-09-01/yandex_staff"

        table = mstand_tables.UserSessionsTable(day=datetime.date(2021, 10, 1),
                                                user_sessions="//user_sessions/pub",
                                                source="bs_chevent_log",
                                                table_groups=self.STAFF_ONLY)
        assert table.path is None


# noinspection PyClassHasNoInit
class TestCachePaths:
    def test_cache_dir_getter(self):
        for source in ServiceSourceEnum.CACHE_SOURCES:
            dir_path = mstand_tables.get_cache_dir(source=source)
            service = ServiceEnum.CACHE_SERVICE_BY_SOURCE[source]
            account = "ynews" if service == ServiceEnum.NEWS else "mstand"
            assert dir_path == "//home/" + account + "/squeeze/testids/" + service + "/0"

    def test_cache_tables_paths(self):
        for source in ServiceSourceEnum.CACHE_SOURCES:
            day = datetime.date(2021, 6, 27)
            table = mstand_tables.CacheTable(source=source, day=day)
            path = table.path

            service = ServiceEnum.CACHE_SERVICE_BY_SOURCE[source]
            account = "ynews" if service == ServiceEnum.NEWS else "mstand"
            assert path == "//home/" + account + "/squeeze/testids/" + service + "/0/20210627"

    def test_cache_unsupported_service(self):
        day = datetime.date(2021, 6, 27)
        with pytest.raises(Exception, match=r"Unsupported service special:yql-ab"):
            mstand_tables.CacheTable(source=ServiceSourceEnum.YQL_AB, day=day)


# noinspection PyClassHasNoInit
class TestSqueezeDaily:
    def test_regular(self):
        day = datetime.date(2016, 6, 27)
        dates = utime.DateRange.deserialize("20160627:20160627")
        table = mstand_tables.MstandDailyTable(day=day,
                                               squeeze_path="//home/mstand/squeeze/",
                                               service="web",
                                               testid="1234",
                                               dates=dates,
                                               filter_hash=None)
        path = table.path
        assert path == "//home/mstand/squeeze/testids/web/1234/20160627"

    def test_filters(self):
        day = datetime.date(2016, 6, 27)
        dates = utime.DateRange.deserialize("20160627:20160627")
        table = mstand_tables.MstandDailyTable(day=day,
                                               squeeze_path="//home/mstand/squeeze/",
                                               service="web",
                                               testid="1234",
                                               dates=dates,
                                               filter_hash="filters")
        path = table.path
        assert path == "//home/mstand/squeeze/testids/web/1234.filters/20160627"

    def test_accumulate_yuids(self):
        day = datetime.date(2020, 12, 25)
        dates = utime.DateRange.deserialize("20201221:20201231")
        table = mstand_tables.MstandDailyTable(day=day,
                                               squeeze_path="//home/mstand/squeeze/",
                                               service="watchlog",
                                               testid="1234",
                                               dates=dates,
                                               filter_hash="filters")
        path = table.path
        assert path == "//home/mstand/squeeze/testids/watchlog/1234.filters:20201221/20201225"


# noinspection PyClassHasNoInit
class TestSqueezeHistory:
    def test_regular(self):
        day = datetime.date(2016, 6, 27)
        testid_dates = utime.DateRange(datetime.date(2016, 6, 28), datetime.date(2016, 6, 29))
        table = mstand_tables.MstandHistoryTable(day=day,
                                                 squeeze_path="//home/mstand/squeeze/",
                                                 service="web",
                                                 testid="1234",
                                                 dates=testid_dates,
                                                 filter_hash=None)
        path = table.path
        assert path == "//home/mstand/squeeze/history/web/1234/20160628_20160629/20160627"

    def test_filters(self):
        day = datetime.date(2016, 6, 27)
        testid_dates = utime.DateRange(datetime.date(2016, 6, 28), datetime.date(2016, 6, 29))
        table = mstand_tables.MstandHistoryTable(day=day,
                                                 squeeze_path="//home/mstand/squeeze/",
                                                 service="web",
                                                 testid="1234",
                                                 dates=testid_dates,
                                                 filter_hash="filters")
        path = table.path
        assert path == "//home/mstand/squeeze/history/web/1234.filters/20160628_20160629/20160627"
        assert table.type == TableTypeEnum.SQUEEZE


# noinspection PyClassHasNoInit
class TestYuidsPaths:
    def test_regular(self):
        day = datetime.date(2016, 6, 27)
        table = mstand_tables.MstandYuidsTable(day=day, yuids_dir="//home/mstand/yuid_testids/")
        path = table.path
        assert path == "//home/mstand/yuid_testids/20160627"
        assert table.type == TableTypeEnum.YUID

    def test_skip_bad_day(self):
        day = datetime.date(2022, 1, 27)
        table = mstand_tables.MstandYuidsTable(day=day, yuids_dir="//home/mstand/yuid_testids/")
        assert table.path is None
        assert table.type == TableTypeEnum.YUID


# noinspection PyClassHasNoInit
class TestVideoSessionsTable:
    def test_regular(self):
        day = datetime.date(2021, 9, 1)
        table = mstand_tables.VideoSessionsTable(day=day)

        assert table.path == "//home/videoquality/vh_analytics/strm_cube_2/2021-09-01/sessions"
        assert table.type == TableTypeEnum.SOURCE


# noinspection PyClassHasNoInit
class TestZenPaths:
    def test_regular_zen_path(self):
        day = datetime.date(2021, 5, 27)
        table = mstand_tables.ZenTable(day=day)
        path = table.path
        assert path == "//logs/zen-events-checked-log/1d/2021-05-27"

    def test_old_zen_logs_path(self):
        day = datetime.date(2020, 8, 27)
        table = mstand_tables.ZenTable(day=day)
        path = table.path
        assert path == "//home/recommender/logs/zen-stats-log/1d/2020-08-27"

    def test_custom_zen_logs(self):
        day = datetime.date(2020, 8, 27)
        table = mstand_tables.ZenTable(day=day, zen_path="//home/zen_logs_sweet_home")
        path = table.path
        assert path == "//home/zen_logs_sweet_home/2020-08-27"

    def test_forbidden_zen_sessions(self):
        day = datetime.date(2020, 8, 27)
        table = mstand_tables.ZenSessionsTable(day=day, no_zen_sessions=True)
        path = table.path
        assert path is None

    def test_regular_zen_sessions(self):
        day = datetime.date(2020, 8, 27)
        table = mstand_tables.ZenSessionsTable(day=day)
        path = table.path
        assert path == "//home/recommender/zen/sessions_aggregates/background/2020-08-27"


class TestSpecialTables:
    def test_alice_table(self):
        day = datetime.date(2020, 8, 27)
        table = mstand_tables.AliceTable(day=day)
        path = table.path
        assert path == "//home/alice/dialog/prepared_logs_expboxes/2020-08-27"

    def test_voice_heart_beats_table(self):
        day = datetime.date(2020, 8, 27)
        table = mstand_tables.VoiceHeartbeatsTable(day=day)
        path = table.path
        assert path == "//home/voice/logs/heartbeats/2020-08-27"

    def test_old_voice_heart_beats_table(self):
        day = datetime.date(2018, 8, 27)
        table = mstand_tables.VoiceHeartbeatsTable(day=day)
        path = table.path
        assert path is None
