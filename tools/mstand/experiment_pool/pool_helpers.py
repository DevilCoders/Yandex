# coding=utf-8
import collections
import logging
from collections import OrderedDict
from copy import deepcopy

import yaqutils.json_helpers as ujson
import yaqutils.time_helpers as utime
import yaqutils.six_helpers as usix

import experiment_pool.deserializer
from user_plugins import PluginStar
from experiment_pool import Experiment
from experiment_pool import LampResult
from experiment_pool import Observation
from experiment_pool import Pool
from experiment_pool import SbsMetricResult  # noqa
from mstand_structs import LampKey


def load_pool(path=None):
    """
    :type path: str
    :rtype: Pool
    """
    pool_data = ujson.load_from_file(path, force_native_json=True)
    pool = experiment_pool.deserializer.deserialize_pool(pool_data)
    pool.log_stats()
    return pool


def build_pool_from_cli_args(cli_args):
    """
    :type cli_args: argparse.Namespace
    :return: Pool
    """
    date_range = utime.DateRange.from_cli_args(cli_args)
    experiment = Experiment(testid=cli_args.testid)
    observation = Observation(obs_id=None, dates=date_range, control=experiment, services=cli_args.services)
    return Pool(observations=[observation])


def load_and_merge_pools(paths):
    """
    :type paths: list[str]
    :rtype: Pool
    """
    pools = [load_pool(path) for path in paths]
    return merge_pools(pools)


def dump_pool(pool, path=None):
    ujson.dump_json_pretty_via_native_json(pool.serialize(), path)


def merge_metric_results(src, dst):
    src_results = src.get_criteria_results_map()
    dst_results = dst.get_criteria_results_map()
    for src_key, src_result in src_results.items():
        if src_key in dst_results:
            logging.info("Duplicate criteria result %s in metric results %s, %s", src_result, src, dst)
        else:
            dst.add_criteria_result(src_result)


def merge_lamps(src, dst):
    existing_keys = [result.lamp_key for result in src]

    for lamp in dst:
        if lamp.lamp_key not in existing_keys:
            src.append(lamp)
            existing_keys.append(lamp.lamp_key)
        else:
            # вот тут необходимо протаскивать в лампочки версию выжимок, но пока ее нет
            pass


def merge_experiments(src, dst):
    dst.extra_data = merge_field_strict(dst.extra_data, src.extra_data, "{} extra_data".format(dst))
    dst.errors = merge_field_strict(dst.errors, src.errors, "{} errors".format(dst))

    src_results = src.get_metric_results_map()
    dst_results = dst.get_metric_results_map()
    # TODO: optimize sequental add_metric_result calls
    for src_key, src_result in src_results.items():
        if src_key in dst_results:
            dst_result = dst_results[src_key]
            merge_metric_results(src_result, dst_result)
        else:
            dst.add_metric_result(src_result)


def merge_sbs_metric_result_lists(left_results, right_results):
    """
    :type left_results: list[SbsMetricResult]
    :type right_results: list[SbsMetricResult]
    :rtype: list[SbsMetricResult]
    """
    if not left_results:
        return right_results
    if not right_results:
        return left_results

    results_map = OrderedDict()
    for right_result in right_results:
        results_map[right_result.key()] = right_result
    for left_result in left_results:
        left_key = left_result.key()
        if left_key in results_map:
            left_values = left_result.sbs_metric_values
            right_values = results_map[left_key].sbs_metric_values
            if len(left_values.single_results) != len(right_values.single_results):
                logging.error("Sizes of single results differ for metric_key %s", left_key)
            if len(left_values.pair_results) != len(right_values.pair_results):
                logging.error("Sizes of pair results differ for metric_key %s", left_key)
        else:
            results_map[left_key] = left_result
    return results_map.values()


def merge_observations(src, dst):
    """
    :type src: Observation
    :type dst: Observation
    Копирует все эксперименты, результаты метрик и критериев из наблюдения src в наблюдение dst.
    Может мутировать содержимое src и dst.
    """
    res_experiments = OrderedDict()
    merge_experiments(src.control, dst.control)
    for dst_exp in dst.experiments:
        res_experiments[dst_exp.key()] = dst_exp

    dst.tags = merge_field_strict(dst.tags, src.tags, "{} tags".format(dst))
    dst.extra_data = merge_field_strict(dst.extra_data, src.extra_data, "{} extra_data".format(dst))

    for src_exp in src.experiments:
        if src_exp.key() in res_experiments:
            matched_exp = res_experiments[src_exp.key()]
            merge_experiments(src_exp, matched_exp)
        else:
            res_experiments[src_exp.key()] = src_exp

    dst.experiments = list(res_experiments.values())

    dst.sbs_workflow_id = merge_field_strict(src.sbs_workflow_id, dst.sbs_workflow_id, "{} sbs_workflow_id".format(dst))
    dst.sbs_metric_results = merge_sbs_metric_result_lists(src.sbs_metric_results, dst.sbs_metric_results)


def merge_field_strict(left, right, name):
    if left is None:
        return right
    if right is None:
        return left
    if left == right:
        return left
    raise Exception("can't merge different {}: {} vs {}".format(name, left, right))


def merge_pools(pools, ignore_extra=False):
    """
    Объединяет несколько пулов в один с сохранением максимального количества данных.
    Может мутировать все пулы и их содержимое.
    :param pools: итератор пулов
    :type pools: iter[Pool]
    :param ignore_extra: выкинуть extra_data
    :type ignore_extra: bool
    :return: объединенный пул
    :rtype: Pool
    """
    res_observations = OrderedDict()
    res_synthetic_summaries = None
    res_extra_data = None
    res_lamps = []
    for pool in pools:
        res_synthetic_summaries = merge_field_strict(res_synthetic_summaries,
                                                     pool.synthetic_summaries,
                                                     "synthetic_summaries")
        if not ignore_extra:
            res_extra_data = merge_field_strict(res_extra_data,
                                                pool.extra_data,
                                                "extra_data")

        merge_lamps(res_lamps, pool.lamps)

        for obs in pool.observations:
            obs_key = obs.key()
            if obs_key in res_observations:
                existing_obs = res_observations[obs_key]
                merge_observations(obs, existing_obs)
            else:
                res_observations[obs_key] = obs

    res_pool = Pool(
        observations=list(res_observations.values()),
        synthetic_summaries=res_synthetic_summaries,
        extra_data=res_extra_data,
        lamps=res_lamps,
    )
    res_pool.sort(sort_exp=False)

    return res_pool


def intersect_pools(pools, ignore_extra=False):
    """
    Возвращает пересечение нескольких пулов по наблюдениям с сохранением максимального количества данных.
    Может мутировать все пулы и их содержимое.
    :param pools: итератор пулов
    :type pools: iter[Pool] | list[Pool]
    :param ignore_extra: выкинуть extra_data
    :type ignore_extra: bool
    :return: объединенный пул
    """
    if not isinstance(pools, list):
        pools = list(pools)

    if pools:
        keys_intersection = {obs.key() for obs in pools[0].observations}
        for pool in pools[1:]:
            keys_intersection.intersection_update(obs.key() for obs in pool.observations)
    else:
        keys_intersection = set()

    res_pool = merge_pools(pools, ignore_extra=ignore_extra)
    res_pool.observations = [obs for obs in res_pool.observations if obs.key() in keys_intersection]

    return res_pool


def get_daily_observations(observation):
    """
    Get daily observations
    :type observation: Observation
    :return: List[Observation]
    """
    observations = []
    for day in observation.dates:
        day_observation = observation.clone()
        day_observation.dates = utime.DateRange(day, day)
        observations.append(day_observation)
    return observations


def separate_observations(pool):
    """
    Separate observations into main and daily
    :type pool: Pool
    :return: Tuple[Observation, List[Observation]]
    """
    if len(pool.observations) == 1:
        return pool.observations[0], []

    main_obs = None
    daily_obs = []
    for obs in pool.observations:
        if obs.dates.number_of_days() == 1:
            daily_obs.append(obs)
        elif main_obs is None:
            main_obs = obs
        else:
            raise Exception("Pool should contain one main observation only")

    if main_obs is None:
        raise Exception("Pool should contain a main observation")

    days = sorted(set(obs.dates.start for obs in daily_obs))
    dates = utime.DateRange(days[0], days[-1])

    if dates != main_obs.dates or dates.number_of_days() != len(days):
        raise Exception("Daily observations do not cover whole a main observation")

    daily_obs.sort(key=lambda x: x.dates.start)
    return main_obs, daily_obs


ALLOWED_MISTAKE = 0.0001


def calculate_percent_of_value(val, diff):
    if abs(val) < ALLOWED_MISTAKE:
        return None
    else:
        return 100 * float(diff) / val


def convert_obs_to_ab_format(obs):
    testids = []
    metrics = {}

    def get_val(x):
        return x if x is not None else 0

    if obs.control:
        testids.append(obs.control.testid)
        for metric in obs.control.metric_results:
            metric_name = metric.metric_name()
            metric_value = get_val(metric.metric_values.significant_value)
            assert metric_name not in metrics
            metrics[metric_name] = [{"val": metric_value}]
    from experiment_pool import MetricColoring
    for experiment in obs.experiments:
        testids.append(experiment.testid)
        for metric in experiment.metric_results:
            metric_name = metric.metric_name()
            if metric_name not in metrics:
                logging.warning("Metric %s not found in control", metric_name)
                continue
            control_val = metrics[metric_name][0]["val"]
            metric_value = get_val(metric.metric_values.significant_value)
            metric_delta = metric_value - control_val
            delta_percent = calculate_percent_of_value(control_val, metric_delta)
            if not metric.criteria_results:
                ab_metric = {
                    "val": metric_value,
                    "delta_val": metric_delta,
                    "delta_prec": 0.0,
                    "pvalue": 0.5,
                }
                if delta_percent is not None:
                    ab_metric["delta_percent"] = delta_percent
                metrics[metric_name].append(ab_metric)
            else:
                for criteria in metric.criteria_results:
                    pvalue = criteria.pvalue if criteria.pvalue is not None else 0.5
                    std_diff_val = criteria.deviations.std_diff if criteria.deviations is not None else 0
                    ab_metric = {
                        "val": metric_value,
                        "delta_val": metric_delta,
                        "delta_prec": std_diff_val,
                        "pvalue": pvalue,
                    }
                    if delta_percent is not None:
                        ab_metric["delta_percent"] = delta_percent
                    if metric.coloring == MetricColoring.LESS_IS_BETTER:
                        if metric_delta > 0:
                            ab_metric["color"] = "red"
                        elif metric_delta < 0:
                            ab_metric["color"] = "green"
                    elif metric.coloring == MetricColoring.MORE_IS_BETTER:
                        if metric_delta > 0:
                            ab_metric["color"] = "green"
                        elif metric_delta < 0:
                            ab_metric["color"] = "red"
                    metrics[metric_name].append(ab_metric)
    sorted_metrics = OrderedDict(sorted(metrics.items()))
    return testids, sorted_metrics


class MetricConversionStar:
    VALIDATED = "controls_validated"
    # releable поддерживается в админке, поэтому требуется данная константа
    RELIABLE = "releable"
    ELITE = "elite"
    ALL = {VALIDATED, RELIABLE, ELITE}


def get_test_meta(mstand_pool):
    # TODO: с добавлением группы по умолчанию ('non-grouped metrics') данная функция потеряла смысл. Возможно,
    #  нужно модифицировать или убрать#

    metrics_meta = []
    for metric_key in sorted(mstand_pool["data"]["metrics"]):
        metrics_meta.append({
            "name": metric_key,
            MetricConversionStar.ELITE: False,
            MetricConversionStar.RELIABLE: False,
            MetricConversionStar.VALIDATED: False
        })

    return {
        "groups": [{
            "metrics": metrics_meta,
            "name": "group name",
            "key": "group key",
            "url": "url",
        }]
    }


def get_star_flags(ab_info):
    if ab_info is None:
        return False, False, False
    if ab_info.star is None:
        return False, False, False
    elite = ab_info.star == PluginStar.ELITE
    reliable = ab_info.star == PluginStar.RELIABLE or elite
    controls_validated = ab_info.star == PluginStar.VALIDATED or reliable or elite
    return controls_validated, reliable, elite


def extract_groups_from_obs(obs):
    non_grouped_metrics = []
    groups = collections.defaultdict(list)

    if obs.control:
        for metric in obs.control.metric_results:
            metric_name = metric.metric_name()
            group = metric.metric_group()
            if group is not None:
                groups[group].append(metric_name)
            else:
                non_grouped_metrics.append(metric_name)
    elif obs.experiments:
        for metric in obs.experiments[0].metric_results:
            metric_name = metric.metric_name()
            group = metric.metric_group()
            if group is not None:
                groups[group].append(metric_name)

    if "non-grouped metrics" in groups:
        default_group = groups["non-grouped metrics"]
        del groups["non-grouped metrics"]
    else:
        default_group = []

    default_group = non_grouped_metrics + default_group

    for group_metrics in groups.values():
        group_metrics.sort()
    default_group.sort()
    sorted_groups = OrderedDict(sorted(groups.items()))
    sorted_groups["non-grouped metrics"] = default_group
    return sorted_groups


def extract_star_flags(obs):
    controls_validated = {}
    reliable = {}
    elite = {}

    if obs.control:
        for metric in obs.control.metric_results:
            metric_name = metric.metric_name()
            controls_validated[metric_name], reliable[metric_name], elite[metric_name] = get_star_flags(metric.ab_info)
    elif obs.experiments:
        for metric in obs.experiments[0].metric_results:
            metric_name = metric.metric_name()
            controls_validated[metric_name], reliable[metric_name], elite[metric_name] = get_star_flags(metric.ab_info)
    return controls_validated, reliable, elite


def get_description(ab_info):
    if ab_info is None:
        return None
    else:
        return ab_info.description


def extract_descriptions(obs):
    descriptions = {}
    if obs.control:
        for metric in obs.control.metric_results:
            metric_name = metric.metric_name()
            if get_description(metric.ab_info) is not None:
                descriptions[metric_name] = get_description(metric.ab_info)
    elif obs.experiments:
        for metric in obs.experiments[0].metric_results:
            metric_name = metric.metric_name()
            if get_description(metric.ab_info) is not None:
                descriptions[metric_name] = get_description(metric.ab_info)
    return descriptions


def get_hname(ab_info):
    if ab_info is None:
        return None
    else:
        return ab_info.hname


def extract_hnames(obs):
    hnames = {}
    if obs.control:
        for metric in obs.control.metric_results:
            metric_name = metric.metric_name()
            if get_hname(metric.ab_info) is not None:
                hnames[metric_name] = get_hname(metric.ab_info)
    elif obs.experiments:
        for metric in obs.experiments[0].metric_results:
            metric_name = metric.metric_name()
            if get_hname(metric.ab_info) is not None:
                hnames[metric_name] = get_hname(metric.ab_info)
    return hnames


def convert_groups_to_ab_format(groups):
    result = []
    for group, metrics in groups.items():
        result.append({
            "key": group,
            "metrics": [OrderedDict(name=metric) for metric in metrics]
        })
    return result


def star_priority(validated, reliable, elite):
    if elite:
        return 0
    elif reliable:
        return 1
    elif validated:
        return 2
    else:
        return 3


def add_star_flags_to_metrics_ab(groups, controls_validated, reliable, elite):
    for group in groups:
        for metric in group["metrics"]:
            metric_name = metric["name"]
            metric[MetricConversionStar.VALIDATED] = controls_validated[metric_name]
            metric[MetricConversionStar.RELIABLE] = reliable[metric_name]
            metric[MetricConversionStar.ELITE] = elite[metric_name]
        group["metrics"].sort(key=lambda x: (star_priority(x[MetricConversionStar.VALIDATED],
                                                           x[MetricConversionStar.RELIABLE],
                                                           x[MetricConversionStar.ELITE]),
                                             x["name"]))


def add_descriptions_to_ab(groups, descriptions):
    for group in groups:
        for metric in group["metrics"]:
            metric_name = metric["name"]
            if descriptions.get(metric_name) is not None:
                metric["description"] = descriptions[metric_name]


def add_hnames_to_ab(groups, hnames):
    for group in groups:
        for metric in group["metrics"]:
            metric_name = metric["name"]
            if hnames.get(metric_name) is not None:
                metric["hname"] = hnames[metric_name]


def convert_pool_to_ab_format(pool, make_test_data=False):
    """
    Конвертирует пул в формат AB описанный тут: https://wiki.yandex-team.ru/serp/experiments/20/adminka/emsformat/
    :param pool: пул
    :type pool: Pool
    :param make_test_data: Сформировать файл для тестирования
    :type make_test_data: bool
    :return: сконвертированный пул
    """

    obs, daily_obs = separate_observations(pool)
    testids, sorted_metrics = convert_obs_to_ab_format(obs)

    timelines = {}
    if daily_obs:
        for metric_name, ab_metric in usix.iteritems(sorted_metrics):
            timelines[metric_name] = OrderedDict({"0": ab_metric})

        for day_obs in daily_obs:
            day_testids, day_sorted_metrics = convert_obs_to_ab_format(day_obs)
            if day_testids != testids:
                raise Exception("Daily and main observations should have the same list of testids")
            for metric_name, ab_metric in usix.iteritems(day_sorted_metrics):
                timelines[metric_name][day_obs.dates.start.strftime("%s")] = ab_metric

    result = {
        "data": {
            "testids": testids,
            "metrics": sorted_metrics,
        },
    }

    groups = convert_groups_to_ab_format(extract_groups_from_obs(obs))
    controls_validated, reliable, elite = extract_star_flags(obs)
    add_star_flags_to_metrics_ab(groups, controls_validated, reliable, elite)
    add_descriptions_to_ab(groups, extract_descriptions(obs))
    add_hnames_to_ab(groups, extract_hnames(obs))

    if timelines:
        ab_details = [
            {"testids": testids, "metric": metric, "timeline": timeline}
            for metric, timeline in usix.iteritems(timelines)
        ]
        result["details"] = sorted(ab_details, key=lambda x: x["metric"])

    if pool.meta:
        result["meta"] = pool.meta.serialize()
    if groups:
        if result.get("meta") is not None:
            result["meta"]["groups"] = groups
        else:
            result["meta"] = {
                "groups": groups,
            }
    if result.get("meta") is None and make_test_data:
        result["meta"] = get_test_meta(result)

    return result


def sort_plugin_keys(metrics):
    metrics_with_names_sorted = sorted(metrics, key=lambda x: x.pretty_name())

    metric_number_map_resorted = OrderedDict()
    for lex_index, key in enumerate(metrics_with_names_sorted):
        metric_number_map_resorted[key] = lex_index

    return metric_number_map_resorted


def enumerate_all_metrics_in_observations(observations, sort_by_name=False):
    """
    :type observations: list[Observation]
    :type sort_by_name: Bool
    :rtype: OrderedDict[PluginKey, int]
    """
    metrics = OrderedDict()
    for obs in observations:
        for exp in [obs.control] + obs.experiments:
            for metric_res in exp.metric_results:
                if metric_res.metric_key not in metrics:
                    metrics[metric_res.metric_key] = len(metrics)

    if sort_by_name:
        metrics = sort_plugin_keys(metrics)

    return metrics


def enumerate_all_metrics_in_lamps(pool, sort_by_name=False):
    """
    :type pool: Pool
    :type sort_by_name: Bool
    :rtype: OrderedDict[PluginKey, int]
    """
    metrics = OrderedDict()
    lamps = pool.lamps
    for lamp in lamps:
        for lamp_res in lamp.lamp_values:
            if lamp_res.metric_key not in metrics:
                metrics[lamp_res.metric_key] = len(metrics)

    if sort_by_name:
        metrics = sort_plugin_keys(metrics)

    return metrics


def enumerate_all_metrics(pool, sort_by_name=False):
    """
    :type pool: Pool
    :type sort_by_name: Bool
    :rtype: OrderedDict[PluginKey, int]
    """
    return enumerate_all_metrics_in_observations(pool.observations, sort_by_name=sort_by_name)


def create_all_users_mode_pool(pool):
    logging.warning("Running in ALL USERS mode")

    new_pool = Pool()

    for observation in pool.observations:
        all_exp = Experiment(testid="all")

        new_observation = Observation(
            obs_id=observation.id,
            dates=observation.dates,
            sbs_ticket=observation.sbs_ticket,
            sbs_workflow_id=observation.sbs_workflow_id,
            sbs_metric_results=observation.sbs_metric_results,
            filters=observation.filters,

            tags=observation.tags,
            extra_data=observation.extra_data,

            control=all_exp,
            experiments=[]
        )

        new_pool.observations.append(new_observation)

    return new_pool


def _obs_compare_key(obs):
    return obs.key_without_experiments()[1:]  # key_without_experiments[0] is observation ID


def _set_of_experiments(obs):
    return set([obs.control.key()] + list(obs.key_of_experiments(include_control=False)))


def _remove_duplicates_group(group):
    if len(group) == 1:
        return group

    new_group = group[:]
    for obs1 in group:
        to_remove = None
        for obs2 in new_group:
            if obs1 == obs2:
                continue

            set_exps1 = _set_of_experiments(obs1)
            set_exps2 = _set_of_experiments(obs2)

            if set_exps1 == set_exps2:
                to_remove = max(obs1, obs2, key=lambda obs: obs.id)
                logging.info("Observations %s and %s are full duplicates, removing %s", obs1, obs2, to_remove)
            elif set_exps1.issubset(set_exps2):
                to_remove = obs1
                logging.info("Observation %s is a subset of %s, removing", obs1, obs2)
            elif set_exps2.issubset(set_exps1):
                to_remove = obs2
                logging.info("Observation %s is a subset of %s, removing", obs2, obs1)

        if to_remove and to_remove in new_group:
            new_group.remove(to_remove)
    return new_group


def remove_duplicate_observations(observations):
    observation_groups = collections.defaultdict(list)

    for obs in observations:
        observation_groups[_obs_compare_key(obs)].append(obs)

    new_observations = []
    for group in observation_groups.values():
        new_observations.extend(_remove_duplicates_group(group))

    return new_observations


def remove_duplicates(pool):
    return Pool(remove_duplicate_observations(pool.observations))


def generate_metric_key_id_map(pool):
    # enumerate all metric keys in pool by numeric IDs
    metric_key_id_map = {}
    for metric_id, metric_key in enumerate(sorted(pool.all_metric_keys())):
        metric_key_id_map[metric_key] = metric_id
    return metric_key_id_map


def reverse_metric_id_key_map(metric_key_id_map):
    # enumerate all metric keys in pool by numeric IDs (reverse map)
    reversed_map = {}
    for key, val in usix.iteritems(metric_key_id_map):
        reversed_map[val] = key
    return reversed_map


def append_lamps_to_pool(dst_pool, lamp_pool):
    lamps_dict = {}
    answer = deepcopy(dst_pool)

    for observation in lamp_pool.observations:
        controlid = observation.control.testid
        obsid = observation.id
        dates = observation.dates
        for experiment in [observation.control] + observation.experiments:
            version = experiment.metric_results[-1].version
            key = LampKey(testid=experiment.testid, control=controlid, observation=obsid,
                          dates=dates, version=version)
            if experiment.metric_results:  # sometimes control gets overwritten by []
                lamps_dict[key] = [result for result in experiment.metric_results]

    for observation in answer.observations:
        controlid = observation.control.testid
        obsid = observation.id
        dates = observation.dates

        for experiment in [observation.control] + observation.experiments:
            version = experiment.metric_results[-1].version
            key = LampKey(testid=experiment.testid, control=controlid, observation=obsid,
                          dates=dates, version=version)
            answer.lamps.append(LampResult(lamp_key=key, lamp_values=lamps_dict[key]))

    return answer
