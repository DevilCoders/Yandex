import yaqutils.six_helpers as usix
import yaqutils.time_helpers as utime
import session_squeezer.squeezer_common as sq_common

ALICE_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "app", "type": "string"},
    {"name": "expboxes", "type": "string"},  # MSTAND-1516
    {"name": "fielddate", "type": "string"},
    {"name": "generic_scenario", "type": "string"},  # MSTAND-1617
    {"name": "input_type", "type": "string"},
    {"name": "intent", "type": "string"},
    {"name": "other", "type": "any"},
    {"name": "platform", "type": "string"},
    {"name": "query", "type": "string"},
    {"name": "reply", "type": "string"},
    {"name": "req_id", "type": "string"},
    {"name": "server_time_ms", "type": "int64"},
    {"name": "session_id", "type": "string"},
    {"name": "session_sequence", "type": "int64"},
    {"name": "skill_id", "type": "string"},
    {"name": "version", "type": "string"},
]

COLUMN_NAMES = {item["name"] for item in ALICE_YT_SCHEMA}


class ActionsSqueezerAlice(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: Rename column from "expoboxes" to "expboxes" (MSTAND-1516)
    3: Changed path from //home/voice/... to //home/alice/...
    4: Changed ts source from server_time to client_time (MSTAND-1569)
    5: Add generic_scenario column (MSTAND-1617)
    6: Add extra columns and heartbeats rows (MSTAND-1630)
    7: Extend generic_scenario field (MSTAND-1717)
    """
    VERSION = 7

    YT_SCHEMA = ALICE_YT_SCHEMA
    USE_LIBRA = False

    def __init__(self):
        super(ActionsSqueezerAlice, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        assert not args.result_actions

        heartbeats_is_matched = set()
        heartbeats = []

        for row in args.container:
            """:type row: dict[str]"""
            if row["input_type"] != "heartbeat":
                squeezed = {key: value for key, value in usix.iteritems(row) if key in COLUMN_NAMES}
                exp_bucket = self.check_experiments_by_testids(args, row["testids"])
                squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket
                args.result_actions.append(squeezed)

                if check_heartbeats_ts(row, args.day):
                    heartbeats_is_matched |= exp_bucket.matched

            else:
                heartbeats.append(row)

        if args.result_experiments:
            for row in heartbeats:
                """:type row: dict[str]"""
                squeezed = {key: value for key, value in usix.iteritems(row) if key in COLUMN_NAMES}
                exp_bucket = sq_common.ExpBucketInfo(matched=heartbeats_is_matched)
                squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket
                args.result_actions.append(squeezed)


def check_heartbeats_ts(row, day):
    """
    :type row: dict[str]
    :type day: datetime.date
    :rtype bool
    """
    dates = utime.DateRange(day, day)
    min_timestamp = dates.start_timestamp - 15 * utime.Period.MINUTE
    max_timestamp = dates.end_timestamp + 15 * utime.Period.MINUTE
    return min_timestamp <= row["ts"] <= max_timestamp
