import collections
import hashlib
import logging
import numbers
import os
import traceback

import mstand_metric_helpers.online_metric_helpers as online_mhelp
import mstand_utils.mstand_paths_utils as mstand_upaths
import session_metric.user_filters as user_filters
import session_squeezer.services as squeezer_services
import yaqutils.file_helpers as ufile
import yaqutils.json_helpers as ujson
import yaqutils.misc_helpers as umisc
import yaqutils.six_helpers as usix
import yaqutils.time_helpers as utime

from experiment_pool import MetricDataType
from experiment_pool import MetricResult
from experiment_pool import MetricType
from experiment_pool import MetricValues
from experiment_pool import MetricValueType
from metrics_api import ExperimentForAPI
from metrics_api import ObservationForAPI
from metrics_api.online import UserActionForAPI
from metrics_api.online import UserActionsForAPI
from mstand_enums.mstand_online_enums import FiltersEnum
from user_plugins import PluginContainer  # noqa
from user_plugins import PluginKey  # noqa

NUM_BUCKETS = 100


class MetricBucket(object):
    def __init__(self, value_type):
        self.sum = 0
        self.count = 0

        assert value_type in MetricValueType.ALL
        self.value_type = value_type

    def add_value(self, value):
        self.sum += value
        self.count += 1

    @property
    def average(self):
        return float(self.sum) / self.count if self.count else 0

    def get_value(self):
        if self.value_type == MetricValueType.AVERAGE:
            return self.average
        else:
            return self.sum


def _yt_metric_key(metric_id):
    """
    :type metric_id: int
    :rtype: str
    """
    return "m_{}".format(metric_id)


def _bucketize(rows, metric_keys, value_types):
    """
    :type rows: __generator[dict]
    :type metric_keys: dict[int, PluginKey]
    :type value_types: dict[int, str]
    :rtype: __generator[dict]
    """

    bucket_lists = {}
    for key, value_type in value_types.items():
        bucket_lists[key] = [MetricBucket(value_type) for _ in usix.xrange(NUM_BUCKETS)]

    for index, row in enumerate(rows):
        if index and index % 100000 == 0:
            logging.info("buckets: processed %d rows", index)

        bucket_index = row["bucket"]

        for metric_id in metric_keys:
            yt_key = _yt_metric_key(metric_id)
            one_user_values = row.get(yt_key, [])
            for value in one_user_values:
                if isinstance(value, numbers.Number):
                    bucket_lists[metric_id][bucket_index].add_value(value)
                else:
                    raise Exception("Tried to use buckets with non-number metric value: {}".format(repr(value)))

    for bucket_index in usix.xrange(NUM_BUCKETS):
        values = {}
        for key, bucket_list in bucket_lists.items():
            yt_key = _yt_metric_key(key)
            value = bucket_list[bucket_index].get_value()
            values[yt_key] = [value]
        yield values


class MetricResultStat(object):
    def __init__(self):
        self.number_count = 0
        self.number_sum = 0
        self.row_count = 0
        self.value_count = collections.Counter()


def _get_file_md5(filename):
    m = hashlib.md5()
    with open(filename, 'rb') as f:
        for chunk in iter(lambda: f.read(1024), b''):
            m.update(chunk)
    return m.hexdigest()


def _get_metric_values(rows, metric_keys, value_types, output_files):
    """
    :type rows: __generator[dict[str]]
    :type metric_keys: dict[int, PluginKey]
    :type value_types: dict[int, str]
    :type output_files: dict[int, str]
    :rtype: dict[int, MetricValues]
    """
    logging.info("reading/dumping of metric data started")

    result_stats = {metric_id: MetricResultStat() for metric_id in metric_keys.keys()}
    """:type: dict[int, MetricResultStat]"""

    open_files = {metric_id: ufile.fopen_write(path) for metric_id, path in usix.iteritems(output_files)}
    """:type: dict[int, file]"""

    md5_files = {metric_id: hashlib.md5() for metric_id in metric_keys.keys()}

    for index, row in enumerate(rows):
        if index and index % 100000 == 0:
            logging.info("processed %d rows", index)
        for metric_id in metric_keys:
            yt_key = _yt_metric_key(metric_id)
            one_user_values = row.get(yt_key)
            if one_user_values:
                stat = result_stats[metric_id]
                stat.row_count += 1
                stat.value_count[len(one_user_values)] += 1

                metric_fd = open_files.get(metric_id)
                if metric_fd:
                    row_str = dump_one_metric_row(metric_fd, one_user_values)
                    md5_files[metric_id].update(row_str.encode())

                for value in one_user_values:
                    if isinstance(value, numbers.Number):
                        stat.number_count += 1
                        stat.number_sum += value

    for metric_id, metric_fd in open_files.items():
        metric_fd.close()

        real_md5 = _get_file_md5(metric_fd.name)
        if md5_files[metric_id].hexdigest() != real_md5:
            error_message = "check md5 fail for {}: {} != {}".format(
                metric_fd.name, md5_files[metric_id].hexdigest(), real_md5
            )
            raise Exception(error_message)

    if "DEBUG_FILE_INFO" in os.environ:
        for filename in output_files.values():
            size = ufile.get_file_size(filename)
            line_count = sum(1 for _ in ufile.fopen_read(filename))
            logging.info("Debug output file info:\n\tname: %s\n\tsize: %i\n\tline count: %i", filename, size, line_count)

    logging.info("reading/dumping of metric data completed")

    metric_values_by_id = {}
    for metric_id, stat in usix.iteritems(result_stats):
        if stat.number_count > 0:
            number_average = float(stat.number_sum) / stat.number_count
        else:
            number_average = None

        data_file_path = output_files.get(metric_id)
        data_file = os.path.basename(data_file_path) if data_file_path else None
        value_type = value_types[metric_id]
        metric_values_by_id[metric_id] = MetricValues(
            count_val=stat.number_count,
            sum_val=stat.number_sum,
            average_val=number_average,
            data_type=MetricDataType.VALUES,
            data_file=data_file,
            row_count=stat.row_count,
            value_type=value_type,
        )
        logging.info("Value count stat for metric %s, file %s: %r",
                     metric_keys[metric_id].str_key(), data_file, stat.value_count)

        if "DEBUG_CHECK_METRIC_FILE" in os.environ:
            _check_metric_file(output_files.get(metric_id))

    return metric_values_by_id


def _check_metric_file(output_path):
    if output_path:
        value_count = collections.Counter()
        tab_first_count = 0
        for row in ufile.fopen_read(output_path):
            value_count[len(row.split("\t"))] += 1
            tab_first_count += row.startswith("\t")
        logging.info("Debug file stat:\n\tname: %s\n\tvalue count: %r\n\tfirst tab count: %d", output_path, value_count, tab_first_count)


def _dump_value(value):
    if not isinstance(value, numbers.Number):
        value = umisc.to_unicode_recursive(value)
    return ujson.dump_to_str(value)


def dump_one_metric_row(metric_fd, values):
    """
    :type metric_fd:
    :type values: list
    :rtype: str
    """
    row_str = "\t".join(_dump_value(v) for v in values) + "\n"
    metric_fd.write(row_str)
    return row_str


def extract_metric_value_types(metric_container):
    """
    :type metric_container: PluginContainer
    :rtype: dict[int, str]
    """

    return {metric_id: metric_instance.value_type
            for metric_id, metric_instance in usix.iteritems(metric_container.plugin_instances)}


def read_metric_results_impl(rows, metric_container, experiment_name, output_files, use_buckets, table_path=None):
    """
    :type rows: __generator[dict[str]]
    :type metric_container: PluginContainer
    :type experiment_name: str
    :type output_files: dict[int, str] | None
    :type use_buckets: bool
    :type table_path: str | None
    :rtype: list[MetricResult]
    """

    logging.info("read_metric_results_impl started: %s", experiment_name)

    metric_keys = metric_container.plugin_key_map
    value_types = extract_metric_value_types(metric_container)

    if use_buckets:
        rows = _bucketize(rows, metric_keys, value_types)

    if output_files is None:
        output_files = {}

    logging.info("Started metric values aggregation")
    values_by_metric_id = _get_metric_values(rows, metric_keys, value_types, output_files)
    logging.info("Metric values aggregation completed")

    results = []
    for metric_id, value in usix.iteritems(values_by_metric_id):
        coloring = metric_container.plugin_instances[metric_id].coloring
        metric_key = metric_container.plugin_key_map[metric_id]
        ab_info = metric_container.plugin_ab_infos.get(metric_id)
        one_metric_result = MetricResult(
            metric_key=metric_key,
            metric_type=MetricType.ONLINE,
            metric_values=value,
            coloring=coloring,
            ab_info=ab_info,
        )
        if table_path:
            one_metric_result.result_table_path = table_path
            one_metric_result.result_table_field = _yt_metric_key(metric_id)

        results.append(one_metric_result)
    return results


# This function is designed to execute on MR, be careful with logging.
def calc_metric_for_user_impl(metric_container, yuid, rows, experiment, observation, use_buckets):
    """
    :type metric_container: PluginContainer
    :type yuid: str
    :type rows: __generator[dict[str]]
    :type experiment: Experiment
    :type observation: Observation
    :type use_buckets: bool
    :rtype dict[str] | None
    """

    actions = []
    actions_history = []
    actions_future = []
    for row in rows:
        action = UserActionForAPI.from_row(row)
        if row.get("_history"):
            actions_history.append(action)
        elif row.get("_future"):
            actions_future.append(action)
        else:
            actions.append(action)

    if not actions:
        return None

    if use_buckets:
        bucket = get_user_bucket(yuid, actions)
        if bucket is None:
            return None
    else:
        bucket = None

    user_actions = UserActionsForAPI(
        user=yuid,
        actions=actions,
        experiment=ExperimentForAPI(experiment),
        observation=ObservationForAPI(observation),
        history=actions_history,
        future=actions_future
    )
    values = calc_metric_for_actions(metric_container, yuid, user_actions, experiment, observation)

    if use_buckets:
        assert bucket is not None
        values["bucket"] = bucket

    return values


def get_user_bucket(yuid, actions):
    date = utime.format_date(actions[0].date, pretty=True)
    testid = actions[0].data.get("testid")
    prefix = "[{}: testid {}]".format(date, testid)

    for action in actions:
        if "bucket" not in action.data:
            raise Exception("{} User {} has no bucket value, try updating squeeze".format(prefix, yuid))

    all_buckets = set(action.data.get("bucket") for action in actions)
    all_buckets.discard(None)
    all_buckets.discard(-1)

    if not all_buckets:
        logging.error("%s User %s has no valid buckets", prefix, yuid)
        return None

    if len(all_buckets) > 1:
        logging.error("%s User %s has multiple buckets: %s", prefix, yuid, sorted(all_buckets))
        return None

    bucket = all_buckets.pop()
    if not (0 <= bucket < NUM_BUCKETS):
        raise Exception("User {} has invalid bucket: {}, should be 0 <= bucket < {}".format(yuid, bucket, NUM_BUCKETS))

    return bucket


# This function is designed to execute on MR, be careful with logging.
def calc_metric_for_actions(metric_container, yuid, user_actions, experiment, observation):
    """
    :type metric_container: PluginContainer
    :type yuid: str
    :type user_actions: UserActionsForAPI
    :type experiment: Experiment
    :type observation: Observation
    :rtype dict
    """
    results_by_metric_id = {}
    for metric_id, metric_instance in usix.iteritems(metric_container.plugin_instances):
        user_filter_name = getattr(metric_instance, "user_filter", FiltersEnum.USER)
        user_filter = user_filters.get_filter(user_filter_name)
        user_actions_filtered = user_actions.apply_filter(user_filter)
        if user_actions_filtered:
            try:
                raw_metric_result = online_mhelp.apply_online_metric(metric_instance, user_actions_filtered)
                one_user_values = online_mhelp.convert_online_metric_value_to_list(raw_metric_result)
            except Exception as exc:
                metric_key = metric_container.plugin_key_map[metric_id]
                metric_info = "metric {} with ID {}".format(metric_key, metric_id)
                error_info = "{}, details: {}".format(exc, traceback.format_exc())
                context_info = "yuid={}, exp={}, obs={}".format(yuid, experiment, observation)
                logging.error("Exception in {} at {}: {}".format(metric_info, context_info, error_info))
                raise
            if one_user_values:
                yt_key = _yt_metric_key(metric_id)
                results_by_metric_id[yt_key] = one_user_values
    return results_by_metric_id


class MetricCalculator(object):
    def __init__(self, metric_container, use_buckets):
        """
        :type metric_container: PluginContainer
        :type use_buckets: bool
        """
        self.metric_container = metric_container
        self.check_metric_filters()

        self.use_buckets = use_buckets

    def calc_metric_for_user(self, yuid, rows, experiment, observation):
        """
        :type yuid: str
        :type rows: __generator[dict[str]]
        :type experiment: Experiment
        :type observation: Observation
        :rtype: dict[str] | None
        """
        return calc_metric_for_user_impl(
            metric_container=self.metric_container,
            yuid=yuid,
            rows=rows,
            experiment=experiment,
            observation=observation,
            use_buckets=self.use_buckets
        )

    def check_metric_filters(self):
        logging.info("Checking user filters...")
        for metric_id, instance in usix.iteritems(self.metric_container.plugin_instances):
            user_filter = getattr(instance, "user_filter", None)
            user_filters.check_filter_correctness(user_filter)
            if user_filter:
                logging.info("Will use filter '%s' for metric %s", user_filter,
                             self.metric_container.plugin_key_map[metric_id])
        logging.info("User filters are correct")

    def read_metric_results(self, rows, experiment_name, output_files, table_path=None):
        """
        :type rows: __generator[dict[str]]
        :type experiment_name: str,
        :type output_files: dict[int, str]
        :type table_path: str | None
        :rtype: list[MetricResult]
        """
        return read_metric_results_impl(
            rows=rows,
            metric_container=self.metric_container,
            experiment_name=experiment_name,
            output_files=output_files,
            use_buckets=self.use_buckets,
            table_path=table_path,
        )

    def get_results_schema(self):
        results_schema = [
            {"name": "user_id", "type": "string"},
            {"name": "metric_id", "type": "string"},
            {"name": "index", "type": "int32"},
            {"name": "bucket", "type": "int32"},
        ]
        default_schema = [{"name": "value", "type": "any"}]
        schema_info = {sch["name"]: (sch["type"], "base") for sch in results_schema}
        for plugin in self.metric_container.plugin_instances.values():
            plugin_class_name = plugin.__class__.__name__
            for sch in getattr(plugin, "schema", default_schema):
                column_name = sch["name"]
                column_type = sch["type"]
                if column_name not in schema_info:
                    results_schema.append(sch)
                    schema_info[column_name] = (sch["type"], plugin_class_name)
                else:
                    info_type, info_src = schema_info[column_name]
                    assert info_type == column_type, f"There is a problem with type of '{column_name}' column:\n\t'{info_src}' -> '{info_type}'\n\t'{plugin_class_name}' -> '{column_type}'"
        return results_schema


class MetricSourceInfo(object):
    def __init__(self, table, history=False, future=False):
        """
        :type table: str
        :type history: bool
        :type future: bool
        """
        assert not (history and future)
        self.table = table
        self.history = history
        self.future = future

    @staticmethod
    def patch_user_actions(rows, info_index):
        """
        :type rows: __generator[dict[str]]
        :type info_index: list[MetricSourceInfo]
        :rtype: __generator[dict[str]]
        """
        for row in rows:
            row_index = row["@table_index"]
            row_info = info_index[row_index]
            row["_history"] = row_info.history
            row["_future"] = row_info.future
            yield row


class MetricSources(object):
    def __init__(self, sources, history, future):
        """
        :type sources: list[str]
        :type history: list[str]
        :type future: list[str]
        """
        self._sources = sources
        self._history = history
        self._future = future

        self._all_tables = self._sources + self._history + self._future
        self._sources_end = len(self._sources)
        self._history_end = self._sources_end + len(self._history)
        self._future_end = self._history_end + len(self._future)

    def __str__(self):
        return "MetricSources({}, history={}, future={})".format(self._sources, self._history, self._future)

    def __repr__(self):
        return str(self)

    def all_tables(self):
        return self._all_tables

    def all_tables_info(self):
        for table in self._sources:
            yield MetricSourceInfo(table)
        for table in self._history:
            yield MetricSourceInfo(table, history=True)
        for table in self._future:
            yield MetricSourceInfo(table, future=True)

    @staticmethod
    def from_experiment(experiment, observation, services, squeeze_path, history=None, future=None):
        """
        :type experiment: experiment_pool.Experiment
        :type observation: experiment_pool.Observation
        :type services: list[str]
        :type squeeze_path: str
        :type history: int | None
        :type future: int | None
        :rtype: MetricSources
        """
        testid = experiment.testid
        dates = observation.dates
        filter_hash = observation.filters.filter_hash
        source_tables = MetricSources._get_paths(testid, dates, filter_hash, services, squeeze_path)

        if history:
            history_dates = dates.range_before(history)
            source_history = MetricSources._get_paths_history(testid, history_dates, dates, filter_hash,
                                                              services, squeeze_path)
        else:
            source_history = []

        if future:
            future_dates = dates.range_after(future)
            source_future = MetricSources._get_paths_history(testid, future_dates, dates, filter_hash,
                                                             services, squeeze_path)
        else:
            source_future = []

        return MetricSources(source_tables, source_history, source_future)

    @staticmethod
    def _get_paths(testid, dates, filter_hash, services, squeeze_path):
        """
        :type testid: str
        :type dates: yaqutils.time_helpers.DateRange
        :type filter_hash: str
        :type services: list[str]
        :type squeeze_path: str
        :rtype: list[str]
        """
        all_tables = []
        for service in services:
            tables = mstand_upaths.get_squeeze_paths(
                dates=dates,
                testid=testid,
                filter_hash=filter_hash if squeezer_services.has_filter_support(service) else None,
                service=service,
                squeeze_path=squeeze_path,
            )
            all_tables.extend(tables)
        return all_tables

    @staticmethod
    def _get_paths_history(testid, dates, testid_dates, filter_hash, services, squeeze_path):
        """
        :type testid: str
        :type dates: yaqutils.time_helpers.DateRange
        :type testid_dates: yaqutils.time_helpers.DateRange
        :type filter_hash: str
        :type services: list[str]
        :type squeeze_path: str
        :rtype: list[str]
        """
        all_tables = []
        for service in services:
            tables = mstand_upaths.get_squeeze_history_paths(
                dates=dates,
                testid=testid,
                filter_hash=filter_hash if squeezer_services.has_filter_support(service) else None,
                testid_dates=testid_dates,
                service=service,
                squeeze_path=squeeze_path,
            )
            all_tables.extend(tables)
        return all_tables
