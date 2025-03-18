import datetime

import pytest

import adminka.ab_cache
import adminka.pool_fetcher
import yaqutils.time_helpers as utime
from adminka.ab_cache import AdminkaCachedApi
from adminka.filter_pool import PoolFilter


# noinspection PyClassHasNoInit
class TestPoolFetcher:
    def test_date_correctness(self, session):
        fetcher = adminka.pool_fetcher.PoolFetcher(
            observation_ids=["1", "2"],
            dates=None,
            include_start_date=False,
            include_end_date=False,
            tag=None,
            extra_data=None,
            pool_filter=PoolFilter(queue_id=1, remove_filtered=False, adminka_session=session),
            adminka_session=session,
        )
        pool = fetcher.fetch_pool()
        obs1 = pool.observations[0]
        obs2 = pool.observations[1]

        assert obs1.dates == utime.DateRange(
            start=datetime.date(2016, 1, 6),
            end=datetime.date(2016, 1, 6)
        )

        assert obs2.dates == utime.DateRange(
            start=datetime.date(2016, 1, 6),
            end=datetime.date(2016, 1, 9)
        )

        fetcher2 = adminka.pool_fetcher.PoolFetcher(
            observation_ids=["1", "2"],
            dates=utime.DateRange(
                start=datetime.date(2015, 12, 31),
                end=datetime.date(2016, 3, 1)
            ),
            include_start_date=False,
            include_end_date=False,
            tag=None,
            extra_data=False,
            pool_filter=PoolFilter(queue_id=1, remove_filtered=False, adminka_session=session),
            adminka_session=session,
        )
        pool2 = fetcher2.fetch_pool()
        assert pool.observations == pool2.observations

    def test_wrong_dates(self, session):
        with pytest.raises(Exception):
            fetcher = adminka.pool_fetcher.PoolFetcher(
                observation_ids=["1", "2"],
                dates=utime.DateRange(
                    start=datetime.date(2015, 1, 1),
                    end=datetime.date(2016, 1, 1),
                ),
                include_start_date=False,
                include_end_date=False,
                tag=None,
                extra_data=False,
                pool_filter=PoolFilter(queue_id=1, remove_filtered=False, adminka_session=session),
                adminka_session=session,
            )
            fetcher.fetch_pool()

    def test_extra_data(self, session):
        fetcher = adminka.pool_fetcher.PoolFetcher(
            observation_ids=["1"],
            dates=None,
            include_start_date=False,
            include_end_date=False,
            tag=None,
            extra_data=True,
            pool_filter=PoolFilter(queue_id=1, remove_filtered=False, adminka_session=session),
            adminka_session=session,
        )
        pool = fetcher.fetch_pool()

        assert pool.observations[0].extra_data
        assert pool.observations[0].extra_data["adminka"]["ticket"] == "TEST-0001"

    def test_sbs_info(self, session):
        fetcher = adminka.pool_fetcher.PoolFetcher(
            observation_ids=["5", "6"],
            dates=None,
            include_start_date=False,
            include_end_date=False,
            tag=None,
            extra_data=True,
            pool_filter=PoolFilter(queue_id=1, remove_filtered=False, adminka_session=session),
            adminka_session=session,
        )
        pool = fetcher.fetch_pool()

        assert len(pool.observations) == 2
        obs1 = pool.observations[0]
        obs2 = pool.observations[1]
        assert obs1.extra_data
        assert obs1.extra_data["adminka"]["ticket"] == "TEST-0004"
        assert obs1.sbs_ticket == "SIDEBYSIDE-111"
        assert obs1.control.testid == "2"
        assert obs1.control.sbs_system_id == "1"
        assert obs1.experiments[0].testid == "3"
        assert obs1.experiments[0].sbs_system_id == "2"

        assert obs2.extra_data
        assert obs2.extra_data["adminka"]["ticket"] == "TEST-0005"
        assert obs2.sbs_ticket == "SIDEBYSIDE-222"
        assert obs2.control.testid == "2"
        assert obs2.control.sbs_system_id == "8"
        assert obs2.experiments[0].testid == "3"
        assert obs2.experiments[0].sbs_system_id == "5"

    @pytest.mark.parametrize("str_dates, queue_id, answer", [
        ("20150101:20170101", None, {"1", "2", "5", "6", "7", "8", "11"}),
        ("20160101:20160201", None, {"1", "2", "5", "6", "7", "8", "11"}),
        ("20160102:20160201", None, {"11"}),
        ("20160110:20170101", None, set()),
        ("20160101:20160205", 1, {"11"}),
    ])
    def test_dates_correctness(self, session, monkeypatch, str_dates, queue_id, answer):
        def get_observation_list(this, dates, tag=None):
            return sorted(this._observation_cache.values(), key=lambda x: -int(x["obs_id"]))

        monkeypatch.setattr(AdminkaCachedApi, "get_observation_list", get_observation_list)

        fetcher = adminka.pool_fetcher.PoolFetcher(
            observation_ids=None,
            dates=utime.DateRange.deserialize(str_dates),
            include_start_date=False,
            include_end_date=False,
            tag=None,
            extra_data=None,
            pool_filter=PoolFilter(queue_id=queue_id, adminka_session=session),
            adminka_session=session,
        )
        pool = fetcher.fetch_pool()
        assert {o.id for o in pool.observations} == answer
