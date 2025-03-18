# coding=utf-8
import logging
import time

import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime

import adminka.date_validation as adm_dv
import adminka.fetch_observations as adm_fetch_obs
import mstand_utils.args_helpers as mstand_uargs
from adminka.ab_cache import AdminkaCachedApi
from adminka.filter_pool import PoolFilter
from experiment_pool import Pool


def fill_serpset_ids(session, pool):
    """
    :type session: AdminkaCachedApi
    :type pool: Pool
    :rtype:
    """

    logging.info("Filling serpset ids in pool")
    session.preload_serpset_ids(pool.all_testids())

    for obs in pool.observations:
        for exp in obs.all_experiments():
            exp.serpset_id = session.get_testid_serpset(exp.testid)

    logging.info("Serpset ids filling done")


class PoolFetcher(object):
    def __init__(
            self,
            observation_ids,
            dates,
            include_start_date,
            include_end_date,
            tag,
            extra_data,
            pool_filter,
            adminka_session=None,
    ):
        self.observation_ids = observation_ids

        self.dates = dates
        self.tag = tag

        self.include_start_date = include_start_date
        self.include_end_date = include_end_date

        self.extra_data = extra_data
        self.session = adminka_session or AdminkaCachedApi()
        self.pool_filter = pool_filter or PoolFilter(adminka_session=self.session)

    def need_date_validation(self):
        return self.dates or not self.include_start_date or not self.include_end_date

    @staticmethod
    def from_cli_args(cli_args, adminka_session=None):
        pool_filter = PoolFilter.from_cli_args(cli_args, adminka_session=adminka_session)
        return PoolFetcher(
            observation_ids=cli_args.observations,
            dates=utime.DateRange.from_cli_args(cli_args),
            include_start_date=cli_args.include_start_date,
            include_end_date=cli_args.include_end_date,
            tag=cli_args.tag,
            extra_data=cli_args.extra_data,
            pool_filter=pool_filter,
            adminka_session=pool_filter.session,
        )

    @staticmethod
    def add_cli_args(parser):
        mstand_uargs.add_dates(parser, required=False)
        mstand_uargs.add_observation_ids(parser, help_message="observation IDs (will disable filtration)")
        parser.add_argument(
            "--include-start-date",
            action="store_true",
            help="include observations from start date",
        )
        parser.add_argument(
            "--include-end-date",
            action="store_true",
            help="include observations from end date",
        )
        parser.add_argument(
            "--tag",
            help="only include observations with this tag"
        )
        parser.add_argument(
            "--extra-data",
            help="get observations' metadata from AB",
            action="store_true"
        )
        PoolFilter.add_cli_args(parser)

    def fetch_pool(self):
        """
        :rtype: experiment_pool.pool.Pool
        """
        time_start = time.time()
        observations = adm_fetch_obs.get_observations(
            self.session,
            self.observation_ids,
            self.dates,
            self.tag,
            self.extra_data,
        )
        observations = list(observations)
        pool = Pool(observations)
        logging.info("Found %d observations", len(pool.observations))
        time_finish_get = time.time()

        if not self.observation_ids:
            logging.info("Run pool filtration")
            pool = self.pool_filter.filter(pool)
            strict_date_validation = False
        else:
            logging.info("Not filtering explicitly requested observation IDs")
            strict_date_validation = True

        time_finish_filter = time.time()

        if self.need_date_validation():
            pool = adm_dv.fix_pool_dates(
                self.session,
                pool,
                self.include_start_date,
                self.include_end_date,
                self.dates,
                strict=strict_date_validation
            )
        time_finish_validation = time.time()

        fill_serpset_ids(self.session, pool)

        time_finish = time.time()
        timings = (
            ("whole process", umisc.calc_elapsed(time_start, time_finish)),
            ("get observations", umisc.calc_elapsed(time_start, time_finish_get)),
            ("filter observations", umisc.calc_elapsed(time_finish_get, time_finish_filter)),
            ("validate observations", umisc.calc_elapsed(time_finish_filter, time_finish_validation)),
            ("get serpset ids", umisc.calc_elapsed(time_finish_validation, time_finish)),
        )
        logging.info("Timings:%s", umisc.to_lines("{:22s}: {}".format(name, duration) for name, duration in timings))

        self.session.log_stats()
        return pool


def fetch_pool_by_observation_id(
        observation_id,
        adminka_session=None,
        extra_data=False,
        validate_testids=True,
):
    """
    :param observation_id: int | None
    :param adminka_session: object
    :type extra_data: bool
    :type validate_testids: bool
    :return:
    """
    if adminka_session is None:
        adminka_session = AdminkaCachedApi()

    observation = adm_fetch_obs.get_observation_by_id(
        adminka_session,
        observation_id,
        extra_data=extra_data,
        validate_testids=validate_testids,
    )

    pool = Pool([observation])

    return pool
