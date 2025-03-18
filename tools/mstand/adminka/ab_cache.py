import datetime
import functools
import logging
import multiprocessing.dummy as mpdummy
import sys
import traceback

import adminka.ab_observation
import adminka.ab_task
import adminka.activity
import adminka.testid
import yaqutils.json_helpers as ujson
import yaqutils.time_helpers as utime
from yaqab.ab_client import AbClient

AB_CONCURRENT_QUERIES = 8
OBSERVATIONS_LIMIT = 1000


def worker_fetch_testid_data(arg):
    """
    :type arg: tuple[callable, str]
    :return:
    """
    adminka_api_function, testid = arg
    try:
        return adminka_api_function(testid)
    except Exception as exc:
        logging.error("adminka api call(testid=%s) failed: %s\n%s", testid, exc, traceback.format_exc())
        raise


def parallel_fetch_data_by_testid_list(testids, adminka_api_function, description):
    """
    :type testids: list[str]
    :type adminka_api_function: callable
    :param description: str
    :return:
    """
    logging.info("fetching %s for %d testids", description, len(testids))

    thread_pool = mpdummy.Pool(AB_CONCURRENT_QUERIES)
    args = [(adminka_api_function, testid) for testid in testids]
    result_list = thread_pool.map(worker_fetch_testid_data, args)

    logging.info("%s fetching done", description)
    return dict(zip(testids, result_list))


class AdminkaCachedApi(object):
    _cache_fields = (
        "_activity_cache",
        "_task_cache",
        "_testid_cache",
        "_serpset_id_cache",
        "_observation_cache",
        "_query_basket_tags_cache",
    )

    def __init__(self, path=None, auth_token=None):
        """
        :type path: str | None
        :type auth_token: str | None
        """
        self.client = AbClient(auth_token=auth_token)

        self._activity_cache = {}
        self._task_cache = {}
        self._testid_cache = {}
        self._serpset_id_cache = {}
        self._observation_cache = {}
        self._query_basket_tags_cache = {}
        self._split_change_cache = {}

        self.allow_requests = path is None
        if path is not None:
            self._init_from_file(path)

    def get_task_info_for_observation(self, observation_id):
        """
        :type observation_id: str
        :rtype: dict[str] | None
        """
        observation_info = self.get_observation_info(observation_id)
        ticket = observation_info.get("ticket")
        return self.get_task_info_for_ticket(ticket)

    def get_task_info_for_ticket(self, ticket):
        """
        :type ticket: str
        :rtype: dict[str] | None
        """
        if not ticket:
            return None
        if self.allow_requests and ticket not in self._task_cache:
            self._task_cache[ticket] = adminka.ab_task.get_info(self.client, ticket)
        return self._task_cache[ticket]

    def get_testid_activity(self, testid):
        """
        :type testid: str
        :rtype: list[dict[str]]
        """
        if self.allow_requests and testid not in self._activity_cache:
            self._activity_cache[testid] = self.client.get_testid_activity(testid)
        return self._activity_cache[testid]

    def get_testid_serpset(self, testid):
        """
        :type testid: str
        :rtype: str
        """
        if self.allow_requests and testid not in self._serpset_id_cache:
            self._serpset_id_cache[testid] = adminka.testid.get_testid_serpset(
                self.client, testid, self._query_basket_tags_cache
            )
        return self._serpset_id_cache[testid]

    def preload_testids(self, testids):
        """
        :type testids: list[str]
        """
        if self.allow_requests:
            logging.info("Preloading testids")
            self._testid_cache.update(adminka.testid.get_testid_info_dict(self.client, testids))
            logging.info("Testids preloading done")

    def preload_activity(self, testids):
        """
        :type testids: list[str]
        """
        if self.allow_requests:
            get_testid_activity = functools.partial(adminka.activity.get_testid_activity, self.client)
            self._activity_cache.update(
                parallel_fetch_data_by_testid_list(testids, get_testid_activity, "activity")
            )

    def preload_serpset_ids(self, testids):
        """
        :type testids: list[str]
        """
        if self.allow_requests:
            for testid in testids:
                self._serpset_id_cache[testid] = adminka.testid.get_testid_serpset(
                    self.client, testid, self._query_basket_tags_cache
                )

    def get_testid_info(self, testid):
        """
        :type testid: str
        :rtype: dict[str]
        """
        if testid is None:
            # MSTAND-726
            raise Exception("Testid is empty in get_testid_info (ABT API misuse).")

        if self.allow_requests and testid not in self._testid_cache:
            logging.info("get_testid_info for testid %s", testid)
            data = list(adminka.testid.get_testids_info_batched(self.client, [testid]))
            self._testid_cache[testid] = data[0] if data else {}
        return self._testid_cache[testid]

    def get_observation_info(self, observation_id):
        """
        :type observation_id: str
        :rtype: dict[str]
        """
        if self.allow_requests and observation_id not in self._observation_cache:
            # logging.debug("Observation %s not in cache, requesting.", observation_id)
            self._observation_cache[observation_id] = self.client.get_observation(str(observation_id))
        return self._observation_cache[observation_id]

    def get_observation_list(self, dates, tag=None):
        """
        :type dates: utime.DateRange
        :type tag: str
        :rtype: list[dict[str]]
        """
        if not self.allow_requests:
            return []

        logging.info("getting observation list %s", dates)

        control_obs_ids = {obs["obs_id"] for obs in self.client.get_observations(dates, tag=tag, strict_dates=True)}
        logging.info("got %d observations", len(control_obs_ids))

        def worker_fetch_observations_data_range(skip):
            logging.info("Getting from %d to %d", skip + 1, skip + OBSERVATIONS_LIMIT)
            return self.client.get_observations(dates, full=True, tag=tag, skip=skip, limit=OBSERVATIONS_LIMIT,
                                                strict_dates=True)

        def worker_fetch_observation_data_by_id(obs_id):
            logging.info("Getting observation data for obs_id %d", obs_id)
            return self.client.get_observation(obs_id)

        observations = []
        thread_pool = mpdummy.Pool(AB_CONCURRENT_QUERIES)

        args = [skip for skip in range(0, len(control_obs_ids), OBSERVATIONS_LIMIT)]
        for obs in thread_pool.imap(worker_fetch_observations_data_range, args):
            observations.extend(obs)
            logging.info("Got %d observations, total: %d", len(obs), len(observations))

        got_obs_ids = {obs["obs_id"] for obs in observations}
        reload_obs_ids = control_obs_ids - got_obs_ids
        logging.info("Need to reload %d observations", len(reload_obs_ids))
        for obs in thread_pool.imap(worker_fetch_observation_data_by_id, reload_obs_ids):
            observations.append(obs)
            logging.info("Got observation data, total: %d", len(observations))

        for observation in observations:
            self._observation_cache[observation["obs_id"]] = observation

        observations.sort(key=lambda x: -x["obs_id"])
        return observations

    def get_split_change_info(self, testids, dates):
        """
        :type testids: list[str]
        :type dates: utime.DateRange
        :rtype: dict
        """
        if not dates.start or not self.allow_requests:
            return {}
        if not dates.end:
            dates = utime.DateRange(dates.start, datetime.date.today())
        # TODO can reuse a lot more here
        testids = tuple(sorted(set(testids)))
        if (testids, dates) not in self._split_change_cache:
            self._split_change_cache[testids, dates] = adminka.testid.get_split_change_info(self.client, testids, dates)
        return self._split_change_cache[testids, dates]

    def log_stats(self):
        logging.debug(
            "Adminka cache size: %s",
            sys.getsizeof(self),
        )
        logging.debug(
            'Adminka cache entries: activity=%s, task=%s, testid=%s, serpset=%s, observation=%s',
            len(self._activity_cache),
            len(self._task_cache),
            len(self._testid_cache),
            len(self._serpset_id_cache),
            len(self._observation_cache),
        )

    def dump_cache(self, path):
        data = {
            name: getattr(self, name)
            for name in self._cache_fields
            if getattr(self, name)
        }
        ujson.dump_to_file(data, path)

    def _init_from_file(self, path):
        data = ujson.load_from_file(path)
        for name in self._cache_fields:
            setattr(self, name, data.get(name, {}))
