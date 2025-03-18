import time

import yaqutils.six_helpers as usix
import session_squeezer.squeezer_common as sq_common

TURBO_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "ad_block_area", "type": "int64"},
    {"name": "ad_clicks", "type": "uint64"},
    {"name": "ad_revenue_fact_awaps", "type": "double"},
    {"name": "ad_revenue_fact_direct", "type": "double"},
    {"name": "ad_revenue_forecast_direct", "type": "double"},
    {"name": "ad_revenue_forecast_direct_ya", "type": "double"},
    {"name": "ad_revenue_forecast_not_direct_ya", "type": "double"},
    {"name": "ad_views_direct", "type": "uint64"},
    {"name": "ui", "type": "string"},
]

COLUMN_NAMES = {item["name"] for item in TURBO_YT_SCHEMA}


class ActionsSqueezerTurbo(sq_common.ActionsSqueezer):
    """
    1: initial version
    """
    VERSION = 1

    YT_SCHEMA = TURBO_YT_SCHEMA
    USE_LIBRA = False

    def __init__(self):
        super(ActionsSqueezerTurbo, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        assert not args.result_actions

        for row in args.container:
            """:type row: dict[str]"""
            squeezed = {key: value for key, value in usix.iteritems(row) if key in COLUMN_NAMES}
            squeezed["ts"] = int(time.mktime(args.day.timetuple()))

            testids = set(row["abt_testid"].split("\t"))
            exp_bucket = self.check_experiments_by_testids(args, testids)
            squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket
            args.result_actions.append(squeezed)
