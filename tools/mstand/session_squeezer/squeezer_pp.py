import session_squeezer.squeezer_common as sq_common

PP_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "reqid", "type": "string"},
]

COLUMN_NAMES = {item["name"]: item["type"] for item in PP_YT_SCHEMA}


class ActionsSqueezerPp(sq_common.ActionsSqueezer):
    """
    1: initial version
    """
    VERSION = 1

    YT_SCHEMA = PP_YT_SCHEMA
    USE_LIBRA = False

    def __init__(self) -> None:
        super(ActionsSqueezerPp, self).__init__()

    def get_actions(self, args: sq_common.ActionSqueezerArguments) -> None:
        for row in args.container:
            """:type row: dict[str]"""
            squeezed = {key: value for key, value in row.items() if key in COLUMN_NAMES}
            squeezed["ts"] //= 1000

            testids = {str(testid) for testid in row["testids"]}
            exp_bucket = sq_common.ActionsSqueezer.check_experiments_by_testids(args, testids)
            squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket

            args.result_actions.append(squeezed)
