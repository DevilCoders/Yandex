import session_squeezer.squeezer_common as sq_common


OTT_IMPRESSIONS_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "add_to_favorites", "type": "int64"},
    {"name": "cardPosition", "type": "string"},
    {"name": "eventSubtype", "type": "string"},
    {"name": "eventType", "type": "string"},
    {"name": "heuristics_session_length", "type": "int64"},
    {"name": "impressionType", "type": "string"},
    {"name": "mlSessionId", "type": "string"},
    {"name": "path", "type": "string"},
    {"name": "path0", "type": "string"},
    {"name": "path1", "type": "string"},
    {"name": "path2", "type": "string"},
    {"name": "path3", "type": "string"},
    {"name": "path4", "type": "string"},
    {"name": "path_reversed", "type": "string"},
    {"name": "pathId", "type": "string"},
    {"name": "pathId_reversed", "type": "string"},
    {"name": "selectionId", "type": "string"},
    {"name": "selectionName", "type": "string"},
    {"name": "selectionPosition", "type": "string"},
    {"name": "serviceName", "type": "string"},
    {"name": "session_length", "type": "string"},
    {"name": "sessionId", "type": "string"},
    {"name": "sessionStartDate", "type": "string"},
    {"name": "sessionStartTimestamp", "type": "int64"}
]

COLUMN_NAMES = {item["name"] for item in OTT_IMPRESSIONS_YT_SCHEMA}


class ActionsSqueezerOttImpressions(sq_common.ActionsSqueezer):
    """
    1: initial version
    """
    VERSION = 1

    YT_SCHEMA = OTT_IMPRESSIONS_YT_SCHEMA
    USE_LIBRA = False

    def __init__(self) -> None:
        super(ActionsSqueezerOttImpressions, self).__init__()

    def get_actions(self, args: sq_common.ActionSqueezerArguments) -> None:
        sq_common.get_kinopoisk_actions(args, column_names=COLUMN_NAMES,
                                        testids_field_name="testIds", convert_timestamp=True)
