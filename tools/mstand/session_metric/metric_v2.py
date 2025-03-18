import logging
import math
import os
import random
import subprocess
import tempfile

import yaqutils.file_helpers as ufile

from experiment_pool import CriteriaResult
from experiment_pool import MetricDataType
from experiment_pool import MetricResult
from experiment_pool import MetricType
from experiment_pool import MetricValues
from experiment_pool import MetricValueType
from experiment_pool import Deviations
from user_plugins import PluginKey


def calc_v2_ttest(data_file_name, apply_file_name):
    with ufile.fopen_read(data_file_name, use_unicode=False) as file_data:
        with ufile.fopen_read(apply_file_name, use_unicode=False) as file_apply:
            control_count = 0
            control_sum = 0
            control_sum2 = 0
            experiment_count = 0
            experiment_sum = 0
            experiment_sum2 = 0
            for line in file_data:
                apply_line = file_apply.readline().strip()
                apply_values = apply_line.split('\t')
                apply_value = float(apply_values[4])
                line_values = line.split('\t')
                target_value = float(line_values[1])
                diff_value = target_value - apply_value
                if int(line_values[0]) == 0:
                    control_count += 1
                    control_sum += diff_value
                    control_sum2 += diff_value * diff_value
                else:
                    experiment_count += 1
                    experiment_sum += diff_value
                    experiment_sum2 += diff_value * diff_value
            if control_count < 2 or experiment_count < 2:
                diff = 0
                pvalue = 1
                deviations = Deviations(std_exp=0, std_control=0, std_diff=0)
            else:
                diff = experiment_sum / experiment_count - control_sum / control_count
                experiment_dispersion = ((experiment_sum2 - (experiment_sum ** 2) /
                                          experiment_count) / (experiment_count ** 2))
                control_dispersion = (control_sum2 - (control_sum ** 2) / control_count) / (control_count ** 2)
                diff_dispersion = experiment_dispersion + control_dispersion
                pvalue = math.erfc(abs(diff) / math.sqrt(diff_dispersion) / math.sqrt(2))
                deviations = Deviations(std_exp=math.sqrt(experiment_dispersion),
                                        std_control=math.sqrt(control_dispersion),
                                        std_diff=math.sqrt(diff_dispersion))
            criteria = CriteriaResult(PluginKey('TTest'), pvalue=pvalue, deviations=deviations)
            metric_values = MetricValues(
                count_val=1,
                sum_val=diff,
                average_val=diff,
                data_type=MetricDataType.VALUES,
                value_type=MetricValueType.AVERAGE,
                row_count=1
            )
            return criteria, metric_values


def gbdt_learn_all(algorithm_params, data_file_name, model_file_name):
    subprocess.check_call(['./gbdt',
                           '-f', data_file_name,
                           '-o', model_file_name,
                           '-a', data_file_name,
                           '-w', str(algorithm_params['learning_rate']) if algorithm_params else '0.05',
                           '-n', str(algorithm_params['learning_nodes']) if algorithm_params else '30',
                           '-i', str(algorithm_params['learning_trees']) if algorithm_params else '100',
                           '--minsamplesinnode', '100',
                           '-T', '4',
                           '-m', 'MSE',
                           '-r', '123'])


def gbdt_learn_sample(algorithm_params, data_file_name, model_file_name, rows, all_rows):
    with tempfile.NamedTemporaryFile(prefix='data-sample-', suffix='.txt') as data_file_sample:
        random.seed(123)
        data_file_sample_name = data_file_sample.name
        for line in open(data_file_name, 'r'):
            if all_rows > 0 and random.randint(0, all_rows - 1) < rows:
                data_file_sample.write(line)
                rows -= 1
            all_rows -= 1
        data_file_sample.flush()
        logging.info('Sample size ' + str(os.fstat(data_file_sample.fileno()).st_size / (2 ** 20)) + 'Mb')
        subprocess.check_call(['./gbdt',
                               '-f', data_file_sample_name,
                               '-o', model_file_name,
                               '-a', data_file_name,
                               '-w', str(algorithm_params['learning_rate']) if algorithm_params else '0.05',
                               '-n', str(algorithm_params['learning_nodes']) if algorithm_params else '30',
                               '-i', str(algorithm_params['learning_trees']) if algorithm_params else '100',
                               '--minsamplesinnode', '100',
                               '-T', '4',
                               '-m', 'MSE',
                               '-r', '123'])


def matrixnet_learn_all(algorithm_params, data_file_name):
    subprocess.check_call([
        './matrixnet',
        '-f', data_file_name,
        '-w', str(algorithm_params['learning_rate']) if algorithm_params else '0.05',
        '-n', str(algorithm_params['learning_depth']) if algorithm_params else '6',
        '-i', str(algorithm_params['learning_trees']) if algorithm_params else '100',
        '--target', 'MSE',
        '-T', '4',
        '--sDebug', '-s', '4',
        '-r', '123',
    ])
    subprocess.check_call([
        './matrixnet',
        '--apply',
        '-f', data_file_name,
    ])


def matrixnet_learn_sample(algorithm_params, data_file_name, rows, all_rows):
    with tempfile.NamedTemporaryFile(prefix='data-sample-', suffix='.txt') as data_file_sample:
        random.seed(123)
        # data_file_sample_name = data_file_sample.name
        for line in open(data_file_name, 'r'):
            if all_rows > 0 and random.randint(0, all_rows - 1) < rows:
                data_file_sample.write(line)
                rows -= 1
            all_rows -= 1
        data_file_sample.flush()
        logging.info('Sample size ' + str(os.fstat(data_file_sample.fileno()).st_size / (2 ** 20)) + 'Mb')
        subprocess.check_call([
            './matrixnet',
            '-f', data_file_sample,
            '-w', str(algorithm_params['learning_rate']) if algorithm_params else '0.05',
            '-n', str(algorithm_params['learning_depth']) if algorithm_params else '6',
            '-i', str(algorithm_params['learning_trees']) if algorithm_params else '100',
            '--target', 'MSE',
            '-T', '4',
            '--sDebug', '-s', '4',
            '-r', '123',
        ])
        subprocess.check_call([
            './matrixnet',
            '--apply',
            '-f', data_file_name,
        ])


def calc_v2_metric_result(
        metric_key,
        coloring,
        records,
        data_file_name,
        model_file_name,
        dbg_info,
        algorithm,
        algorithm_params,
):
    """
    :type metric_key: user_plugins.PluginKey
    :type coloring: str
    :type records: int
    :type data_file_name: str
    :type model_file_name: str
    :type dbg_info: str
    :type algorithm: str
    :type algorithm_params: dict
    :return: MetricResult
    """
    max_data_size = 2000 * (2 ** 20)
    data_size = os.stat(data_file_name).st_size
    logging.info('Data size ' + str(data_size / (2 ** 20)) + 'Mb')
    if algorithm == 'gbdt':
        if data_size <= max_data_size:
            gbdt_learn_all(algorithm_params, data_file_name, model_file_name)
        else:
            gbdt_learn_sample(algorithm_params, data_file_name, model_file_name,
                              (records * max_data_size) / data_size, records)
        apply_file_name = data_file_name + '.apply'
    elif algorithm == 'matrixnet':
        if data_size <= max_data_size:
            matrixnet_learn_all(algorithm_params, data_file_name)
        else:
            matrixnet_learn_sample(algorithm_params, data_file_name,
                                   (records * max_data_size) / data_size, records)
        apply_file_name = data_file_name + '.matrixnet'
    else:
        assert False
    logging.info('Learning complete ' + dbg_info)
    criteria_result, metric_values = calc_v2_ttest(data_file_name, apply_file_name)
    logging.info('Calc metric complete ' + dbg_info)
    os.remove(apply_file_name)
    return MetricResult(
        metric_key=metric_key,
        metric_type=MetricType.ONLINE,
        metric_values=metric_values,
        coloring=coloring,
        criteria_results=[criteria_result]
    )


def load_data_from_file(
        data_file,
        data_input_file,
        side_idx,
        algorithm,
):
    """
    :type data_file: file
    :type data_input_file: str
    :type side_idx: int
    :type algorithm: str
    """
    records = 0
    if algorithm == 'gbdt':
        row_format = '%s\t%s\t0\t%s\n'
    else:
        row_format = '%s\t%s\t0\t1\t%s\n'
    with open(data_input_file, 'r') as input_file:
        for line in input_file:
            fields = line.strip().split('\t')
            if len(fields) < 3:
                continue
            if fields[0] != "1":
                logging.error('1 target expect %s found' % fields[0])
                return
            target = fields[1]
            features_count = int(fields[2])
            features = fields[3:3+features_count]
            data_file.write(row_format % (side_idx, target, '\t'.join(features)))
            records += 1
    data_file.flush()
    return records


def calc_v2_on_files(
        metric_key,
        coloring,
        control_file,
        experiment_file,
        algorithm,
        algorithm_params,
):
    """
    :type metric_key: user_plugins.PluginKey
    :type coloring: str
    :type control_file: str
    :type experiment_file: str
    :type algorithm: str
    :type algorithm_params: dict
    :return: MetricResult
    """
    logging.info('calc_v2 for ' + control_file + '/' + experiment_file)
    with tempfile.NamedTemporaryFile(prefix='mstand-', suffix='.txt') as data_file:
        data_file_name = data_file.name
        with tempfile.NamedTemporaryFile(prefix='mstand-model-', suffix=".txt") as model_file:
            model_file_name = model_file.name
            records = load_data_from_file(data_file=data_file, data_input_file=control_file, side_idx=0, algorithm=algorithm)
            logging.info('Loaded control users features ' + control_file)
            records += load_data_from_file(data_file=data_file, data_input_file=experiment_file, side_idx=1, algorithm=algorithm)
            logging.info('Loaded experiment users features ' + experiment_file)

            return calc_v2_metric_result(
                metric_key=metric_key,
                coloring=coloring,
                records=records,
                data_file_name=data_file_name,
                model_file_name=model_file_name,
                dbg_info='(' + control_file + '/' + experiment_file + ')',
                algorithm=algorithm,
                algorithm_params=algorithm_params
            )


def metric_v2_experiment_key(experiment, observation):
    return experiment.key(), observation.key()


def calc_metric_for_pool_local(
        pool,
        base_dir,
        algorithm,
        algorithm_params,
):
    """
    :type pool: experiment_pool.pool.Pool
    :type base_dir: str
    :type algorithm: str
    :type algorithm_params: dict
    """
    metric_results = {}
    for observation in pool.observations:
        if not observation.control.testid:
            continue
        logging.info('Start metric calculations for observation ' + str(observation))
        control_file = os.path.join(base_dir, observation.control.metric_results[0].metric_values.data_file)
        metric_key = observation.control.metric_results[0].metric_key
        coloring = observation.control.metric_results[0].coloring
        for experiment in observation.experiments:
            experiment_file = os.path.join(base_dir, experiment.metric_results[0].metric_values.data_file)
            logging.info('Calculating metric for ' + str(observation.control) + '/' + str(experiment))
            experiment_result = calc_v2_on_files(
                metric_key=metric_key,
                coloring=coloring,
                control_file=control_file,
                experiment_file=experiment_file,
                algorithm=algorithm,
                algorithm_params=algorithm_params,
            )
            metric_results[metric_v2_experiment_key(experiment, observation)] = experiment_result
        metric_results[metric_v2_experiment_key(observation.control, observation)] = MetricResult(
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

    for observation in pool.observations:
        if not observation.control.testid:
            continue
        control_key = metric_v2_experiment_key(observation.control, observation)
        observation.control.clear_metric_results()
        observation.control.add_metric_result(metric_results[control_key])
        for experiment in observation.experiments:
            if experiment.testid:
                experiment_key = metric_v2_experiment_key(experiment, observation)
                experiment.clear_metric_results()
                experiment.add_metric_result(metric_results[experiment_key])
