import yaqutils.url_helpers as uurl

import session_squeezer.squeezer_common as sq_common

MORDA_YT_SCHEMA = [
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
    {"name": "enabled_test_ids", "type": "any"},
    {"name": "first_screen_height", "type": "int64"},
    {"name": "fraud_bits", "type": "int64"},  # TODO: remove this field generation for morda?
    {"name": "hasmisspell", "type": "boolean"},
    {"name": "is_match", "type": "boolean"},
    {"name": "m_content", "type": "string"},
    {"name": "maxrelevpredict", "type": "double"},
    {"name": "minrelevpredict", "type": "double"},
    {"name": "path", "type": "string"},
    {"name": "portal_exp", "type": "any"},
    {"name": "pos", "type": "int64"},  # TODO: remove this field generation for morda?
    {"name": "query", "type": "string"},
    {"name": "referer", "type": "string"},
    {"name": "reqid", "type": "string"},
    {"name": "restype", "type": "string"},
    {"name": "servicetype", "type": "string"},
    {"name": "sources", "type": "any"},
    {"name": "szm", "type": "string"},
    {"name": "testid", "type": "string"},
    {"name": "type", "type": "string"},
    {"name": "url", "type": "string"},
    {"name": "userpersonalization", "type": "boolean"},
    {"name": "userregion", "type": "int64"},
    {"name": "vars", "type": "string"},
    {"name": "visual_pos", "type": "int64"},  # TODO: remove this field generation for morda?
    {"name": "wiz_show", "type": "int64"},
    {"name": "wizard", "type": "boolean"},
    {"name": "blocks", "type": "any"},
    {"name": "is_dynamic_click", "type": "boolean"},
    {"name": "is_misc_click", "type": "boolean"},
    {"name": "ui", "type": "string"},
    {"name": "puid", "type": "string"},
    {"name": "from_block", "type": "string"},  # MSTAND-1588
]

MORDA_SAMPLE = u"""{"action_index":0,"blocks":null,"browser":"Firefox","bucket":null,"clid":"","correctedquery":"",
"dir_show":0,"domregion":"ru","dwelltime_on_service":null,"enabled_test_ids":[],"fraud_bits":null,
"hasmisspell":false,"is_match":true,"m_content":"big","maxrelevpredict":null,"minrelevpredict":null,"path":null,
"portal_exp":["www_yes","hascar_0","banner_horizontal","fuid_yes","clid_preset","L_yes","gpauto_no"],"pos":null,
"query":"","referer":"other","reqid":"1464809006.95163.22902.22506","restype":null,"servicetype":"morda","sources":[
],"szm":"1:1920x1080:1920x901","testid":"0","ts":1464809006,"type":"request","url":null,"userpersonalization":false,
"userregion":213,"vars":null,"visual_pos":null,"wiz_show":0,"wizard":false,"yuid":"y100000001429258217"}"""


class ActionsSqueezerMorda(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: add has_ento flag to request
    3: rename exp->portal_exp
       add enabled_test_ids
       use GetEnabledTestInfo
    4: add blocks
    5: add techevent tech/timing
    6: add szm from cookie
       fix clicks parser
       add dynamic_clicks_count to block description
    7: add blocks for desktop
       add dwelltimes for clicks and dynamic clicks in blocks
       and first click timestamp to blocks
    8: MSTAND-1462 fix bugs
    9: add puid field MSTAND-1545
    10: add durations field MSTAND-1590
    11: add block's visibility
    12: add zen rows visibility
    13: add first screen height on desktop
    14: change testids source depending on the presence of triggered_testids_filter (MSTAND-1832)
    """
    VERSION = 14

    YT_SCHEMA = MORDA_YT_SCHEMA
    SAMPLE = MORDA_SAMPLE

    def __init__(self):
        super(ActionsSqueezerMorda, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        for request in args.container.GetRequests():
            if request.IsA("TPortalRequestProperties"):
                exp_bucket = ActionsSqueezerMorda.check_experiments(args, request)
                collect_morda_actions(args.result_actions, exp_bucket, request)

    @staticmethod
    def check_experiments(args, request):
        """
        :type args: ActionSqueezerArguments
        :type request: libra TRequest
        :rtype ExpBucketInfo
        """
        result = sq_common.ExpBucketInfo()
        enabled_testids = set(get_enabled_testids(request))

        def _is_enabled_testid_match(exp):
            return exp.all_users or exp.all_for_history or exp.testid in enabled_testids

        for exp in args.experiments:
            result.buckets[exp] = sq_common.ActionsSqueezer.get_bucket(request, exp)
            if exp.has_triggered_testids_filter:
                is_testid_matched = _is_enabled_testid_match(exp)
            else:
                is_testid_matched = sq_common.ActionsSqueezer.is_testid_match(request, exp)

            if is_testid_matched:
                args.result_experiments.add(exp)
                if sq_common.ActionsSqueezer.is_filters_match(request, args.libra_filters.get(exp)):
                    result.matched.add(exp)
        return result


class ActionsSqueezerZenMorda(ActionsSqueezerMorda):
    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        for request in args.container.GetRequests():
            if request.IsA("TPortalRequestProperties"):
                exp_bucket = self.check_experiments_fake(args)
                collect_morda_actions(args.result_actions, exp_bucket, request)


def collect_morda_actions(known_actions, exp_bucket, request):
    base_data = sq_common.prepare_base_data(exp_bucket, request, add_ui=True)
    request_data = sq_common.build_request_data(base_data, request)
    request_data["m_content"] = request.MContent
    blocks = list(get_all_blocks(request))
    clicks = request.GetClicks()
    join_more_click_to_block(clicks, blocks)
    request_data["blocks"] = [block.build_block_data() for block in blocks]
    request_data["portal_exp"] = request.PortalTestIDs
    request_data["enabled_test_ids"] = list(get_enabled_testids(request))
    request_data["szm"] = parse_yp(parse_cookie(request.Cookies).get("yp")).get("szm")
    request_data["puid"] = request.PassportUID
    request_data["first_screen_height"] = request.FirstScreenHeight
    known_actions.append(request_data)
    append_portal_clicks(known_actions, base_data, request)
    sq_common.append_techevents(known_actions, base_data, request)


def join_more_click_to_block(clicks, blocks):
    clicks_timestamp = {click.Timestamp for block in blocks for click in block.clicks + block.dynamic_clicks}
    for click in clicks:
        if click.Timestamp not in clicks_timestamp:
            for block in blocks:
                if hasattr(block.block, "Path") and hasattr(click, "Path") and block.block.Path + "." in click.Path:
                    if click.IsDynamic:
                        block.dynamic_clicks.append(click)
                    else:
                        block.clicks.append(click)


def append_portal_clicks(known_actions, base_data, request):
    for click in request.GetClicks():
        parent = click.GetParent()
        if parent:
            position = parent.Position
        else:
            position = None
        click_data = sq_common.build_click_data(base_data, click, "morda", pos=position, add_types=True)
        known_actions.append(click_data)


def get_enabled_testids(request):
    return (test_info.TestID for test_info in request.GetEnabledTestInfo())


def parse_cookie(cookie):
    parsed_cookie = dict()
    if cookie:
        cookie = uurl.unquote(cookie)
        for field in cookie.split("; "):
            parts = field.split("=", 1)
            if len(parts) == 2:
                parsed_cookie[parts[0]] = parts[1]
    return parsed_cookie


def parse_yp(yp):
    parsed_yp = dict()
    if yp:
        for field in yp.split("#"):
            parts = field.split(".", 2)
            if len(parts) == 3:
                parsed_yp[parts[1]] = parts[2]
    return parsed_yp


MAX_DWELLTIME = 2 ** 63 - 1


class MordaBlockDescription(object):
    def __init__(
            self,
            block,
            libra_pos,
            restype,
            pos,
            parent=None,
            path=None,
    ):
        """
        :type block: libra.TBlock
        :type libra_pos: int
        :type restype: str
        :type pos: int
        :type path: str | None
        :type parent: str | None
        """
        self.block = block
        self.libra_pos = libra_pos
        self.restype = restype
        self.pos = pos
        self.path = path
        self.parent = parent
        self.clicks = self.block.GetClicks()
        self.dynamic_clicks = list(self.get_dynamic_clicks())
        self.durations = list(self.get_durations())
        self.visible = self.visibility()

    def build_block_data(self):
        data = {
            "pos": self.pos,
            "restype": self.restype,
            "clicks_count": len(self.clicks),
            "height": self.block.Height,
            "clicks_total_dwelltime": min(sum(click.DwellTimeOnService for click in self.clicks), MAX_DWELLTIME),
            "visible": self.visible
        }
        if self.path is not None:
            data["path"] = self.path
        if self.parent is not None:
            data["parent"] = self.parent
        if self.clicks or self.dynamic_clicks:
            data["first_click_timestamp"] = min(click.Timestamp for click in self.clicks + self.dynamic_clicks)
        if self.durations:
            data["durations"] = self.durations
        return data

    def get_dynamic_clicks(self):
        for event in self.block.GetOwnEvents():
            if event.IsA("TClick"):
                if event.IsDynamic:
                    yield event

    def get_durations(self):
        main_result = self.block.GetMainResult()
        if main_result:
            for event in main_result.GetOwnEvents():
                if "longwatch" in event.Path:
                    yield event.Duration

    def visibility(self):
        result = self.block.GetMainResult() or self.block
        visible = False
        for event in result.GetOwnEvents():
            if event.IsA("TPortalShow") and (event.Path.endswith(".realshow") or event.Path.endswith(".card")):
                visible = True
        return visible


def get_all_blocks(request):
    """
    :type request: libra.TRequest
    :rtype: __generator[MordaBlockDescription]
    """
    for libra_pos, block in enumerate(request.GetMainBlocks()):
        main_result = block.GetMainResult()
        if main_result:
            if main_result.IsA("TPortalResult"):
                yield MordaBlockDescription(
                    block=block,
                    libra_pos=libra_pos,
                    restype="morda",
                    pos=main_result.Position,
                    path=main_result.Path,
                    parent=main_result.ParentPath
                )
        else:
            for result in block.GetChildren():
                if result.IsA("TPortalZenResult"):
                    yield MordaBlockDescription(
                        block=result,
                        libra_pos=libra_pos,
                        restype="morda",
                        pos=result.Position,
                        path=result.Path,
                        parent=result.ParentPath
                    )
