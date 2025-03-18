# coding=utf-8
import itertools
import json
import logging
import os

import yt.yson as yson

import mstand_utils.testid_helpers as utestid
import session_squeezer.services as squeezer_services
import session_squeezer.suggest
import session_squeezer.validation

from mstand_enums.mstand_online_enums import TableTypeEnum, ServiceSourceEnum, ServiceEnum
from session_squeezer.squeezer_common import ActionSqueezerArguments, EXP_BUCKET_FIELD


def libra_create_filter(filters, init_factory=True):
    if not filters.libra_filters:
        raise Exception("Trying to create libra filter with no filters!")

    if init_factory:
        libra_init_filter_factory()

    # noinspection PyUnresolvedReferences,PyPackageRequirements
    import libra
    libra_filter = libra.TRequestFilter()
    for name, value in filters.libra_filters:
        try:
            libra_filter.Add(name, value)
        except Exception:
            logging.error("bad filter %s: %s", name, value)
            raise
    libra_filter.Init()

    return libra_filter


def libra_init_filter_factory():
    # noinspection PyUnresolvedReferences,PyPackageRequirements
    import libra
    libra.InitFilterFactory("geodata4.bin")


def libra_init_uatraits():
    # noinspection PyUnresolvedReferences,PyPackageRequirements
    import libra
    libra.InitUatraits("browser.xml")


def read_blockstat(path="blockstat.dict"):
    result = {}
    with open(path, "r") as f:
        for line in f:
            if ";" in line:
                k, v = line.strip().split(";", 1)
                result[k] = v
    return result


class UserSessionsSqueezer(object):
    def __init__(self, experiments, day, cache_filters=None, enable_cache=False):
        """
        :type experiments: set[ExperimentForSqueeze]
        :type day: datetime.date
        :type cache_filters: dict[str] | None
        :type enable_cache: bool
        """

        self.experiments = set(experiments)
        self.testids = {e.testid for e in self.experiments}
        self.history_mode = any(e.all_for_history for e in self.experiments)
        self.libra_filters = None
        self.libra_uatraits_initialized = False

        self.services = sorted({e.service for e in self.experiments})
        self.yuids_strict = any(ServiceEnum.SOURCES.get(service) in ServiceSourceEnum.YUIDS_STRICT
                                for service in self.services)
        self.enable_cache = enable_cache
        if self.enable_cache:
            self.use_libra = False
        else:
            self.use_libra = any(squeezer_services.SQUEEZERS[service].USE_LIBRA for service in self.services)
        self.use_any_filter = any(squeezer_services.SQUEEZERS[service].USE_ANY_FILTER for service in self.services)
        if self.use_libra or self.enable_cache:
            # can't work in combo libra+plain mode
            assert all(squeezer_services.SQUEEZERS[service].USE_LIBRA for service in self.services)
        elif self.use_any_filter:
            assert all(squeezer_services.SQUEEZERS[service].USE_ANY_FILTER for service in self.services)
        else:
            bad_observations = {
                exp.observation.id
                for exp in self.experiments
                if exp.filters.libra_filters
            }
            if bad_observations:
                message = "Observations {} use ralib/libra filters. " \
                          "Services {} do not support them. " \
                          "So you can calculate observations without filters only."
                raise Exception(message.format(sorted(bad_observations), self.services))
        self.day = day
        self.blockstat = None

        sources = {ServiceEnum.SOURCES[service] for service in self.services}
        assert len(sources) == 1
        self.source = sources.pop()
        if self.source == ServiceSourceEnum.SEARCH_STAFF:
            self.yuid_prefixes = ["y", "is"]
        elif self.source in ServiceSourceEnum.YUID_PREFIXES_IGNORE:
            self.yuid_prefixes = None
        else:
            self.yuid_prefixes = ["y", "uu/"]  # MSTAND-1130

        self.yuid_testids_field = "value"

        self.ignore_invalid_timestamps = self.source in ServiceSourceEnum.IGNORE_INVALID_TIMESTAMPS
        self.unwrap_container = len(self.services) > 1 or squeezer_services.SQUEEZERS[self.services[0]].UNWRAP_CONTAINER
        self.is_zen_testids = any(
            ServiceEnum.SOURCES[service] in ServiceSourceEnum.YUIDS_ZEN
            for service in self.services
        )
        self.is_bro_testids = any(utestid.testid_is_bro(exp.testid) for exp in experiments)
        self.debug_run_mode = os.environ.get("DEBUG_RUN_MODE")

        self.raw_cache_filters = cache_filters
        self.cache_filters = None

    def yuid_is_good(self, yuid):
        """
        :type yuid: str
        :rtype: bool
        """
        if not self.yuid_prefixes:
            return True
        if not yson.is_unicode(yuid):
            return False
        return any(yuid.startswith(prefix) for prefix in self.yuid_prefixes)

    def squeeze_session(self, yuid, rows, table_types):
        """
        :type yuid: str
        :type rows: __generator[dict[str]]
        :type table_types: list[int]
        :rtype: __generator[(ExperimentForSqueeze, dict[str])]
        """
        try:
            return self._squeeze_session(yuid, rows, table_types)
        except Exception:
            logging.error("Squeeze session error, yuid: %s", yuid, exc_info=True)
            raise

    def squeeze_web_session(self, yuid, rows):
        """
        :type yuid: str
        :type rows: __generator[dict[str]]
        :rtype: __generator[(ExperimentForSqueeze, dict[str])]
        """
        try:
            return self._squeeze_session_impl(yuid, rows)
        except Exception:
            logging.error("Squeeze session error, yuid: %s", yuid, exc_info=True)
            raise

    def squeeze_nile_session(self, yuid, container):
        """
        :type yuid: str
        :type container: libra.TRequestsContainer
        :rtype: __generator
        """
        try:
            return self._squeeze_session_impl(
                yuid,
                recs=None,
                container=container,
            )
        except Exception:
            logging.error("Squeeze session error, yuid: %s", yuid, exc_info=True)
            raise

    def _squeeze_session(self, yuid, rows, table_types):
        """
        :type yuid: str
        :type rows: __generator[dict[str]]
        :type table_types: list[int]
        :rtype: __generator[(ExperimentForSqueeze, dict[str])]
        """
        if not self.yuid_is_good(yuid):
            return []

        if {TableTypeEnum.SOURCE} == set(table_types):
            return self._squeeze_session_impl(
                yuid,
                rows,
            )

        def get_type(r):
            table_index = r["table_index"] if "table_index" in r else r["@table_index"]
            return table_types[table_index]

        user_testids = set()
        for group_type, group in itertools.groupby(rows, key=get_type):
            if group_type == TableTypeEnum.YUID:
                user_testids.update(self._read_user_testids(group))
            else:
                return self._squeeze_session_impl(
                    yuid,
                    group,
                    user_testids,
                )
        return []

    def squeeze_toloka(self, yuid, row, toloka_table):
        """
        :type yuid: str
        :type row: dict[str]
        :type toloka_table: str
        :rtype: __generator[(ExperimentForSqueeze, dict[str])]
        """
        possible_exps = self.experiments
        if not possible_exps:
            return

        assert all(service == ServiceEnum.TOLOKA for service in self.services)
        service_squeezers = squeezer_services.create_service_squeezers(self.services)
        """:type: dict[str, ActionsSqueezer]"""

        assert not self.use_libra
        for service in self.services:
            exps = {exp for exp in possible_exps if exp.service == service}
            arguments = ActionSqueezerArguments(row, exps, self.libra_filters, self.blockstat, self.day)
            if toloka_table == "ep":
                service_squeezers[service].get_actions_ep(arguments)
            elif toloka_table == "wps":
                service_squeezers[service].get_actions_wps(arguments)
            elif toloka_table == "rv56":
                service_squeezers[service].get_actions_rv56(arguments)
            elif toloka_table == "worker_daily_statistics":
                service_squeezers[service].get_actions_worker_daily_statistics(arguments)
            else:
                raise Exception("unknown toloka_table: {}".format(toloka_table))

            matched_exps = arguments.result_experiments
            if not matched_exps:
                continue

            actions = arguments.result_actions
            self._add_action_index_and_sort(actions)
            # TODO: fix validation for toloka (UTC vs Moscow time for example)
            # session_squeezer.validation.validate_actions(yuid, self.day, actions)
            for action in actions:
                request_matched_exps = action.pop(EXP_BUCKET_FIELD)
                for experiment in matched_exps:
                    is_match = experiment in request_matched_exps.matched
                    extended_action = dict(
                        action,
                        yuid=yuid,
                        servicetype=service,
                        testid=experiment.testid,
                        is_match=is_match,
                    )
                    yield experiment, extended_action

    def _check_timestamp(self, service: str) -> bool:
        return not (
            self.source in ServiceSourceEnum.SKIP_TIMESTAMP_CHECK
            or service in ServiceEnum.SKIP_TIMESTAMP_CHECK
        )

    def _precheck_experiment(self, exp, user_testids):
        """
        :type exp: ExperimentForSqueeze
        :type user_testids: set[str] | None
        :rtype: bool
        """
        # can't precheck experiments without yuid_testids table
        if user_testids is None:
            return True

        if exp.all_users:
            return True

        # temporary check for a_* testids (not all yuid_testids tables has a_* testids)
        if not self.yuids_strict and utestid.testid_is_adv(exp.testid):
            # temporary mode: accept all a_* testids
            return True

        return exp.testid in user_testids

    def _precheck_experiments(self, user_testids):
        """
        :type user_testids: set[str] | None
        :rtype: set[ExperimentForSqueeze]
        """
        if user_testids is None:
            if not self.use_libra:
                assert "watchlog" not in self.services, "watchlog cannot be used without yuid_testids"
            assert not self.history_mode
        return {
            exp for exp in self.experiments
            if self._precheck_experiment(exp, user_testids)
        }

    def _lazy_init_libra(self):
        assert self.use_libra, "libra mustn't use"

        if not self.libra_uatraits_initialized:
            libra_init_uatraits()
            self.libra_uatraits_initialized = True

        if self.libra_filters is None:
            self.libra_filters = {}
            experiments_with_filters = [exp for exp in self.experiments if exp.filters.libra_filters]
            if experiments_with_filters:
                libra_init_filter_factory()
                for exp in experiments_with_filters:
                    self.libra_filters[exp] = libra_create_filter(exp.filters, init_factory=False)

        if self.cache_filters is None and self.raw_cache_filters:
            self.cache_filters = {}
            libra_init_filter_factory()
            for uid, filters in self.raw_cache_filters.items():
                self.cache_filters[uid] = libra_create_filter(filters, init_factory=False)

    def _lazy_init_blockstat(self):
        if self.blockstat is None and self.use_libra:
            self.blockstat = read_blockstat()

    def _squeeze_session_impl(self, yuid, recs, user_testids=None, container=None):
        """
        :type yuid: str
        :type recs: __generator[dict]
        :type user_testids: set[str] | None
        :rtype: __generator[(ExperimentForSqueeze, dict[str])]
        """
        possible_exps = self._precheck_experiments(user_testids)
        if not possible_exps:
            return

        if self.debug_run_mode == "DO_NOTHING":
            return
        elif self.debug_run_mode == "READ_ONLY":
            for _ in recs:
                pass
            return
        elif self.debug_run_mode == "PARSE_LIBRA_ONLY":
            _ = self._create_libra_container(yuid, recs)
            return

        service_squeezers = squeezer_services.create_service_squeezers(self.services, self.enable_cache)
        """:type: dict[str, ActionsSqueezer]"""

        if container:
            self._lazy_init_libra()
        elif self.use_libra:
            container = self._create_libra_container(yuid, recs)
        elif self.unwrap_container:
            container = list(recs)
        else:
            container = recs

        if container is None:
            # skip fat user
            return

        self._lazy_init_blockstat()
        for service in self.services:
            exps = {exp for exp in possible_exps if exp.service == service}
            # TODO: move some arguments into ActionsSqueezer objects
            arguments = ActionSqueezerArguments(
                container, exps, self.libra_filters, self.blockstat, self.day, self.cache_filters,
            )

            try:
                service_squeezers[service].get_actions(arguments)
            except (UnicodeDecodeError, yson.yson_types.NotUnicodeError) as exc:
                logging.warning("Skip user (yuid: %s) due to unicode decode error: %s", yuid, exc)
                if service in ServiceEnum.SKIP_UNICODE_DECODE_ERROR:
                    continue
                raise
            except ValueError as exc:
                logging.warning("For user (yuid: %s) was got value error: %s", yuid, exc)
                raise

            matched_exps = arguments.result_experiments
            actions = arguments.result_actions

            # TODO: remove after fixing https://st.yandex-team.ru/LOGSTAT-5474
            session_squeezer.suggest.fix_suggest(actions)

            self._add_action_index_and_sort(actions)
            day = self.day if self._check_timestamp(service) else None
            valid_actions = session_squeezer.validation.validate_actions(
                yuid=yuid,
                day=day,
                service=service,
                fall_on_error=not self.ignore_invalid_timestamps,
                actions=actions,
            )
            if valid_actions:
                for action in actions:
                    request_matched_exps = action.pop(EXP_BUCKET_FIELD)
                    for experiment in matched_exps:
                        is_match = experiment in request_matched_exps.matched
                        bucket = request_matched_exps.buckets.get(experiment)
                        extended_action = dict(
                            action,
                            yuid=str(yuid) if service == ServiceEnum.YQL_AB else yuid,
                            servicetype=service,
                            testid=experiment.testid,
                            is_match=is_match,
                        )
                        if bucket is not None:
                            extended_action["bucket"] = bucket

                        yield experiment, extended_action

    def _create_libra_container(self, yuid, recs):
        self._lazy_init_libra()
        # noinspection PyUnresolvedReferences,PyPackageRequirements
        import libra
        try:
            services = [ServiceEnum.LIBRA[service] for service in self.services if service in ServiceEnum.LIBRA]
            return libra.Parse(recs, "blockstat.dict", services)
        except Exception as exc:
            if "fat user" in str(exc):
                return None
            logging.error("Error parsing session _get_all_actions: yuid=%s, day=%s, %s", yuid, self.day, exc)
            raise

    @staticmethod
    def _add_action_index_and_sort(actions):
        """
        :type actions: list[dict]
        """
        for action_index, action in enumerate(actions):
            action["action_index"] = action_index
        actions.sort(key=lambda a: (a["ts"], a["action_index"]))

    def _read_user_testids(self, rows):
        """
        Read from yuid_testids tables
        """
        testids = set()
        for row in rows:
            raw_testids = row[self.yuid_testids_field]
            if self.is_zen_testids:
                testids.update(t.split(",")[0] for t in raw_testids.split(";"))
            elif self.is_bro_testids:
                testids.update("mbro__{}".format(t) for t in json.loads(raw_testids.split("\t")[0])[::2])
            else:
                testids.update(raw_testids.split("\t"))
        return testids
