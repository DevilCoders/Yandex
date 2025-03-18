# coding=utf-8
import yaqutils.url_helpers as uurl

import session_squeezer.squeezer_common as sq_common

VIDEO_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "browser", "type": "string"},
    {"name": "bucket", "type": "int64"},
    {"name": "clid", "type": "string"},
    {"name": "correctedquery", "type": "string"},
    {"name": "dir_show", "type": "int64"},
    {"name": "domregion", "type": "string"},
    {"name": "duration", "type": "int64"},
    {"name": "dwelltime_on_service", "type": "int64"},
    {"name": "fraud_bits", "type": "int64"},
    {"name": "hasmisspell", "type": "boolean"},
    {"name": "is_match", "type": "boolean"},
    {"name": "maxrelevpredict", "type": "double"},
    {"name": "minrelevpredict", "type": "double"},
    {"name": "page", "type": "int64"},
    {"name": "path", "type": "string"},
    {"name": "playing_duration", "type": "int64"},
    {"name": "pos", "type": "int64"},
    {"name": "query", "type": "string"},
    {"name": "referer", "type": "string"},
    {"name": "reqid", "type": "string"},
    {"name": "restype", "type": "string"},
    {"name": "servicetype", "type": "string"},
    {"name": "sources", "type": "any"},
    {"name": "suggest", "type": "any"},
    {"name": "target", "type": "string"},
    {"name": "testid", "type": "string"},
    {"name": "ticks", "type": "int64"},
    {"name": "type", "type": "string"},
    {"name": "url", "type": "string"},
    {"name": "userpersonalization", "type": "boolean"},
    {"name": "userregion", "type": "int64"},
    {"name": "vars", "type": "string"},
    {"name": "videodata", "type": "any"},
    {"name": "visual_pos", "type": "int64"},
    {"name": "wiz_show", "type": "int64"},
    {"name": "wizard", "type": "boolean"},
    {"name": "queryregion", "type": "int64"},
    {"name": "from_block", "type": "string"},  # MSTAND-1588
    {"name": "pay_detector_predict", "type": "double"},  # MSTAND-2067
    {"name": "query_about_one_product", "type": "double"},  # MSTAND-2067
    {"name": "query_about_many_products", "type": "double"},  # MSTAND-2067
    {"name": "ecom_classifier_prob", "type": "float"},  # MSTAND-2101
    {"name": "ecom_classifier_result", "type": "boolean"},  # MSTAND-2101
]

VIDEO_SAMPLE = u"""{"action_index":0,"browser":"Opera","bucket":null,"clid":"",
"correctedquery":"черниговская татьяна владимировна","dir_show":0,"domregion":"by","duration":null,
"dwelltime_on_service":null,"fraud_bits":null,"hasmisspell":false,"is_match":true,"maxrelevpredict":null,
"minrelevpredict":null,"page":0,"path":null,"playing_duration":null,"pos":null,
"query":"черниговская татьяна владимировна","queryregion":null,"referer":"other",
"reqid":"1464779519844025-4974939474768078460106293-iva1-0884-V","restype":null,"servicetype":"video","sources":[],
"suggest":null,"target":null,"testid":"0","ticks":null,"ts":1464779519,"type":"request","url":null,
"userpersonalization":true,"userregion":157,"vars":null,"videodata":{"is_porno":false,"is_serial":false,
"parent_reqid":"1464779511605301-4772335417162571051106473-man1-3594","path":"wizard","player_events":[],
"results":[{"clicks":[],"duration":5790,"modification_time":1450254865,"player_id":null,"source":"VIDEO",
"url":"http://www.youtube.com/watch?v=4TJxgRUwNVU","view_time_info":{}},{"clicks":[],
"duration":5636,"modification_time":1434033288,"player_id":null,"source":"VIDEO",
"url":"http://www.youtube.com/watch?v=nEGmdlJEr8M","view_time_info":{}},{"clicks":[],"duration":3148,
"modification_time":1352617200,"player_id":null,"source":"VIDEO","url":"http://www.youtube.com/watch?v=R0ZSO1aH4CI",
"view_time_info":{}},{"clicks":[],"duration":4800,"modification_time":1395519360,"player_id":null,"source":"VIDEO",
"url":"http://www.youtube.com/watch?v=8G1RNHGXsAQ","view_time_info":{}},{"clicks":[],"duration":4801,
"modification_time":1372435083,"player_id":null,"source":"VIDEO","url":"http://vimeo.com/69326416",
"view_time_info":{}},{"clicks":[],"duration":6198,"modification_time":1421218800,"player_id":null,"source":"VIDEO",
"url":"http://www.youtube.com/watch?v=NdYu-Er73jU","view_time_info":{}},{"clicks":[],"duration":3313,
"modification_time":1419750000,"player_id":null,"source":"VIDEO","url":"http://www.youtube.com/watch?v=D2UjG9uOaYc",
"view_time_info":{}},{"clicks":[],"duration":2535,"modification_time":1394735629,"player_id":null,"source":"VIDEO",
"url":"http://www.youtube.com/watch?v=DFffqNAYXYk","view_time_info":{}},{"clicks":[],"duration":3392,
"modification_time":1429023976,"player_id":null,"source":"VIDEO","url":"http://www.youtube.com/watch?v=ThjJC7pTLT4",
"view_time_info":{}},{"clicks":[],"duration":6115,"modification_time":1423387241,"player_id":null,"source":"VIDEO",
"url":"http://www.youtube.com/watch?v=BbcX8FHJWo4","view_time_info":{}}],"type":"search_request",
"vnorm_dopp_query":"владимировна татьяна черниговский","vnorm_query":"черниговская татьяна владимировна",
"wizard_clicks":[],"wizard_results":[]},"visual_pos":null,"wiz_show":0,"wizard":false,"yuid":"y1000001501447865687"}"""


class ActionsSqueezerVideo(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: add has_ento flag to request
    3: add techevent tech/timing
    4: add winner_type to videodata
    5: add related_url to videodata
    6: add normalized query to videodata
    7: add path and source info
    8: add video blender data
    9: add player events data and player_id result field
    10: add video fragments info
    11: use //user_sessions/pub/video/.../clean tables only (MSTAND-2016)
    12: add pay_detector_predict, query_about_one_product, query_about_many_products (MSTAND-2067)
    13: add ecom_prob (MSTAND-2092)
    14: rename ecom_prob -> ecom_classifier_result and add ecom_classifier_prob (MSTAND-2101)
    """
    VERSION = 14

    YT_SCHEMA = VIDEO_YT_SCHEMA
    SAMPLE = VIDEO_SAMPLE

    def __init__(self):
        super(ActionsSqueezerVideo, self).__init__()

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        for request in args.container.GetRequests():
            if request.IsA("TVideoRequestProperties"):
                exp_bucket = self.check_experiments(args, request)
                collect_video_actions(args.result_actions, exp_bucket, request)


def collect_video_actions(known_actions, exp_bucket, request):
    base_data = sq_common.prepare_base_data(exp_bucket, request)
    request_data = sq_common.build_request_data(base_data, request)

    request_data["videodata"] = prepare_video_request_data(request)
    sq_common.enrich_common_request_data(request, request_data)

    known_actions.append(request_data)
    sq_common.append_clicks(known_actions, base_data, request)
    append_video_heartbeat(known_actions, base_data, request)
    sq_common.append_misc_clicks(known_actions, base_data, request)
    append_durations(known_actions, base_data, request)
    sq_common.append_techevents(known_actions, base_data, request)


def prepare_video_request_data(item):
    fragment_begin = None
    fragment_film_id = None
    request_data = {}
    results = request_data.setdefault("results", [])
    wizard_results = request_data.setdefault("wizard_results", [])
    wizard_clicks = request_data.setdefault("wizard_clicks", [])
    player_events = request_data.setdefault("player_events", [])
    parsed_url = uurl.urlparse(item.FullRequest)
    cgi_params = uurl.parse_qs(parsed_url.query)
    if item.ProducingReqId:
        request_data["parent_reqid"] = item.ProducingReqId
    else:
        parent_reqid = cgi_params.get("parent_reqid")
        if not parent_reqid:
            parent_reqid = cgi_params.get("related_reqid")
        if not parent_reqid:
            parent_reqid = cgi_params.get("parent-reqid")
        if not parent_reqid:
            parent_reqid = cgi_params.get("reqid")
        if parent_reqid:
            request_data["parent_reqid"] = parent_reqid[0]
    path = cgi_params.get("path")
    if path:
        request_data["path"] = path[0]
    source = cgi_params.get("source")
    if source:
        request_data["source"] = source[0]
        if source[0] == "fragment":
            t = cgi_params.get("t")
            film_id = cgi_params.get("filmId")
            if t and film_id:
                try:
                    fragment_begin = int(t[0])
                except ValueError:
                    pass
                else:
                    fragment_film_id = film_id[0]
    if item.IsA("TRelatedVideoRequestProperties"):
        related_url = cgi_params.get("related_url")
        if related_url:
            request_data["related_url"] = related_url[0]
    is_serial = item.IsA("TMiscRequestProperties") and "vserial" in item.RelevValues
    is_porno = item.IsA("TMiscRequestProperties") and item.SearchPropsValues.get("VIDEO.VideoPorno.vidprn") == "ipq1"
    if item.IsA("TMiscRequestProperties") and item.SearchPropsValues.get("REPORT.winner_types"):
        request_data["winner_type"] = item.SearchPropsValues.get("REPORT.winner_types")
    request_data["is_serial"] = is_serial
    request_data["is_porno"] = is_porno
    if item.IsA("TMiscRequestProperties"):
        if "vnorm" in item.RelevValues:
            request_data["vnorm_query"] = item.RelevValues.get("vnorm")
        if "vnorm_dopp" in item.RelevValues:
            request_data["vnorm_dopp_query"] = item.RelevValues.get("vnorm_dopp")
        for key, value in item.SearchPropsValues.items():
            if key.startswith("UPPER.ApplyVideoBlender.IntentPos/"):
                wizard_name = key[34:]
                wizard_position = value
                wizard_results.append({"wizard_name": wizard_name, "wizard_position": wizard_position})
    for click in item.GetClicks():
        if click.Path.startswith("358."):
            wizard_clicks.append({"path": click.Path, "ts": click.Timestamp})
    for event in item.GetYandexTechEvents():
        if event.IsA("TVideoPlayerEvent"):
            player_events.append({
                "ts": event.Timestamp,
                "cts": event.ClientTimestamp,
                "time": event.Time,
                "previous_time": event.PreviousTime,
                "url": event.Url,
                "path": event.Path,
            })
    for block in item.GetMainBlocks():
        result = block.GetMainResult()
        if not result.IsA("TVideoResult"):
            continue
        result_data = {
            "url": result.Url,
            "player_id": result.PlayerId,
            "duration": result.Duration,
            "source": result.VideoSource,
            "modification_time": result.ModificationTime
        }
        if fragment_film_id and fragment_film_id == result.FilmId:
            result_data["fragment_info"] = {
                "begin": fragment_begin,
            }
        clicks_data = result_data.setdefault("clicks", [])
        for click in result.GetClicks():
            click_data = {
                "ts": click.Timestamp,
                "path": click.Path,
            }
            dur = item.FindVideoDurationInfo(click)
            if dur:
                click_data["view_duration"] = dur.PlayingDuration
            hb_single = item.FindVideoHeartbeat(click, "SINGLE")
            if hb_single:
                click_data["hb-single"] = hb_single.Ticks
            hb_repeat = item.FindVideoHeartbeat(click, "REPEAT")
            if hb_repeat:
                click_data["hb-repeat"] = hb_repeat.Ticks
            clicks_data.append(click_data)
        view_time_data = result_data.setdefault("view_time_info", {})
        dur = item.FindVideoDurationInfo(result)
        if dur:
            view_time_data["view_duration"] = dur.PlayingDuration
        hb_single = item.FindVideoHeartbeat(result, "SINGLE")
        if hb_single:
            view_time_data["hb-single"] = hb_single.Ticks
        hb_repeat = item.FindVideoHeartbeat(result, "REPEAT")
        if hb_repeat:
            view_time_data["hb-repeat"] = hb_repeat.Ticks
        results.append(result_data)
    if item.IsA("TRelatedVideoRequestProperties"):
        request_data["type"] = "related_request"
    elif item.IsA("TYandexVideoMordaRequest"):
        request_data["type"] = "morda_request"
    else:
        request_data["type"] = "search_request"
    return request_data


def append_durations(known_actions, base_data, request):
    for duration in request.GetVideoDurationInfos():
        known_actions.append(build_duration_data(base_data, duration))


def build_duration_data(base_data, duration):
    return dict(
        base_data,
        ts=duration.Timestamp,
        type="duration",
        duration=duration.Duration,
        playing_duration=duration.PlayingDuration,
        url=duration.Url,
    )


def append_video_heartbeat(known_actions, base_data, request):
    events = request.GetOwnEvents()
    if not events:
        return
    for event in events:
        action_data = build_heartbeat_data(base_data, event)
        if action_data:
            known_actions.append(action_data)


def build_heartbeat_data(base_data, event):
    if event.IsA("TVideoHeartbeat") and event.Type == "repeat":
        return dict(
            base_data,
            ts=event.Timestamp,
            type="heartbeat",
            ticks=event.Ticks,
            url=event.Url,
            duration=event.Duration,
        )
    else:
        return None
