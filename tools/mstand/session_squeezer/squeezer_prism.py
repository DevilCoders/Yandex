import itertools
import time
import yaqutils.six_helpers as usix
import session_squeezer.squeezer_common as sq_common

PRISM_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "CPT", "type": "double"},
    {"name": "cluster", "type": "string"},
    {"name": "norm_serp_revenue", "type": "double"},
]

COLUMN_NAMES = {item["name"]: item["type"] for item in PRISM_YT_SCHEMA}


class ActionsSqueezerPrism(sq_common.ActionsSqueezer):
    """
    1: initial version
    """
    VERSION = 1

    YT_SCHEMA = PRISM_YT_SCHEMA
    USE_LIBRA = False

    def __init__(self):
        super(ActionsSqueezerPrism, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        testids = set()

        for group_type, group in itertools.groupby(args.container, key=lambda x: x["table_index"]):
            if group_type == 0:
                for row in group:
                    testids.update(row["value"].split("\t"))

            else:
                for row in group:
                    """:type row: dict[str]"""
                    squeezed = {key: value for key, value in usix.iteritems(row) if key in COLUMN_NAMES}
                    squeezed["yuid"] = row["key"]
                    squeezed["ts"] = int(time.mktime(args.day.timetuple()))

                    exp_bucket = self.check_experiments_by_testids(args, testids)
                    squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket

                    args.result_actions.append(squeezed)
