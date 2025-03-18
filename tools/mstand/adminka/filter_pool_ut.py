# coding=utf-8
import datetime
import os.path
import pytest

from adminka.ab_cache import AdminkaCachedApi
import adminka.filter_pool as fp
import experiment_pool.observation_ut as obs_ut
import yaqutils.time_helpers as utime
from adminka.filter_pool import PoolFilter
from experiment_pool import Pool


@pytest.fixture
def folder_path(data_path, folder):
    return os.path.join(data_path, folder)


@pytest.fixture
def session_from_path(folder_path):
    return AdminkaCachedApi(path=os.path.join(folder_path, "cache.json"))


# noinspection PyClassHasNoInit
class TestPoolFilter:
    def test_accept_no_id(self):
        no_id = obs_ut.generate_observation(obs_id=None)
        assert PoolFilter(remove_filtered=True).is_accepted(no_id)
        assert not PoolFilter(aspect="qqqq", queue_id=12345, dim_id=67890, min_duration=1000).is_accepted(no_id)

    def test_accept_no_id_duration(self):
        dates = utime.DateRange(datetime.date(2017, 1, 1), datetime.date(2017, 1, 10))
        obs = obs_ut.generate_observation(obs_id=None, dates=dates)

        assert PoolFilter(min_duration=10).is_accepted(obs)
        assert not PoolFilter(min_duration=11).is_accepted(obs)
        assert PoolFilter(max_duration=10).is_accepted(obs)
        assert not PoolFilter(max_duration=9).is_accepted(obs)

        assert PoolFilter(min_duration=5, max_duration=15).is_accepted(obs)
        assert not PoolFilter(min_duration=11, max_duration=15).is_accepted(obs)
        assert not PoolFilter(min_duration=5, max_duration=9).is_accepted(obs)

    def test_accept_all(self):
        ob = obs_ut.generate_observation(
            obs_id="1",
            control_id="1",
            experiment_ids=("2", "3", "4", "5")
        )
        assert PoolFilter().is_accepted(ob)

    def test_aspect(self, session):
        obs1 = obs_ut.generate_observation("1")
        obs2 = obs_ut.generate_observation("2")
        obs3 = obs_ut.generate_observation("3")

        assert PoolFilter(aspect="test", adminka_session=session).is_accepted(obs1)
        assert not PoolFilter(aspect="test", adminka_session=session).is_accepted(obs2)
        assert not PoolFilter(aspect="not test", adminka_session=session).is_accepted(obs1)
        assert not PoolFilter(aspect="not test", adminka_session=session).is_accepted(obs2)
        assert not PoolFilter(aspect="test", adminka_session=session).is_accepted(obs3)

    def test_queue_id(self, session):
        exp1_2 = obs_ut.generate_observation(
            control_id="1",
            experiment_ids=("2",)
        )

        exp1_3 = obs_ut.generate_observation(
            control_id="1",
            experiment_ids=("3",)
        )

        exp2_4 = obs_ut.generate_observation(
            control_id="2",
            experiment_ids=("4",)
        )

        assert not PoolFilter(queue_id=1, adminka_session=session).is_accepted(exp1_2)
        assert not PoolFilter(queue_id=2, adminka_session=session).is_accepted(exp1_2)
        assert PoolFilter(queue_id=1, adminka_session=session).is_accepted(exp1_3)
        assert PoolFilter(queue_id=2, adminka_session=session).is_accepted(exp2_4)
        assert not PoolFilter(queue_id=1000, adminka_session=session).is_accepted(exp1_2)
        assert not PoolFilter(queue_id=1000, adminka_session=session).is_accepted(exp1_3)
        assert not PoolFilter(queue_id=1000, adminka_session=session).is_accepted(exp2_4)

    def test_dim_id(self, session):
        dates = utime.DateRange(
            datetime.date(2016, 1, 1),
            datetime.date(2016, 4, 1)
        )
        exp1_2 = obs_ut.generate_observation(
            dates=dates,
            control_id="1",
            experiment_ids=("2",)
        )

        exp1_3 = obs_ut.generate_observation(
            dates=dates,
            control_id="1",
            experiment_ids=("3",)
        )

        assert PoolFilter(dim_id=1, adminka_session=session).is_accepted(exp1_2)
        assert PoolFilter(dim_id=2, adminka_session=session).is_accepted(exp1_2)
        assert PoolFilter(dim_id=1, adminka_session=session).is_accepted(exp1_3)
        assert not PoolFilter(dim_id=2, adminka_session=session).is_accepted(exp1_3)
        assert not PoolFilter(dim_id=3, adminka_session=session).is_accepted(exp1_2)
        assert not PoolFilter(dim_id=3, adminka_session=session).is_accepted(exp1_2)

    def test_service(self, session):
        obs1 = obs_ut.generate_observation(obs_id="1")
        obs2 = obs_ut.generate_observation(obs_id="2")
        obs3 = obs_ut.generate_observation(obs_id="3")

        assert PoolFilter(service="test", adminka_session=session).is_accepted(obs1)
        assert PoolFilter(service="test2", adminka_session=session).is_accepted(obs2)
        assert not PoolFilter(service="test2", adminka_session=session).is_accepted(obs1)
        assert not PoolFilter(service="test", adminka_session=session).is_accepted(obs3)

    def test_platform(self, session):
        exp1_2 = obs_ut.generate_observation(control_id="1", experiment_ids=("2",))
        exp1_3 = obs_ut.generate_observation(control_id="1", experiment_ids=("3",))

        assert PoolFilter(platform="test", adminka_session=session).is_accepted(exp1_2)
        assert PoolFilter(platform="test2", adminka_session=session).is_accepted(exp1_2)
        assert PoolFilter(platform="test", adminka_session=session).is_accepted(exp1_3)
        assert not PoolFilter(platform="test2", adminka_session=session).is_accepted(exp1_3)

    @pytest.mark.parametrize("folder", ["web_auto_uitype_filters"])
    def test_uitype_filters(self, session_from_path):
        test_dates = utime.DateRange(
            datetime.date(2018, 2, 5),
            datetime.date(2018, 2, 15),
        )
        exp = obs_ut.generate_observation(obs_id="83208", dates=test_dates,
                                          control_id="66383", experiment_ids=("66384", "66385"))
        pool_filter = PoolFilter(platform="desktop", adminka_session=session_from_path)

        assert not pool_filter.check_platform(exp)

    def test_region(self, session):
        exp1_2 = obs_ut.generate_observation(control_id="1", experiment_ids=("2",))
        exp1_3 = obs_ut.generate_observation(control_id="1", experiment_ids=("3",))

        assert PoolFilter(regions=[1], adminka_session=session).is_accepted(exp1_2)
        assert PoolFilter(regions=[1, 2, 3], adminka_session=session).is_accepted(exp1_2)
        assert PoolFilter(regions=[1, 2, 99], adminka_session=session).is_accepted(exp1_2)
        assert not PoolFilter(regions=[99], adminka_session=session).is_accepted(exp1_2)
        assert PoolFilter(regions=[4, 5, 6], adminka_session=session).is_accepted(exp1_2)
        assert PoolFilter(regions=[1, 2, 3, 4, 5, 6], adminka_session=session).is_accepted(exp1_2)
        assert PoolFilter(regions=[1, 2, 3, 4, 5, 6, 1, 2, 3, 4, 5, 6], adminka_session=session).is_accepted(
            exp1_2)
        assert not PoolFilter(regions=[4, 5, 6], adminka_session=session).is_accepted(exp1_3)

    def test_min_duration(self):
        ob = obs_ut.generate_observation(
            dates=utime.DateRange(
                datetime.date(2016, 1, 1),
                datetime.date(2016, 1, 2),
            )
        )

        assert PoolFilter(min_duration=1).is_accepted(ob)
        assert PoolFilter(min_duration=0).is_accepted(ob)
        assert PoolFilter(min_duration=-3).is_accepted(ob)
        assert not PoolFilter(min_duration=3000).is_accepted(ob)

    def test_max_duration(self):
        ob = obs_ut.generate_observation(
            dates=utime.DateRange(
                datetime.date(2016, 1, 1),
                datetime.date(2016, 1, 10),
            )
        )

        assert PoolFilter(max_duration=10).is_accepted(ob)
        assert PoolFilter(max_duration=9999).is_accepted(ob)
        assert not PoolFilter(max_duration=-5).is_accepted(ob)
        assert not PoolFilter(max_duration=0).is_accepted(ob)
        assert not PoolFilter(max_duration=9).is_accepted(ob)

    def test_tags(self, session):
        obs1 = obs_ut.generate_observation("1", tags=["tag", "another_tag"])

        assert PoolFilter(tag_whitelist={"tag"}, adminka_session=session).is_accepted(obs1)
        assert PoolFilter(tag_whitelist={"tag", "another_tag"}, adminka_session=session).is_accepted(obs1)
        assert PoolFilter(tag_whitelist={"another_tag"}, adminka_session=session).is_accepted(obs1)
        assert not PoolFilter(tag_whitelist={"tag", "some_other_tag"}, adminka_session=session).is_accepted(
            obs1)

        assert not PoolFilter(tag_blacklist={"tag"}, adminka_session=session).is_accepted(obs1)
        assert not PoolFilter(tag_blacklist={"another_tag"}, adminka_session=session).is_accepted(obs1)
        assert not PoolFilter(tag_blacklist={"tag", "another_tag"}, adminka_session=session).is_accepted(obs1)
        assert not PoolFilter(tag_blacklist={"tag", "some_other_tag"}, adminka_session=session).is_accepted(
            obs1)

    def test_obs_white(self, session):
        pfilter = PoolFilter(obs_whitelist={"1", "2"}, adminka_session=session)
        obs1 = obs_ut.generate_observation("1")
        assert pfilter.is_accepted(obs1)
        obs3 = obs_ut.generate_observation("3")
        assert not pfilter.is_accepted(obs3)
        obs_none = obs_ut.generate_observation(obs_id=None)
        assert not pfilter.is_accepted(obs_none)

    def test_obs_black(self, session):
        pfilter = PoolFilter(obs_blacklist={"1", "2"}, adminka_session=session)
        obs1 = obs_ut.generate_observation("1")
        assert not pfilter.is_accepted(obs1)
        obs3 = obs_ut.generate_observation("3")
        assert pfilter.is_accepted(obs3)
        obs_none = obs_ut.generate_observation(obs_id=None)
        assert pfilter.is_accepted(obs_none)

    def test_obs_both(self, session):
        pfilter = PoolFilter(obs_whitelist={"1", "2"}, obs_blacklist={"2", "3"}, adminka_session=session)
        obs1 = obs_ut.generate_observation("1")
        assert pfilter.is_accepted(obs1)
        obs2 = obs_ut.generate_observation("2")
        assert not pfilter.is_accepted(obs2)
        obs3 = obs_ut.generate_observation("3")
        assert not pfilter.is_accepted(obs3)
        obs4 = obs_ut.generate_observation("4")
        assert not pfilter.is_accepted(obs4)
        obs_none = obs_ut.generate_observation(obs_id=None)
        assert not pfilter.is_accepted(obs_none)

    def test_remove_endless(self):
        ob = obs_ut.generate_observation(
            dates=utime.DateRange(None, None)
        )
        assert PoolFilter(remove_endless=False).is_accepted(ob)
        assert not PoolFilter(remove_endless=True).is_accepted(ob)

    def test_filtered(self, session):
        ob1 = obs_ut.generate_observation(obs_id="1")
        ob2 = obs_ut.generate_observation(obs_id="2")

        assert PoolFilter(remove_filtered=True, adminka_session=session).is_accepted(ob1)
        assert not PoolFilter(remove_filtered=True, adminka_session=session).is_accepted(ob2)

    def test_filter_observations(self, session):
        ob1 = obs_ut.generate_observation(obs_id="1")
        ob2 = obs_ut.generate_observation(obs_id="2")

        pool = Pool([ob1, ob2])
        assert PoolFilter(remove_filtered=True, adminka_session=session).filter(pool).observations == [ob1]

    def test_preload_testids(self, monkeypatch, session):
        def run_filter(**kwargs):
            PoolFilter(adminka_session=session, **kwargs).filter(Pool())

        run_filter()
        run_filter(aspect="test")
        run_filter(dim_id=1)
        run_filter(min_duration=1)
        run_filter(remove_filtered=True)
        assert not hasattr(session, "testids_preloaded")
        run_filter(queue_id=1)
        assert hasattr(session, "testids_preloaded")

    def test_from_args(self):
        class MockArgs(object):
            def __init__(self):
                self.aspect = "aspect"
                self.queue_id = 1
                self.dim_id = 2
                self.min_duration = 3
                self.max_duration = 10
                self.remove_filtered = True
                self.remove_endless = True
                self.remove_salt_changes = False
                self.only_regular_salt_changes = True
                self.service = "test"
                self.platform = "test"
                self.regions = [1, 2, 3]
                self.tags = ["good_tag", "another_good_tag"]
                self.remove_tags = ["bad_tag"]
                self.observation_ids = ["good_id", "another_good_id"]
                self.remove_observation_ids = ["bad_id"]
                self.only_main_observations = False
                self.ab_token_file = "~/.ab/token"

        args = MockArgs()
        pf = PoolFilter.from_cli_args(args)
        assert args.aspect == pf.aspect
        assert args.queue_id == pf.queue_id
        assert args.dim_id == pf.dim_id
        assert args.min_duration == pf.min_duration
        assert args.remove_filtered == pf.remove_filtered
        assert args.remove_endless == pf.remove_endless
        assert args.remove_salt_changes == pf.remove_salt_changes
        assert args.only_regular_salt_changes == pf.only_regular_salt_changes
        assert args.service == pf.service
        assert args.platform == pf.platform
        assert set(args.regions) == pf.regions
        assert set(args.tags) == pf.tag_whitelist
        assert set(args.remove_tags) == pf.tag_blacklist
        assert set(args.observation_ids) == pf.obs_whitelist
        assert set(args.remove_observation_ids) == pf.obs_blacklist

    def test_trim_one(self):
        dates = utime.DateRange(datetime.date(2016, 10, 1), datetime.date(2016, 12, 23))
        ob = obs_ut.generate_observation(dates=dates)
        fp.trim_days_single(ob, day_from=4)
        assert ob.dates == utime.DateRange(
            datetime.date(2016, 10, 4),
            datetime.date(2016, 12, 23)
        )
        fp.trim_days_single(ob, day_from=2, day_to=5)
        assert ob.dates == utime.DateRange(
            datetime.date(2016, 10, 5),
            datetime.date(2016, 10, 8)
        )
        fp.trim_days_single(ob, day_to=10)
        assert ob.dates == utime.DateRange(
            datetime.date(2016, 10, 5),
            datetime.date(2016, 10, 8)
        )
        fp.trim_days_single(ob, day_from=3)
        assert ob.dates == utime.DateRange(
            datetime.date(2016, 10, 7),
            datetime.date(2016, 10, 8)
        )

    def test_trim(self):
        observations = [
            obs_ut.generate_observation(
                obs_id="1",
                dates=utime.DateRange(
                    datetime.date(2016, 1, 1),
                    datetime.date(2016, 1, 30)
                )
            ),
            obs_ut.generate_observation(
                obs_id="2",
                dates=utime.DateRange(
                    datetime.date(2016, 1, 1),
                    datetime.date(2016, 1, 4),
                )
            ),
            obs_ut.generate_observation(
                obs_id="3",
                dates=utime.DateRange(
                    datetime.date(2016, 1, 1),
                    datetime.date(2016, 1, 5),
                )
            ),
        ]

        trimmed_observations = [
            obs_ut.generate_observation(
                obs_id="1",
                dates=utime.DateRange(
                    datetime.date(2016, 1, 3),
                    datetime.date(2016, 1, 5)
                )
            ),
            obs_ut.generate_observation(
                obs_id="3",
                dates=utime.DateRange(
                    datetime.date(2016, 1, 3),
                    datetime.date(2016, 1, 5),
                )
            ),
        ]

        fp.trim_days(observations, 3, 5, False)
        assert observations == trimmed_observations

    def test_trim_truncated(self):
        observations = [
            obs_ut.generate_observation(
                obs_id="1",
                dates=utime.DateRange(
                    datetime.date(2016, 1, 1),
                    datetime.date(2016, 1, 30)
                )
            ),
            obs_ut.generate_observation(
                obs_id="2",
                dates=utime.DateRange(
                    datetime.date(2016, 1, 1),
                    datetime.date(2016, 1, 4),
                )
            ),
            obs_ut.generate_observation(
                obs_id="3",
                dates=utime.DateRange(
                    datetime.date(2016, 1, 1),
                    datetime.date(2016, 1, 5),
                )
            ),
        ]

        trimmed_observations = [
            obs_ut.generate_observation(
                obs_id="1",
                dates=utime.DateRange(
                    datetime.date(2016, 1, 3),
                    datetime.date(2016, 1, 5)
                )
            ),
            obs_ut.generate_observation(
                obs_id="2",
                dates=utime.DateRange(
                    datetime.date(2016, 1, 3),
                    datetime.date(2016, 1, 4),
                )
            ),
            obs_ut.generate_observation(
                obs_id="3",
                dates=utime.DateRange(
                    datetime.date(2016, 1, 3),
                    datetime.date(2016, 1, 5),
                )
            ),
        ]

        fp.trim_days(observations, 3, 5, True)
        assert observations == trimmed_observations

    def test_overlap(self):
        observations = [
            obs_ut.generate_observation(
                obs_id="1",
                dates=utime.DateRange(
                    datetime.date(2020, 1, 1),
                    datetime.date(2020, 1, 7)
                )
            ),
            obs_ut.generate_observation(
                obs_id="2",
                dates=utime.DateRange(
                    datetime.date(2020, 1, 1),
                    datetime.date(2020, 1, 12),
                )
            ),
            obs_ut.generate_observation(
                obs_id="3",
                dates=utime.DateRange(
                    datetime.date(2020, 1, 1),
                    datetime.date(2020, 1, 6),
                )
            ),
        ]

        result_observations = [
            obs_ut.generate_observation(
                obs_id="1",
                dates=utime.DateRange(
                    datetime.date(2020, 1, 1),
                    datetime.date(2020, 1, 7)
                )
            ),
            obs_ut.generate_observation(
                obs_id="2",
                dates=utime.DateRange(
                    datetime.date(2020, 1, 1),
                    datetime.date(2020, 1, 7),
                )
            ),
            obs_ut.generate_observation(
                obs_id="2",
                dates=utime.DateRange(
                    datetime.date(2020, 1, 3),
                    datetime.date(2020, 1, 9),
                )
            ),
            obs_ut.generate_observation(
                obs_id="2",
                dates=utime.DateRange(
                    datetime.date(2020, 1, 5),
                    datetime.date(2020, 1, 11),
                )
            ),
        ]

        fp.split_overlap(observations, 7, 2)
        assert observations == result_observations
