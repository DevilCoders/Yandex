import logging
import re

import yaqutils.url_helpers as uurl
import session_squeezer.squeezer_common as sq_common
import yaqutils.misc_helpers as umisc

NEWS_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},
    {"name": "experiments", "type": "any"},
    {"name": "puid", "type": "string"},
    {"name": "service", "type": "string"},
    {"name": "parent_id", "type": "string"},
    {"name": "has_parent", "type": "boolean"},

    {"name": "type", "type": "string"},
    {"name": "device", "type": "string"},
    {"name": "project", "type": "string"},

    {"name": "domregion", "type": "string"},
    {"name": "dwelltime", "type": "int64"},
    {"name": "host", "type": "string"},
    {"name": "full_request", "type": "string"},
    {"name": "title_url", "type": "string"},
    {"name": "widget_aliases", "type": "any"},

    {"name": "state", "type": "string"},
    {"name": "persistent_id", "type": "string"},
    {"name": "stid", "type": "string"},
    {"name": "robot", "type": "int64"},
    {"name": "no_banner", "type": "int64"},
    {"name": "from_mark", "type": "string"},
    {"name": "story_rubric", "type": "string"},
    {"name": "adb", "type": "int64"},
    {"name": "i18n", "type": "string"},
    {"name": "doclang", "type": "string"},
    {"name": "region_attr", "type": "int64"},
    {"name": "temperature", "type": "any"},
    {"name": "agency_id", "type": "any"},

    {"name": "page", "type": "string"},
    {"name": "click_ts", "type": "int64"},
    {"name": "path", "type": "string"},
    {"name": "path_pos", "type": "string"},
    {"name": "pos", "type": "int64"},
    {"name": "query", "type": "string"},
    {"name": "clicked_url", "type": "string"},
    {"name": "external_host_name", "type": "string"},
    {"name": "has_video", "type": "boolean"},

    {"name": "reqid", "type": "string"},
    {"name": "request_path", "type": "string"},

    {"name": "url_path", "type": "string"},
    {"name": "vars", "type": "any"},

    {"name": "events_count", "type": "int64"},
    {"name": "event_type", "type": "string"},
    {"name": "events_data", "type": "any"},
    {"name": "heart_beat_time", "type": "float"},
]

TAMUS_RULES = {
    "root": "$page | $subpage",
    "news": "#root[@service = \"news\"]",
    "news_story": "#news[@page = \"story\"]",
    "news_story_title": "#news_story//App/NewsStory/Subtitle",
    "news_story_widget": "#news_story//App/Widget",
    "news_desktop_video": "#news_neo//App/NewsStory/MediaStack2",
    "sport": "#root[@service = \"sport\"]",
    "sport_story": "#sport[@page = \"story\"]",
    "sport_story_title": "#sport_story//App/SportStory/Subtitle",
    "sport_story_widget": "#sport_story//App/Widget",
    "sport_desktop_video": "#sport_story//App/NewsStory/MediaStack2",
}

TECH_EVENTS_BASE_LIST = [
    "add-favorite", "beforeunload", "blur", "delete-favorite", "focus", "orientationchange", "page-scroll", "resize",
    "favorite-settings-open"
]

TECH_EVENTS_CUSTOM_LIST = [
    "heart-beat",
]

TECH_EVENTS_WITH_DATA_LIST = [
    "story-report", "add-favorite", "delete-favorite", "favorite-settings-open",
]

ML_ATTRS = [
    "robot", "region", "state", "persistent_id", "stid",
    "no_banner", "from", "rubric", "adb", "i18n", "doclang", "temperature", "agency_id",
]

ML_ATTRS_RENAME = {
    "from": "from_mark",
    "rubric": "story_rubric",
    "region": "region_attr"
}

MAX_HEART_BEAT_TIME = 1e5


class ActionsSqueezerNews(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: remove SINK requests
    3: add blocks
    4: add events
    5: configure for baobab
    6: configure for neo
    7: add `external_host_name`
    8: add `title_url`
    9: add `widget_aliases`
    10: add pos to `path_pos`
    11: add neo baobab tech events, add `has_video`
    12: add ML attributes, upd tech events
    13: add `clicked_url`
    14: add heart-beats
    15: add experiments (list of testids)
    16: add `temperature`,add `favorite-settings-open` to tech events
    17: add `service`, `parent_id`, add `is_parent`, add `puid`
    18: add `service`, `page` to tech events, `from_mark` to heartbeat
    19: change `is_parent` to `has_parent`, added title_link to sport
    20: change featrures for ability to get them for different services
    21: add `agency_id`, fixed min heart-beat
    """
    VERSION = 21

    YT_SCHEMA = NEWS_YT_SCHEMA

    def __init__(self):
        super(ActionsSqueezerNews, self).__init__()
        self.visited_reqids = set()  # unique for user

    def get_actions(self, args):
        """
        :type args: sq_common.ActionSqueezerArguments
        """
        for request in args.container.GetRequests():
            is_duplicate, is_news, is_baobab, is_news_turbo, is_news_neo = self.check_request(request)
            if is_duplicate:
                continue
            if is_news or is_news_turbo:
                self.visited_reqids.add(request.ReqId)
                exp_bucket = self.check_experiments(args, request)
                collect_news_actions(args.result_actions, exp_bucket, request, args.blockstat,
                                     is_news, is_baobab, is_news_turbo, is_news_neo)

    def check_request(self, request):
        is_duplicate = request.ReqId in self.visited_reqids
        is_news = request.IsA("TNewsRequestProperties")
        is_baobab = request.IsA("TBaobabProperties")
        is_news_turbo = False
        is_news_neo = False
        if is_baobab:
            if request.BaobabTree() is not None:
                root_attrs = request.BaobabTree().get_show().tree.root.attrs
                is_news_turbo = (root_attrs.get("subservice") == "news" and root_attrs.get("service") == "turbo")
                is_news_neo = ((root_attrs.get("service") in ("news", "sport")) and root_attrs.get("project") == "neo")
            else:
                logging.warning("BaobabTree is None")
        return is_duplicate, is_news, is_baobab, is_news_turbo, is_news_neo


def collect_news_actions(known_actions, exp_bucket, request, blockstat, is_news, is_baobab, is_news_turbo, is_news_neo):
    base_data = sq_common.prepare_base_data(exp_bucket, request)
    base_data = enrich_base_data(request, base_data, is_news, is_baobab, is_news_turbo, is_news_neo)
    request_data = build_news_request(known_actions, base_data, request)
    append_news_clicks(known_actions, base_data, request, request_data, blockstat)
    append_news_events(known_actions, base_data, request, request_data)


def build_news_request(known_actions, base_data, request):
    host, full_request, decoded_query, url_path = extract_from_url(request, base_data)
    page = get_page(base_data, request, url_path)
    features = get_request_features(request)
    puid = request.PassportUID

    experiments = []
    for testid in request.GetTestInfo():
        experiments.append(testid.TestID)
    request_data = dict(
        base_data,
        type="request",
        query=decoded_query,
        host=host,
        full_request=full_request,
        page=page,
        dwelltime=request.DwellTimeOnService,
        experiments=experiments,
        puid=puid,
    )
    request_data.update(features)
    known_actions.append(request_data)
    return request_data


def get_page(base_data, request, url_path):
    project = base_data["project"]
    if project == "nerpa":
        path = url_path
        if path.startswith("/news/story"):
            return "story"
        elif path == "/news" \
                or path.startswith("/news/rubric/index") \
                or path.startswith("/news/?") \
                or path.startswith("/news?"):
            return "main"
        elif path.startswith("/news/rubric") or path.startswith("/news/region"):
            return "rubric"
        elif path.startswith("/news/theme"):
            return "theme"
        elif path.startswith("/news/instory"):
            return "instory"
        elif path.startswith("/yandsearch"):
            return "search"
        elif path.startswith("/sport"):
            return "sport"
        else:
            logging.error("Cannot determine nerpa page, {}".format(path))
    elif project in ("turbo-news", "neo"):
        return request.BaobabTree().get_show().tree.root.attrs.get("page")


def enrich_base_data(request, base_data, is_news, is_baobab, is_news_turbo, is_news_neo):
    project = get_project(request, is_news, is_baobab, is_news_turbo, is_news_neo)
    device = None
    if project in ("turbo-news", "neo"):
        device = request.BaobabTree().get_show().tree.root.attrs.get("ui")
    else:
        if request.IsA("TMobileYandexNewsRequest"):
            device = "touch"
        elif request.IsA("TYandexNewsRequest"):
            device = "desktop"
        else:
            raise Exception
    base_data.update(dict(
        project=project,
        device=device,
        ts=request.Timestamp,
    ))
    return base_data


def get_project(request, is_news, is_baobab, is_news_turbo, is_news_neo):
    if is_news_turbo:
        assert request.BaobabTree() is not None, "Turbo baobab is None"
        return "turbo-news"
    elif is_news and is_baobab and is_news_neo:
        assert request.BaobabTree() is not None, "Neo baobab is None"
        return "neo"
    elif is_news:
        return "nerpa"
    else:
        logging.error("Cannot determine project")


def extract_from_url(request, base_data):
    host = request.Host if hasattr(request, "Host") else ""
    full_request = request.FullRequest if hasattr(request, "Host") else ""
    encoded_query = re.findall(r"text=([0-9a-zA-Z%\-_.~+]+)&", full_request)
    decoded_query = uurl.unquote(encoded_query[0]) if encoded_query else None
    comps = uurl.urlparse(full_request)
    url_path = comps.path
    return host, full_request, decoded_query, url_path


def get_request_features(request):
    import tamus
    service = "unknown"
    parent_id = None
    has_parent = False
    title_url = None
    widget_aliases = []
    has_video = False
    ml_attrs = {}
    agency_id = None

    if request.BaobabTree() is not None:
        root_attrs = request.BaobabTree().get_show().tree.root.attrs
        service = root_attrs.get("service")
        parent_id = root_attrs.get("parent_id")
        if parent_id:
            has_parent = True

    joiners = request.BaobabAllTrees()

    if joiners:
        marks = tamus.check_rules_multiple_joiners_merged(TAMUS_RULES, joiners)
        for block in marks.get_blocks("{}_story_title".format(service)):
            title_url = block.attrs.get("url")
            agency_id = block.attrs.get("agency_id")
        for block in marks.get_blocks("{}_story_widget".format(service)):
            widget_aliases.append(block.attrs.get("alias"))
        for block in marks.get_blocks("{}_desktop_video".format(service)):
            has_video = True
        for block in marks.get_blocks(service):
            attrs = block.attrs
            if not block.attrs:
                logging.error(
                    "Tamus marker #news contains empty or null attrs dictionary.\n"
                    + "reqid={}".format(request.ReqId)
                )
                continue
            for attr in attrs:
                if attr in ML_ATTRS and attr not in ML_ATTRS_RENAME:
                    ml_attrs[attr] = attrs[attr]
                elif attr in ML_ATTRS and attr in ML_ATTRS_RENAME:
                    renamed_attr = ML_ATTRS_RENAME[attr]
                    ml_attrs[renamed_attr] = attrs[attr]

    features = {
        "title_url": title_url,
        "widget_aliases": widget_aliases,
        "has_video": has_video,
        "service": service,
        "parent_id": parent_id,
        "has_parent": has_parent,
        "agency_id": agency_id,
    }
    features.update(ml_attrs)
    return features


def append_news_clicks(known_actions, base_data, request, request_data, blockstat):
    def append_news_clicks_nerpa(known_actions, base_data, request, blockstat):
        for click in request.GetClicks():
            path = parse_path(click.ConvertedPath, blockstat)
            click_vars = click.GetVars()
            pos = find_pos_in_vars(click_vars)
            if click.GetParent():
                pos = click.GetParent().Position
            click_data = dict(
                base_data,
                click_ts=click.Timestamp,
                type="click",
                path=path,
                pos=pos,
                vars=click_vars,
                dwelltime=click.DwellTimeOnService if hasattr(click, "DwellTimeOnService") else None,
            )
            known_actions.append(click_data)

    def append_news_clicks_baobab(known_actions, base_data, request):
        # noinspection PyUnresolvedReferences,PyPackageRequirements
        import baobab

        joiner = request.BaobabTree()
        show = joiner.get_show()
        for click in joiner.get_events_by_block_subtree(show.tree.root):
            if isinstance(click, baobab.common.Click):
                clicked_block = show.tree.get_block_by_id(click.block_id)
                libra_click = None
                try:
                    libra_click = request.FindClickByBaobabEventID(click.event_id)
                except Exception:
                    logging.warning("Cannot find click by baobab event id")
                dwelltime = None
                if libra_click is not None and hasattr(libra_click, "DwellTimeOnService"):
                    dwelltime = libra_click.DwellTimeOnService
                block_features = get_host_props(clicked_block)
                click_data = dict(
                    base_data,
                    click_ts=int(click.client_timestamp if click.client_timestamp is not None else 0),
                    type="click",
                    path=get_baobab_path(clicked_block),
                    path_pos=get_baobab_path(clicked_block, add_pos=True),
                    pos=get_baobab_block_index(clicked_block),
                    clicked_url=block_features["clicked_url"],
                    external_host_name=block_features["host_name"],
                    dwelltime=dwelltime,
                    widget_aliases=request_data.get("widget_aliases"),
                    page=request_data.get("page"),
                    service=request_data.get("service"),
                    agency_id=block_features["agency_id"],
                )
                known_actions.append(click_data)
                if not libra_click:
                    logging.warning("Error getting libra click from baobab. Path: {}".format(
                        get_baobab_path(clicked_block))
                    )

    if base_data["project"] == "nerpa":
        append_news_clicks_nerpa(known_actions, base_data, request, blockstat)
    elif base_data["project"] in ("turbo-news", "neo"):
        append_news_clicks_baobab(known_actions, base_data, request)
    else:
        logging.error("No project found")


def find_pos_in_vars(block_vars):
    for key, value in block_vars:
        if key == "pos" and value.startswith("p"):
            return umisc.optional_int(value[1:])
    return None


def parse_block(block):
    block_vars = block.GetVars()
    block_pos = find_pos_in_vars(block_vars)
    return {
        "path": block.Path,
        "pos": block_pos,
    }


def check_baobab_strings(title, logging_comment=""):
    pattern = r"^[a-zA-Z0-9\_\-]+$"
    is_ok = re.match(pattern, title)
    if not is_ok:
        logging.warning(
            "{} {} does not fit into \"{}\" pattern".format(title, logging_comment, pattern)
        )


def get_baobab_path(block, add_pos=False):
    # noinspection PyUnresolvedReferences,PyPackageRequirements
    import baobab

    def parse_block_with_attrs(block, white_list, add_pos=False):
        result = [block.name]
        for attr in block.attrs:
            if attr in white_list:
                check_baobab_strings(attr, logging_comment="attribute name of {}".format(block.name))
                check_baobab_strings(block.attrs[attr], logging_comment="attribute content of {}".format(block.name))
                result.append("{}={}".format(attr, block.attrs[attr]))
        if add_pos:
            same_name_pos = get_baobab_block_index(block, same_name=True)
            result.append("{}={}".format("pos", same_name_pos))

        return "@".join(result)

    return "/".join(["{}".format(parse_block_with_attrs(parent, ["page", "link_page"], add_pos=add_pos))
                     for parent in baobab.common.get_blocks_from_root_list(block)])


def get_baobab_block_index(block, same_name=False):
    if block.parent is None:
        return -1
    idx = 0
    child = block.parent.first_child
    while child is not None:
        if child.id == block.id:
            return idx
        if same_name:
            if child.name == block.name:
                idx += 1
        else:
            idx += 1
        child = child.next_sibling
    return idx


def append_news_events(known_actions, base_data, request, request_data):
    joiner = request.BaobabTree()
    if not joiner:
        return
    techevents_counter = dict.fromkeys(TECH_EVENTS_BASE_LIST, 0)
    techevents_counter.update(dict.fromkeys(TECH_EVENTS_CUSTOM_LIST, 0))
    heart_beats_counter = 0.
    for techevent in request.GetYandexTechEvents():
        tech = techevent.BaobabTech(joiner)
        if tech:
            techevent_type = str(tech.type)
            if techevent_type in techevents_counter:
                techevents_counter[techevent_type] += 1
            if techevent_type in TECH_EVENTS_WITH_DATA_LIST:
                if has_data(tech):
                    techevent_attrs_data = tech.data
                    techevent_data = dict(
                        base_data,
                        type="event",
                        event_type=techevent_type,
                        events_count=1,
                        events_data=techevent_attrs_data,
                        page=request_data["page"],
                        service=request_data["service"],
                    )
                    known_actions.append(techevent_data)
            if techevent_type == "heart-beat":
                if has_data(tech):
                    techevents_counter["heart-beat"] += 1
                    techevent_attrs_data = tech.data
                    if isinstance(techevent_attrs_data.get("time"), float):
                        heart_beats_counter += techevent_attrs_data.get("time")

    for event_type in techevents_counter:
        if event_type not in TECH_EVENTS_WITH_DATA_LIST and event_type not in TECH_EVENTS_CUSTOM_LIST:
            if techevents_counter[event_type] > 0:
                techevent_data = dict(
                    base_data,
                    type="event",
                    event_type=event_type,
                    events_count=techevents_counter[event_type],
                    page=request_data["page"],
                    service=request_data["service"],
                )
                known_actions.append(techevent_data)

    if techevents_counter["heart-beat"] > 0:
        heart_beats_counter = max(min(heart_beats_counter, MAX_HEART_BEAT_TIME), 0)
        techevent_data = dict(
            base_data,
            type="event",
            event_type="heart-beat",
            heart_beat_time=heart_beats_counter,
            page=request_data["page"],
            service=request_data["service"],
            from_mark=request_data.get("from_mark")
        )
        known_actions.append(techevent_data)


def parse_path(path, blockstat):
    parts = path.split(".")
    return "".join("/" + blockstat.get(p, p) for p in parts)


def get_host_props(block):
    host_name = None
    clicked_url = None
    agency_id = None
    if "url" in block.attrs:
        clicked_url = block.attrs.get("url")

    if block.name == "Link":
        parent = block.parent
        agency_id = block.attrs.get("agency_id")
        if parent.name in ("SourceInText", "StoryTailItem", "Snippet", "StoryOrigin"):
            parent_attrs = parent.attrs
            if "sourceName" in parent_attrs:
                host_name = parent_attrs.get("sourceName")
            else:
                logging.warning("No \"sourceName\" in Tail item in story")
            if not clicked_url and ("url" in parent_attrs):
                clicked_url = parent_attrs.get("url")

    if not isinstance(clicked_url, str):
        logging.error(
            "clicked_url is not String but {}\n".format(type(clicked_url))
            + "clicked_url = {}\n".format(str(clicked_url))
            + "block name is {}".format(block.name)
        )
        clicked_url = None

    res = {
        "clicked_url": clicked_url,
        "host_name": host_name,
        "agency_id": agency_id,
    }
    return res


def has_data(tech):
    try:
        return tech.data
    except Exception:
        logging.error("Tech.attr error")
