# coding=utf-8
import datetime
import functools
import uuid
import logging
import os
import time
import traceback
from collections import defaultdict

import yt.wrapper as yt
import yt.yson as yson

import session_yt.versions_yt as versions_yt
import mstand_utils.yt_helpers as mstand_uyt
import pytlib.yt_io_helpers as yt_io
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime
import yaqutils.json_helpers as ujson
import yaqutils.file_helpers as ufile
import yaqutils.six_helpers as usix

from experiment_pool import MetricResult
from mstand_structs import LampKey
from mstand_structs import SqueezeVersions
from session_metric.metric_calculator import MetricCalculator  # noqa
from session_metric.metric_calculator import MetricSourceInfo
from mstand_utils.yt_options_struct import YtJobOptions, ResultOptions


class CalcYTIndexes(object):

    def __init__(self, experiments):
        """
        :type experiments: dict[ExperimentForCalculation, MetricSources]
        """
        self.experiments = sorted(experiments)
        """:type: list[ExperimentForCalculation]"""

        table_to_exp = defaultdict(list)
        """:type: dict[str, list[dict]]"""

        for exp_index, exp in enumerate(self.experiments):
            sources = experiments[exp]
            for info in sources.all_tables_info():
                table_to_exp[info.table].append(
                    {
                        "_experiment": exp_index,
                        "_history": info.history,
                        "_future": info.future,
                    }
                )

        self.tables = sorted(table_to_exp)
        """:type: list[str]"""

        self.table_to_exp_index = [table_to_exp[table] for table in self.tables]
        """:type: list[list[dict]]"""


class CalcYTBatchMapper(object):
    def __init__(self, info_index):
        """
        :type info_index: list[list[dict]]
        """
        self.info_index = info_index

    def __call__(self, row):
        """
        :type row: dict
        :rtype: __generator[dict]
        """
        table_index = row.pop("@table_index")
        for info in self.info_index[table_index]:
            yield dict(row, **info)


def split_into_rows(result, exp_index, yuid, bucket, split_values_and_schematize):
    rows = []
    for metric, values in result.items():
        assert isinstance(values, list)
        for i, value in enumerate(values):
            row = {
                "user_id": yuid,
                "metric_id": metric,
                "index": i,
            }
            if split_values_and_schematize and isinstance(value, dict):
                row.update(**value)
            else:
                row["value"] = value
            if bucket:
                row["bucket"] = bucket
            if exp_index:
                row["@table_index"] = exp_index
            rows.append(row)
    return rows


def split_results(result, exp_index, yuid, split_values_and_schematize):
    if result:
        bucket = result.pop("bucket", None)
        return split_into_rows(result, exp_index, yuid, bucket, split_values_and_schematize)
    else:
        return []


class CalcYTBatchReducer(object):
    def __init__(self, calculator, experiments, split_metric_results, split_values_and_schematize):
        """
        :type calculator: MetricCalculator
        :type experiments: list[ExperimentForCalculation]
        :type split_metric_results: bool
        :type split_values_and_schematize: bool
        """
        self.calculator = calculator
        self.experiments = experiments
        self.split_metric_results = split_metric_results
        self.split_values_and_schematize = split_values_and_schematize

    def __call__(self, keys, rows):
        exp_index = keys["_experiment"]
        exp = self.experiments[exp_index]
        result = self.calculator.calc_metric_for_user(
            yuid=keys["yuid"],
            rows=rows,
            experiment=exp.experiment,
            observation=exp.observation,
        )
        if self.split_metric_results or self.split_values_and_schematize:
            for result_to_yield in split_results(result, exp_index, keys["yuid"], self.split_values_and_schematize):
                yield result_to_yield
        else:
            if result:
                result["@table_index"] = exp_index
                yield result


class CalcYTSingleReducer(object):
    def __init__(self, calculator, experiment, info_index, split_metric_results):
        """
        :type calculator: MetricCalculator
        :type experiment: ExperimentForCalculation
        :type info_index: list[MetricSourceInfo]
        :type split_metric_results: bool
        """
        self.calculator = calculator
        self.experiment = experiment
        self.info_index = info_index
        self.split_metric_results = split_metric_results

    def __call__(self, keys, rows):
        """
        :type keys: dict[str]
        :type rows: __generator[dict[str]]
        """
        rows = MetricSourceInfo.patch_user_actions(rows, self.info_index)
        result = self.calculator.calc_metric_for_user(
            yuid=keys["yuid"],
            rows=rows,
            experiment=self.experiment.experiment,
            observation=self.experiment.observation,
        )
        if self.split_metric_results:
            for result_to_yield in split_results(result, None, keys["yuid"]):
                yield result_to_yield
        else:
            if result:
                yield result


def get_tmp_dir_name():
    return ".tmp_downloaded_tables_{}".format(uuid.uuid4())


def get_tmp_local_file_name(experiment):
    """
    :type experiment: ExperimentForCalculation
    :rtype: str
    """
    return ".tmp_downloaded_table_{}_{}_{}".format(experiment.testid, experiment.dates, uuid.uuid4())


def get_tmp_local_file_path(tmp_dir, tmp_local_file):
    return os.path.join(tmp_dir, tmp_local_file)


def download_using_cli(tmp_table, tmp_dir, tmp_local_file, download_threads, transaction, client):
    """
    :type tmp_table: str
    :type tmp_dir: str
    :type tmp_local_file: str
    :type download_threads: int
    :type transaction: yt.Transaction
    :type client: yt.client.Yt
    """
    logging.info("Downloading table '%s' into '%s' as '%s' using cli",
                 tmp_table,
                 tmp_dir,
                 tmp_local_file)
    command = ["yt",
               "--proxy",
               client.config["proxy"]["url"],
               "--tx",
               transaction.transaction_id,
               "read-table",
               "--format",
               "json",
               "--config",
               "{read_parallel={enable=%true;max_thread_count=" + str(download_threads) + ";}}",
               tmp_table]
    try:
        with ufile.fopen_write(get_tmp_local_file_path(tmp_dir, tmp_local_file)) as fd:
            umisc.run_command(command, stdout_fd=fd, log_command_line=True)
    except Exception:
        logging.error("Downloading connected error, probably metric results contain 'inf' or 'nan' values")
        raise


def download_using_api(tmp_table, tmp_dir, tmp_local_file, download_threads, client):
    """
    :type tmp_table: str
    :type tmp_dir: str
    :type tmp_local_file: str
    :type download_threads: int
    :type client: yt.client.Yt
    """
    logging.info("Downloading table '%s' into '%s' as '%s' using api",
                 tmp_table,
                 tmp_dir,
                 tmp_local_file)
    with ufile.fopen_write(get_tmp_local_file_path(tmp_dir, tmp_local_file)) as fd:
        for row in yt.read_table(tmp_table,
                                 client=client,
                                 enable_read_parallel=download_threads):
            ujson.dump_to_fd(row, fd)
            fd.write("\n")


def download_raw_tables(tmp_tables,
                        experiments,
                        client,
                        transaction,
                        download_threads,
                        use_yt_cli):
    """
    :type tmp_tables: list[str]
    :type experiments: list[ExperimentForCalculation]
    :type client: yt.client.Yt
    :type transaction: yt.Transaction
    :type download_threads: int
    :type use_yt_cli: bool
    :rtype tuple(str, list[str])
    """
    count = len(experiments)
    tmp_dir = get_tmp_dir_name()
    logging.info("Creating temporary directory for downloaded tables: %s", tmp_dir)
    os.mkdir(tmp_dir)
    logging.info("Downloading raw tables using %s threads...", download_threads)
    tmp_local_files = []
    time_start = time.time()
    for pos, (tmp_table, experiment) in enumerate(zip(tmp_tables, experiments)):
        tmp_local_file = get_tmp_local_file_name(experiment)
        tmp_local_files.append(tmp_local_file)
        if use_yt_cli:
            download_using_cli(tmp_table, tmp_dir, tmp_local_file, download_threads, transaction, client)
        else:
            download_using_api(tmp_table, tmp_dir, tmp_local_file, download_threads, client)
        umisc.log_progress("metric results downloading", pos, count)
    time_end = time.time()
    logging.info("Raw tables have been downloaded in %s: %s", datetime.timedelta(seconds=time_end - time_start),
                 umisc.to_lines(tmp_local_files))
    return tmp_dir, tmp_local_files


def remove_tmp_local_files(tmp_dir, tmp_local_files):
    """
    :type tmp_dir: str
    :type tmp_local_files: list[str]
    """
    logging.info("Removing temporary local files: %s", umisc.to_lines(tmp_local_files))
    for tmp_local_file in tmp_local_files:
        os.remove(get_tmp_local_file_path(tmp_dir, tmp_local_file))
    logging.info("Temporary local files are removed")
    os.rmdir(tmp_dir)
    logging.info("Temporary directory %s is removed", tmp_dir)


def sort_wrapper(tmp_table, yt_job_options, client, sort_by):
    """
    :type tmp_table: str
    :type yt_job_options: YtJobOptions
    :type client: yt.client.Yt
    :type sort_by: list[str]
    :rtype yt.Operation
    """
    sort_spec = yt_io.get_yt_operation_spec(title="sort metric results in '{}'".format(tmp_table),
                                            yt_pool=yt_job_options.pool,
                                            use_porto_layer=False)
    return yt.run_sort(
        tmp_table,
        spec=sort_spec,
        client=client,
        sync=False,
        sort_by=sort_by,
    )


def sort_results(tmp_tables,
                 yt_job_options,
                 client,
                 download_threads,
                 sort_by):
    """
    :type tmp_tables: list[str]
    :type yt_job_options: YtJobOptions
    :type client: yt.client.Yt
    :type download_threads: int
    :type sort_by: list[str]
    """
    logging.info("Will sort results in tables: %s", umisc.to_lines(tmp_tables))
    time_start = time.time()
    worker = functools.partial(
        sort_wrapper,
        yt_job_options=yt_job_options,
        client=client,
        sort_by=sort_by,
    )
    mstand_uyt.async_yt_operations_poll(worker, tmp_tables, download_threads)
    time_end = time.time()

    logging.info("Tables have been sorted in %s: %s", datetime.timedelta(seconds=time_end - time_start),
                 umisc.to_lines(tmp_tables))


def get_max_version_for_exp(sources, client):
    paths = sources.all_tables()
    all_versions = versions_yt.get_all_versions(paths, client)
    max_version = SqueezeVersions.get_newest(all_versions.values())

    return max_version


@mstand_uyt.retry_yt_operation_default(times=5)
def metric_calculation_batch(
        experiments,
        calculator,
        result_files,
        client,
        yt_job_options,
        result_options,
        download_threads,
        use_yt_cli,
        add_acl,
):
    """
    :type experiments: dict[ExperimentForCalculation, MetricSources]
    :type calculator: MetricCalculator
    :type result_files: dict[ExperimentForCalculation, dict[int, str]]
    :type client: yt.client.Yt
    :type yt_job_options: YtJobOptions
    :type result_options: ResultOptions
    :type download_threads: int
    :type use_yt_cli: bool
    :type add_acl: bool
    :rtype: dict[ExperimentForCalculation, list[MetricResult]]
    """
    if not experiments:
        return {}

    indexes = CalcYTIndexes(experiments)

    with yt.Transaction(client=client) as transaction:
        result_table_schema = calculator.get_results_schema() if result_options.split_values_and_schematize else None
        tmp_tables = [
            create_temp_table(
                experiment=exp,
                client=client,
                ttl_days=result_options.results_ttl,
                schema=result_table_schema,
            )
            for exp in indexes.experiments
        ]
        try:
            run_metric_calculation_batch(
                calculator=calculator,
                indexes=indexes,
                destination_tables=tmp_tables,
                client=client,
                yt_job_options=yt_job_options,
                split_metric_results=result_options.split_metric_results,
                add_acl=add_acl,
                split_values_and_schematize=result_options.split_values_and_schematize,
            )
            if result_options.split_metric_results or result_options.split_values_and_schematize:
                sort_results(tmp_tables, yt_job_options, client, download_threads, ["user_id", "metric_id", "index"])
            all_results = get_metric_results_batch(
                calculator=calculator,
                experiments=indexes.experiments,
                tmp_tables=tmp_tables,
                result_files=result_files,
                client=client,
                yt_job_options=yt_job_options,
                result_options=result_options,
                download_threads=download_threads,
                transaction=transaction,
                use_yt_cli=use_yt_cli,
            )
        finally:
            for tmp in tmp_tables:
                if yt.exists(tmp, client=client):
                    yt.remove(tmp, client=client)

        for experiment, sources in experiments.items():
            max_version = get_max_version_for_exp(sources, client)

            for res in all_results[experiment]:
                res.version = max_version

        return all_results


def get_metric_operation_title(experiments):
    """
    :type experiments: list[ExperimentForCalculation]
    :rtype: str
    """
    names = ", ".join("<{}, {}>".format(exp.testid, exp.dates) for exp in experiments)
    return "mstand metric {}".format(names)


def list_all_files_from_batch(container):
    """
    :type container: PluginContainer
    :return: list[str]
    """
    all_files = set()

    for instance in container.plugin_instances.values():
        if hasattr(instance, "list_files"):
            instance_files = instance.list_files()
            full_paths = [umisc.get_user_module_absolute_path(instance_file) for instance_file in instance_files]
            all_files.update(full_paths)

    return list(all_files)


def run_metric_calculation_batch(
        calculator,
        indexes,
        destination_tables,
        client,
        yt_job_options,
        split_metric_results,
        add_acl,
        split_values_and_schematize,
):
    """
    :type calculator: MetricCalculator
    :type indexes: CalcYTIndexes
    :type destination_tables: list[str]
    :type client: yt.client.Yt
    :type yt_job_options: YtJobOptions
    :type split_metric_results: bool
    :type add_acl: bool
    :type split_values_and_schematize: bool
    """
    files = list_all_files_from_batch(calculator.metric_container)
    experiments = indexes.experiments
    source_tables = indexes.tables
    mapper = CalcYTBatchMapper(indexes.table_to_exp_index)
    reducer = CalcYTBatchReducer(calculator, experiments, split_metric_results, split_values_and_schematize)

    yt_spec = yt_io.get_yt_operation_spec(max_failed_job_count=2,
                                          data_size_per_job=yt.common.MB * yt_job_options.data_size_per_job,
                                          data_size_per_map_job=yt.common.MB * 300,
                                          partition_data_size=yt.common.MB * yt_job_options.data_size_per_job,
                                          memory_limit=yt.common.MB * yt_job_options.memory_limit,
                                          enable_input_table_index=True,
                                          check_input_fully_consumed=True,
                                          title=get_metric_operation_title(indexes.experiments),
                                          yt_pool=yt_job_options.pool,
                                          use_porto_layer=not usix.is_arcadia(),
                                          tentative_job_spec=yt_job_options.get_tentative_job_spec())
    if add_acl:
        yt_spec["acl"] = [
            {
                "subjects": ["idm-group:44732"],
                "action": "allow",
                "permissions": ["read"],
            },
        ]

    logging.info(
        "Will calculate metric for %d experiment %s\nusing %d tables %s",
        len(experiments),
        umisc.to_lines(experiments),
        len(source_tables),
        umisc.to_lines(source_tables),
    )

    time_start = time.time()
    try:
        yt.run_map_reduce(
            mapper=mapper,
            reducer=reducer,
            source_table=source_tables,
            destination_table=destination_tables,
            reduce_by=["_experiment", "yuid"],
            sort_by=["_experiment", "yuid", "ts", "action_index"],
            spec=yt_spec,
            client=client,
            reduce_local_files=files,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )
    except yt.YtOperationFailedError as e:
        if "stderrs" in e.attributes:
            e.message = e.message + "\n" + yt.format_operation_stderrs(e.attributes["stderrs"])
        raise
    time_end = time.time()

    logging.info(
        "%d experiments calculated in %s: %s",
        len(experiments),
        datetime.timedelta(seconds=time_end - time_start),
        umisc.to_lines(experiments),
    )


def get_result_table_path(tmp_table, copy_result_to):
    """
    :type tmp_table: str
    :type copy_result_to: str | None
    :rtype str | None
    """
    if copy_result_to:
        table_name = os.path.basename(tmp_table).rsplit("_", 1)[0]
        return yt.ypath_join(copy_result_to, table_name)
    else:
        return None


def aggregate_rows(rows):
    user_id = None
    metric_id = None
    bucket = None
    current_values = []
    current_result = {}
    for row in rows:
        if user_id is None:
            user_id = row["user_id"]
            metric_id = row["metric_id"]
            bucket = row.get("bucket")
            current_values.append(row["value"])
        else:
            if user_id == row["user_id"]:
                if metric_id != row["metric_id"]:
                    current_result[metric_id] = current_values
                    current_values = []
                    metric_id = row["metric_id"]
                current_values.append(row["value"])
            else:
                current_result[metric_id] = current_values
                current_values = []
                metric_id = row["metric_id"]
                user_id = row["user_id"]
                current_values.append(row["value"])
                if bucket:
                    current_result["bucket"] = bucket
                bucket = row.get("bucket")
                yield current_result
                current_result = {}
    current_result[metric_id] = current_values
    if bucket:
        current_result["bucket"] = bucket
    yield current_result


def get_metric_results_single(
        calculator,
        experiment,
        tmp_table,
        tmp_dir,
        tmp_local_file,
        output_files,
        result_options,
        split_metric_results,
):
    """
    :type calculator: MetricCalculator
    :type experiment: ExperimentForCalculation
    :type tmp_table: str
    :type tmp_dir: str
    :type tmp_local_file: str
    :type output_files: dict[int, str]
    :type result_options: ResultOptions
    :type split_metric_results: bool
    :rtype: list[MetricResult]
    """
    testid = experiment.testid
    dates = experiment.dates

    result_table_path = get_result_table_path(tmp_table, result_options.copy_result_to)

    experiment_name = "{} {}".format(testid, dates)
    if result_options.results_only:
        results = calculator.read_metric_results(
            rows=[],
            experiment_name=experiment_name,
            output_files=output_files,
            table_path=result_table_path,
        )

    else:
        logging.info(
            "Will parse results for experiment %s %s from %s",
            testid,
            dates,
            tmp_table,
        )

        tmp_file_path = get_tmp_local_file_path(tmp_dir, tmp_local_file)

        if "DEBUG_FILE_INFO" in os.environ:
            size = ufile.get_file_size(tmp_file_path)
            line_count = sum(1 for _ in ufile.fopen_read(tmp_file_path))
            logging.info("Debug input file info:\n\tname: %s\n\tsize: %i\n\tline count: %i", tmp_file_path, size, line_count)

        time_start = time.time()
        with ufile.fopen_read(tmp_file_path) as fd:
            if split_metric_results:
                rows = aggregate_rows(map(ujson.load_from_str, fd))
            else:
                rows = map(ujson.load_from_str, fd)
            results = calculator.read_metric_results(
                rows=rows,
                experiment_name=experiment_name,
                output_files=output_files,
                table_path=result_table_path,
            )
        time_end = time.time()

        logging.info(
            "Experiment %s %s was parsed in %s",
            testid,
            dates,
            datetime.timedelta(seconds=time_end - time_start),
        )

    return results


def get_metric_result_single_wrapper(tuple_of_data, calculator, result_options, tmp_dir, split_metric_results):
    """
    :type tuple_of_data: tuple(ExperimentForCalculation, str, dict[int, str])
    :type calculator: MetricCalculator
    :type result_options: ResultOptions
    :type tmp_dir: str
    :type split_metric_results: bool
    :rtype: (ExperimentForCalculation, list[MetricResult] | None, str | None)
    """
    experiment, tmp_table, tmp_local_file, output_files = tuple_of_data
    try:
        result = get_metric_results_single(
            calculator=calculator,
            experiment=experiment,
            tmp_table=tmp_table,
            tmp_dir=tmp_dir,
            tmp_local_file=tmp_local_file,
            output_files=output_files,
            result_options=result_options,
            split_metric_results=split_metric_results,
        )
        return experiment, result, None
    except BaseException as exc:
        logging.info("Error in get_metric_result_single for %s: %s, %s", experiment, exc, traceback.format_exc())
        return experiment, None, umisc.exception_to_string(exc)


def merge_wrapper(tuple_of_data, yt_job_options, client, transform=True):
    """
    :type tuple_of_data: tuple(ExperimentForCalculation, str)
    :type yt_job_options: YtJobOptions
    :type client: yt.client.Yt
    :type transform: bool
    :rtype yt.Operation
    """
    experiment, result_table_path = tuple_of_data
    title = get_metric_operation_title([experiment])
    merge_spec = yt_io.get_yt_operation_spec(title="merge " + title,
                                             combine_chunks=True,
                                             force_transform=True if transform else None,
                                             yt_pool=yt_job_options.pool,
                                             use_porto_layer=False)
    if transform:
        yt.set(result_table_path + "/@compression_codec", "brotli_5", client=client)
        yt.set(result_table_path + "/@erasure_codec", "lrc_12_2_2", client=client)
    return yt.run_merge(
        result_table_path,
        result_table_path,
        spec=merge_spec,
        client=client,
        sync=False,
    )


def prepare_arguments_for_merging(experiments, tmp_tables, copy_result_to):
    """
    :type experiments: list[ExperimentForCalculation]
    :type tmp_tables: list[str]
    :type copy_result_to: str
    :rtype list[tuple(ExperimentForCalculation, str)]
    """
    arguments = []
    for experiment, tmp_table in zip(experiments, tmp_tables):
        arguments.append((experiment, get_result_table_path(tmp_table, copy_result_to)))
    return arguments


def save_results(tmp_tables,
                 experiments,
                 result_options,
                 yt_job_options,
                 client,
                 download_threads,
                 transform=True):
    """
    :type tmp_tables: list[str]
    :type experiments: list[ExperimentForCalculation]
    :type result_options: ResultOptions
    :type yt_job_options: YtJobOptions
    :type client: yt.client.Yt
    :type download_threads: int
    :type transform: bool
    """
    logging.info("Will save results to %s", result_options.copy_result_to)
    time_start = time.time()
    result_tables_paths = []
    for tmp_table, experiment in zip(tmp_tables, experiments):
        result_table_path = get_result_table_path(tmp_table, result_options.copy_result_to)
        result_tables_paths.append(result_table_path)
        logging.info(
            "Experiment %s %s will be saved in %s",
            experiment.testid,
            experiment.dates,
            result_table_path,
        )
        if result_options.results_ttl:
            preserve_expiration_time = True
        else:
            preserve_expiration_time = False
        yt.move(
            tmp_table,
            result_table_path,
            recursive=False,
            force=True,
            preserve_account=False,
            preserve_expiration_time=preserve_expiration_time,
            client=client,
        )
    arguments = prepare_arguments_for_merging(experiments, tmp_tables, result_options.copy_result_to)
    worker = functools.partial(
        merge_wrapper,
        yt_job_options=yt_job_options,
        client=client,
        transform=transform,
    )
    if transform:
        logging.info("Will combine chunks of saved tables and compress them")
    else:
        logging.info("Will combine chunks of saved tables")
    mstand_uyt.async_yt_operations_poll(worker, arguments, download_threads)
    time_end = time.time()

    logging.info("Tables have been saved in %s: %s\nto: %s", datetime.timedelta(seconds=time_end - time_start),
                 umisc.to_lines(tmp_tables), umisc.to_lines(result_tables_paths))


def remove_tables(tmp_tables, client):
    """
    :type tmp_tables: list[str]
    :type client: yt.client.Yt
    """
    logging.info("Removing temporary tables: %s", umisc.to_lines(tmp_tables))
    for tmp_table in tmp_tables:
        yt.remove(tmp_table, client=client)
    logging.info("Temporary tables were removed")


def prepare_arguments_for_parsing(experiments, tmp_tables, tmp_local_files, result_files):
    """
    :type experiments: list[ExperimentForCalculation]
    :type tmp_tables: list[str]
    :type tmp_local_files: list[str]
    :type result_files: dict[ExperimentForCalculation, dict[int, str]]
    :rtype list[tuple(ExperimentForCalculation, str, dict[int, str] | None)]
    """
    arguments = []
    for exp, tmp, tmp_local_file in zip(experiments, tmp_tables, tmp_local_files):
        output_files = result_files.get(exp)
        arguments.append((exp, tmp, tmp_local_file, output_files))
    return arguments


def get_metric_results_batch(
        calculator,
        experiments,
        tmp_tables,
        result_files,
        client,
        yt_job_options,
        result_options,
        download_threads,
        transaction,
        use_yt_cli,
):
    """
    :type calculator: MetricCalculator
    :type experiments: list[ExperimentForCalculation]
    :type tmp_tables: list[str]
    :type result_files: dict[ExperimentForCalculation, dict[int, str]]
    :type client: yt.client.Yt
    :type yt_job_options: YtJobOptions
    :type result_options: ResultOptions
    :type download_threads: int
    :type transaction: yt.Transaction
    :type use_yt_cli: bool
    :rtype: dict[ExperimentForCalculation, list[MetricResult]]
    """
    if result_options.copy_result_to:
        all_results = get_metric_results_batch_fake(
            calculator=calculator,
            experiments=experiments,
            result_options=result_options,
            split_metric_results=result_options.split_metric_results,
            tmp_tables=tmp_tables,
        )
        save_results(tmp_tables, experiments, result_options, yt_job_options, client, download_threads)

    else:
        logging.info(
            "Will download and parse results for %d experiments: %s\n from tables: %s",
            len(experiments),
            umisc.to_lines(experiments),
            umisc.to_lines(tmp_tables),
        )

        assert len(experiments) == len(tmp_tables)
        count = len(experiments)

        time_start = time.time()
        tmp_dir, tmp_local_files = download_raw_tables(tmp_tables,
                                                       experiments,
                                                       client,
                                                       transaction,
                                                       download_threads,
                                                       use_yt_cli)
        worker = functools.partial(
            get_metric_result_single_wrapper,
            calculator=calculator,
            result_options=result_options,
            tmp_dir=tmp_dir,
            split_metric_results=result_options.split_metric_results,
        )
        arguments = prepare_arguments_for_parsing(experiments=experiments,
                                                  tmp_tables=tmp_tables,
                                                  tmp_local_files=tmp_local_files,
                                                  result_files=result_files)
        logging.info("Trying to parse experiment results using %s threads", download_threads)
        # TODO: extract parsing from transaction
        results_iter = umisc.par_imap_unordered(worker, arguments, max_threads=1)
        all_results = {}
        for pos, (exp, result, error) in enumerate(results_iter):
            if error is not None:
                raise Exception(error)
            all_results[exp] = result
            umisc.log_progress("metric results parsing", pos, count)
        # TODO: guarantee removing of tables in case of errors
        remove_tmp_local_files(tmp_dir, tmp_local_files)
        time_end = time.time()

        logging.info(
            "%d experiments downloaded and parsed in %s: %s",
            count,
            datetime.timedelta(seconds=time_end - time_start),
            umisc.to_lines(experiments),
        )

        debug_yt_copy_result_path = os.environ.get("DEBUG_YT_COPY_RESULT_PATH")
        if debug_yt_copy_result_path:
            path_with_ts = os.path.join(debug_yt_copy_result_path, str(time.time()))
            yt.create("map_node", path_with_ts, recursive=True)
            debug_result_options = ResultOptions(
                copy_result_to=path_with_ts,
                results_only=True,
            )
            save_results(tmp_tables, experiments, debug_result_options, yt_job_options, client, download_threads)

        else:
            remove_tables(tmp_tables, client)

    return all_results


def get_metric_results_batch_fake(
        calculator,
        experiments,
        result_options,
        split_metric_results,
        tmp_tables,
):
    """
    :type calculator: MetricCalculator
    :type experiments: list[ExperimentForCalculation]
    :type result_options: ResultOptions
    :type split_metric_results: bool
    :type tmp_tables: list[str]
    :rtype: dict[ExperimentForCalculation, list[MetricResult]]
    """
    all_results = {}
    for exp, tmp in zip(experiments, tmp_tables):
        all_results[exp] = get_metric_results_single(
            calculator=calculator,
            experiment=exp,
            tmp_table=tmp,
            tmp_dir="",
            tmp_local_file="",
            output_files={},
            result_options=result_options,
            split_metric_results=split_metric_results,
        )

    return all_results


def create_temp_table(experiment, client, ttl_days, schema):
    """
    :type experiment: ExperimentForCalculation
    :type client: yt.client.Yt
    :type ttl_days: int | None
    :type schema: list | None
    :rtype: str
    """
    prefix = "mstand_calc_results_{}_{}_{}_{}_".format(
        experiment.observation.id,
        experiment.testid,
        utime.format_date(experiment.dates.start),
        utime.format_date(experiment.dates.end),
    )
    if ttl_days:
        ttl_ms = ttl_days * 1000 * 60 * 60 * 24
    else:
        ttl_ms = None

    attributes = {"schema": schema} if schema else None

    return yt.create_temp_table("//tmp", prefix, client=client, expiration_timeout=ttl_ms, attributes=attributes)


def ensure_yt_dir_exists(path):
    if not yt.exists(path):
        logging.info("Folder %s doesn't exists. Try to create it.", path)
        yt.create("map_node", path=path, recursive=True)
        logging.info("Folder %s was created.", path)
    else:
        node_type = yt.get_type(path)
        assert node_type == "map_node", "the path must point to a folder, not a {}".format(node_type)
        logging.info("Folder %s exists.", path)


class MetricBackendYT(object):
    ONLINE_LAMPS_TABLE = "lamps_cache"

    def __init__(
            self,
            config,
            yt_job_options,
            result_options,
            download_threads,
            allow_empty_tables=False,
            use_yt_cli=None,
            split_metric_results=False,
            add_acl=True,
    ):
        """
        :type config: dict[str]
        :type yt_job_options: YtJobOptions
        :type result_options: ResultOptions
        :type download_threads: int
        :type allow_empty_tables: bool
        :type use_yt_cli: bool | None
        :type split_metric_results: bool
        :type add_acl: bool
        """
        self._config = config
        self._yt_job_options = yt_job_options
        self._result_options = result_options

        if self._result_options.split_values_and_schematize:
            assert self._result_options.copy_result_to, "cannot use --split-values-and-schematize without --copy-results-to-yt-dir"
            assert self._result_options.results_only, "cannot use --split-values-and-schematize without --yt-results-only"

        if not self._result_options.copy_result_to:
            assert not self._result_options.results_only, "cannot use --yt-results-only without --copy-results-to-yt-dir"
            assert not self._result_options.results_ttl, "cannot use --yt-results-ttl without --copy-results-to-yt-dir"
        else:
            ensure_yt_dir_exists(self._result_options.copy_result_to)

        self.download_threads = download_threads
        self.allow_empty_tables = allow_empty_tables
        if use_yt_cli is None:
            self.use_yt_cli = self.check_yt_cli_availability()
        else:
            self.use_yt_cli = use_yt_cli
        self.split_metric_results = split_metric_results
        self.add_acl = add_acl

    @staticmethod
    def check_yt_cli_availability():
        try:
            umisc.run_command(["yt", "--version"])
            return True
        except:
            return False

    @staticmethod
    def from_cli_args(cli_args, yt_config):
        return MetricBackendYT(
            config=yt_config,
            yt_job_options=YtJobOptions.from_cli_args(cli_args),
            result_options=ResultOptions.from_cli_args(cli_args),
            download_threads=cli_args.download_threads,
            allow_empty_tables=cli_args.allow_empty_tables,
            split_metric_results=cli_args.split_metric_results,
            use_yt_cli=cli_args.use_yt_cli,
        )

    def find_bad_tables(self, tables):
        """
        :type tables: list[str]
        :rtype: list[str]
        """
        client = self._get_client()
        not_exists = []
        empty = []
        for path in tables:
            if not yt.exists(path, client=client):
                not_exists.append(path)
            elif yt.is_empty(path, client=client):
                empty.append(path)

        if empty:
            logging.warning("Following tables are empty: %s", umisc.to_lines(empty))
            if not self.allow_empty_tables:
                raise Exception("Empty tables are not allowed, use '--allow-empty-tables' if they are necessary")

        return not_exists

    def get_all_versions(self, paths):
        """
        :type paths: list[str]
        :rtype: dict[str, SqueezeVersions]
        """
        return versions_yt.get_all_versions(paths, client=self._get_client())

    @property
    def require_threads_for_paralleling(self):
        return self.download_threads > 1

    def calc_metric_batch(self, experiments, calculator, result_files):
        """
        :type experiments: dict[ExperimentForCalculation, MetricSources]
        :type calculator: MetricCalculator
        :type result_files: dict[ExperimentForCalculation, dict[int, str]]
        :rtype: dict[ExperimentForCalculation, list[MetricResult]]
        """
        return metric_calculation_batch(
            experiments=experiments,
            calculator=calculator,
            result_files=result_files,
            client=self._get_client(),
            yt_job_options=self._yt_job_options,
            result_options=self._result_options,
            download_threads=self.download_threads,
            use_yt_cli=self.use_yt_cli,
            add_acl=self.add_acl,
        )

    def _get_client(self):
        if self._config is None:
            return None
        return yt.client.Yt(config=self._config)

    def _read_lamps_cache(self, path):
        """
        :type path: str
        :rtype: generator
        """
        client = self._get_client()
        full_path = os.path.join(path, self.ONLINE_LAMPS_TABLE)
        logging.info("Getting cached lamps from {}".format(full_path))
        table = yt.TablePath(full_path, append=True, client=client)
        table_exists = yt.exists(table, client=client)

        if not table_exists:
            logging.warning("Table {} not found, creating one".format(full_path))
            yt.create("table", table, client=client)

        table = yt.read_table(table, client=client)
        return table

    def get_lamps_from_cache(self, pool_lamp_keys, path):
        """
        :type path: str
        :type pool_lamp_keys: set[LampKey]
        :rtype: list[tuple[LampKey, list[MetricResult]]]
        """
        table = self._read_lamps_cache(path)
        rows = []

        for row in table:
            values = [MetricResult.deserialize(i) for i in row["values"]]
            key = LampKey.deserialize(row["key"])
            if key in pool_lamp_keys:
                rows.append((key, values))

        return rows

    def write_lamps_to_cache(self, lamps, path):
        """
        :type path: str
        :type lamps: Pool
        """
        full_path = os.path.join(path, self.ONLINE_LAMPS_TABLE)
        logging.info("Caching lamps, path: {}".format(full_path))
        client = self._get_client()
        table = yt.TablePath(full_path, append=True, client=client)
        all_lamps = []
        for lamp in lamps.lamps:
            key = lamp.lamp_key
            values = lamp.lamp_values
            yson_key = key.serialize()
            yson_list_values = yson.YsonList([val.serialize() for val in values])
            all_lamps.append({"key": yson_key, "values": yson_list_values})

        with yt.Transaction(client=client):
            yt.lock(table, mode="shared", client=client)
            yt.write_table(table, all_lamps, client=client)
