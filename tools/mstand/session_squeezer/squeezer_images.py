# -*- coding: utf-8 -*-

import json
import re

import session_squeezer.squeezer_common as sq_common
import yaqutils.url_helpers as uurl
import yaqutils.six_helpers as usix

IMAGES_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "blocks", "type": "any"},
    {"name": "bucket", "type": "int64"},
    {"name": "cts", "type": "int64"},
    {"name": "domregion", "type": "string"},
    {"name": "dwelltime_on_service", "type": "int64"},
    {"name": "grid", "type": "any"},
    {"name": "is_match", "type": "boolean"},
    {"name": "page", "type": "int64"},
    {"name": "path", "type": "string"},
    {"name": "query", "type": "string"},
    {"name": "dnorm", "type": "string"},
    {"name": "synnorm", "type": "string"},
    {"name": "parent_reqid", "type": "string"},
    {"name": "reqid", "type": "string"},
    {"name": "results", "type": "int64"},
    {"name": "serpid", "type": "string"},
    {"name": "servicetype", "type": "string"},
    {"name": "source", "type": "string"},
    {"name": "source-serpid", "type": "string"},
    {"name": "testid", "type": "string"},
    {"name": "type", "type": "string"},
    {"name": "ui", "type": "string"},
    {"name": "url", "type": "string"},
    {"name": "vars", "type": "any"},
    {"name": "timings", "type": "any"},
    {"name": "pay_detector_predict", "type": "double"},  # MSTAND-2067
    {"name": "query_about_one_product", "type": "double"},  # MSTAND-2067
    {"name": "query_about_many_products", "type": "double"},  # MSTAND-2067
    {"name": "ecom_classifier_prob", "type": "float"},  # MSTAND-2101
    {"name": "ecom_classifier_result", "type": "boolean"},  # MSTAND-2101
]

IMAGES_SAMPLE = u"""{"action_index":0,"blocks":{"related_queries":{"row":5}},"bucket":null,"cts":null,
"dnorm":"пешта по стол","domregion":"ru","dwelltime_on_service":null,"grid":{"heights":[180,183,178,173,188],
"viewport":[1366,667],"widths":[[230,212,270,228,337],[196,182,183,180,284,244],[238,267,267,267,238],
[230,259,298,259,231],[252,282,188,285,270]]},"is_match":true,"page":0,"path":null,"query":"стол пешта по",
"reqid":"1464792462088558-16647597805119734638162968-7-038-IMG","results":30,"serpid":"Zu1FjxHfgGzDyK8QIOQcow",
"servicetype":"images","source":"wiz","source-serpid":null,"synnorm":"пешта стол","testid":"0",
"timings":{"time_to_visible_serp":null},"ts":1464792462,"type":"request","ui":"desktop", "url": "","vars":null,
"yuid":"y1000000071415298185"}"""


class ActionsSqueezerImages(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: add dnorm and synnorm queries
    3: add hovers
    4: add hover vars
    5: add timings metrics (visible serp)
    6: add try/except for timings field
    7: add parent-reqid and url
    8: use //user_sessions/pub/images/.../clean tables only (MSTAND-2016)
    9: add pay_detector_predict, query_about_one_product, query_about_many_products (MSTAND-2067)
    10: add ecom_prob (MSTAND-2092)
    11: rename ecom_prob -> ecom_classifier_result and add ecom_classifier_prob (MSTAND-2101)
    """
    VERSION = 11

    YT_SCHEMA = IMAGES_YT_SCHEMA
    SAMPLE = IMAGES_SAMPLE

    def __init__(self):
        super(ActionsSqueezerImages, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        for request in args.container.GetRequests():
            if request.IsA("TImagesRequestProperties"):
                exp_bucket = self.check_experiments(args, request)
                collect_images_actions(args.result_actions, exp_bucket, request, args.blockstat)


def collect_images_actions(known_actions, exp_bucket, request, blockstat):
    ui = UiType.get_ui_type(request)
    if ui is None:
        return
    base_data = sq_common.prepare_base_data(exp_bucket, request)
    serpid = request.SerpID
    request_squeezer = RequestSqueezerImages(blockstat, ui, serpid, known_actions, base_data)
    request_squeezer.collect_actions(request)


class RequestSqueezerImages(object):
    PATTERN_OPEN = re.compile(r"^serp/results/(keyboard|slideshow|thumb)/|^(preview|fullscreen)/(imgclick|next|prev)")
    PATTERN_GREENURL = re.compile(r"/(site|url|title|link|button|commercial/thumb/market|collections)")
    PATTERN_DONT_TAKE = re.compile(r"/(open|thmb_speed)")  # (r"/(open|thmb_speed|hover)")

    PATH_PREFIXES = {
        "desktop": "/image/new/",
        "touch": "/image/touch/",
    }

    def __init__(self, blockstat_dict, ui, serpid, known_actions, base_data):
        self.ui = ui
        self.output = known_actions
        self.blockstat_dict = blockstat_dict
        self.base_data = dict(base_data, ui=ui, serpid=serpid)

    @classmethod
    def _get_event_type(cls, path):
        if cls.PATTERN_OPEN.search(path):
            return "img_open"
        elif cls.PATTERN_GREENURL.search(path):
            return "greenurl"
        return "other"

    def _omit_path_prefix(self, path):
        prefix = self.PATH_PREFIXES.get(self.ui, "")
        return path[len(prefix):] if path.startswith(prefix) else path

    def _parse_vars(self, vars_list):
        result = {}
        if isinstance(vars_list, str):
            vars_list = [x.split("=", 1) for x in vars_list.split(",") if "=" in x]
        for key, value in vars_list:
            converted_key = key[1:] if key.startswith("-") else self.blockstat_dict.get(key, key)
            result[converted_key] = int(value) if value.isdigit() else value
        return result

    def collect_actions(self, request):
        if self.ui is None:
            return

        self.collect_request_data(request)

        for click in request.GetClicks():
            self.append_user_action(click)
        for event in request.GetOwnEvents():
            if not event.IsA("TClick"):
                self.append_user_action(event)

    @staticmethod
    def is_related_queries_vars_valid(block):
        # related queries blockstat shows with only 1 param "-row"
        return len([True for var in block.GetVars() if var[0] == "-row" and var[1] != "NaN"])

    def get_request_related_queries_positions(self, request):
        result = {}
        if request.IsA("TBlockstatRequestProperties"):
            for block in request.GetBSBlocks():
                if "serp/related_query" in block.Path and self.is_related_queries_vars_valid(block):
                    for name, value in block.GetVars():
                        if name == "-row":
                            result["row"] = int(value)
        return result

    def get_request_collections_positions(self, request):
        result = {}
        if request.IsA("TBlockstatRequestProperties"):
            for block in request.GetBSBlocks():
                if "serp/collections" in block.Path:
                    for name, value in block.GetVars():
                        if name == "-row":
                            result["row"] = int(value)
        return result

    @staticmethod
    def get_request_fresh_positions(request):
        result = {}
        fresh_positions = []
        if request.IsA("TImagesRequestProperties"):
            for block in request.GetMainBlocks():
                res = block.GetMainResult()
                if hasattr(res, "Source") and res.Source in ["IMAGESQUICK", "IMAGESULTRA"]:
                    fresh_positions.append(res.Position)

        if fresh_positions:
            result["pos"] = fresh_positions

        return result

    def get_request_grid_viewport(self, request):
        result = {}
        for e in request.GetYandexTechEvents():
            if e.Path in ["image.new.serp.list.loaded",
                          "image.touch.serp.list.loaded",
                          "8.228.471.436.1007",
                          "8.584.471.436.1007"]:
                params = self._parse_vars(e.Vars)
                if params:
                    # noinspection PyBroadException
                    try:
                        result["viewport"] = [int(i) for i in params["viewport"].split(";")]
                        result["heights"] = [int(i) for i in str(params["heights"]).split(";")]
                        result["widths"] = [
                            [int(width) for width in row.split("-")] for row in params["widths"].split(";")
                        ]
                    except Exception:
                        pass
        return result

    def get_request_load_time(self, request):
        timings = {}
        time_to_visible_serp = 0
        for e in request.GetOwnEvents():
            if e.Path == "690.2096.207":
                try:
                    perf_vars = self._parse_vars(e.Vars)
                    if perf_vars["id"] in [2540, 2542]:
                        time_to_visible_serp = max(time_to_visible_serp, float(perf_vars["time"]))
                except:
                    pass
        if time_to_visible_serp != 0:
            timings["time_to_visible_serp"] = time_to_visible_serp
        else:
            timings["time_to_visible_serp"] = None
        return timings

    @staticmethod
    def get_request_cgi_params(request):
        result = {}

        parsed_url = uurl.urlparse(request.FullRequest)
        cgi_params = uurl.parse_qs(parsed_url.query)
        for param in ("source", "source-serpid"):
            if param in cgi_params:
                result[param] = cgi_params[param][0]

        return result

    def collect_request_data(self, request):
        results = sum(1 for x in request.GetMainBlocks() if x.GetMainResult() is not None)
        request_data = dict(
            self.base_data,
            type="request",
            ts=request.Timestamp,
            query=request.Query,
            page=request.PageNo,
            results=results,
        )
        request_data["dnorm"] = ""
        if "dnorm" in request.RelevValues:
            request_data["dnorm"] = request.RelevValues["dnorm"]

        request_data["synnorm"] = ""
        if "synnorm" in request.RelevValues:
            request_data["synnorm"] = request.RelevValues["synnorm"]

        request_data["parent_reqid"] = request.WebParentReqId if request.WebParentReqId is not None else ""

        seen_blocks = {}

        related_queries = self.get_request_related_queries_positions(request)
        if related_queries:
            seen_blocks["related_queries"] = related_queries

        collections = self.get_request_collections_positions(request)
        if collections:
            seen_blocks["collections"] = collections

        fresh_images = self.get_request_fresh_positions(request)
        if fresh_images:
            seen_blocks["fresh"] = fresh_images

        if seen_blocks:
            request_data["blocks"] = seen_blocks

        grid = self.get_request_grid_viewport(request)
        if grid:
            request_data["grid"] = grid

        timings = self.get_request_load_time(request)
        if timings:
            request_data["timings"] = timings

        cgi_params = self.get_request_cgi_params(request)
        if cgi_params:
            request_data = dict(request_data, **cgi_params)

        sq_common.enrich_common_request_data(request, request_data)

        self.output.append(request_data)

    def append_user_action(self, action):
        action_vars = None
        path = None
        pos = None
        click_source = None
        DwellTimeOnService = None

        if action.IsA("TImageShow"):
            action_type = "img_load"
            action_vars = self._parse_vars(action.Vars)
        elif action.IsA("TClick") or action.IsA("TImageNavig"):
            if action.IsA("TClick"):
                DwellTimeOnService = int(action.DwellTimeOnService)
            path = self._omit_path_prefix(action.ConvertedPath)
            if self.PATTERN_DONT_TAKE.search(path):
                return
            action_type = self._get_event_type(path)
            if action.IsA("TResultClick"):
                if "results/thumb" in path:
                    click_source = "preview" if "&img_url=" in action.Referer else "serp"
                parent = action.GetParent()
                if parent:
                    pos = parent.Position
            elif path == "serp/hover":
                try:
                    raw_vars = []
                    for p in action.GetVars()[1:]:
                        raw_vars.extend(p)
                    action_vars_clean = ",".join([k for k in raw_vars if ":" in k])
                    raw_vars = uurl.unquote(action_vars_clean)
                    action_vars = json.loads(raw_vars)
                except:
                    action_vars = None
            else:
                # universal vars extractor for all images counters
                if hasattr(action, "Vars"):
                    action_vars = self._parse_vars(action.Vars)
                elif hasattr(action, "GetVars"):
                    action_vars = self._parse_vars(action.GetVars())
        elif "471.2177.882" in action.Path:  # TYandexTechEvent "serp/collections/click"
            action_type = "other"
            path = "serp/collections/click"
        else:
            return

        action_type = "images_" + action_type
        if action_vars is not None and "-decoded-url" in action_vars:
            url = uurl.unquote(action_vars["-decoded-url"])
        else:
            url = uurl.unquote(action.Url) if hasattr(action, "Url") and action.Url is not None else ""
        data = dict(
            self.base_data,
            type=action_type,
            ts=action.Timestamp,
            cts=action.ClientTimestamp,
            dwelltime_on_service=DwellTimeOnService,
            url=url
        )
        if path:
            data["path"] = path
        if action_vars:
            vars_description = "vars {!r} for data {!r}".format(action_vars, data)
            data["vars"] = sq_common.fix_positive_int64_recursive(action_vars, vars_description)
        if pos is not None:
            data.setdefault("vars", dict())["pos"] = pos
        if click_source:
            data.setdefault("vars", dict())["click_source"] = click_source

        self.output.append(data)


def build_image_navig_data(base_data, event):
    if event.IsA("TImageNavig"):
        return dict(
            base_data,
            ts=event.Timestamp,
            type="img_navig",
            url=event.Url,
            path=event.Path,
        )
    else:
        return None


class UiType(object):
    mapping = {
        "TDesktopUIProperties": "desktop",
        "TMobileUIProperties": "mobile",
        "TTouchUIProperties": "touch",
        "TMobileAppUIProperties": "mobileapp",
        "TPadUIProperties": "pad",
        "TSiteSearchUIProperties": "sitesearch"
    }

    @classmethod
    def get_ui_type(cls, item):
        for ui_prop, name in usix.iteritems(cls.mapping):
            if item.IsA(ui_prop):
                return name
        return None
