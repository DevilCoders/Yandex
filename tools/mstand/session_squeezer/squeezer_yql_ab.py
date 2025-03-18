import json
import time

from typing import Any
from typing import Dict

import session_squeezer.squeezer_common as sq_common

YQL_AB_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "metrics", "type": "any"},
]


class ActionsSqueezerYqlAb(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: fix the duplicate output row issue (MSTAND-1922)
    3: fix using the first row for different slices and testids (MSTAND-1958)
    """
    VERSION = 3

    YT_SCHEMA = YQL_AB_YT_SCHEMA
    USE_LIBRA = False
    USE_ANY_FILTER = True

    def __init__(self):
        super(ActionsSqueezerYqlAb, self).__init__()

    def get_actions(self, args: sq_common.ActionSqueezerArguments):
        assert len(args.container) == 1

        for row in args.container:
            """:type row: dict[str]"""
            squeezed = {
                "ts": int(time.mktime(args.day.timetuple())),
                "metrics": json.loads(row["yql_ab_metrics"]),
            }

            exp_bucket = check_experiments(args, row)
            squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket
            args.result_actions.append(squeezed)


def check_experiments(args: sq_common.ActionSqueezerArguments, row: Dict[str, Any]) -> sq_common.ExpBucketInfo:
    result = sq_common.ExpBucketInfo()
    for exp in args.experiments:
        if exp.testid == row["testid"] or exp.all_users:
            exp_filter_hash = exp.filter_hash or "d41d8cd98f00b204e9800998ecf8427e"
            if exp_filter_hash == row["slice"]:
                result.buckets[exp] = None
                args.result_experiments.add(exp)
                result.matched.add(exp)
    return result
