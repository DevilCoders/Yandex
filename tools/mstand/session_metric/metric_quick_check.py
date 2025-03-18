# coding=utf-8
import copy
import json

import experiment_pool
import mstand_metric_helpers.online_metric_helpers as mhelp
import session_squeezer.services as squeezer_services
import yaqutils.json_helpers as ujson
import yaqutils.time_helpers as utime

from metrics_api import ExperimentForAPI
from metrics_api import ObservationForAPI
from metrics_api.online import UserActionForAPI
from metrics_api.online import UserActionsForAPI


def create_sample_actions(services):
    actions_for_api_dict = {}  # group sample data by yuid+experiment
    for service in services:
        squeezer = squeezer_services.SQUEEZERS.get(service)
        if squeezer and squeezer.SAMPLE:
            action_json = ujson.load_from_str(squeezer.SAMPLE)

            yuid = action_json["yuid"]
            timestamp = action_json["ts"]
            testid = action_json.get("testid")

            action_for_api = UserActionForAPI(
                user=yuid,
                timestamp=timestamp,
                data=action_json,
            )

            key = (yuid, testid, action_for_api.date)
            actions_for_api = actions_for_api_dict.get(key)
            if actions_for_api is None:
                dates = utime.DateRange(action_for_api.date, action_for_api.date)
                exp = experiment_pool.Experiment(testid=testid)
                obs = experiment_pool.Observation(obs_id="0", dates=dates, control=exp)
                actions_for_api = UserActionsForAPI(
                    user=yuid,
                    actions=[],
                    experiment=ExperimentForAPI(exp),
                    observation=ObservationForAPI(obs),
                )
                actions_for_api_dict[key] = actions_for_api
            actions_for_api.actions.append(action_for_api)
    return actions_for_api_dict.values()


def check_metric_quick(metric, services):
    mhelp.validate_online_metric(metric)
    mhelp.validate_online_metric_files(metric)

    # copy metric to keep original instance empty before sending to YT
    metric_copy = copy.deepcopy(metric)
    for actions_for_api in create_sample_actions(services):
        raw_result = mhelp.apply_online_metric(metric_copy, actions_for_api)
        result = mhelp.convert_online_metric_value_to_list(raw_result)
        json.dumps(result)
