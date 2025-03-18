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
    {"name": "wprid", "type": "string"},
    {"name": "wprid_ys", "type": "string"},
    {"name": "rgb", "type": "string"},
    {"name": "pof", "type": "int64"},
    {"name": "pp", "type": "int64"},
    {"name": "market_surplus_events", "type": "any"},
]

MAX_RECORDS_COUNT = 25000


class ActionsSqueezerMarketWebReqid(sq_common.ActionsSqueezer):
    """ Note that wprid is shifted by 1, since it uses `market_ys` cookie.
    1: initial version, mostly copied from squeezer_market_sessions_stat.py
    2: Add value of category_id for MARKET_CLICK_ORDER_PLACED_EVENT, add pof field
    3: Add experimental wprid_ys
    4: Add market surplus events explicitly
    """
    VERSION = 4

    YT_SCHEMA = MARKET_SESSIONS_STAT_YT_SCHEMA

    USE_LIBRA = False

    ALLOWED_EVENT_TYPES = {
        scarab_common.EventType.MARKET_FRONT_ACCESS_EVENT,
        scarab_common.EventType.MARKET_CLICK_EVENT,
        scarab_common.EventType.MARKET_CPA_CLICK_EVENT,
        scarab_common.EventType.MARKET_CLICK_ORDER_PLACED_EVENT,
        scarab_common.EventType.MARKET_OFFER_CARD_CLICK_EVENT,
    }

    MARKET_CARD_PP = [
        6,  13,  21,  26,  27,  46,  47,  49,  51,  52,  59,  61,  62,  63,  64,  65, 106, 116, 140, 141,
        144, 146, 147, 162, 164, 168, 169, 172, 173, 174, 200, 201, 205, 206, 207, 208, 209, 210, 211, 214,
        219, 234, 238, 239, 241, 242, 246, 247, 248, 249, 251, 252, 267, 269, 272, 273, 290, 292, 601, 602,
        603, 604, 606, 610, 613, 614, 616, 630, 631, 632, 634, 636, 639, 647, 648, 651, 652, 656, 657, 658,
        659, 662, 664, 668, 669, 690, 692, 2030, 2044, 2630, 2644
    ]

    def __init__(self):
        super(ActionsSqueezerMarketWebReqid, self).__init__()

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

    def squeeze_market_event(self, event):
        ts = event.timestamp // 1000
        access_type = None
        request = None
        status = None
        method = None
        user_interface = None
        wprid = None
        wprid_ys = None
        if event.type == scarab_common.EventType.MARKET_FRONT_ACCESS_EVENT:
            access_type = event.request_access_type
            request = event.request_raw
            status = event.status.value
            method = event.method
            user_interface = str(event.user_interface)
            cookes_dict = self.cookies2dict(event.cookies)
            wprid = cookes_dict.get("market_ys", None)
            wprid_ys = cookes_dict.get("ys", None)

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
        return {
            "ts": ts,
            "action_index": 0,
            "access_type": access_type,
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
            "wprid": wprid,
            "wprid_ys": wprid_ys,
            "rgb": getattr(event, "rgb", None),
            "pof": getattr(event, "pof", None),
            "pp": getattr(event, "pp", None),
            "market_surplus_events": self.get_surplus_events(event)
        }

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

                if self.allowed_market_event(event):
                    squeezed = self.squeeze_market_event(event)
                    squeezed["yuid"] = yuid
                    exp_bucket = self.check_experiments_fake(args)
                    squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket
                    actions.append(squeezed)
                    actions_count += 1
            except:
                logging.error("Suspicious record {}".format(row["value"]))
                raise

        args.result_actions += actions

    def allowed_market_event(self, event):
        """
        :type event:
        :rtype: bool
        """
        if event.type not in self.ALLOWED_EVENT_TYPES:
            return False

        if event.type == scarab_common.EventType.MARKET_FRONT_ACCESS_EVENT:
            if event.is_technical:
                return False

        sep_ts = 1509483600  # 2017-11-01 00:00:00 GMT+3
        if event.timestamp // 1000 <= sep_ts:
            return False

        return True

    def get_surplus_events(self, event):
        # click - useful market click
        # cpc_click - clickout to shop
        # cpc_click_card - clickout to shop from market card page
        # cpa_click - add to cart
        # *_pof - only clicks with 3-digit pof, helps to determine wizard clicks
        surplus_events = []
        if event.type == scarab_common.EventType.MARKET_CLICK_EVENT:
            surplus_events += ["click", "cpc_click"]
            if getattr(event, "pp", None) in self.MARKET_CARD_PP:
                surplus_events += ["cpc_click_card"]
        elif event.type == scarab_common.EventType.MARKET_CPA_CLICK_EVENT:
            surplus_events += ["click", "cpa_click"]

        if surplus_events:
            pof = getattr(event, "pof", None)
            if pof is not None and 100 <= pof < 1000:
                surplus_events += [(e + '_pof') for e in surplus_events]
        return surplus_events

    def check_request(self, request):
        return request["value"][0] == "{"
