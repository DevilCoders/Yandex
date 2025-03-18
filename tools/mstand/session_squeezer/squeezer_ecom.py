import session_squeezer.squeezer_common as sq_common

ECOM_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "domain", "type": "string"},
    {"name": "multiplier", "type": "float"},
    {"name": "order_id", "type": "string"},
    {"name": "revenue", "type": "float"},
    {"name": "source", "type": "string"},
]

COLUMN_NAMES = {item["name"]: item["type"] for item in ECOM_YT_SCHEMA}


class ActionsSqueezerEcom(sq_common.ActionsSqueezer):
    """
    1: initial version
    """
    VERSION = 1

    YT_SCHEMA = ECOM_YT_SCHEMA
    USE_LIBRA = False

    def __init__(self):
        super(ActionsSqueezerEcom, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        for row in args.container:
            """:type row: dict[str]"""
            squeezed = dict(self._parse_value(row["value"]))
            squeezed["yuid"] = row["key"]

            exp_bucket = sq_common.ActionsSqueezer.check_experiments_fake(args)
            squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket

            args.result_actions.append(squeezed)

    @staticmethod
    def _parse_value(value):
        values = [v.split("=", 1) for v in ("ts=" + value).split("\t")]
        for item in values:
            if len(item) != 2:
                continue

            k, v = item
            column_type = COLUMN_NAMES.get(k)
            if not column_type:
                continue

            if column_type.startswith("int"):
                yield k, int(v)
            elif column_type == "float":
                yield k, float(v)
            else:
                yield k, v
