#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-
import calendar
import datetime
from copy import deepcopy

import session_squeezer.squeezer_common as sq_common

TOLOKA_YT_SCHEMA = [
    {"name": "action_index", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "servicetype", "type": "string"},
    {"name": "testid", "type": "string"},

    # action info
    {"name": "type", "type": "string"},
    {"name": "ts", "type": "int64"},

    # worker info
    {"name": "yuid", "type": "string"},
    {"name": "worker_attributes_computed", "type": "any"},
    {"name": "worker_attributes_profile", "type": "any"},

    # assignment info
    {"name": "assignment_id", "type": "string"},
    {"name": "pool_id", "type": "int64"},
    {"name": "project_id", "type": "string"},
    {"name": "requester_id", "type": "string"},
    {"name": "requester_company_is_yandex", "type": "boolean"},
    {"name": "pool_training", "type": "boolean"},
    {"name": "assignment_status", "type": "string"},
    {"name": "task_point", "type": "any"},
    {"name": "assignment_price", "type": "double"},

    # assignment correctness
    {"name": "pool_post_accept", "type": "boolean"},
    {"name": "n_microtasks", "type": "int64"},
    {"name": "assignment_gs_count", "type": "int64"},
    {"name": "assignment_gs_correct_count", "type": "int64"},
    {"name": "assignment_gs_wrong_count", "type": "int64"},

    # assignment timestamps
    {"name": "assignment_start_time", "type": "int64"},
    {"name": "assignment_submit_time", "type": "int64"},
    {"name": "assignment_skip_time", "type": "int64"},
    {"name": "assignment_expire_time", "type": "int64"},
    {"name": "assignment_approve_time", "type": "int64"},
    {"name": "assignment_reject_time", "type": "int64"},

    # entry & selection info
    {"name": "entry_id", "type": "string"},
    {"name": "target", "type": "string"},
    # {"name": "eligible_pools", "type": "any"},
    # {"name": "context", "type": "any"},

    # worker_daily_statistics info
    {"name": "gs_correct", "type": "double"},
    {"name": "pa_correct", "type": "double"},
    {"name": "mv_correct", "type": "double"},
    {"name": "gs_total", "type": "int64"},
    {"name": "pa_total", "type": "int64"},
    {"name": "mv_total", "type": "int64"},
    {"name": "accuracy_gs", "type": "double"},
    {"name": "accuracy_pa", "type": "double"},
    {"name": "accuracy_mv", "type": "double"},
]


class ActionsSqueezerToloka(sq_common.ActionsSqueezer):
    """f
    1: initial version
    2: add worker_pool_selection, add results_v56, add mmh3 hash
    3: fix result_v56 timestamps
    4: original results_v56 table
    5: new version (toloka metrics system compatible)
    6: fixed hashing by adding abs()
    7: timestamps changed to UTC, fixed row duplication
    8: remove automerged rows from rv56, add registration ts, use testids instead of hash for experiment matching
    9: add ordered list of pools shown upon entry into EP squeezes, add pool_id and assignment_id into WPS squeezes, extract get_registration_ts method
    10: update task search results page info: add eligble_pools and context columns, remove deprecated shown_pools, n_shown_pools and n_available_pools columns
    """

    VERSION = 10
    YT_SCHEMA = TOLOKA_YT_SCHEMA
    USE_LIBRA = False

    def __init__(self):
        super(ActionsSqueezerToloka, self).__init__()

    def get_actions(self, args):
        raise Exception("use special toloka methods instead of get_actions in ActionsSqueezerToloka")

    @staticmethod
    def get_registration_ts(worker_attributes):
        registration_datetime_string = worker_attributes["createdDate"]
        registration_datetime = datetime.datetime.strptime(registration_datetime_string[:19], "%Y-%m-%dT%H:%M:%S")
        return calendar.timegm(registration_datetime.timetuple())

    @staticmethod
    def get_computed_attributes(worker_attributes, keys=None):
        default_keys = ["client_type",
                         "device_category",
                         "os_family",
                         "os_version",
                         "os_version_bugfix",
                         "os_version_major",
                         "os_version_minor",
                         "rating",
                         "region_by_ip",
                         "region_by_phone",
                         "registration_date",
                         "registration_platform",
                         "registration_source",
                         "user_agent_family",
                         "user_agent_type",
                         "user_agent_version",
                         "user_agent_version_bugfix",
                         "user_agent_version_major",
                         "user_agent_version_minor"]

        required_keys = keys or default_keys

        return {key: worker_attributes["attrIndex"].get("computed", {}).get(key) for key in required_keys}

    @staticmethod
    def get_profile_attributes(worker_attributes):
        result = worker_attributes["attrIndex"].get("profile", {})
        result["created_at"] = ActionsSqueezerToloka.get_registration_ts(worker_attributes)
        return result

    @staticmethod
    def get_actions_ep(args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        row = args.container
        assert isinstance(row, dict)
        if not row.get("request"):
            return

        squeezed = {"type": "entry"}

        squeezed["ts"] = row["timestamp"]
        squeezed["yuid"] = row["worker_id"]

        squeezed["worker_attributes_computed"] = ActionsSqueezerToloka.get_computed_attributes(row["request"]["workerAttributes"])
        squeezed["worker_attributes_profile"] = ActionsSqueezerToloka.get_profile_attributes(row["request"]["workerAttributes"])

        squeezed["entry_id"] = row["ref_uuid"]
        squeezed["target"] = row["target"]
        # squeezed["eligible_pools"] = [{"poolId": pool["poolId"],
        #                                "projectId": pool["projectId"],
        #                                "ownerId": pool["ownerId"],
        #                                "groupUuid": pool.get("groupUuid"),
        #                                "projectGrade": pool.get("projectGrade"),
        #                                "reward": pool.get("reward"),
        #                                "hints": bool(pool.get("hints"))}
        #                               for pool in row["eligible_pools"]]

        exp_bucket = ActionsSqueezerToloka.check_experiments_toloka(args, row["worker_id"])
        squeezed.update({"action_index": 0, sq_common.EXP_BUCKET_FIELD: exp_bucket})
        args.result_actions.append(squeezed)

    @staticmethod
    def get_actions_wps(args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        row = args.container
        assert isinstance(row, dict)
        if not row.get("worker_attributes"):
            return

        squeezed = {"type": "pool_selection"}

        squeezed["ts"] = row["start_time"]
        squeezed["yuid"] = row["worker_id"]

        squeezed["worker_attributes_computed"] = ActionsSqueezerToloka.get_computed_attributes(row["worker_attributes"])
        squeezed["worker_attributes_profile"] = ActionsSqueezerToloka.get_profile_attributes(row["worker_attributes"])

        squeezed["entry_id"] = row["ref_uuid"]
        squeezed["pool_id"] = row["pool_id"]
        squeezed["assignment_id"] = row["assignment_id"]

        # squeezed["context"] = {"visibleGroupsUuids": (row.get("context") or {}).get("visibleGroupsUuids")}

        exp_bucket = ActionsSqueezerToloka.check_experiments_toloka(args, row["worker_id"])
        squeezed.update({"action_index": 0, sq_common.EXP_BUCKET_FIELD: exp_bucket})
        args.result_actions.append(squeezed)

    @staticmethod
    def rv56_query(day):
        fields_to_get = ", ".join(["worker_id",
                                   "worker_attributes",
                                   "assignment_assignment_id",
                                   "pool_id",
                                   "project_id",
                                   "requester_id",
                                   "requester_company_is_yandex",
                                   "pool_training",
                                   "assignment_automerged",
                                   "assignment_status",
                                   "task_point",
                                   "assignment_price",
                                   "pool_post_accept",
                                   "task_suite_checkpoint_task_ids",
                                   "assignment_gs_count",
                                   "assignment_gs_correct_count",
                                   "assignment_gs_wrong_count",
                                   "assignment_start_time",
                                   "assignment_submit_time",
                                   "assignment_skip_time",
                                   "assignment_expire_time",
                                   "assignment_approve_time",
                                   "assignment_reject_time"])

        ts1 = calendar.timegm(day.timetuple())
        ts2 = calendar.timegm((day + datetime.timedelta(days=1)).timetuple())
        t_cond = "({} BETWEEN {} AND {})"
        timestamp_names = ["assignment_start_time",
                           "assignment_submit_time",
                           "assignment_skip_time",
                           "assignment_expire_time",
                           "assignment_approve_time",
                           "assignment_reject_time"]

        filters = " OR ".join(t_cond.format(timestamp_name, ts1, ts2) for timestamp_name in timestamp_names)
        return fields_to_get + "\n" + "WHERE " + filters

    @staticmethod
    def get_actions_rv56(args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        row = args.container
        assert isinstance(row, dict)
        if not row.get("worker_attributes") or row["assignment_automerged"]:
            return

        squeezed = {}

        squeezed["yuid"] = row["worker_id"]

        squeezed["worker_attributes_computed"] = ActionsSqueezerToloka.get_computed_attributes(row["worker_attributes"])
        squeezed["worker_attributes_profile"] = ActionsSqueezerToloka.get_profile_attributes(row["worker_attributes"])

        squeezed["assignment_id"] = row["assignment_assignment_id"]
        squeezed["pool_id"] = row["pool_id"]
        squeezed["project_id"] = row["project_id"]
        squeezed["requester_id"] = row["requester_id"]
        squeezed["requester_company_is_yandex"] = row["requester_company_is_yandex"]
        squeezed["pool_training"] = row["pool_training"]
        squeezed["assignment_status"] = row["assignment_status"]
        squeezed["task_point"] = row["task_point"]
        squeezed["assignment_price"] = float(row["assignment_price"])

        squeezed["pool_post_accept"] = row["pool_post_accept"]
        squeezed["n_microtasks"] = len(row["task_suite_checkpoint_task_ids"])
        squeezed["assignment_gs_count"] = row["assignment_gs_count"]
        squeezed["assignment_gs_correct_count"] = row["assignment_gs_correct_count"]
        squeezed["assignment_gs_wrong_count"] = row["assignment_gs_wrong_count"]

        squeezed["assignment_start_time"] = row["assignment_start_time"]
        squeezed["assignment_submit_time"] = row["assignment_submit_time"]
        squeezed["assignment_skip_time"] = row["assignment_skip_time"]
        squeezed["assignment_expire_time"] = row["assignment_expire_time"]
        squeezed["assignment_approve_time"] = row["assignment_approve_time"]
        squeezed["assignment_reject_time"] = row["assignment_reject_time"]

        exp_bucket = ActionsSqueezerToloka.check_experiments_toloka(args, row["worker_id"])
        squeezed.update({"action_index": 0, sq_common.EXP_BUCKET_FIELD: exp_bucket})

        ts1 = calendar.timegm(args.day.timetuple())
        ts2 = calendar.timegm((args.day + datetime.timedelta(days=1)).timetuple())

        start_time = squeezed["assignment_start_time"]
        if start_time and (ts1 <= start_time < ts2):
            squeezed_local = deepcopy(squeezed)
            squeezed_local["type"] = "assignment_start"
            squeezed_local["ts"] = start_time
            args.result_actions.append(squeezed_local)

        submit_time = squeezed["assignment_submit_time"]
        if submit_time and (ts1 <= submit_time < ts2):
            squeezed_local = deepcopy(squeezed)
            squeezed_local["type"] = "assignment_submition"
            squeezed_local["ts"] = submit_time
            args.result_actions.append(squeezed_local)

        skip_time = squeezed["assignment_skip_time"]
        if skip_time and (ts1 <= skip_time < ts2):
            squeezed_local = deepcopy(squeezed)
            squeezed_local["type"] = "assignment_skip"
            squeezed_local["ts"] = skip_time
            args.result_actions.append(squeezed_local)

        expire_time = squeezed["assignment_expire_time"]
        if expire_time and (ts1 <= expire_time < ts2):
            squeezed_local = deepcopy(squeezed)
            squeezed_local["type"] = "assignment_expiration"
            squeezed_local["ts"] = expire_time
            args.result_actions.append(squeezed_local)

        approve_time = squeezed["assignment_approve_time"]
        if approve_time and (ts1 <= approve_time < ts2):
            squeezed_local = deepcopy(squeezed)
            squeezed_local["type"] = "assignment_approval"
            squeezed_local["ts"] = approve_time
            args.result_actions.append(squeezed_local)

        reject_time = squeezed["assignment_reject_time"]
        if reject_time and (ts1 <= reject_time < ts2):
            squeezed_local = deepcopy(squeezed)
            squeezed_local["type"] = "assignment_rejection"
            squeezed_local["ts"] = reject_time
            args.result_actions.append(squeezed_local)

    @staticmethod
    def get_actions_worker_daily_statistics(args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        row = args.container
        assert isinstance(row, dict)
        if not row.get("worker_attributes"):
            return

        squeezed = {"type": "worker_daily_statistics"}

        squeezed["yuid"] = row["worker_id"]

        squeezed["worker_attributes_computed"] = ActionsSqueezerToloka.get_computed_attributes(row["worker_attributes"], keys=["time_class_14_days"])
        squeezed["worker_attributes_profile"] = ActionsSqueezerToloka.get_profile_attributes(row["worker_attributes"])

        squeezed["project_id"] = row["project_id"]
        squeezed["gs_correct"] = row["gs_correct"]
        squeezed["pa_correct"] = row["pa_correct"]
        squeezed["mv_correct"] = row["mv_correct"]
        squeezed["gs_total"] = row["gs_total"]
        squeezed["pa_total"] = row["pa_total"]
        squeezed["mv_total"] = row["mv_total"]
        squeezed["accuracy_gs"] = row["accuracy_gs"]
        squeezed["accuracy_pa"] = row["accuracy_pa"]
        squeezed["accuracy_mv"] = row["accuracy_mv"]

        date = datetime.datetime.strptime(row["date"], "%Y-%m-%d")
        squeezed["ts"] = calendar.timegm(date.timetuple())

        exp_bucket = ActionsSqueezerToloka.check_experiments_toloka(args, row["worker_id"])
        squeezed.update({"action_index": 0, sq_common.EXP_BUCKET_FIELD: exp_bucket})
        args.result_actions.append(squeezed)

    @staticmethod
    def check_experiments_toloka(args, worker_id):
        """
        :type worker_id: str
        :type args: ActionSqueezerArguments
        :rtype: ExpBucketInfo
        """
        row = args.container
        assert isinstance(row, dict)
        if row.get("worker_attributes"):
            worker_attributes = row["worker_attributes"]
        elif row.get("request"):
            worker_attributes = row["request"]["workerAttributes"]
        else:
            return sq_common.ExpBucketInfo()

        computed_attributes = worker_attributes["attrIndex"]["computed"]
        testid_triples = (computed_attributes.get("ab_toloka_testids") or "").split(";")
        worker_testids = {testid_triple.split(",")[0] for testid_triple in testid_triples if testid_triple}

        matched_experiments = {exp for exp in args.experiments if exp.experiment.testid in worker_testids}
        args.result_experiments = matched_experiments
        return sq_common.ExpBucketInfo(matched=matched_experiments)
