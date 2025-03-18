# coding=utf-8
import itertools
import logging

import yaqutils.time_helpers as utime
from experiment_pool import CriteriaResult
from experiment_pool import Experiment
from experiment_pool import MetricColoring
from experiment_pool import MetricDataType
from experiment_pool import MetricResult
from experiment_pool import MetricType
from experiment_pool import MetricValues
from experiment_pool import Observation
from experiment_pool import Pool
from user_plugins import PluginKey


def get_coloring(color_info, metric_name):
    if metric_name in color_info:
        return color_info.get(metric_name).get('coloring', 'more-is-better')
    return 'more-is-better'


def create_metric_result(data, metric_name, average, pvalue, invalid_days, color_info):
    metric_key = PluginKey(name=metric_name)

    metric_values = MetricValues(
        count_val=None,
        average_val=average,
        sum_val=None,
        data_type=MetricDataType.VALUES)

    metric_result = MetricResult(
        metric_key=metric_key,
        metric_values=metric_values,
        metric_type=MetricType.ONLINE,
        coloring=get_coloring(color_info, metric_name),
        invalid_days=invalid_days
    )
    if pvalue is not None:
        criteria_name = data.get('criteria_name', 'unknown')
        criteria_key = PluginKey(name=criteria_name)
        crit_result = CriteriaResult(criteria_key, pvalue)
        metric_result.criteria_results.append(crit_result)

    return metric_result


def deserialize_observation(data, color_info):
    obs_id = data['observation_id'] if 'observation_id' in data else None
    date_from = utime.parse_date_msk(data.get('date_from'))
    date_to = utime.parse_date_msk(data.get('date_to'))
    obs_dates = utime.DateRange(date_from, date_to)

    testids = [str(testid) for testid in data.get('experiments', [])]
    serpset_ids = [str(serpset_id) for serpset_id in data.get('serpsets', [])]

    control = Experiment(
        testid=testids[0] if len(testids) > 0 else None,
        serpset_id=serpset_ids[0] if len(serpset_ids) > 0 else None,
    )

    metric_name = data.get('metric_name')
    if metric_name:
        averages = data.get('average', [])
        if not averages:
            raise Exception("Pool has metric_name, but no metric results!")
        metric_averages = [float(average) for average in averages]
        pvalues = [float(pvalue) for pvalue in data.get('pvalues', [])]
        # Для старых пулов считаем, что invalid_days одинаковое для всех экспериментов.
        # Это не совсем правильно, т.к. в старых пулах записывается суммарное число дней,
        # в которые не посчитался хотя бы один эксперимент, но понять, какой эксперимент
        # когда не посчитался, уже не получится.
        invalid_days = int(data.get('invalid_days', 0))
        control_average = metric_averages[0]
        metric_key = PluginKey(name=metric_name)
        metric_values = MetricValues(count_val=None,
                                     sum_val=None,
                                     average_val=control_average,
                                     data_type=MetricDataType.VALUES)

        control_result = MetricResult(metric_key=metric_key,
                                      metric_values=metric_values,
                                      coloring=MetricColoring.NONE,
                                      metric_type=MetricType.ONLINE,
                                      invalid_days=invalid_days)
        control.add_metric_result(control_result)
    else:
        metric_averages = []
        pvalues = []
        invalid_days = 0

    obs = Observation(obs_id, obs_dates, control)

    for testid, serpset_id, average, pvalue in itertools.zip_longest(testids[1:], serpset_ids[1:], metric_averages[1:], pvalues):
        exp = Experiment(testid=testid, serpset_id=serpset_id)
        obs.experiments.append(exp)
        if average is None:
            logging.info("No average value in %s, not adding metric results", exp)
            continue
        exp_result = create_metric_result(data, metric_name, average, pvalue, invalid_days, color_info)
        exp.add_metric_result(exp_result)

    return obs


def deserialize_observation_repeated(obs, data, color_info):
    metric_name = data.get('metric_name')
    if metric_name is None:
        logging.info("No metric name in observation %s", obs)
        return

    averages = [float(average) for average in data.get('average', [])]
    control_average = averages[0]
    exp_averages = averages[1:]
    pvalues = [float(pvalue) for pvalue in data.get('pvalues', [])]
    invalid_days = int(data.get('invalid_days', 0))

    metric_values = MetricValues(count_val=None,
                                 sum_val=None,
                                 average_val=control_average,
                                 data_type=MetricDataType.VALUES)
    metric_key = PluginKey(name=metric_name)
    control_result = MetricResult(metric_key=metric_key,
                                  metric_values=metric_values,
                                  coloring=MetricColoring.NONE,
                                  invalid_days=invalid_days,
                                  metric_type=MetricType.ONLINE)
    obs.control.add_metric_result(control_result)

    num_experiments = len(obs.experiments)
    num_averages = len(exp_averages)
    num_pvalues = len(pvalues)
    logging.debug(
        "Observation %s: experiments: %d, averages: %d, pvalues: %d",
        obs,
        num_experiments,
        num_averages,
        num_pvalues
    )

    if num_experiments < num_averages:
        raise Exception("Too many metric results")
    if num_averages < num_pvalues:
        raise Exception("Too many p-values")

    for exp, average, pvalue in itertools.zip_longest(obs.experiments, exp_averages, pvalues):
        if average is None:
            continue
        metric_values = MetricValues(count_val=None,
                                     sum_val=None,
                                     average_val=average,
                                     data_type=MetricDataType.VALUES)
        metric_key = PluginKey(name=metric_name)
        metric_result = MetricResult(
            metric_key=metric_key,
            metric_values=metric_values,
            metric_type=MetricType.ONLINE,
            coloring=get_coloring(color_info, metric_name),
            invalid_days=invalid_days
        )
        if pvalue is not None:
            criteria_name = data.get('criteria_name', 'unknown')
            criteria_key = PluginKey(name=criteria_name)
            crit_result = CriteriaResult(criteria_key, pvalue)
            metric_result.criteria_results.append(crit_result)
        exp.add_metric_result(metric_result)


def observation_key(data):
    return (
        data.get('observation_id'),
        data.get('date_to'),
        data.get('date_from'),
        data.get('experiment_date_to'),
        data.get('experiment_date_from'),
        tuple(data.get('experiments', [])),
        tuple(data.get('serpsets', []))
    )


def deserialize_pool(data):
    logging.warning("WARNING: You are operating with old pool. Consider transition to new pool format.")
    pool = Pool()
    observations_by_key = {}
    color_info = data.get('metrics', {})
    for observation in data['observations']:
        key = observation_key(observation)
        # В старых пулах есть случаи, когда одно наблюдение упоминается в пуле несколько раз, т.к. на нем посчитано
        # несколько метрик. Здесь мы пытаемся отловить такие наблюдения и собрать их в одно с несколькими метриками.
        if key in observations_by_key:
            deserialize_observation_repeated(observations_by_key[key], observation, color_info)
        else:
            obs = deserialize_observation(observation, color_info)
            observations_by_key[key] = obs
            pool.observations.append(obs)
    return pool
