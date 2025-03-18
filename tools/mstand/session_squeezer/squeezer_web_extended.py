import re

import yaqutils.six_helpers as usix
import mstand_utils.baobab_helper as ubaobab
import session_squeezer.squeezer_common as sq_common

WEB_EXTENDED_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "blocks", "type": "any"},
    {"name": "bucket", "type": "int64"},
    {"name": "domregion", "type": "string"},
    {"name": "is_match", "type": "boolean"},
    {"name": "mousetrack", "type": "any"},
    {"name": "page", "type": "int64"},
    {"name": "reqid", "type": "string"},
    {"name": "servicetype", "type": "string"},
    {"name": "ui", "type": "string"},
    {"name": "testid", "type": "string"},
    {"name": "type", "type": "string"},
    {"name": "actions_will_be_copied_to", "type": "string"},  # MSTAND-1113
    {"name": "actions_was_copied_from", "type": "string"},  # MSTAND-1113
    {"name": "fork_uid_produce_actions", "type": "boolean"},  # MSTAND-1113
    {"name": "fork_banner_id", "type": "any"},  # MSTAND-1113
    {"name": "session_events", "type": "any"},  # MMA-4506
]

ENTITYSEARCH_TVT_EVENT_PREFIX = "690.2873."
ENTITYSEARCH_TVT_MUSIC_EVENT_PREFIX = "690.56.1950.471."
RE_PITEM = re.compile(r"p(?P<time>\d+)$")
MAX_ENTITYSEARCH_TVT_EVENT_SECONDS = 15

ETHER_BLOCKS = {
    "ether_wizard",
}
ETHER_EVENTS = {
    "player-events.heartbeat",
}

WEB_DESKTOP_EXTENDED_SAMPLE = u"""{"action_index":1,"actions_was_copied_from":null,"actions_will_be_copied_to":null,
"blocks":[{"banner_relevance":0.0,"clicks_count":0,"fraud_bits":0,"height":null,"placement":"main","pos":0,
"restype":"dir","visual_pos":0},{"clicks_count":0,"height":null,"placement":"main","pos":0,
"relevpredict":0.0877646815,"restype":"web","snip_len_str":2.22276,"visual_pos":1},{"clicks_count":0,"height":null,
"path":"/snippet/market/offers/title","placement":"main","pos":1,"restype":"wiz","visual_pos":2},{"clicks_count":0,
"height":null,"placement":"main","pos":2,"relevpredict":0.0548544502,"restype":"web","snip_len_str":2.18343,
"visual_pos":3},{"clicks_count":1,"height":null,"placement":"main","pos":3,"relevpredict":0.0596170659,
"restype":"web","snip_len_str":1.96292,"visual_pos":4},{"clicks_count":0,"height":null,"placement":"main","pos":4,
"relevpredict":0.0765161367,"restype":"web","snip_len_str":2.34512,"visual_pos":5},{"clicks_count":0,"height":null,
"placement":"main","pos":5,"relevpredict":0.0628870841,"restype":"web","snip_len_str":2.07336,"visual_pos":6},
{"clicks_count":1,"height":null,"placement":"main","pos":6,"relevpredict":0.0684742906,"restype":"web",
"snip_len_str":1.97221,"visual_pos":7},{"clicks_count":1,"height":null,"placement":"main","pos":7,
"relevpredict":0.0720922395,"restype":"web","snip_len_str":2.81288,"visual_pos":8},{"clicks_count":0,"height":null,
"placement":"main","pos":8,"relevpredict":0.0426423284,"restype":"web","snip_len_str":2.9574,"visual_pos":9},
{"clicks_count":0,"height":null,"placement":"main","pos":9,"relevpredict":0.0928336521,"restype":"web",
"snip_len_str":2.8172,"visual_pos":10},{"clicks_count":0,"height":null,"placement":"main","pos":10,
"relevpredict":0.0595484011,"restype":"web","snip_len_str":2.42485,"visual_pos":11}],
"bucket":null,"domregion":"ru","fork_banner_id":null,"fork_uid_produce_actions":null,"is_match":true,
"mousetrack":null,"page":0,"reqid":"1464764931106542-1117995425065018649580110-man1-3576",
"servicetype":"web-desktop-extended","testid":"0","ts":1464764931,"type":"request-extended",
"yuid":"y1000000021462957634"}"""

WEB_TOUCH_EXTENDED_SAMPLE = u"""{"action_index":0,"actions_was_copied_from":null,"actions_will_be_copied_to":null,
"blocks":[{"clicks_count":1,"height":null,"placement":"main","pos":0,"relevpredict":0.1342300134,"restype":"web",
"snip_len_str":5.35319,"visual_pos":0},{"clicks_count":1,"height":null,"placement":"main","pos":1,
"relevpredict":0.1455625332,"restype":"web","snip_len_str":2.76069,"visual_pos":1},{"clicks_count":0,"height":null,
"placement":"main","pos":2,"relevpredict":0.116737066,"restype":"web","snip_len_str":5.43288,"visual_pos":2},
{"clicks_count":0,"height":null,"placement":"main","pos":3,"relevpredict":0.1190805687,"restype":"web",
"snip_len_str":4.95065,"visual_pos":3},{"clicks_count":0,"height":null,"placement":"main","pos":4,
"relevpredict":0.138459858,"restype":"web","snip_len_str":6.46709,"visual_pos":4},{"clicks_count":0,"height":null,
"placement":"main","pos":5,"relevpredict":0.1187407528,"restype":"web","snip_len_str":6.57139,"visual_pos":5},
{"clicks_count":0,"height":null,"placement":"main","pos":6,"relevpredict":0.1193161633,"restype":"web",
"snip_len_str":6.59369,"visual_pos":6},{"clicks_count":0,"height":null,"placement":"main","pos":7,
"relevpredict":0.1067633121,"restype":"web","snip_len_str":4.44117,"visual_pos":7},{"clicks_count":0,"height":null,
"placement":"main","pos":8,"relevpredict":0.1122138185,"restype":"web","snip_len_str":5.38033,"visual_pos":8},
{"clicks_count":0,"height":null,"placement":"main","pos":9,"relevpredict":0.1203170255,"restype":"web",
"snip_len_str":4.70028,"visual_pos":9}],
"bucket":null,"domregion":"ru","fork_banner_id":null,"fork_uid_produce_actions":null,"is_match":true,
"mousetrack":null,"page":0,"reqid":"1464799571134933-606482-ws27-406-TCH2","servicetype":"web-touch-extended",
"testid":"0","ts":1464799571,"type":"request-extended","yuid":"y1000001831427632125"} """


class ActionsSqueezerWebDesktopExtended(sq_common.ActionsSqueezer):
    """
    1: initial version with blocks+mousetrack
    2: fix bug with active_dwell_time in mousetrack
    3: fix another bug with active_dwell_time in mousetrack
    4: new mousetrack fields
    5: add BSBlocks (for lipka@)
    6: remove BSBlocks
    7: add wiz_parallel
    8: add entitysearch tvt in blocks
    9: add BSBlocks (for snadira@)
    10: add raw mousertracking + converted_path + url + source + name (MSTAND-1311)
    11: update libra (height in parallel blocks)
    12: add is_fact
    13: add is_bno
    14: add base_type
    15: add is_big_bno
    16: update libra to 4312871 revision
    17: blndr_view_type + fresh_age + robot_dater_fresh_age + shard + adapters
    18. add banner_id and log_id into block description (MSTAND-1480)
    19: add baobab_attrs into blocks for MSTAND-1410
    20: add more markers, move all them to a separate dictionary
    21: fix baobab_attrs MSTAND-1410
    22: add intent into blocks field (MSTAND-1707)
    23: add Ether events for MMA-4506
    24: add plugin markers MSTAND-1836
    25: add medical_host_quality marker for MSTAND-1870
    26: add overlay trees events MSTAND-1881
    27: fix using of mousetrack (MSTAND-1902)
    28: define adv type using tamus rules (MSTAND-1915)
    29: add proximapredict to main blocks (MSTAND-1987)
    30: add real_pos to baobab_attrs (MSTAND-1974)
    31: fix advert url (MSTAND-1973)
    32: add cost_plus to main blocks (MSTAND-2013)
    33: move cost_plus into markers (MSTAND-2014)
    34: fix getting of adv type (MSTAND-2103)
    35: discard casting of log_id to int (MSTAND-2106)
    36: use binary squeezer by default ONLINE-256
    37: fix direct restype ONLINE-309
    38: fix getting and saving of cost_plus in binary mode MSTAND-2160
    39: fix is_fact definition in binary mode (GODASUP-830)
    """
    VERSION = 39

    YT_SCHEMA = WEB_EXTENDED_YT_SCHEMA
    SAMPLE = WEB_DESKTOP_EXTENDED_SAMPLE

    def __init__(self):
        super(ActionsSqueezerWebDesktopExtended, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """

        session_events = collect_session_events(args.container.GetSessionYandexTechEvents())

        for request in args.container.GetRequests():
            if self.check_request(request):
                if sq_common.is_bro_testids(args.experiments):
                    exp_bucket = self.check_experiments_fake(args)
                else:
                    exp_bucket = self.check_experiments(args, request)
                collect_web_extended_actions(args.result_actions, exp_bucket, request, args.blockstat, session_events)

    def check_request(self, request):
        return request.IsA("TYandexWebRequest")


class ActionsSqueezerWebTouchExtended(ActionsSqueezerWebDesktopExtended):
    """
    1-9: same as ActionsSqueezerWebDesktopExtended
    10: merge touch + pad + mobileapp
    11: add raw mousertracking + converted_path + url + source + name (MSTAND-1311)
    12: update libra (height in parallel blocks)
    13: add is_fact
    14: add is_bno
    15: add base_type
    16+: same as ActionsSqueezerWebDesktopExtended
    """

    SAMPLE = WEB_TOUCH_EXTENDED_SAMPLE

    def __init__(self):
        super(ActionsSqueezerWebTouchExtended, self).__init__()

    def check_request(self, request):
        return (request.IsA("TTouchYandexWebRequest")
                or request.IsA("TPadYandexWebRequest")
                or request.IsA("TMobileAppYandexWebRequest"))


def collect_web_extended_actions(known_actions, exp_bucket, request, blockstat=None, session_events=None):
    base_data = sq_common.prepare_base_data(exp_bucket, request, add_ui=True)
    request_data = build_request_data_extended(base_data, request, blockstat=blockstat, session_events=session_events)
    if request_data:
        known_actions.append(request_data)


def build_request_data_extended(base_data, request, blockstat=None, session_events=None):
    mousetrack = decode_mousetrack(request)
    blocks = list(get_blocks_with_mousetrack(request, mousetrack=mousetrack, blockstat=blockstat))
    result = dict(
        base_data,
        ts=request.Timestamp,
        type="request-extended",
        blocks=blocks,
        session_events=session_events,
    )
    if mousetrack:
        result["mousetrack"] = convert_mousetrack(mousetrack)
    return result


def decode_mousetrack(request):
    """
    :type request: libra.TRequest
    :rtype: mousetrack_decoder.MouseTrackRequestData | None
    """
    import mousetrack_decoder
    mousetrack = mousetrack_decoder.MouseTrackRequestData()
    # noinspection PyBroadException
    try:
        mousetrack.extractFromRequest(request)
    except:
        # logging.error("skip bad mousetrack for %s", request.ReqId)
        return None
    if not mousetrack.hasMouseTrackData:
        return None
    return mousetrack


def parse_otype(otype_from_log):
    if "-" in otype_from_log:
        return None, None
    elif "/" in otype_from_log:
        return otype_from_log.split("/")[:2]
    else:
        return otype_from_log, None


MARKERS_FIELD_MAP = {
    "blndrViewType": "blndr_view_type",
    "FreshAge": "fresh_age",
    "news_wizard_video_thumb": "news_wizard_video_thumb",
    "RobotDaterFreshAge": "robot_dater_fresh_age",
    "xl_video_player": "xl_video_player",
    "MedicalHostQuality": "medical_host_quality",
    "CostPlus": "cost_plus",
}

MARKERS_PLUGIN_FIELD_MAP = {
    "DisabledP": "disabled_p",
    "FilteredP": "filtered_p",
    "Plugins": "plugins",
}


def get_blocks_with_mousetrack(request, mousetrack=None, blockstat=None):
    """
    :type request: libra.TRequest
    :type mousetrack: mousetrack_decoder.MouseTrackRequestData | None
    :type blockstat: dict[str, str]
    :rtype: __generator[dict]
    """
    baobab_block_position = ubaobab.get_block_position(request)
    is_entity_search_tvt_saved = False
    for description in sq_common.get_all_blocks(request):
        block_data = description.build_block_data()
        if mousetrack:
            if description.parallel:
                mousetrack_block = mousetrack.parallelBlocks[description.libra_pos]
            else:
                mousetrack_block = mousetrack.mainBlocks[description.libra_pos]

            if isinstance(mousetrack_block.blockCounterId, str):
                block_counter_id = ubaobab.get_block_id(request, description.block)
            else:
                block_counter_id = description.block.BlockCounterId
            assert block_counter_id == mousetrack_block.blockCounterId, \
                (mousetrack.hasBaobab, block_counter_id, mousetrack_block.blockCounterId, request.ICookie)

            block_data["mousetrack"] = convert_block_mousetrack(mousetrack_block)
        if "entity_search" in block_data.get("path", "") and not is_entity_search_tvt_saved:
            block_data["tvt"], block_data["tvt_wmusic"] = get_request_entitysearch_tvt(request, blockstat)
            is_entity_search_tvt_saved = True
        if is_fact_result(description.block):
            block_data["is_fact"] = True
        if is_bno_result(description.block):
            block_data["is_bno"] = True
        if is_big_bno_result(description.block):
            block_data["is_big_bno"] = True

        try:
            main_result = description.block.GetMainResult()
        except AttributeError:
            main_result = None
        if main_result:
            try:
                markers = main_result.Markers
            except AttributeError:
                markers = None
            if markers:
                markers_data = {
                    v: markers[k]
                    for k, v in usix.iteritems(MARKERS_FIELD_MAP)
                    if k in markers
                }
                markers_plugins_data = {
                    v: markers[k].split('|')
                    for k, v in usix.iteritems(MARKERS_PLUGIN_FIELD_MAP)
                    if k in markers
                }
                markers_data.update(markers_plugins_data)
                if markers_data:
                    block_data["markers"] = markers_data
                block_data["intent"] = markers.get('Slices', '').split(':')[0]
            try:
                shard = main_result.Shard
            except AttributeError:
                shard = None
            if shard:
                block_data["shard"] = shard

        try:
            adapters = description.block.Adapters
        except AttributeError:
            adapters = None
        if adapters:
            block_data["adapters"] = adapters

        if "entity_search" in block_data.get("path", ""):
            search_props = request.SearchPropsValues
            log_data = search_props.get("UPPER.EntitySearch.Log")
            if log_data:
                parts = log_data.split("|")
                onto_id = parts[3]
                lst_onto_id = parts[4]
                o_type, o_sub_type = parse_otype(parts[2])
                lst_type = parts[2]
            else:
                onto_id = None
                lst_onto_id = None
                o_type = None
                o_sub_type = None
                lst_type = None
            if "upper" in block_data.get("path", ""):
                block_data["LstOntoID"] = lst_onto_id
                block_data["LstType"] = lst_type
            else:
                block_data["OType"] = o_type
                block_data["OntoID"] = onto_id
                block_data["OSubType"] = o_sub_type

        baobab_attrs = ubaobab.get_block_attrs(request, description.block)
        if baobab_attrs is not None:
            baobab_attrs["real_pos"] = baobab_block_position.get(baobab_attrs["block_id"])
            block_data["baobab_attrs"] = baobab_attrs

        yield block_data


def convert_mousetrack(mousetrack):
    """
    :type mousetrack: mousetrack_decoder.MouseTrackRequestData
    :rtype: dict
    """

    viewport_width = sq_common.positive_int64_or_none(mousetrack.viewportWidth, "mousetrack.viewportWidth")
    viewport_height = sq_common.positive_int64_or_none(mousetrack.viewportHeight, "mousetrack.viewportHeight")
    max_scroll_y_top = sq_common.positive_int64_or_none(mousetrack.maxScrollYTop, "max_scroll_y_top")
    max_scroll_y_bottom = sq_common.positive_int64_or_none(mousetrack.maxScrollYBottom, "mousetrack.maxScrollYBottom")

    return {
        "dwell_time": mousetrack.dwellTime,
        "active_dwell_time": mousetrack.activeDwellTime,
        "viewport_width": viewport_width,
        "viewport_height": viewport_height,
        "max_scroll_y_top": max_scroll_y_top,
        "max_scroll_y_bottom": max_scroll_y_bottom,
        "raw_packets": mousetrack.rawPackets,
    }


def convert_block_mousetrack(block):
    """
    :type block: mousetrack_decoder.MouseTrackRequestData.BlockData
    :rtype: dict
    """
    width = sq_common.positive_int64_or_none(block.width, "block.width")
    height = sq_common.positive_int64_or_none(block.height, "block.height")
    left = sq_common.positive_int64_or_none(block.left, "block.left")
    top = sq_common.positive_int64_or_none(block.top, "block.top")
    return {
        "width": width,
        "height": height,
        "left": left,
        "top": top,
        "is_initially_visible": block.isInitiallyVisible,
        "is_visible": block.isVisible,
        "mouse_over_duration": block.mouseOverDuration,
        "is_clicked": block.isClicked,
        "read_time_multiple": block.readTimeMultiple,
        "read_time_uniform": block.readTimeUniform,
        "read_time_top": block.readTimeTop,
    }


def get_request_entitysearch_tvt(request, blockstat):
    """
    OBJECTS-8574
    :type request: libra.TRequest
    :type blockstat: dict[str, str]
    :rtype: (float, float)
    """
    if blockstat is None:
        return 0.0, 0.0
    last_seconds, result = 0, 0.0
    music_add = 0
    for techevent in request.GetYandexTechEvents():
        if techevent.Path.startswith(ENTITYSEARCH_TVT_EVENT_PREFIX):
            last_item = techevent.Path.split(".")[-1]
            match = RE_PITEM.match(blockstat.get(last_item, ""))
            if match is not None:
                cur_seconds = int(match.group("time"))
                if cur_seconds < last_seconds:
                    result += min(last_seconds + 1, MAX_ENTITYSEARCH_TVT_EVENT_SECONDS) / 2.0
                result += cur_seconds
                last_seconds = cur_seconds
        elif techevent.Path.startswith(ENTITYSEARCH_TVT_MUSIC_EVENT_PREFIX):
            music_add += 10
    if last_seconds:
        result += min(last_seconds + 1, MAX_ENTITYSEARCH_TVT_EVENT_SECONDS) / 2.0
    return result, result + music_add


UNION_FACTS_WIZNAMES = {
    "calories_fact",
    "currencies",
    "distance_fact",
    "graph",
    "entity-fact",
    "entity_fact",
    "suggest_fact",
    "table_fact",
    "wikipedia_fact",
}


def is_fact_result(block):
    """
    clone of https://a.yandex-team.ru/arc/trunk/arcadia/quality/functionality/scripts/surplusplus/mt_squeeze/result_metrics.py?rev=3777009#L171
    """
    main_result = block.GetMainResult()
    if main_result.IsA("TOrganicResultProperties"):
        # try-catch is faster than if "Markers" in children.__dict__ and "Rule" in children.Markers:
        try:
            markers = main_result.Markers
            rule_marker = markers.get("Rule")
            if rule_marker == "Facts":
                return True
        except AttributeError:
            pass
    if main_result.IsA("TWizardResult") or main_result.IsA("TBlenderWizardResult"):
        if main_result.Name in UNION_FACTS_WIZNAMES:
            return True
    return False


def is_bno_result(block):
    """
    MSTAND-1366
    logic from https://a.yandex-team.ru/arc/trunk/arcadia/junk/shubert/SNIPPETS-4540/request.py?blame=true&rev=3849887#L114
    """
    main_result = block.GetMainResult()
    if not main_result:
        return False
    markers = getattr(main_result, "Markers", None)
    if not markers:
        return False
    if "Plugins" not in markers:
        return False
    plugins = markers["Plugins"].split("|")
    return "bno" in plugins


def is_big_bno_result(block):
    """
    MSTAND-1386
    """
    main_result = block.GetMainResult()
    if not main_result:
        return False
    markers = getattr(main_result, "Markers", None)
    if not markers:
        return False
    return markers.get("UniAnswerComb") == "media_turbo_bna"


def collect_session_events(session_tech_events):
    result = []
    for event in session_tech_events:
        player_data = build_ether_player_data(event)
        if player_data and player_data["path"] in ETHER_EVENTS and player_data["from_block"] in ETHER_BLOCKS:
            result.append(player_data)
    return result


def build_ether_player_data(event):
    if event.IsA("TTVOnlinePlayerEvent"):
        return dict(
            ts=event.Timestamp,
            type="session-ether-player-event",
            from_block=event.FromBlock,
            path=event.Path,
            mute=event.Mute,
        )
