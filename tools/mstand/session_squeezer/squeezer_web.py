# coding=utf-8
import session_squeezer.squeezer_common as sq_common

WEB_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "browser", "type": "string"},
    {"name": "bucket", "type": "int64"},
    {"name": "clid", "type": "string"},
    {"name": "correctedquery", "type": "string"},
    {"name": "dir_show", "type": "int64"},
    {"name": "domregion", "type": "string"},
    {"name": "dwelltime_on_service", "type": "int64"},
    {"name": "fraud_bits", "type": "int64"},
    {"name": "hasmisspell", "type": "boolean"},
    {"name": "is_match", "type": "boolean"},
    {"name": "maxrelevpredict", "type": "double"},
    {"name": "minrelevpredict", "type": "double"},
    {"name": "page", "type": "int64"},
    {"name": "path", "type": "string"},
    {"name": "pos", "type": "int64"},
    {"name": "query", "type": "string"},
    {"name": "queryregion", "type": "int64"},
    {"name": "referer", "type": "string"},
    {"name": "reqid", "type": "string"},
    {"name": "restype", "type": "string"},
    {"name": "servicetype", "type": "string"},
    {"name": "ui", "type": "string"},
    {"name": "sources", "type": "any"},
    {"name": "suggest", "type": "any"},
    {"name": "testid", "type": "string"},
    {"name": "type", "type": "string"},
    {"name": "url", "type": "string"},
    {"name": "userpersonalization", "type": "boolean"},
    {"name": "userregion", "type": "int64"},
    {"name": "vars", "type": "string"},
    {"name": "visual_pos", "type": "int64"},
    {"name": "visual_pos_ad", "type": "int64"},  # MSTAND-1355
    {"name": "wiz_show", "type": "int64"},
    {"name": "wizard", "type": "boolean"},
    {"name": "suggestion", "type": "string"},
    {"name": "target", "type": "string"},
    {"name": "request_params", "type": "any"},
    {"name": "msid", "type": "string"},
    {"name": "actions_will_be_copied_to", "type": "string"},  # MSTAND-1113
    {"name": "actions_was_copied_from", "type": "string"},  # MSTAND-1113
    {"name": "fork_uid_produce_actions", "type": "boolean"},  # MSTAND-1113
    {"name": "fork_banner_id", "type": "any"},  # MSTAND-1113
    {"name": "filmlistprediction", "type": "double"},
    {"name": "is_permission_requested", "type": "boolean"},  # MSTAND-1446
    {"name": "baobab_attrs", "type": "any"},  # MSTAND-1410
    {"name": "from_block", "type": "string"},  # MSTAND-1588
    {"name": "mute", "type": "boolean"},  # MSTAND-1653
    {"name": "channel_id", "type": "string"},  # MSTAND-1653
    {"name": "full_screen", "type": "boolean"},  # MSTAND-1653
    {"name": "cluster", "type": "string"},  # MSTAND-1708
    {"name": "cluster_3k", "type": "string"},  # MSTAND-1963
    {"name": "duration", "type": "int32"},  # MSTAND-1750
    {"name": "hp_detector_predict", "type": "double"},  # MSTAND-1786
    {"name": "original_url", "type": "string"},  # MSTAND-1824
    {"name": "pay_detector_predict", "type": "double"},  # MSTAND-1880
    {"name": "rearr_values", "type": "any"},  # MSTAND-1880
    {"name": "user_history", "type": "any"},  # MSTAND-1867
    {"name": "is_visible", "type": "boolean"},  # MSTAND-1983
    {"name": "query_about_one_product", "type": "double"},  # MSTAND-2057
    {"name": "query_about_many_products", "type": "double"},  # MSTAND-2057
    {"name": "block_id", "type": "string"},  # ONLINE-292
    {"name": "click_id", "type": "string"},  # MSTAND-2169
    {"name": "baobab_path", "type": "string"},  # ONLINE-292
    {"name": "placement", "type": "string"},  # ONLINE-292
    {"name": "wizard_name", "type": "string"},  # ONLINE-292
    {"name": "ecom_classifier_prob", "type": "float"},  # MSTAND-2101
    {"name": "ecom_classifier_result", "type": "boolean"},  # MSTAND-2101
    {"name": "search_props", "type": "any"},  # SURPLUSTOOLS-22
    {"name": "is_reload", "type": "boolean"},  # MSTAND-2210
    {"name": "is_reload_back_forward", "type": "boolean"},  # MSTAND-2210
]

WEB_DESKTOP_SAMPLE = u"""{"action_index":1,"actions_was_copied_from":null,"actions_will_be_copied_to":null,
"browser":"Opera","bucket":null,"clid":"9582","correctedquery":"schneider electric вд63 2п 25a 30ma","dir_show":1,
"domregion":"ru","dwelltime_on_service":null,"fork_banner_id":null,"fork_uid_produce_actions":null,"fraud_bits":null,
"hasmisspell":false,"is_match":true,"maxrelevpredict":0.0928337,"minrelevpredict":0.0277846,"page":0,"path":null,
"pos":null,"query":"schneider electric вд63 2п 25a 30ma","queryregion":43,"referer":"other",
"reqid":"1464764931106542-1117995425065018649580110-man1-3576","request_params":["text","clid","lr","redircnt"],
"msid":null,"restype":null,"servicetype":"web","sources":["WEB","WEB","WEB","WEB","WEB","WEB","WEB","WEB","WEB","WEB"],
"suggest":null,"suggestion":null,"target":null,"testid":"0","ts":1464764931,"type":"request","url":null,
"userpersonalization":true,"userregion":43,"vars":null,"visual_pos":null,"wiz_show":0,"wizard":false,
"yuid":"y1000000021462957634"}"""

WEB_TOUCH_SAMPLE = u"""{"action_index":0,"actions_was_copied_from":null,"actions_will_be_copied_to":null,
"browser":"ChromeMobile","bucket":null,"clid":"","correctedquery":"7961 какой оператор и регион","dir_show":0,
"domregion":"ru","dwelltime_on_service":null,"fork_banner_id":null,"fork_uid_produce_actions":null,"fraud_bits":null,
"hasmisspell":false,"is_match":true,"maxrelevpredict":0.145563,"minrelevpredict":0.0474899,"page":0,"path":null,
"pos":null,"query":"7961 какой оператор и регион","queryregion":213,"referer":"services",
"reqid":"1464799571134933-606482-ws27-406-TCH2","request_params":["cookies","rpt","app_req_id","text","snip",
"myreqid","numdoc","wizextra","service","l10n","app_platform","bregionid","lr","exp_flags","banner_ua","search_props",
"bpage","yandexuid","tld","device","mob_exp_ids","factors","p","ui","btype","rearr"],"restype":null,
"servicetype":"touch","sources":["WEB","WEB","WEB","WEB","WEB","WEB","WEB","WEB","WEB","WEB"],
"suggest":{"Status":"not_used","TimeSinceFirstChange":9336,"TimeSinceLastChange":730,"TotalInputTime":null,
"TpahLog":null,"UserInput":"7961 какой оператор и регион","UserKeyPressesCount":28},"suggestion":null,"target":null,
"testid":"0","ts":1464799571,"type":"request","url":null,"userpersonalization":false,"userregion":213,"vars":null,
"visual_pos":null,"wiz_show":0,"wizard":false,"yuid":"y1000001831427632125","msid":null}"""


class ActionsSqueezerWebDesktop(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: add has_ento flag to request
    3: TYandexNavSuggestClick
    4: suggest.TotalInputTimeInMilliseconds
    5: add techevent tech/timing
    6: remove has_ento flag, add cgi params(keys only)
    7: add wiz_parallel
    8: add personalization_new_order MSTAND-1360 + add visual_pos_ad MSTAND-1355
    9: add msid
    10: add "player-events.heartbeat" tech event for MSTAND-1414
    11: add is_permission_requested column for MSTAND-1446
    12: add baobab_attrs for MSTAND-1410
    13: add from_block for MSTAND-1588
    14: add mute, channel_id, full_screen for MSTAND-1653
    15: add cluster column for MSTAND-1708
    16: add duration column for MSTAND-1750
    17: remove personalization_new_order column for MSTAND-1765
    18: add hp_detector_predict column for MSTAND-1786
    19: fix checking of experiments MSTAND-1822
    20: add original_url column for MSTAND-1824
    21: add pay_detector_predict and a few values from RearrValues: MSTAND-1880
    22: add overlay trees events: MSTAND-1881
    23: add user_history from search props for MSTAND-1867
    24: add recommendation events: MSTAND-1891
    25: add cluster_3k column for MSTAND-1963
    26: add is_visible column for MSTAND-1983
    27: improve determination of result block id for events in overlay trees MSTAND-2051
    28: add query_about_one_product, query_about_many_products MSTAND-2057
    29: add ecom_prob MSTAND-2092
    30: fix getting of adv type (MSTAND-2103)
    31: add baobab_path, block_id, placement, wizard_name ONLINE-292
    32: rename ecom_prob -> ecom_classifier_result and add ecom_classifier_prob MSTAND-2101
    33: use binary squeezer by default ONLINE-256
    34: fix direct restype ONLINE-309
    35: add some rearr values into rearr_values dictionary: MSTAND-2151, MSTAND-2135
    36: fix getting values from the relev dictionary: MSTAND-2155
    37: add relev_values column: MSTAND-2118
    38: add user_ip column: MSTAND-2157
    39: add search_props column: SURPLUSTOOLS-22
    40: add click_id column: MSTAND-2169
    41: add is_reload and is_reload_back_forward: MSTAND-2210
    """
    VERSION = 41

    YT_SCHEMA = WEB_YT_SCHEMA
    SAMPLE = WEB_DESKTOP_SAMPLE

    def __init__(self):
        super(ActionsSqueezerWebDesktop, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        for request in args.container.GetRequests():
            if self.check_request(request):
                if sq_common.is_bro_testids(args.experiments):
                    exp_bucket = self.check_experiments_fake(args)
                else:
                    exp_bucket = self.check_experiments(args, request)
                collect_web_actions(args.result_actions, exp_bucket, request)

        if args.result_experiments:
            event_exp_bucket = sq_common.ExpBucketInfo()
            for event in args.container.GetOwnEvents():
                collect_nav_suggest(args.result_actions, event_exp_bucket, event)

    def check_request(self, request):
        return request.IsA("TYandexWebRequest")


class ActionsSqueezerWebTouch(ActionsSqueezerWebDesktop):
    """
    1-7: same as ActionsSqueezerWebDesktop
    8: merge touch + pad + mobileapp
    9: add personalization_new_order MSTAND-1360 + add visual_pos_ad MSTAND-1355
    10+: same as ActionsSqueezerWebDesktop
    """
    VERSION = ActionsSqueezerWebDesktop.VERSION

    SAMPLE = WEB_TOUCH_SAMPLE

    def __init__(self):
        super(ActionsSqueezerWebTouch, self).__init__()

    def check_request(self, request):
        return (request.IsA("TTouchYandexWebRequest")
                or request.IsA("TPadYandexWebRequest")
                or request.IsA("TMobileAppYandexWebRequest"))


def collect_web_actions(known_actions, exp_bucket, request):
    base_data = sq_common.prepare_base_data(exp_bucket, request, add_ui=True)
    request_data = sq_common.build_request_data(
        base_data, request,
        add_request_params=True,
        add_web_params=True,
    )
    known_actions.append(request_data)
    sq_common.append_clicks(known_actions, base_data, request, ad_replace=True, add_baobab_attrs=True,
                            add_original_url=True, is_web=True)
    sq_common.append_misc_clicks(known_actions, base_data, request, add_original_url=True)
    sq_common.append_techevents(known_actions, base_data, request, add_baobab_attrs=True)
    append_tv_online_player_events(known_actions, base_data, request)


def collect_nav_suggest(known_actions, exp_bucket, event):
    if event.IsA("TYandexNavSuggestClick"):
        data = {
            "ts": event.Timestamp,
            "type": "nav-suggest-click",
            "url": event.Url,
            "suggestion": event.Suggestion,
            sq_common.EXP_BUCKET_FIELD: exp_bucket,
        }
        known_actions.append(data)


def append_tv_online_player_events(known_actions, base_data, request):
    events = request.GetOwnEvents()
    if not events:
        return
    for event in events:
        action_data = build_tv_online_player_data(base_data, event)
        if action_data:
            known_actions.append(action_data)


def build_tv_online_player_data(base_data, event):
    if event.IsA("TTVOnlinePlayerEvent"):
        return dict(
            base_data,
            ts=event.Timestamp,
            type="player-event",
            from_block=event.FromBlock,
            path=event.Path,
            mute=event.Mute,
            channel_id=event.ChannelID,
            full_screen=event.FullScreen,
            duration=getattr(event, 'Duration', None),
        )
