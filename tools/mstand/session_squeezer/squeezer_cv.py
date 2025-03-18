import json

from typing import Dict
from typing import List
from typing import Optional
from typing import Union

import session_squeezer.squeezer_common as sq_common
import yaqutils.url_helpers as uurl

from yaqtypes import JsonDict

CBIR_YT_SCHEMA = [
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
    {"name": "cbir_id", "type": "string"},
    {"name": "rpt", "type": "string"},
    {"name": "url", "type": "string"},
    {"name": "cbir_page", "type": "string"},
    {"name": "testid", "type": "string"},
    {"name": "type", "type": "string"},
    {"name": "ui", "type": "string"},
    {"name": "vars", "type": "any"},
    {"name": "timings", "type": "any"}
]


class ActionsSqueezerCv(sq_common.ActionsSqueezer):
    """
    1: initial version
    """
    VERSION = 1

    YT_SCHEMA = CBIR_YT_SCHEMA

    def __init__(self) -> None:
        super(ActionsSqueezerCv, self).__init__()

    def get_actions(self, args: sq_common.ActionSqueezerArguments) -> None:
        for request in args.container.GetRequests():
            if request.IsA("TCbirRequestProperties"):
                exp_bucket = self.check_experiments(args, request)
                collect_cbir_actions(args.result_actions, exp_bucket, request, args.blockstat)


def collect_cbir_actions(known_actions: List[JsonDict], exp_bucket: sq_common.ExpBucketInfo, request: "libra.TRequest", blockstat: Dict[str, str]) -> None:
    ui = UiType.get_ui_type(request)
    if ui is None:
        return
    base_data = sq_common.prepare_base_data(exp_bucket, request)
    serpid = request.SerpID
    request_squeezer = RequestSqueezerCbir(blockstat, ui, serpid, known_actions, base_data)
    request_squeezer.collect_actions(request)


class RequestSqueezerCbir(object):

    def __init__(self, blockstat_dict: Dict[str, str], ui: str, serpid: str, known_actions: List[JsonDict], base_data: JsonDict) -> None:
        self.ui = ui
        self.output = known_actions
        self.blockstat_dict = blockstat_dict
        self.base_data = dict(base_data, ui=ui, serpid=serpid)

    def _parse_vars(self, vars_list: Union[str, List[Union[str, str]]]) -> JsonDict:
        result = {}
        if isinstance(vars_list, str):
            vars_list = [x.split("=", 1) for x in vars_list.split(",") if "=" in x]
        for key, value in vars_list:
            converted_key = key[1:] if key.startswith("-") else self.blockstat_dict.get(key, key)
            result[converted_key] = int(value) if value.isdigit() else value
        return result

    def collect_actions(self, request: "libra.TRequest") -> None:
        if self.ui is None:
            return

        self.collect_request_data(request)

        for click in request.GetClicks():
            self.append_user_action(click)
        for event in request.GetOwnEvents():
            if not event.IsA("TClick"):
                self.append_user_action(event)

    def get_request_load_time(self, request: "libra.TRequest") -> JsonDict:
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
    def get_request_cgi_params(request: "libra.TRequest") -> JsonDict:
        result = {}

        parsed_url = uurl.urlparse(request.FullRequest)
        cgi_params = uurl.parse_qs(parsed_url.query)
        for param in ("source", "source-serpid", "cbir_id", "rpt", "url", "cbir_page"):  # TODO cgi-list
            if param in cgi_params:
                result[param] = cgi_params[param][0]

        return result

    def collect_request_data(self, request: "libra.TRequest") -> None:
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

        request_data["parent_reqid"] = request.WebParentReqId if request.WebParentReqId is not None else ''

        seen_blocks = {}

        if seen_blocks:
            request_data["blocks"] = seen_blocks

        timings = self.get_request_load_time(request)
        if timings:
            request_data["timings"] = timings

        cgi_params = self.get_request_cgi_params(request)
        if cgi_params:
            request_data = dict(request_data, **cgi_params)

        self.output.append(request_data)

    def append_user_action(self, action: "libra.TEvent") -> None:
        action_vars = None
        path = None
        pos = None
        click_source = None
        DwellTimeOnService = None

        if action.IsA("TImageShow"):
            action_vars = self._parse_vars(action.Vars)
        elif action.IsA("TClick") or action.IsA("TImageNavig"):
            if action.IsA("TClick"):
                DwellTimeOnService = int(action.DwellTimeOnService)
            path = action.ConvertedPath

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
                if hasattr(action, "Vars"):
                    action_vars = self._parse_vars(action.Vars)
                elif hasattr(action, "GetVars"):
                    action_vars = self._parse_vars(action.GetVars())
        else:
            return

        data = dict(
            self.base_data,
            ts=action.Timestamp,
            type='action',
            cts=action.ClientTimestamp,
            dwelltime_on_service=DwellTimeOnService,
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
    def get_ui_type(cls, item: "libra.TEvent") -> Optional[str]:
        for ui_prop, name in cls.mapping.items():
            if item.IsA(ui_prop):
                return name
        return None
