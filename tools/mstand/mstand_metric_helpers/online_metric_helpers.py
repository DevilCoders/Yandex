import os
import yaqutils.misc_helpers as umisc
from types import GeneratorType

from metrics_api.online import UserActionsForAPI


def is_user_metric(instance):
    return hasattr(instance, "value_by_user") or hasattr(instance, "__call__")  # for backward compatibility


def is_session_metric(instance):
    return hasattr(instance, "value_by_session")


def is_request_metric(instance):
    return hasattr(instance, "value_by_request")


def is_bucket_metric(instance):
    if hasattr(instance, "use_buckets"):
        return instance.use_buckets
    return not is_user_metric(instance)


def apply_online_metric(metric_instance, user_actions):
    """
    :type metric_instance: instance
    :type user_actions: UserActionsForAPI
    :rtype: int | generator | list[int]
    """

    # a simple case: in per-user metric we need to return a single value
    if is_user_metric(metric_instance):
        if hasattr(metric_instance, "value_by_user"):
            return metric_instance.value_by_user(user_actions)
        else:
            return metric_instance(user_actions)

    # complicated case: session splitter may be default or user-defined
    # and there are two types of computation methods: value_by_session, value_by_request

    splitter = None  # used for spliting sessions/requests
    method = None  # used for calculating metric values

    if is_session_metric(metric_instance):
        if hasattr(metric_instance, "split_sessions"):
            splitter = metric_instance.split_sessions
        else:
            splitter = split_sessions_default
        method = metric_instance.value_by_session

    if is_request_metric(metric_instance):
        splitter = split_requests_default
        method = metric_instance.value_by_request

    # wrapping list[UserActionForAPI] in UserActionsForAPI`
    session_actions_api = UserActionsForAPI.header_from_other(user_actions)
    result = []
    session_actions = splitter(user_actions)

    for actions in session_actions:
        # applying the chosen method to get _several_ metric values for a _single_ user
        session_actions_api.actions = actions
        session_result = method(session_actions_api)
        if session_result is not None:
            result.append(session_result)

    return result


def split_sessions_default(user_actions):
    """
    :type user_actions: UserActionsForAPI
    :rtype: list[list[UserActionForAPI]]
    """
    action_array = user_actions.actions
    assert len(action_array) > 0, "User {} has no actions!".format(user_actions.user)
    sessions = []
    prev_action_timestamp = 0

    for action in action_array:
        time_delta = action.timestamp - prev_action_timestamp
        if time_delta > 30 * 60:
            sessions.append([action])
        else:
            sessions[-1].append(action)
        prev_action_timestamp = action.timestamp

    return sessions


def split_requests_default(user_actions):
    """
    :type user_actions: UserActionsForAPI
    :rtype: list[list[UserActionForAPI]]
    """
    action_array = user_actions.actions
    assert len(action_array) > 0, "User {} has no actions!".format(user_actions.user)
    sessions = []
    request_position = dict()

    for action in action_array:
        reqid = action.data["reqid"]
        if reqid in request_position:
            pos = request_position[reqid]
            sessions[pos].append(action)
        else:
            request_position[reqid] = len(request_position)
            sessions.append([action])

    return sessions


def validate_online_metric(metric_instance):
    metric_types = 0

    metric_types += is_user_metric(metric_instance)  # True == 1 guaranteed
    metric_types += is_session_metric(metric_instance)
    metric_types += is_request_metric(metric_instance)

    if metric_types > 1:
        raise Exception("Ambiguous metric type.")

    if metric_types == 0:
        raise Exception("Metric is not callable.")


def convert_online_metric_value_to_list(obj):
    """
    :type obj: object
    :rtype: list
    """
    if obj is None:
        return []
    if isinstance(obj, list):
        return obj
    if isinstance(obj, (GeneratorType, tuple)):
        return list(obj)
    return [obj]


def validate_online_metric_files(metric_instance):
    if not hasattr(metric_instance, "list_files"):
        return

    instance_files = metric_instance.list_files()
    full_paths = [umisc.get_user_module_absolute_path(instance_file) for instance_file in instance_files]
    for filename in full_paths:
        if not os.path.exists(filename):
            raise Exception("Metric file not found: {}".format(filename))
