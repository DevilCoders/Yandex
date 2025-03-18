import itertools
import session_squeezer.squeezer_common as sq_common
from session_squeezer.squeezer_news import ActionsSqueezerNews
from session_squeezer.squeezer_web import (
    ActionsSqueezerWebDesktop,
    ActionsSqueezerWebTouch,
)
from session_squeezer.squeezer_web_extended import (
    ActionsSqueezerWebDesktopExtended,
    ActionsSqueezerWebTouchExtended,
)


class ActionsSqueezerCacheDesktop(ActionsSqueezerWebDesktop):
    USE_LIBRA = False
    USE_ANY_FILTER = True

    def __init__(self):
        super(ActionsSqueezerCacheDesktop, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        _get_actions(args, add_without_reqid=True)


class ActionsSqueezerCacheTouch(ActionsSqueezerWebTouch):
    USE_LIBRA = False
    USE_ANY_FILTER = True

    def __init__(self):
        super(ActionsSqueezerCacheTouch, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        _get_actions(args, add_without_reqid=True)


class ActionsSqueezerCacheDesktopExtended(ActionsSqueezerWebDesktopExtended):
    USE_LIBRA = False
    USE_ANY_FILTER = True

    def __init__(self):
        super(ActionsSqueezerCacheDesktopExtended, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        _get_actions(args, add_without_reqid=False)


class ActionsSqueezerCacheTouchExtended(ActionsSqueezerWebTouchExtended):
    USE_LIBRA = False
    USE_ANY_FILTER = True

    def __init__(self):
        super(ActionsSqueezerCacheTouchExtended, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        _get_actions(args, add_without_reqid=False)


class ActionsSqueezerCacheNews(ActionsSqueezerNews):
    USE_LIBRA = False
    USE_ANY_FILTER = True

    def __init__(self):
        super(ActionsSqueezerCacheNews, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        _get_actions(args, add_without_reqid=False)


def _get_actions(args, add_without_reqid):
    """
    :type args: sq_common.ActionSqueezerArguments
    """
    reqid_info = {}
    for group_type, group in itertools.groupby(args.container, key=lambda x: x["@table_index"]):
        if group_type == 0:
            for row in group:
                reqid_info[row["reqid"]] = row

        elif group_type == 1:
            without_reqid = []

            for row in group:
                reqid = row["reqid"]
                if reqid:
                    exp_bucket = sq_common.ExpBucketInfo()
                    if reqid in reqid_info:
                        exp_bucket = check_experiments_test(
                            args,
                            dict(reqid_info[reqid]["test_buckets"]),
                            set(reqid_info[reqid]["filters"])
                        )
                    row[sq_common.EXP_BUCKET_FIELD] = exp_bucket
                    args.result_actions.append(row)
                else:
                    without_reqid.append(row)

            if args.result_experiments and add_without_reqid:
                event_exp_bucket = sq_common.ExpBucketInfo()
                for row in without_reqid:
                    assert row["type"] == "nav-suggest-click"

                    row[sq_common.EXP_BUCKET_FIELD] = event_exp_bucket
                    args.result_actions.append(row)


def check_experiments_test(args, test_buckets, filters):
    """
    :type args: ActionSqueezerArguments
    :type test_buckets: dict[str,int]
    :type filters: set[str]
    :rtype ExpBucketInfo
    """
    result = sq_common.ExpBucketInfo()
    for exp in args.experiments:
        if exp.all_users or exp.all_for_history or exp.testid in test_buckets:
            result.buckets[exp] = test_buckets.get(exp.testid)
            args.result_experiments.add(exp)
            if exp.filters.filter_hash is None or exp.filters.filter_hash in filters:
                result.matched.add(exp)
    return result
