import logging
import time
import session_squeezer.squeezer_common as sq_common

YUID_REQID_TESTID_FILTER_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "reqid", "type": "string"},
    {"name": "test_buckets", "type": "any"},
    {"name": "filters", "type": "any"},
]


class ActionsSqueezerYuidReqidTestidFilter(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: use binary squeezer by default ONLINE-256
    """
    VERSION = 2

    YT_SCHEMA = YUID_REQID_TESTID_FILTER_YT_SCHEMA

    def __init__(self):
        super(ActionsSqueezerYuidReqidTestidFilter, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        for request in args.container.GetRequests():
            test_buckets = {str(t.TestID): t.Bucket for t in request.GetTestInfo()}
            if not test_buckets:
                continue
            filters = [
                uid for
                uid, libra_filter in args.cache_filters.items()
                if _check_filter(libra_filter, request)
            ]

            squeezed = {
                "ts": int(time.mktime(args.day.timetuple())),
                "reqid": request.ReqId,
                "test_buckets": test_buckets,
                "filters": filters,
            }

            exp_bucket = self.check_experiments_fake(args)
            squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket
            args.result_actions.append(squeezed)


def _check_filter(libra_filter, request):
    try:
        return libra_filter.Filter(request)
    except Exception as exc:
        logging.warning("reqid: %s, error: %s", request.ReqId, exc)
        return False
