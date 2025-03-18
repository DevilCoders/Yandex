import json
import session_squeezer.squeezer_common as sq_common

YA_METRICS_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "event", "type": "string"},
    {"name": "params", "type": "any"},
    {"name": "os", "type": "string"},
    {"name": "os_version", "type": "string"},
    {"name": "browser", "type": "string"},
    {"name": "browser_version", "type": "string"},
]


class ActionsSqueezerYaMetricsToloka(sq_common.ActionsSqueezer):
    """
    1: initial version
    """
    VERSION = 1

    YT_SCHEMA = YA_METRICS_YT_SCHEMA
    USE_LIBRA = False

    def __init__(self):
        super(ActionsSqueezerYaMetricsToloka, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        assert not args.result_actions

        testids = sq_common.get_testids(args.container, "metrikaexp")
        if not testids:
            return

        # noinspection PyUnresolvedReferences,PyPackageRequirements
        import uatraits
        detector = uatraits.detector("browser.xml")

        for row in args.container:
            """:type row: dict[str]"""
            if not row["url"].startswith("goal") or row["url"].endswith("api-request"):
                continue

            useragent = detector.detect(row["useragent"])

            squeezed = {
                "ts": int(row["ts"]),
                "event": row["url"].split("/")[-1].replace("%3A", ":"),
                "params": json.loads(row["params"]) if row["params"] else None,
                "os": useragent.get("OSFamily"),
                "os_version": useragent.get("OSVersion"),
                "browser": useragent.get("BrowserName"),
                "browser_version": useragent.get("BrowserVersion"),
            }

            exp_bucket = self.check_experiments_by_testids(args, testids)
            squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket
            args.result_actions.append(squeezed)
