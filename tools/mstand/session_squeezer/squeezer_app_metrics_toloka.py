import hashlib
import json
import struct

from collections.abc import Iterable
from collections import defaultdict
from functools import partial
from functools import wraps
from itertools import tee

import session_squeezer.squeezer_common as sq_common

APP_METRICS_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "event", "type": "string"},
    {"name": "params", "type": "any"},
    {"name": "os", "type": "string"},
    {"name": "os_version", "type": "string"},
    {"name": "device_id", "type": "string"},
]


class ActionsSqueezerAppMetricsToloka(sq_common.ActionsSqueezer):
    """
    1: initial version
    """
    VERSION = 1

    YT_SCHEMA = APP_METRICS_YT_SCHEMA
    USE_LIBRA = False

    def __init__(self):
        super(ActionsSqueezerAppMetricsToloka, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        assert not args.result_actions

        testids = get_testids(args)
        if not testids:
            return

        for row in args.container:
            """:type row: dict[str]"""
            if not self.allowed_event(row):
                continue

            squeezed = {
                "ts": int(row["EventTimestamp"]),
                "event": row["EventName"],
                "params": get_params_from_row(row),
                "os": row["OperatingSystem"],
                "os_version": row["OSVersion"],
                "device_id": row["DeviceID"],
            }

            exp_bucket = self.check_experiments_by_testids(args, testids)
            squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket
            args.result_actions.append(squeezed)

    @staticmethod
    def allowed_event(row):
        return row["EventName"] in ALLOWED_EVENTS


def get_params_from_row(row):
    """
    :type row: dict
    :rtype: dict | None
    """
    return get_params(row["ParsedParams_Key1"], row["ParsedParams_Key2"])


def get_testids(args):
    """
    :type args: sq_common.ActionSqueezerArguments
    :rtype: set[str]
    """
    testids = set()
    for row in args.container:
        if row["EventName"] == "environment_config":
            params = get_params_from_row(row) or {}
            testid = params.get("testid")
            if testid:
                testids.update(testid)
    return testids


"""
Methods source: https://yql.yandex-team.ru/Operations/XVLXQ53udppzPZzNdpcNSuxvoYPdy-1uwkaPBIus0r8=
Owner: cogwheelhead
"""

ALLOWED_EVENTS = {
    "available_tasks_list_loaded",
    "available_tasks_view_switched",

    "contact_us_click",
    "contact_us_sent",
    "detailed_day_info_clicked",
    "download_offline_map",
    "feedback_message",
    "feedback_rating",

    "filter_state",
    "filter_state_available",
    "filter_state_map",

    "first_assignment",
    "first_auth",
    "first_green_balance",
    "first_launch",
    "first_map_assignment",
    "first_map_task_done",
    "first_map_task_view",
    "first_nonmap_assignment",
    "first_nonmap_task_done",
    "first_nonmap_task_view",
    "first_pin_tasks_view",
    "first_real_assignment",
    "first_real_task_done",
    "first_task_done",
    "first_task_view",
    "first_tasks_view_in_list",
    "first_training_assignment",
    "first_training_task_done",
    "first_withdraw",

    "forum",
    "grade_project",
    "login",
    "logout",

    "maintenance_shown",

    "map_pin_format_switched",
    "map_task_highlighting_clicked",

    "message_from_dialog",
    "message_from_task",

    "money",

    "no_available_tasks",
    "no_available_tasks_in_first_map_region",
    "not_allowed_to_withdraw",

    "open_map_by_default_switch",
    "pin_tasks_list_loaded",

    "referral_entrance",
    "referral_share",

    "registration",

    "run_about",
    "run_faq",
    "run_general_task_map",
    "run_help",
    "run_incomes",
    "run_messages",
    "run_money",
    "run_pin_format_setting",
    "run_pin_task_info",
    "run_pin_task_list",
    "run_profile",
    "run_profile_edit",
    "run_rating_skills",
    "run_referral_bonuses",
    "run_registration",
    "run_settings",
    "run_support_rate_app",
    "run_task_complaint",
    "run_task_group_info",
    "run_task_group_map",

    "task_done",
    "task_expire",
    "task_finish",
    "task_skip",

    "taxes"
}


def get_toloka_user_id(uid):
    salt = "_!"
    m = hashlib.md5()
    pack = struct.pack("q", uid)
    m.update(pack)
    m.update(salt)
    return m.hexdigest()


def is_collection(obj):
    return isinstance(obj, Iterable) and not isinstance(obj, str)


def maybe(function=None, fallback=None):
    if function is None:
        return partial(maybe, fallback=fallback)

    @wraps(function)
    def wrapper(*args, **kwargs):
        try:
            return function(*args, **kwargs)
        except:
            return fallback(*args, **kwargs) if callable(fallback) else fallback

    return wrapper


@maybe
def get_params(stringified_keys, stringified_values):
    keys = json.loads(stringified_keys)
    values, check_values = tee(map(maybe(json.loads, fallback=lambda x: x), json.loads(stringified_values)))
    assert all(not is_collection(v) for v in check_values)
    params = defaultdict(list)
    for k, v in zip(keys, values):
        params[k].append(v if k != 'testid' else str(v))

    result = {k: v if (len(v) > 1 or k == 'testid') else v[0] for k, v in params.items()}
    return result
