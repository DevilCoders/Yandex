import session_squeezer.squeezer_common as sq_common


ZEN_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},
]


class ActionsSqueezerZen(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: version which works with original tables
    3: add using of session tables
    4: added various item_types
    5: add video-events, change ts priority, add shorts
    6: KOAN-847 added carousels, fixed dwelltime for narratives
    7: KOAN-1145 fix last session missing, change sessions splitter, fix pos, remove dwell_time calculation and similar:click handler
    8: Add white list of events for session handler
    9: Change path to background sessions log
    10: Add ads events, grid_type(24.03.2020), card_width(24.03.2020), preview handler.
    11: Fix duplicates in teeth
    12: Fix of cutting dwelltime by subscription button at the article page. Remove small teaser from morda.
    13: Fix bug with subscription button.
    14: Support group_ids (digital zen testids), add item_height
    15: Determine the multiple_hash function, add NOT_CONTINUE_SESSION_EVENTS
    16: Add clicks to NOT_CONTINUE_SESSION_EVENTS
    17: Fix carousel position missing
    18: Massive refactoring, some bugfixes, answers and complete_read
    19: flow mode and two bugfixes with testids
    20: Save real item_id and real item_type in squeeze
    21: include heartbeats with item_id=0
    22: add ecom
    23: add events in articles, ignore zero pos in view
    24: fixed empty interview_id
    25: fixed heartbeats with item_id=0
    26: new log: changed uid and yandexuid to uint64, removed service_type
    27: supported forced_rid, copied real rid to real_rid
    28: stored card_type into real_card_type, linked teeth cards to articles
    29: parent_type+parent_id instead of card_id
    30: add is_short
    31: add video_duration_seconds, video_has_sound fields and share event
    32: begin to squeeze using yql (MSTAND-2097)
    33: fix formation of the is_match field (MSTAND-2120)
    """
    VERSION = 33

    YT_SCHEMA = ZEN_YT_SCHEMA
    USE_LIBRA = False
    UNWRAP_CONTAINER = False
