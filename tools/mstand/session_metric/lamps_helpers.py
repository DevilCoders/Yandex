import logging

from copy import deepcopy

import experiment_pool.pool_helpers as phelp
import mstand_metric_helpers.common_metric_helpers as mhelp
import mstand_utils.args_helpers as mstand_uargs

from experiment_pool import MetricResult  # noqa
from experiment_pool import Observation
from mstand_enums.mstand_online_enums import ServiceEnum
from mstand_structs import LampKey
from session_metric import MetricRunner

ONLINE_LAMPS_BATCH = "sample_metrics/online/online-lamps-batch.json"


def enumerate_all_versions_in_pool(pool):
    result = {}
    for obs in pool.observations:
        for exp in [obs.control] + obs.experiments:
            result[obs.id + exp.testid] = exp.metric_results[-1].version
    return result


def get_lamps_keys_from_pool(pool):
    """
    :type pool: Pool
    :rtype: set(LampKey)
    """
    keys = set()

    for obs in pool.observations:
        dates = obs.dates
        control = obs.control.testid
        experiments = obs.all_experiments()

        for exp in experiments:
            version = exp.metric_results[-1].version
            keys.add(LampKey(observation=obs.id, control=control, dates=dates, testid=exp.testid, version=version))
    return keys


def get_experiments_from_cache(observation, cache, versions):
    exp_list = []
    control = observation.control
    for exp in [observation.control] + observation.experiments:
        version = versions[observation.id + exp.testid]
        key = LampKey(testid=exp.testid, control=control.testid,
                      observation=observation.id, dates=observation.dates, version=version)

        for row_key, row_value in cache:
            if key == row_key:
                exp.metric_results = row_value
                exp_list.append(exp)
    return exp_list


def get_experiments_to_calculate(observation, cache, versions):
    exp_list = []
    calculate_control = False
    control = observation.control
    control_version = versions[observation.id + control.testid]
    control_key = LampKey(testid=control.testid, control=control.testid,
                          observation=observation.id, dates=observation.dates, version=control_version)

    for exp in observation.experiments:
        version = versions[observation.id + exp.testid]
        key = LampKey(testid=exp.testid, control=control.testid,
                      observation=observation.id, dates=observation.dates, version=version)

        if key not in (row[0] for row in cache):
            exp_list.append(exp)

    if control_key not in (row[0] for row in cache):
        calculate_control = True
    return exp_list, calculate_control


def pool_from_lamps(lamps_pool, cache, cache_mode):
    """
    :type lamps_pool: Pool
    :type cache: list[tuple[LampKey, list[MetricResult]]]
    :type cache_mode: bool
    :rtype: Pool, bool
    """

    empty = True
    pool = deepcopy(lamps_pool)
    pool.clear_metric_results()
    versions = enumerate_all_versions_in_pool(lamps_pool)

    new_obs_list = []
    for obs in pool.observations:
        control = obs.control
        if cache_mode:
            new_exp_list = get_experiments_from_cache(obs, cache, versions)
            calc_control = False
        else:
            new_exp_list, calc_control = get_experiments_to_calculate(obs, cache, versions)

        if new_exp_list or calc_control:
            empty = False
            new_obs_list.append(Observation(experiments=new_exp_list,
                                            control=control,
                                            dates=obs.dates,
                                            obs_id=obs.id))

    result = phelp.Pool(observations=new_obs_list)
    return result, empty


def calc_lamps_for_pool(lamps_args, lamps_to_calculate, calc_backend):
    lamps_container = mhelp.create_container_from_cli_args(lamps_args, lamps_mode=True)
    lamps_runner = MetricRunner.from_cli_args(lamps_args, lamps_container, calc_backend)

    services = ServiceEnum.convert_aliases(lamps_args.services)
    lamps_runner.calc_for_pool(
        pool=lamps_to_calculate,
        services_with_meta=services,
        save_to_dir=lamps_args.save_to_dir,
        save_to_tar=lamps_args.save_to_tar,
        threads=lamps_args.threads,
        batch_min=lamps_args.experiment_batch_min,
        batch_max=lamps_args.experiment_batch_max,
    )


def calc_lamps(calc_backend, cli_args, pool):
    lamps_args = mstand_uargs.create_lamps_args(cli_args, ONLINE_LAMPS_BATCH)
    pool_lamp_keys = get_lamps_keys_from_pool(pool)
    cache = calc_backend.get_lamps_from_cache(pool_lamp_keys, cli_args.squeeze_path)

    lamps_to_download, empty_downloads = pool_from_lamps(pool, cache, True)
    lamps_to_calculate, empty_calculations = pool_from_lamps(pool, cache, False)
    lamps_pool = phelp.Pool()

    if not empty_downloads:
        lamps_pool = lamps_to_download
    if not empty_calculations:
        logging.info("Lamps calculation started")
        calc_lamps_for_pool(lamps_args, lamps_to_calculate, calc_backend)

        lamps_pool = phelp.merge_pools([lamps_to_download, lamps_to_calculate])
        lamps_to_calculate = phelp.append_lamps_to_pool(dst_pool=lamps_to_calculate, lamp_pool=lamps_to_calculate)
        calc_backend.write_lamps_to_cache(lamps_to_calculate, cli_args.squeeze_path)

    result = phelp.append_lamps_to_pool(pool, lamps_pool)
    return result
