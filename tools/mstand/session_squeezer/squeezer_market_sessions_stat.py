import logging
import json
import session_squeezer.squeezer_common as sq_common
from session_squeezer.squeezer_common import ActionSqueezerArguments  # noqa

import scarab.main as scarab
import scarab.common.events as scarab_common

MARKET_SESSIONS_STAT_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "access_type", "type": "string"},
    {"name": "bucket", "type": "int64"},
    {"name": "category_id", "type": "int64"},
    {"name": "cpc_trans", "type": "int64"},
    {"name": "fraud_diagnosis", "type": "string"},
    {"name": "hyper_id", "type": "string"},
    {"name": "is_cpa", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "item_count", "type": "int64"},
    {"name": "method", "type": "string"},
    {"name": "offer_id", "type": "string"},
    {"name": "offer_price", "type": "double"},
    {"name": "order_id", "type": "int64"},
    {"name": "orders", "type": "int64"},
    {"name": "referer", "type": "string"},
    {"name": "reqid", "type": "string"},
    {"name": "request", "type": "string"},
    {"name": "servicetype", "type": "string"},
    {"name": "show_uid", "type": "string"},
    {"name": "status", "type": "int64"},
    {"name": "testid", "type": "string"},
    {"name": "type", "type": "string"},
    {"name": "user_interface", "type": "string"},
    {"name": "object_id", "type": "string"},
    {"name": "place", "type": "string"},
    {"name": "wprid", "type": "string"},
    {"name": "rgb", "type": "string"},
    {"name": "pof", "type": "int64"},
]

MAX_RECORDS_COUNT = 25000


class ActionsSqueezerMarketSessionsStat(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: Filter only non-technical events
    3: Add clicks, cpa-clicks and show event to squeeze and add 'type' field
    4: Remove records duplicating
    5: Add offer card clicks and caterory id field to squeeze
    6: Remove show events from squeeze and fix filtering card model events
    7: Add referer and access_type to squeeze schema
    8: Make referer field definition more relyable
    9: Add fields for custom market metrics: request, status, method
    10: Add field for MARKET_FRONT_ACCESS_EVENT: user_interface
    11: Add show_uid and fraud_diagnosis for click events
    12: Add cpc_trans, is_cpa, item_count, offer_id, offer_price, order_id, orders for click events
    13: Add recommond events
    14: Add hyper_id field
    15: Add wprid field
    16: Skip 'fat' users (over 25000 records)
    17: Add rgb field
    18: Add value of category_id for MARKET_CLICK_ORDER_PLACED_EVENT, add pof field
    """
    VERSION = 18

    YT_SCHEMA = MARKET_SESSIONS_STAT_YT_SCHEMA

    USE_LIBRA = False

    def __init__(self):
        super(ActionsSqueezerMarketSessionsStat, self).__init__()

    @staticmethod
    def cookies2dict(cookies):
        pairs = cookies.replace(" ", "").split(";")
        pairs = [p.split("=", 1) for p in pairs if '=' in p]
        cook_dict = {k: v for k, v in pairs}
        return cook_dict

    @staticmethod
    def get_category_id(event):
        if event.type == scarab_common.EventType.MARKET_FRONT_ACCESS_EVENT and hasattr(event, "request_params") and event.request_params is not None:
            params = json.loads(event.request_params.replace('\\', '\\\\'))
            if "productId" in params and "categoryId" in params:
                return int(params["categoryId"])
        elif event.type in [scarab_common.EventType.MARKET_CLICK_EVENT, scarab_common.EventType.MARKET_CPA_CLICK_EVENT,
                            scarab_common.EventType.MARKET_OFFER_CARD_CLICK_EVENT, scarab_common.EventType.MARKET_CLICK_ORDER_PLACED_EVENT]:
            return int(event.hyper_category_id)
        return None

    @staticmethod
    def get_timestamp(event):
        ets = event.timestamp // 1000  # In events timestamp in milliseconds
        # Old treshold: sep_ts = 1512075600  # 2017-12-01 00:00:00 GMT+3
        sep_ts = 1509483600  # 2017-11-01 00:00:00 GMT+3
        if event.type == scarab_common.EventType.MARKET_CLICK_ORDER_PLACED_EVENT:
            if ets >= sep_ts and ets <= sep_ts + 3 * 60 * 60:
                return None
            if ets < sep_ts:
                return ets - 3 * 60 * 60
        return ets

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        actions = []
        actions_count = 0
        for row in args.container:
            if actions_count > MAX_RECORDS_COUNT:
                return
            if not self.check_request(row):
                continue
            yuid = str(row["key"])
            try:
                event = scarab.deserialize_event_from_str(row["value"])

                if not event:
                    raise Exception("Cannot parse record")

                if self.allowed_event(event):
                    actions_count += 1
                    ts = self.get_timestamp(event)
                    if ts is None:
                        continue
                    exp_bucket = check_experiments(args, event)
                    atype = None
                    request = None
                    status = None
                    method = None
                    user_interface = None
                    object_id = None
                    place = None
                    wprid = None
                    if event.type == scarab_common.EventType.MARKET_FRONT_ACCESS_EVENT:
                        atype = event.request_access_type
                        request = event.request_raw
                        status = event.status.value
                        method = event.method
                        user_interface = str(event.user_interface)
                        wprid = self.cookies2dict(event.cookies).get("market_ys", None)
                    elif event.type in (scarab_common.EventType.MARKET_RECOMMEND_CLICK_EVENT, scarab_common.EventType.MARKET_RECOMMEND_PREVIEW_EVENT):
                        user_interface = str(event.user_interface)
                        object_id = event.object_id
                        place = event.place

                    if not wprid and getattr(event, "web_parent_request_id", None):
                        wprid = event.web_parent_request_id.value
                    referer = None
                    if hasattr(event, "referer") and event.referer:
                        referer = event.referer.encode("ascii", "ignore")
                    fraud_diagnosis = getattr(event, "fraud_diagnosis", None)
                    if event.type == scarab_common.EventType.MARKET_CPA_CLICK_EVENT:
                        fraud_diagnosis = "0"
                    offer_price = getattr(event, "offer_price", None)
                    if offer_price is not None:
                        offer_price = float(offer_price)
                    squeezed = {
                        "yuid": yuid,
                        "ts": ts,
                        "action_index": 0,
                        "access_type": atype,
                        "category_id": self.get_category_id(event),
                        "cpc_trans": getattr(event, "cpc_trans", None),
                        "fraud_diagnosis": fraud_diagnosis,
                        "hyper_id": getattr(event, "hyper_id", None),
                        "is_cpa": getattr(event, "is_cpa", None),
                        "item_count": getattr(event, "item_count", None),
                        "method": method,
                        "offer_id": getattr(event, "offer_id", None),
                        "offer_price": offer_price,
                        "order_id": getattr(event, "order_id", None),
                        "orders": getattr(event, "orders", None),
                        "referer": referer,
                        "reqid": str(event.request_id),
                        "request": request,
                        "show_uid": getattr(event, "show_uid", None),
                        "status": status,
                        "type": str(event.type),
                        "user_interface": user_interface,
                        "object_id": object_id,
                        "place": place,
                        "wprid": wprid,
                        "rgb": getattr(event, "rgb", None),
                        "pof": getattr(event, "pof", None),
                        sq_common.EXP_BUCKET_FIELD: exp_bucket,
                    }
                    actions.append(squeezed)
            except:
                logging.error("Suspicious record {}".format(row["value"]))
                raise
        args.result_actions += actions

    @staticmethod
    def allowed_event(event):
        """
        :type event:
        :rtype: bool
        """
        allowed_event_types = {
            scarab_common.EventType.MARKET_FRONT_ACCESS_EVENT,
            scarab_common.EventType.MARKET_CLICK_EVENT,
            scarab_common.EventType.MARKET_CPA_CLICK_EVENT,
            scarab_common.EventType.MARKET_CLICK_ORDER_PLACED_EVENT,
            scarab_common.EventType.MARKET_OFFER_CARD_CLICK_EVENT,
            scarab_common.EventType.MARKET_RECOMMEND_CLICK_EVENT,
            scarab_common.EventType.MARKET_RECOMMEND_PREVIEW_EVENT,
        }
        if event.type not in allowed_event_types:
            return False

        if event.type == scarab_common.EventType.MARKET_FRONT_ACCESS_EVENT:
            if event.is_technical:
                return False
        return True

    def check_request(self, request):
        return request["value"][0] == "{"


def is_testid_match(request, exp):
    """
    :type request: scarab TEvent
    :type exp: ExperimentForSqueeze
    :rtype: bool
    """
    if exp.all_users or exp.all_for_history:
        return True
    return has_test_id(request, exp.testid)


def has_test_id(request, testid):
    for exp in request.experiments:
        if exp.test_id == testid:
            return True
    return False


def get_bucket(request, exp):
    for experiment in request.experiments:
        if experiment.test_id == exp.testid:
            return experiment.bucket
    return None


def check_experiments(args, request):
    """
    :type args: ActionSqueezerArguments
    :type request: scarab TEvent
    :rtype ExpBucketInfo
    """
    result = sq_common.ExpBucketInfo()
    for exp in args.experiments:
        result.buckets[exp] = get_bucket(request, exp)
        if is_testid_match(request, exp):
            args.result_experiments.add(exp)
            result.matched.add(exp)
    return result
