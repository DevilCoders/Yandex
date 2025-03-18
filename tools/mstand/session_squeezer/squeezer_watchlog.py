import logging

from typing import Any
from typing import Dict

import session_squeezer.squeezer_common as sq_common
import yaqutils.url_helpers as uurl
import yt.yson

WATCHLOG_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "referer_host_path", "type": "string"},
    {"name": "servicetype", "type": "string"},
    {"name": "testid", "type": "string"},
    {"name": "type", "type": "string"},
    {"name": "url", "type": "string"},
]

WATCHLOG_SAMPLE = u"""{"action_index":0,"bucket":null,"is_match":true,"referer_host_path":"www.google.com.do/",
"servicetype":"watchlog","testid":"0","ts":1464811741,"type":"watchlog-TRAFFIC","url":"http://pornord.biz/",
"yuid":"y1000000001464581243"}"""


class ActionsSqueezerWatchlog(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: save referer host/path instead of full referer
    """
    VERSION = 2

    YT_SCHEMA = WATCHLOG_YT_SCHEMA
    SAMPLE = WATCHLOG_SAMPLE
    USE_LIBRA = False

    def __init__(self):
        super(ActionsSqueezerWatchlog, self).__init__()

    def get_actions(self, args: sq_common.ActionSqueezerArguments) -> None:
        WATCHLOG_ACTIONS_LIMIT = 100000

        assert not args.result_actions
        is_fat = False
        yuid = None
        row_count = 0
        not_unicode_row_count = 0
        for row in args.container:
            """:type row: dict[str]"""
            row_count += 1
            if not is_fat:
                if row_count - not_unicode_row_count < WATCHLOG_ACTIONS_LIMIT:
                    try:
                        squeezed = self.squeeze_watchlog_one(args, row)
                        args.result_actions.append(squeezed)
                    except yt.yson.yson_types.NotUnicodeError:
                        not_unicode_row_count += 1
                        yuid = row["key"]
                else:
                    is_fat = True
                    yuid = row["key"]
                    args.result_actions[:] = []
        if is_fat:
            logging.warning("skip fat user %s in watchlog (has %s rows)", yuid, row_count)
            args.result_actions[:] = []

        if not_unicode_row_count > 0:
            logging.warning("skip %d non-unicode events for user %s in watchlog (has %s rows)",
                            not_unicode_row_count, yuid, row_count)

    @staticmethod
    def squeeze_watchlog_one(args: sq_common.ActionSqueezerArguments, row: Dict[str, Any]) -> Dict[str, Any]:
        yuid = row["key"]
        timestamp = int(row["subkey"])
        fields = sq_common.parse_session_value(yuid, row["value"])
        action_type = "watchlog {}".format(fields.get("type")) if fields.get("type") else "watchlog"
        url = fields.get("url")
        referer_raw = fields.get("referer", "")
        try:
            referer_parsed = uurl.urlparse(referer_raw)
            referer_host_path = referer_parsed.netloc + referer_parsed.path
        except ValueError:
            logging.error("bad referer for %s at %d: %s", yuid, timestamp, referer_raw)
            referer_host_path = None
        exp_bucket = sq_common.ActionsSqueezer.check_experiments_fake(args)
        squeezed = {
            "ts": timestamp,
            "type": action_type,
            "url": url,
            "referer_host_path": referer_host_path,
            sq_common.EXP_BUCKET_FIELD: exp_bucket,
        }
        return squeezed
