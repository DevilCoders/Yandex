import yaqutils.six_helpers as usix
import session_squeezer.squeezer_common as sq_common

ETHER_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "ad_events", "type": "any"},
    {"name": "ad_tracking_events", "type": "any"},
    {"name": "channel", "type": "string"},
    {"name": "content_duration", "type": "int32"},
    {"name": "midrolls_count", "type": "int32"},
    {"name": "prerolls_count", "type": "int32"},
    {"name": "price", "type": "int32"},
    {"name": "ref_from", "type": "string"},
    {"name": "shows", "type": "any"},
    {"name": "sources_aggr", "type": "string"},
    {"name": "stream_block", "type": "string"},
    {"name": "UUID", "type": "string"},
    {"name": "video_content_id", "type": "string"},
    {"name": "view_time", "type": "int32"},
    {"name": "view_time_non_muted", "type": "int32"},
    {"name": "vsid", "type": "string"},
    {"name": "winhits", "type": "any"},
]

COLUMN_NAMES = {item["name"] for item in ETHER_YT_SCHEMA}


class ActionsSqueezerEther(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: add view_time column (MSTAND-1733)
    """
    VERSION = 2

    YT_SCHEMA = ETHER_YT_SCHEMA
    USE_LIBRA = False

    def __init__(self):
        super(ActionsSqueezerEther, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        assert not args.result_actions

        testids = sq_common.get_testids(args.container, "test_buckets")
        if not testids:
            return

        for row in args.container:
            """:type row: dict[str]"""
            squeezed = {key: value for key, value in usix.iteritems(row) if key in COLUMN_NAMES}

            squeezed["shows"] = [item for item in row["ad_events"] if is_show(item)]
            squeezed["winhits"] = [item for item in row["ad_events"] if is_winhit(item)]

            squeezed["prerolls_count"] = len([item for item in squeezed["shows"] if is_preroll(item)])
            squeezed["midrolls_count"] = len([item for item in squeezed["shows"] if is_midroll(item)])

            exp_bucket = self.check_experiments_by_testids(args, testids)
            squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket
            args.result_actions.append(squeezed)


def ether_mapper(row):
    if row["yandexuid"] is not None and is_ether(row["ref_from"]):
        row["yuid"] = row["yandexuid"]
        row["ts"] = row["timestamp"]
        yield row


def is_ether(ref_from):
    return ref_from in (
        "efir",
        "efir_touch",
        "efir_turboapp",
        "morda",
        "morda_touch",
        "streamhandler_appsearch",
        "streamhandler_other",
        "videohub",
        "videohub_touch",
    )


def is_show(item):
    return all((
        item["countertype"] == 1,
        item["win"] == 1,
        item["DspID"] not in (5, 10),
    ))


def is_winhit(item):
    return all((
        item["countertype"] == 0,
        item["win"] == 1,
        item["DspID"] not in (5, 10),
    ))


def is_preroll(item):
    return item["video_type"] == "preroll"


def is_midroll(item):
    return item["video_type"] == "midroll"
