# -*- coding: utf-8 -*-
import argparse
import collections
import datetime
import functools
import logging
import os
import time
import traceback

import experiment_pool  # noqa
import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.log_helpers as mstand_ulog
import mstand_utils.mstand_paths_utils as mstand_upaths
import mstand_utils.mstand_tables as mstand_tables
import mstand_utils.testid_helpers as utestid
import session_squeezer.services as squeezer_services
import yaqutils.args_helpers as uargs
import yaqutils.json_helpers as ujson
import yaqutils.misc_helpers as umisc
import yaqutils.nirvana_helpers as unirv
import yaqutils.six_helpers as usix
import yaqutils.time_helpers as utime

from experiment_pool import Pool, Experiment, Observation
from mstand_enums.mstand_online_enums import (
    RectypeEnum, ServiceSourceEnum, ServiceEnum, SqueezeResultEnum, TableGroupEnum
)
from mstand_structs import SqueezeVersions
from session_squeezer.experiment_for_squeeze import ExperimentForSqueeze
from session_squeezer.squeezer import UserSessionsSqueezer


class SqueezeRunner(object):
    def __init__(
            self,
            paths_params,
            squeeze_backend,
            no_zen_sessions=False,
            services=None,
            history=None,
            future=None,
            use_processes=False,
            replace=False,
            replace_old=False,
            threads=1,
            allow_errors=False,
            use_filters=False,
            min_versions=None,
            enable_tech=True,
            enable_cache=False,
            enable_binary=False,
            table_groups=None,
    ):
        """
        :type paths_params: PathsParams
        :type squeeze_backend: callable
        :type services: list[str] | set[str] | None
        :type history: int | None
        :type future: int | None
        :type use_processes: bool
        :type replace: bool
        :type replace_old: bool
        :type threads: int
        :type allow_errors: bool
        :type min_versions: SqueezeVersions
        :type enable_tech: bool
        :type enable_cache: bool
        :type enable_binary: bool
        :type table_groups: list[str]
        """
        self.paths_params = paths_params
        self.no_zen_sessions = no_zen_sessions
        self.enable_tech = enable_tech
        self.enable_cache = enable_cache
        self.enable_binary = enable_binary

        if not self.enable_binary and any(s in ServiceEnum.SQUEEZE_BIN_SUPPORTED for s in ServiceEnum.extend_services(services)):
            assert paths_params.squeeze_path != mstand_tables.DEFAULT_SQUEEZE_PATH, (
                "It is forbidden to write squeezes into {} using the nile squeezer. "
                "Switch --enable-binary option on or set a different squeeze path".format(mstand_tables.DEFAULT_SQUEEZE_PATH)
            )

        self.squeeze_backend = squeeze_backend
        assert callable(self.squeeze_backend.squeeze_one_day)
        assert callable(self.squeeze_backend.get_existing_paths)
        assert callable(self.squeeze_backend.prepare_dirs)
        assert callable(self.squeeze_backend.get_cache_checker)
        assert callable(self.squeeze_backend.write_log)

        market_sources_found = False
        other_sources_found = False
        services_without_aliases = ServiceEnum.convert_aliases(services)
        squeezer_services.assert_services(services_without_aliases)
        for service in ServiceEnum.extend_services(services_without_aliases):
            source = ServiceEnum.SOURCES[service]
            if source in ServiceSourceEnum.YUIDS_STRICT:
                assert self.paths_params.yuids_path
            if source in ServiceSourceEnum.MARKET:
                market_sources_found = True
            else:
                other_sources_found = True

        self.history = history
        if self.history:
            assert self.history > 0
        self.future = future
        if self.future:
            assert self.future > 0

        if self.history or self.future:
            if other_sources_found:
                assert self.paths_params.yuids_path, "cannot use history/future without yuid_testids tables"
            if market_sources_found:
                assert self.paths_params.yuids_market_path, "cannot use history/future without yuid_testids tables"

        self.versions = squeezer_services.get_squeezer_versions(services_without_aliases)

        self.use_processes = use_processes
        self.replace = replace
        self.replace_old = replace_old
        self.threads = threads
        self.allow_errors = allow_errors
        self.use_filters = use_filters
        self.min_versions = min_versions if min_versions else SqueezeVersions()
        self.cache = ServiceEnum.check_cache_services(services)

        self._check_squeezer_versions(self.min_versions)

        if self.replace_old and not self.min_versions.is_empty():
            raise Exception("Both replace_old and min_versions specified in SqueezeRunner")

        self.table_groups = table_groups
        if self.table_groups != [TableGroupEnum.CLEAN]:
            if paths_params.squeeze_path == mstand_tables.DEFAULT_SQUEEZE_PATH:
                raise Exception("It is allowed to write squeezes into {} for the CLEAN group only".format(mstand_tables.DEFAULT_SQUEEZE_PATH))
            if self.enable_cache:
                raise Exception("Cache is supported for the CLEAN group only")

        self.squeeze_results = {}

    @staticmethod
    def from_cli_args(cli_args, backend, use_processes=False):
        min_versions_json = ujson.load_from_str(cli_args.min_versions.strip()) if cli_args.min_versions else {}
        min_versions = SqueezeVersions.deserialize(min_versions_json)
        paths_params = mstand_upaths.PathsParams.from_cli_args(cli_args=cli_args)

        return SqueezeRunner(
            paths_params=paths_params,
            no_zen_sessions=cli_args.no_zen_sessions,
            services=cli_args.services,
            history=cli_args.history,
            future=cli_args.future,
            replace=cli_args.replace,
            replace_old=cli_args.replace_old,
            threads=cli_args.threads,
            allow_errors=cli_args.allow_errors,
            use_processes=use_processes,
            squeeze_backend=backend,
            use_filters=getattr(cli_args, "use_filters", False),
            min_versions=min_versions,
            enable_tech=cli_args.enable_tech,
            enable_cache=cli_args.enable_cache,
            enable_binary=cli_args.enable_binary,
            table_groups=cli_args.table_groups,
        )

    @staticmethod
    def add_cli_args(
            parser: argparse.ArgumentParser,
            default_sessions: str = mstand_tables.DEFAULT_USER_SESSIONS_PATH,
            default_squeeze: str = mstand_tables.DEFAULT_SQUEEZE_PATH,
            default_yuid: str = mstand_tables.DEFAULT_YUIDS_PATH,
            default_yuid_market: str = mstand_tables.DEFAULT_YUID_MARKET_PATH,
            default_zen: str = mstand_tables.DEFAULT_ZEN_PATH,
            default_zen_sessions: str = mstand_tables.DEFAULT_ZEN_SESSIONS_PATH,
    ):
        uargs.add_boolean_argument(
            parser,
            "--replace",
            help_message="replace if table exists",
        )
        uargs.add_boolean_argument(
            parser,
            "--replace-old",
            help_message="replace if table has older version",
        )
        mstand_upaths.PathsParams.add_cli_args(parser=parser,
                                               default_sessions=default_sessions,
                                               default_squeeze=default_squeeze,
                                               default_yuid=default_yuid,
                                               default_yuid_market=default_yuid_market,
                                               default_zen=default_zen,
                                               default_zen_sessions=default_zen_sessions)
        mstand_uargs.add_no_zen_sessions_flag(parser)
        mstand_uargs.add_list_of_online_services(parser, possible=ServiceEnum.get_all_services())
        mstand_uargs.add_history(parser)
        uargs.add_threads(parser, default=3)
        uargs.add_boolean_argument(
            parser,
            "--allow-errors",
            help_message="skip errors and remove such experiments from pool",
        )
        mstand_uargs.add_lower_reducer_key(parser)
        mstand_uargs.add_upper_reducer_key(parser)
        mstand_uargs.add_start_mapper_index(parser)
        mstand_uargs.add_end_mapper_index(parser)
        mstand_uargs.add_min_versions(parser)
        mstand_uargs.add_ignore_triggered_testids_filter(parser)
        mstand_uargs.add_list_of_table_groups(parser)
        uargs.add_boolean_argument(
            parser,
            name="enable-tech",
            help_message="enable using of tech tables",
            inverted_name="disable-tech",
            inverted_help_message="disable using of tech tables",
            default=True,
        )
        uargs.add_boolean_argument(
            parser,
            name="enable-cache",
            help_message="enable using of cache tables",
            inverted_name="disable-cache",
            inverted_help_message="disable using of cache tables",
            default=False,
        )
        uargs.add_boolean_argument(
            parser,
            name="enable-binary",
            help_message="enable using of the binary squeezer",
            inverted_name="disable-binary",
            inverted_help_message="disable using of the binary squeezer",
            default=True,
        )

    @staticmethod
    def observation_from_cli_args(cli_args, session):
        # avoid requests-related modules import in YT
        import adminka.filter_fetcher as adm_flt_fetch
        import adminka.pool_validation as adm_pool_validation

        date_range = utime.DateRange.from_cli_args(cli_args)
        experiments = [Experiment(testid=t) for t in cli_args.testids]
        assert experiments
        observation = Observation(
            obs_id=cli_args.observation_id,
            dates=date_range,
            control=experiments[0],
            experiments=experiments[1:],
        )
        adm_pool_validation.init_observation_services(
            observation=observation,
            session=session,
            cli_services=cli_args.services,
            use_filters=cli_args.use_filters,
        ).crash_on_error()
        if cli_args.use_filters:
            adm_flt_fetch.fetch_obs(observation, session,
                                    allow_bad_filters=squeezer_services.check_allow_any_filters(cli_args.services),
                                    ignore_triggered_testids_filter=cli_args.ignore_triggered_testids_filter)
        return observation

    def squeeze_observation(self, observation):
        """
        :type observation: experiment_pool.Observation
        """
        experiments = ExperimentForSqueeze.from_observation(observation)
        if self.allow_errors:
            errors = []
        else:
            errors = None
        self._squeeze_experiments(experiments, errors)

    def squeeze_pool(self, pool):
        """
        :type pool: experiment_pool.Pool
        :rtype: experiment_pool.Pool
        """
        experiments = ExperimentForSqueeze.from_pool(pool)
        if self.allow_errors:
            errors = []
        else:
            errors = None
        self._squeeze_experiments(experiments, errors)
        return pool_without_bad_experiments(pool, errors, self.history, self.future)

    def squeeze_one_day(self, task):
        """
        :type task: SqueezeTask
        :rtype: str
        """
        day = task.day
        experiments = task.experiments

        destinations, _ = self._find_destinations_to_squeeze(
            day,
            experiments,
            task.exp_dates_for_history,
        )
        if not destinations:
            logging.info(
                "Skip %d testids for %s: %s",
                len(experiments),
                utime.format_date(day),
                umisc.to_lines(sorted(experiments)),
            )
            return SqueezeResultEnum.SKIPPED

        use_bro_yuids_tables = any(utestid.testid_is_bro(exp.testid) for exp in experiments)
        squeeze_paths = mstand_upaths.get_daily_squeeze_paths_by_source(
            source=task.source,
            day=task.day,
            paths_params=self.paths_params,
            services={exp.service for exp in experiments},
            use_bro_yuids_tables=use_bro_yuids_tables,
            no_zen_sessions=self.no_zen_sessions,
            enable_tech=self.enable_tech,
            path_checker=self._check_path,
            exp_dates_for_history=task.exp_dates_for_history,
            dates=task.dates,
            table_groups=self.table_groups,
        )

        yuids_strict = task.source in ServiceSourceEnum.YUIDS_STRICT or use_bro_yuids_tables or task.exp_dates_for_history

        self._check_tables(paths=squeeze_paths, yuids_strict=yuids_strict)

        self.squeeze_backend.prepare_dirs(get_unique_dirs(usix.itervalues(destinations)))

        time_start = time.time()

        with self.squeeze_backend.lock(destinations=destinations):
            time_lock_end = time.time()
            logging.info(
                "Got locks on experiments for %s in %s",
                utime.format_date(day),
                datetime.timedelta(seconds=time_lock_end - time_start),
            )

            destinations_after_lock, ready_tables = self._find_destinations_to_squeeze(
                day,
                experiments,
                task.exp_dates_for_history,
            )

            if destinations_after_lock:
                exps_to_squeeze = set(destinations_after_lock)
                squeezer = UserSessionsSqueezer(
                    exps_to_squeeze,
                    day=day,
                    cache_filters=self.squeeze_backend.get_filters(day) if self.cache else None,
                    enable_cache=task.enable_cache,
                )

                logging.info(
                    "Will squeeze %d experiments for %s %s",
                    len(exps_to_squeeze),
                    utime.format_date(day),
                    umisc.to_lines(sorted(exps_to_squeeze)),
                )

                operation_sid = mstand_ulog.get_operation_sid()
                self.squeeze_backend.write_log(
                    rectype=RectypeEnum.OPERATION_START,
                    data=dict(
                        cache_tables=self.squeeze_backend.get_cache_tables(ready_tables),
                        day=utime.format_date(day),
                        experiments=[str(exp) for exp in exps_to_squeeze],
                        input_tables=squeeze_paths.tables,
                        operation_sid=operation_sid,
                    ),
                )

                self.squeeze_backend.squeeze_one_day(
                    destinations=destinations_after_lock,
                    squeezer=squeezer,
                    paths=squeeze_paths,
                    versions=self.versions,
                    task=task,
                    operation_sid=operation_sid,
                    enable_binary=self.enable_binary,
                )

                time_end = time.time()
                logging.info(
                    "Squeezed %d experiments for %s %s in %s",
                    len(exps_to_squeeze),
                    utime.format_date(day),
                    exps_to_squeeze,
                    datetime.timedelta(seconds=time_end - time_lock_end),
                )

                self.squeeze_backend.write_log(
                    rectype=RectypeEnum.OPERATION_FINISH,
                    data=dict(
                        operation_sid=operation_sid,
                    ),
                )

                return SqueezeResultEnum.SQUEEZED

            else:
                logging.info(
                    "Skip %d testids for %s because squeeze was collect during locks: %s",
                    len(experiments),
                    utime.format_date(day),
                    umisc.to_lines(sorted(experiments)),
                )
                return SqueezeResultEnum.SKIPPED_AFTER_LOCK

    def _find_destinations_to_squeeze(self, day, experiments, history):
        """
        :type day: datetime.date
        :type experiments: set[ExperimentForSqueeze]
        :type history: utils.time_helpers.DateRange | None
        :rtype: tuple[dict[ExperimentForSqueeze, str], set[str]]
        """
        destinations = self._find_destinations(day, experiments, history)
        logging.info("Destinations for squeeze:")
        for efs in sorted(destinations):
            path = destinations[efs]
            logging.info("--> %s: %s", efs, path)
        ready_tables = self._del_ready_destinations_from_dict(day, destinations)
        return destinations, ready_tables

    def _del_ready_destinations_from_dict(self, day, destinations):
        """
        :type day: datetime.date
        :type destinations: dict[ExperimentForSqueeze, str]
        :rtype: set[str]
        """
        ready_tables = self._get_ready_tables(day, destinations)
        if not ready_tables:
            return {}

        existing_exps = {exp for exp, dst in usix.iteritems(destinations) if dst in ready_tables}
        for exp in existing_exps:
            del destinations[exp]

        return ready_tables

    def _get_ready_tables(self, day, destinations):
        """
        :type day: datetime.date
        :type destinations: dict[ExperimentForSqueeze, str]
        :rtype: set[str]
        """
        existing_tables = set(self.squeeze_backend.get_existing_paths(destinations.values()))
        if not existing_tables:
            return set()

        if self.replace:
            logging.info(
                "Will replace %d existing tables for %s: %s",
                len(existing_tables),
                utime.format_date(day),
                umisc.to_lines(sorted(existing_tables)),
            )
            return set()

        check_min_versions = not self.min_versions.is_empty()

        if not self.replace_old and not check_min_versions:
            if existing_tables:
                logging.info(
                    "Skip %d existing tables for %s: %s",
                    len(existing_tables),
                    utime.format_date(day),
                    umisc.to_lines(sorted(existing_tables)),
                )
            return existing_tables

        existing_versions = self.squeeze_backend.get_all_versions(existing_tables)

        inverse_destinations = {table: exp for exp, table in usix.iteritems(destinations)}
        assert len(destinations) == len(inverse_destinations)

        current_tables = set()
        old_tables = set()
        older_than_min_versions = set()

        for table, versions in usix.iteritems(existing_versions):
            exp = inverse_destinations[table]
            with_history = exp.all_for_history
            with_filters = bool(exp.observation.filters)
            current_version = self.versions.clone(with_history=with_history, with_filters=with_filters)
            if check_min_versions:
                if versions.is_older_than(self.min_versions):
                    older_than_min_versions.add(table)
                else:
                    current_tables.add(table)
            elif versions.is_older_than(current_version):
                logging.info("Table %s is old: %s", table, versions)
                old_tables.add(table)
            else:
                current_tables.add(table)

        if old_tables:
            logging.info(
                "Will replace %d old tables (current version is %s) for %s: %s",
                len(old_tables),
                self.versions,
                utime.format_date(day),
                umisc.to_lines(sorted(old_tables)),
            )
        if older_than_min_versions:
            logging.info(
                "Will replace {} tables that do not satisfy minimal required squeeze versions: {}".format(
                    len(older_than_min_versions),
                    umisc.to_lines(older_than_min_versions),
                )
            )
        if current_tables:
            logging.info(
                "Skip %d existing tables for %s: %s",
                len(current_tables),
                utime.format_date(day),
                umisc.to_lines(sorted(current_tables)),
            )

        assert existing_tables == (current_tables | old_tables | older_than_min_versions)
        return current_tables

    def _find_destinations(self, day, experiments, history):
        """
        :type day: datetime.date
        :type experiments: set[ExperimentForSqueeze]
        :type history: utils.time_helpers.DateRange | None
        :rtype: dict[ExperimentForSqueeze, str]
        """
        date_str = utime.format_date(day)
        table_dirs = {}
        for exp in experiments:
            table_dirs[exp] = mstand_tables.mstand_experiment_dir(
                squeeze_path=self.paths_params.squeeze_path,
                service=exp.service,
                testid=exp.testid,
                dates=exp.dates,
                filter_hash=exp.filters.filter_hash if squeezer_services.has_filter_support(exp.service) else None,
                history_dates=history,
            )
        return {exp: os.path.join(table_dir, date_str)
                for exp, table_dir in usix.iteritems(table_dirs)}

    def _check_tables(self, paths, yuids_strict):
        """
        :type paths: SqueezePaths
        :type yuids_strict: bool
        """
        bad = []
        existing = list(self.squeeze_backend.get_existing_paths(paths.sources))
        if paths.sources != existing:
            missed = set(paths.sources) - set(existing)
            bad.extend(missed)

        if paths.yuids:
            yuids_existing = list(self.squeeze_backend.get_existing_paths(paths.yuids))
            if paths.yuids != yuids_existing:
                if yuids_strict:
                    yuids_missed = set(paths.yuids) - set(yuids_existing)
                    bad.extend(yuids_missed)
                else:
                    paths.yuids[:] = yuids_existing

        if bad:
            raise Exception("Tables are missing: {}".format(umisc.to_lines(sorted(bad))))

    def _check_path(self, path):
        return next(self.squeeze_backend.get_existing_paths([path]), None) is not None

    @staticmethod
    def _print_parser_lib_versions():
        try:
            # noinspection PyUnresolvedReferences,PyPackageRequirements
            import libra
            logging.warning("libra version (libra revision): %s", libra.Version())
        except ImportError:
            logging.warning("libra.so is missing")

        try:
            # noinspection PyUnresolvedReferences,PyPackageRequirements
            import scarab.meta as scarab_meta
            logging.warning("scarab version: %s", scarab_meta.__version__)
        except ImportError:
            logging.warning("scarab library is missing")

    def _squeeze_experiments(self, experiments, errors=None):
        """
        :type experiments: list[ExperimentForSqueeze]
        :type errors: list[SqueezeTask] | None
        """
        self._print_parser_lib_versions()

        if self.history or self.future:
            all_users_mode = any(utestid.testid_is_all(exp.testid) for exp in experiments)
            if all_users_mode:
                raise Exception("Can't use history/future mode with all-users mode")
            adv_testids_mode = any(utestid.testid_is_adv(exp.testid) for exp in experiments)
            if adv_testids_mode:
                raise Exception("Can't use history/future mode with adv testids")

        cache_checker = None
        if self.enable_cache:
            cache_checker = self.squeeze_backend.get_cache_checker(
                min_versions=self.min_versions,
                need_replace=self.replace or self.replace_old,
            )

        squeeze_tasks = SqueezeTask.prepare_squeeze_tasks(
            experiments,
            self.history,
            self.future,
            cache_checker,
            self.enable_binary,
        )
        if not squeeze_tasks:
            raise Exception("There is nothing to squeeze")

        self.squeeze_backend.write_log(
            rectype=RectypeEnum.TECH,
            data=dict(
                squeeze_tasks=[task.squeeze_task_data for task in squeeze_tasks],
            ),
        )

        mp_worker = functools.partial(mp_squeeze_one_day, runner=self)
        day_count = len(squeeze_tasks)
        logging.info("Will squeeze %d days: %s", day_count, umisc.to_lines(squeeze_tasks))

        error_messages = []
        self.squeeze_results.clear()
        results_iter = umisc.par_imap_unordered(mp_worker, squeeze_tasks, self.threads, dummy=not self.use_processes)
        for pos, (task, error, result) in enumerate(results_iter):
            if error is not None:
                logging.error("Skip %s with errors: %s", task, error)
                error_messages.append((task, error))
            if result is not None:
                logging.info("%s result is %s", task, result)
                self.squeeze_results[task] = result
            umisc.log_progress("squeeze", pos, day_count)
            unirv.log_nirvana_progress("squeeze", pos, day_count)
        handle_squeeze_errors(errors, error_messages)
        handle_squeeze_results(self.squeeze_results.values())

    @staticmethod
    def _check_squeezer_versions(min_versions):
        for service, version in usix.iteritems(min_versions.service_versions):
            service_version = squeezer_services.SQUEEZERS[service].VERSION
            if service_version < version:
                raise Exception(
                    "Service {} has a version less than the specified ({} < {})".format(
                        service, service_version, version
                    )
                )


def handle_squeeze_errors(bad_tasks, error_messages):
    """
    :type bad_tasks: list[SqueezeTask] | None
    :type error_messages: list[(SqueezeTask, str)]
    """
    if not error_messages:
        return
    error_messages.sort()
    error_days_str = umisc.to_lines(("\n{}\n{}".format(task, error) for task, error in error_messages), fill="")
    if bad_tasks is None:
        raise Exception("Tasks with errors: {}".format(error_days_str))
    logging.info("Tasks with errors: %s", error_days_str)
    for task, _ in error_messages:
        bad_tasks.append(task)


def handle_squeeze_results(squeeze_results):
    """
    :type squeeze_results: list[str] | iter[str]
    :rtype: None
    """
    counter = collections.Counter(squeeze_results)
    logging.info("Squeeze results summary: %s", counter)


def pool_without_bad_experiments(pool, errors, history, future):
    """
    :type pool: experiment_pool.Pool
    :type errors: list[SqueezeTask]
    :type history: int
    :type future: int
    :rtype: experiment_pool.Pool
    """
    if not errors:
        return pool
    logging.info("Will remove bad experiments from pool")

    bad_days = set()
    bad_history = set()
    for task in errors:
        if task.exp_dates_for_history is not None:
            bad_history.add((task.day, task.exp_dates_for_history))
        else:
            bad_days.add(task.day)

    if history or future:
        assert bad_days or bad_history
    else:
        assert bad_days and not bad_history

    new_observations = pool.observations
    if bad_days:
        logging.info("There are %d days with errors: %s", len(bad_days), umisc.to_lines(sorted(bad_days)))

        def check_observation(obs):
            for day in obs.dates:
                if day in bad_days:
                    logging.info("%s removed (errors in %s)", obs, day)
                    return False
            return True

        new_observations = list(filter(check_observation, new_observations))

    if bad_history:
        logging.info("There are %d history days with errors: %s", len(bad_history), umisc.to_lines(sorted(bad_history)))

        def check_observation_history(obs):
            for day in iter_history_days(obs.dates, history, future):
                if (day, obs.dates) in bad_history:
                    logging.info("%s removed (errors in history %s %s)", obs, day, obs.dates)
                    return False
            return True

        new_observations = list(filter(check_observation_history, new_observations))

    new_pool = Pool(new_observations)
    logging.info("There are %d observations in old pool", len(pool.observations))
    logging.info("There are %d observations in new pool", len(new_pool.observations))
    return new_pool


@umisc.hash_and_ordering_from_key_method
class SqueezeTask(object):
    def __init__(self, day, source, dates, dates_for_history=None, enable_cache=False, cache_versions=None,
                 cache_message=None):
        """
        :type day: datetime.date
        :type source: str
        :type dates: yaqutils.time_helpers.DateRange
        :type dates_for_history: utils.time_helpers.DateRange | None
        :type enable_cache: bool
        :type cache_versions: SqueezeVersions | None
        :type cache_message: str | None
        """
        self.day = day
        self.source = source
        self.dates = dates
        self.exp_dates_for_history = dates_for_history
        self.enable_cache = enable_cache
        self.cache_versions = cache_versions
        self.cache_message = cache_message
        self.experiments = set()
        self.task_count = None

        assert source != ServiceSourceEnum.WATCHLOG or dates_for_history is None, \
            "It is impossible to use modes accumulate yuid and history together"

    def __str__(self):
        return "SqueezeTask{}: {}".format(self.key(), self.experiments)

    def __repr__(self):
        return str(self)

    def key(self):
        return self.day, self.source, self.exp_dates_for_history

    @property
    def squeeze_task_data(self):
        return dict(
            day=utime.format_date(self.day, pretty=True),
            source=self.source,
            dates=self.dates.serialize(),
            exp_dates_for_history=self.exp_dates_for_history.serialize() if self.exp_dates_for_history else None,
            enable_cache=self.enable_cache,
            cache_versions=self.cache_versions.serialize() if self.cache_versions else None,
            cache_message=self.cache_message,
            experiments=list(map(str, self.experiments)),
            task_count=self.task_count,
        )

    @staticmethod
    def prepare_squeeze_tasks(experiments, history, future, cache_checker, enable_binary):
        """
        :type experiments: list[ExperimentForSqueeze]
        :type history: int
        :type future: int
        :type cache_checker: callable | None
        :type enable_binary: bool
        :rtype: list[SqueezeTask]
        """
        squeeze_tasks = {}
        for exp in experiments:
            source = ServiceEnum.SOURCES[exp.service]
            use_binary = enable_binary and exp.service in ServiceEnum.SQUEEZE_BIN_SUPPORTED
            cache_source = ServiceEnum.get_cache_source(exp.service)
            for day in exp.dates:
                if cache_checker and cache_source != source:
                    cache_status, cache_info = cache_checker(day, exp.service, exp.filter_hash)
                    if cache_status:
                        key = (day, cache_source, None, use_binary)
                        cache_versions = cache_info
                        cache_message = None
                    else:
                        key = (day, source, None, use_binary)
                        cache_versions = None
                        cache_message = cache_info
                else:
                    key = (day, source, None, use_binary)
                    cache_status = False
                    cache_versions = None
                    cache_message = None
                task = squeeze_tasks.get(key)
                if not task:
                    task = SqueezeTask(
                        day=day,
                        source=key[1],
                        dates=exp.dates,
                        enable_cache=cache_status,
                        cache_versions=cache_versions,
                        cache_message=cache_message,
                    )
                    squeeze_tasks[key] = task
                task.experiments.add(exp)
            if history or future:
                history_dates = exp.dates
                history_exp = ExperimentForSqueeze(
                    experiment=exp.experiment,
                    observation=exp.observation,
                    service=exp.service,
                    all_users=exp.all_users,
                    all_for_history=True,
                )
                for day in iter_history_days(history_dates, history, future):
                    key = (day, source, history_dates, use_binary)
                    history_task = squeeze_tasks.get(key)
                    if not history_task:
                        history_task = SqueezeTask(day, source, exp.dates, dates_for_history=history_dates)
                        squeeze_tasks[key] = history_task
                    history_task.experiments.add(history_exp)

        result = sorted(usix.itervalues(squeeze_tasks))
        for task in result:
            task.task_count = len(result)
        return result


def iter_history_days(dates, history, future):
    if history:
        for day in dates.range_before(history):
            yield day
    if future:
        for day in dates.range_after(future):
            yield day


def iter_experiment_dates_days(dates):
    for day in dates:
        yield day


def mp_squeeze_one_day(squeeze_task, runner):
    """
    :type squeeze_task: SqueezeTask
    :type runner: SqueezeRunner
    :rtype: tuple[SqueezeTask, str | None, str | None]
    """
    try:
        result = runner.squeeze_one_day(squeeze_task)
        return squeeze_task, None, result
    except BaseException as exc:
        logging.info("Exception in mp_squeeze_one_day: %s, %s, %s", squeeze_task, exc, traceback.format_exc())
        return squeeze_task, str(exc), None


def get_unique_dirs(paths):
    return {os.path.dirname(path) for path in paths}
