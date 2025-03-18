import session_squeezer.squeezer_common as sq_common

VH_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "view_time", "type": "int64"},
    {"name": "content_duration", "type": "int64"},
]

COLUMN_NAMES = {item["name"] for item in VH_YT_SCHEMA}


class ActionsSqueezerVH(sq_common.ActionsSqueezer):
    """
    1: initial version
    """
    VERSION = 1

    YT_SCHEMA = VH_YT_SCHEMA
    USE_LIBRA = False

    def __init__(self):
        super(ActionsSqueezerVH, self).__init__()

    def get_actions(self, args: sq_common.ActionSqueezerArguments):
        assert not args.result_actions

        for row in args.container:
            """:type row: dict[str]"""
            squeezed = {key: value for key, value in row.items() if key in COLUMN_NAMES}
            exp_bucket = self.check_experiments_by_testids(args, row["testids"])
            squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket
            args.result_actions.append(squeezed)
