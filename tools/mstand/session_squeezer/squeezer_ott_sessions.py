import session_squeezer.squeezer_common as sq_common


OTT_SESSIONS_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "end_dw_ts", "type": "int64"},
    {"name": "end_ts", "type": "int64"},
    {"name": "first_impression_ts", "type": "uint32"},
    {"name": "first_longview_ts", "type": "uint32"},
    {"name": "first_sessions_ts", "type": "uint32"},
    {"name": "first_start_ts", "type": "uint32"},
    {"name": "first_view_ts", "type": "uint32"},
    {"name": "first_zones_ts", "type": "uint32"},
    {"name": "longviews", "type": "uint64"},
    {"name": "service", "type": "string"},
    {"name": "session_dw_time_sec", "type": "int64"},
    {"name": "session_id", "type": "string"},
    {"name": "session_id_dwelltime", "type": "int32"},
    {"name": "session_time_sec", "type": "int64"},
    {"name": "session_view_time_sec", "type": "int64"},
    {"name": "session_view_time_sec_real", "type": "int64"},
    {"name": "start_date", "type": "string"},
    {"name": "starts", "type": "uint64"},
    {"name": "views", "type": "uint64"},
]

COLUMN_NAMES = {item["name"] for item in OTT_SESSIONS_YT_SCHEMA}


class ActionsSqueezerOttSessions(sq_common.ActionsSqueezer):
    """
    1: initial version
    """
    VERSION = 1

    YT_SCHEMA = OTT_SESSIONS_YT_SCHEMA
    USE_LIBRA = False

    def __init__(self) -> None:
        super(ActionsSqueezerOttSessions, self).__init__()

    def get_actions(self, args: sq_common.ActionSqueezerArguments) -> None:
        sq_common.get_kinopoisk_actions(args, column_names=COLUMN_NAMES, testids_field_name="test_ids")
