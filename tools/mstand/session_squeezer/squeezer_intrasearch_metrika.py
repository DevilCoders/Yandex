import session_squeezer.squeezer_common as sq_common

INTRASEARCH_METRIKA_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "count", "type": "int32"},
    {"name": "currentPage", "type": "int32"},
    {"name": "dateClick", "type": "int64"},
    {"name": "dateRender", "type": "int64"},
    {"name": "event", "type": "string"},
    {"name": "height", "type": "int32"},
    {"name": "id", "type": "string"},
    {"name": "isInViewport", "type": "boolean"},
    {"name": "page", "type": "string"},
    {"name": "pos", "type": "int32"},
    {"name": "reqid", "type": "string"},
    {"name": "target", "type": "string"},
    {"name": "text", "type": "string"},
    {"name": "type", "type": "string"},
    {"name": "url", "type": "string"},
    {"name": "view", "type": "string"},
    {"name": "wizardId", "type": "string"},
]

COLUMN_NAMES = {item["name"] for item in INTRASEARCH_METRIKA_YT_SCHEMA}


class ActionsSqueezerIntrasearchMetrika(sq_common.ActionsSqueezer):
    """
    1: initial version
    """
    VERSION = 1

    YT_SCHEMA = INTRASEARCH_METRIKA_YT_SCHEMA
    USE_LIBRA = False

    def __init__(self):
        super(ActionsSqueezerIntrasearchMetrika, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        testids = {}
        for row in args.container:
            """:type row: dict[str]"""
            if row["event"] == "request":
                testids = self._parse_expbuckets(row.get("expBuckets"))

            squeezed = {k: v for k, v in row.items() if k in COLUMN_NAMES}

            exp_bucket = sq_common.ActionsSqueezer.check_experiments_by_testids(args, testids)
            squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket

            args.result_actions.append(squeezed)

    @staticmethod
    def _parse_expbuckets(expbuckets):
        try:
            testid, _, bucket = expbuckets.split(",")
            return {testid: int(bucket)}
        except:
            return {}
