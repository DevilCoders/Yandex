# -*- coding: utf-8 -*-
import functools
import logging
import os
import tempfile
import datetime
import time
import traceback
from multiprocessing.pool import Pool

import pytlib.yt_io_helpers as yt_io

import mstand_utils.mstand_paths_utils as mstand_upaths
import session_metric.metric_quick_check as mqc
import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime

from experiment_pool import ExperimentForCalculation
from experiment_pool import MetricDataType
from experiment_pool import MetricResult
from experiment_pool import MetricType
from experiment_pool import MetricValues
from experiment_pool import Observation
from experiment_pool import Experiment
from experiment_pool import MetricValueType
from metrics_api import ExperimentForAPI
from metrics_api import ObservationForAPI
from metrics_api.online import UserActionForAPI
from metrics_api.online import UserActionsForAPI
from session_metric.metric_runner import generate_metric_result_file_paths
import experiment_pool.pool_helpers as pool_helpers
from session_metric.metric_v2 import calc_v2_metric_result
from yaqlibenums import YtOperationTypeEnum

# пакет на самом деле называется yandex-yt
# noinspection PyPackageRequirements
import yt.wrapper as yt
# noinspection PyPackageRequirements
import yt.wrapper.errors as yt_errors
# noinspection PyPackageRequirements
import yt.wrapper.table_commands as yt_table_commands


class CalcYTv2Reducer(object):
    def __init__(self, metric, experiment, observation):
        """
        :type metric: callable
        :type experiment: Experiment
        :type observation: Observation
        """
        self.metric = metric
        self.experiment = experiment
        self.observation = observation

    def __call__(self, keys, rows):
        yuid = keys['yuid']
        actions = [UserActionForAPI(row['yuid'], row['ts'], row) for row in rows]
        if actions:
            user_experiment = UserActionsForAPI(
                user=yuid,
                actions=actions,
                experiment=ExperimentForAPI(self.experiment),
                observation=ObservationForAPI(self.observation),
            )
            result = self.metric(user_experiment)
            if result:
                yield {
                    'yuid': yuid,
                    'target': result['target'],
                    'values': result['values']
                }


def reduce_testid(
        dates,
        testid,
        filter_hash,
        metric,
        squeeze_path,
        services,
        client,
        yt_pool
):
    if not services:
        services = ["web"]
    else:
        services = sorted(set(services))
    service_str = "_".join(services)

    history_start = dates.start - datetime.timedelta(days=14)
    history_end = dates.start - datetime.timedelta(days=1)

    src_tables = mstand_upaths.get_squeeze_history_paths(
        dates=utime.DateRange(history_start, history_end),
        testid=testid,
        filter_hash=filter_hash,
        testid_dates=dates,
        service=service_str,
        squeeze_path=squeeze_path
    )

    src_tables += mstand_upaths.get_squeeze_paths(
        dates=dates,
        testid=testid,
        filter_hash=filter_hash,
        service=service_str,
        squeeze_path=squeeze_path
    )

    experiment = Experiment(testid=testid)
    observation = Observation(obs_id="1", dates=dates, control=experiment)
    reducer = CalcYTv2Reducer(metric, experiment, observation)

    if hasattr(metric, 'value_type') and getattr(metric, 'value_type') != MetricValueType.AVERAGE:
        raise Exception('Unsupported metric value type: {}'.format(getattr(metric, 'value_type')))

    tmp_prefix = 'mstand_calc_results.{}_{}_{}.'.format(
        testid,
        utime.format_date(dates.start),
        utime.format_date(dates.end),
    )
    if any(not yt.exists(table, client=client) or yt.is_empty(table, client=client)
           for table in src_tables):
        raise Exception('there is no data for experiment {} ({})'.format(testid, dates))
    tmp_table = yt_table_commands.create_temp_table('//tmp', tmp_prefix, client=client)
    logging.info(
        'Will calculate metric for experiment %s (%s) using these tables: %s',
        testid,
        dates,
        src_tables,
    )

    yt_spec = yt_io.get_yt_operation_spec(yt_pool=yt_pool,
                                          operation_executor_types=YtOperationTypeEnum.REDUCE,
                                          enable_input_table_index=True,
                                          use_porto_layer=False)

    time_start = time.time()
    try:
        yt.run_reduce(
            reducer,
            src_tables,
            tmp_table,
            reduce_by=['yuid'],
            client=client,
            spec=yt_spec,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )
    except yt_errors.YtOperationFailedError as e:
        if 'stderrs' in e.attributes:
            e.message = e.message + "\n" + yt.format_operation_stderrs(e.attributes["stderrs"])
        logging.info('Exception in reduce_testid')
        raise
    time_end = time.time()
    logging.info(
        'Experiment %s (%s) calculated in %s',
        testid,
        dates,
        datetime.timedelta(seconds=time_end - time_start),
    )
    return tmp_table


def mp_reduce_testid(
        experiment,
        metric,
        squeeze_path,
        services,
        transaction_id,
        yt_config,
        yt_pool,
):
    dates = experiment.observation.dates
    testid = experiment.experiment.testid
    # noinspection PyBroadException
    try:
        client = yt.client.Yt(config=yt_config)
        with yt.Transaction(transaction_id=transaction_id, client=client):
            return experiment, None, reduce_testid(
                dates=dates,
                testid=testid,
                filter_hash=experiment.observation.filters.filter_hash,
                metric=metric,
                squeeze_path=squeeze_path,
                services=services,
                client=client,
                yt_pool=yt_pool
            )
    except BaseException:
        message = traceback.format_exc()
        logging.info(message)
        return experiment, message, None


def load_data(
        data_file,
        data_table,
        side_idx,
        client
):
    """
    :type data_file: file
    :type data_table: str
    :type side_idx: int
    :type client: yt.YtClient
    """
    logging.info('data_table=%s', data_table)
    rows = yt.read_table(data_table, client=client)
    records = 0
    for row in rows:
        target = row['target']
        features = row['values']
        fields = [str(side_idx), str(target), '0'] + [str(feature) for feature in features]
        data_file.write('\t'.join(fields) + '\n')
        records += 1
    data_file.flush()
    return records


def calc_v2(
        metric_key,
        coloring,
        control_table,
        experiment_table,
        algorithm,
        algorithm_params,
        client
):
    """
    :type metric_key: user_plugins.PluginKey
    :type coloring: str
    :type control_table: str
    :type experiment_table: str
    :type algorithm: str
    :type algorithm_params: dict
    :type client: yt.YtClient
    :return: MetricResult
    """
    logging.info('calc_v2 for ' + control_table + '/' + experiment_table)
    with tempfile.NamedTemporaryFile(prefix='mstand-', suffix='.txt') as data_file:
        data_file_name = data_file.name
        with tempfile.NamedTemporaryFile(prefix='mstand-model-', suffix=".txt") as model_file:
            model_file_name = model_file.name
            records = load_data(data_file=data_file, data_table=control_table, side_idx=0, client=client)
            logging.info('Loaded control users features ' + control_table)
            records += load_data(data_file=data_file, data_table=experiment_table, side_idx=1, client=client)
            logging.info('Loaded experiment users features ' + experiment_table)

            return calc_v2_metric_result(
                metric_key=metric_key,
                coloring=coloring,
                records=records,
                data_file_name=data_file_name,
                model_file_name=model_file_name,
                dbg_info='(' + control_table + '/' + experiment_table + ')',
                algorithm=algorithm,
                algorithm_params=algorithm_params,
            )


def dump_metric(table, tsv_file, client):
    rows = yt.read_table(table, client=client)
    with open(tsv_file, 'w') as tsv:
        for row in rows:
            targets = row['target']
            if not isinstance(targets, list):
                targets = [targets]
            features = row['values']
            fields = ([str(len(targets))] + [str(target) for target in targets] +
                      [str(len(features))] + [str(feature) for feature in features])
            tsv.write('\t'.join(fields) + '\n')


def fake_metric_result(metric_key, data_file, metric):
    return MetricResult(
        metric_key=metric_key,
        metric_type=MetricType.ONLINE,
        metric_values=MetricValues(
            count_val=1,
            sum_val=0,
            average_val=0,
            data_type=MetricDataType.VALUES,
            data_file=os.path.basename(data_file),
            row_count=1
        ),
        coloring=metric.coloring
    )


def calc_mr_for_pool(
        pool,
        metric,
        metric_key,
        squeeze_path,
        services,
        threads,
        client,
        transaction,
        save_to_dir,
        save_to_tar,
        yt_pool,
):
    """
    :type pool: experiment_pool.pool.Pool
    :type metric: callable
    :type metric_key: user_plugins.PluginKey
    :type squeeze_path: str
    :type services: list[str]
    :type threads: int
    :type client: yt.YtClient
    :type transaction: yt.Transaction
    :type save_to_dir: str | None
    :type save_to_tar: str | None
    "type yt_pool str | None
    """
    logging.info('Testing metric on sample data...')
    mqc.check_metric_quick(metric, services)
    logging.info('Metric self-test passed OK')
    experiments = sorted(ExperimentForCalculation.from_pool(pool))
    if not experiments:
        raise Exception("There is nothing to calculate")

    count = len(experiments)
    threads = umisc.get_optimal_process_number(count, threads)
    process_pool = Pool(threads)

    if not save_to_dir and save_to_tar:
        save_to_dir = "{}.content".format(save_to_tar)
    if save_to_dir:
        ufile.make_dirs(save_to_dir)
        result_files = generate_metric_result_file_paths({0: metric_key}, save_to_dir, experiments)
    else:
        result_files = None

    mp_worker = functools.partial(
        mp_reduce_testid,
        metric=metric,
        squeeze_path=squeeze_path,
        services=services,
        transaction_id=transaction.transaction_id,
        yt_config=client.config,
        yt_pool=yt_pool,
    )
    results = []
    for observation in pool.observations:
        if not observation.control.testid:
            continue
        experiments = ExperimentForCalculation.from_observation(observation)
        observation_results = process_pool.map_async(mp_worker, experiments)
        results.append((observation, observation_results))

    if not (save_to_dir or save_to_tar):
        return results, metric_key, metric.coloring

    for observation, observation_results in results:
        logging.info('Waiting for results for observation ' + str(observation))
        tables = observation_results.get()
        logging.info('Received results for observation ' + str(observation))
        for experiment, exception, table in tables:
            if exception is not None:
                raise Exception('Exception while calculating ' + str(experiment))
            tsv_file = result_files[experiment][0]
            logging.info('Reading table ' + str(table))
            dump_metric(table, tsv_file, client)
            logging.info('Reading table ' + str(table) + ' complete')

    for observation in pool.observations:
        if observation.control.testid:
            control_key = ExperimentForCalculation(observation.control, observation)
            observation.control.add_metric_result(
                fake_metric_result(metric_key, result_files[control_key][0], metric)
            )
        for experiment in observation.experiments:
            if experiment.testid:
                experiment_key = ExperimentForCalculation(experiment, observation)
                experiment.add_metric_result(
                    fake_metric_result(metric_key, result_files[experiment_key][0], metric)
                )

    pool_filename = "pool.json"
    pool_path = os.path.join(save_to_dir, pool_filename)
    logging.info('Dumping pool to result directory')
    pool_helpers.dump_pool(pool, pool_path)
    if save_to_tar:
        logging.info("Creating tar archive %s from %s", save_to_tar, save_to_dir)
        ufile.tar_directory(path_to_pack=save_to_dir, tar_name=save_to_tar, dir_content_only=True)
        logging.info("Creating tar archive %s done", save_to_tar)


def calc_metric_for_pool(
        pool,
        mr_result,
        algorithm,
        algorithm_params,
        client,
):
    """
    :type pool: experiment_pool.pool.Pool
    :type mr_result: tuple()
    :type algorithm: str
    :type algorithm_params: dict
    :type client: yt.YtClient
    """
    metric_results = {}

    mr_results, metric_key, coloring = mr_result
    for observation, observation_results in mr_results:
        tables = observation_results.get()
        first_experiment, _, _ = tables[0]
        has_exception = False
        for experiment, exception, table in tables:
            if exception is not None:
                has_exception = True
        if has_exception:
            raise Exception('Exception while calculating ' + str(observation))
        logging.info('Start metric calculations for observation ' + str(observation))
        control = ExperimentForCalculation(observation.control, observation)
        for experiment, exception, table in tables:
            if experiment.key() == control.key():
                control_table = table
        if not control_table:
            continue
        for experiment, exception, experiment_table in tables:
            if experiment.key() != control.key():
                logging.info('Calculating metric for ' + str(control) + '/' + str(experiment))
                experiment_result = calc_v2(
                    metric_key=metric_key,
                    coloring=coloring,
                    control_table=control_table,
                    experiment_table=experiment_table,
                    algorithm=algorithm,
                    algorithm_params=algorithm_params,
                    client=client
                )
                metric_results[experiment.key()] = experiment_result
        metric_results[control.key()] = MetricResult(
            metric_key=metric_key,
            metric_type=MetricType.ONLINE,
            metric_values=MetricValues(
                count_val=1,
                sum_val=0,
                average_val=0,
                data_type=MetricDataType.VALUES,
                row_count=1,
            ),
            coloring=coloring,
        )
        logging.info('Finished metric calculations for observation ' + str(observation))

    for observation in pool.observations:
        if observation.control.testid is None:
            continue
        control_key = ExperimentForCalculation(observation.control, observation).key()
        observation.control.clear_metric_results()
        observation.control.add_metric_result(metric_results[control_key])
        for experiment in observation.experiments:
            if experiment.testid:
                experiment_key = ExperimentForCalculation(experiment, observation).key()
                experiment.clear_metric_results()
                experiment.add_metric_result(metric_results[experiment_key])
