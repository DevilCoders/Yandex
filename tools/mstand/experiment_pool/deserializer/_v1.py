import logging
import traceback

import yaqutils.time_helpers as utime
import yaqutils.misc_helpers as umisc
from experiment_pool import Experiment
from experiment_pool import MetricResult
from experiment_pool import Observation
from experiment_pool import Pool
from experiment_pool.metric_result import SbsMetricResult
from experiment_pool import LampResult
from experiment_pool.meta import Meta
from yaqutils import MainUtilsException


def deserialize_pool(pool_data):
    pool = Pool()
    observation_list = pool_data['observations']
    logging.info("Parsing pool with %d observations", len(observation_list))
    for obs_index, observation in enumerate(observation_list):
        obs_pos = obs_index + 1
        try:
            obs = deserialize_observation(observation, obs_pos)
            pool.observations.append(obs)
            if obs_pos % 50 == 1:
                logging.info("Processing observation # %s", obs_pos)
        except MainUtilsException as exc:
            logging.error("Cannot deserialize observation # %s: %s, details: %s", obs_pos, exc, traceback.format_exc())
            raise

    pool.extra_data = pool_data.get("extra_data")

    lamps = pool_data.get("lamps")
    if lamps:
        for lamp in lamps:
            pool.lamps.append(LampResult.deserialize(lamp))
    else:
        pool.lamps = []

    if "meta" in pool_data:
        pool.meta = Meta.deserialize(pool_data["meta"])
    return pool


def deserialize_observation(obs_data, obs_pos=None):
    obs_id = obs_data.get('observation_id')

    date_from = utime.parse_date_msk(obs_data.get('date_from'))
    date_to = utime.parse_date_msk(obs_data.get('date_to'))
    obs_dates = utime.DateRange(date_from, date_to)

    try:
        control = deserialize_experiment(obs_data['control'])
    except MainUtilsException as exc:
        logging.error("Cannot deserialize obs. # %s: %s, details: %s", obs_pos, exc, traceback.format_exc())
        raise

    sbs_ticket = obs_data.get("sbs_ticket")
    sbs_workflow_id = obs_data.get("sbs_workflow_id")

    sbs_metric_results_array = obs_data.get("sbs_metric_results")
    if sbs_metric_results_array is not None:
        sbs_metric_results = umisc.deserialize_array(sbs_metric_results_array, SbsMetricResult)
    else:
        sbs_metric_results = None

    obs = Observation(
        obs_id=obs_id,
        dates=obs_dates,
        sbs_ticket=sbs_ticket,
        sbs_workflow_id=sbs_workflow_id,
        sbs_metric_results=sbs_metric_results,
        control=control,
        tags=obs_data.get("tags"),
        extra_data=obs_data.get('extra_data')
    )

    experiment_list = obs_data.get('experiments', [])
    for exp_index, experiment in enumerate(experiment_list):
        exp_pos = exp_index + 1
        try:
            exp = deserialize_experiment(experiment)
            obs.experiments.append(exp)
        except MainUtilsException as exc:
            logging.error("Cannot deserialize exp. # %s in obs. # %s: %s, details: %s",
                          exp_pos, obs_pos, exc, traceback.format_exc())
            raise

    return obs


def deserialize_experiment(exp_data):
    testid = exp_data.get("testid")
    serpset_id = exp_data.get("serpset_id")
    sbs_system_id = exp_data.get("sbs_system_id")

    # testid/serpset_id validation is inside
    extra_data = exp_data.get("extra_data")
    errors = exp_data.get("errors")

    exp = Experiment(
        testid=testid,
        serpset_id=serpset_id,
        sbs_system_id=sbs_system_id,
        extra_data=extra_data,
        errors=errors
    )

    metric_results = []
    for metric_data in exp_data.get("metrics", []):
        metric_results.append(MetricResult.deserialize(metric_data))

    exp.add_metric_results(metric_results)

    return exp
