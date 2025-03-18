import session_squeezer.squeezer_common as sq_common

OBJECT_ANSWER_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "servicetype", "type": "string"},
    {"name": "testid", "type": "string"},
    {"name": "is_match", "type": "boolean"},
    {"name": "reqid", "type": "string"},
    {"name": "query", "type": "string"},
    {"name": "ui", "type": "string"},
    {"name": "service_dom_region", "type": "string"},
    {"name": "puid", "type": "string"},
    {"name": "browser", "type": "string"},
    {"name": "is_ento", "type": "boolean"},
    {"name": "subscription", "type": "string"},
    {"name": "real_onto_id", "type": "string"},
    {"name": "real_lst_onto_id", "type": "string"},
    {"name": "shown_onto_ids", "type": "any"},
    {"name": "is_film_list", "type": "boolean"},
    {"name": "clicks", "type": "any"},
    {"name": "real_shown_onto_ids", "type": "any"},
    {"name": "object_type", "type": "string"},
    {"name": "object_sub_type", "type": "string"},
    {"name": "log", "type": "string"},
    {"name": "view_time", "type": "uint64"},
    {"name": "view_time_non_muted", "type": "uint64"},
    {"name": "video_wizard_info", "type": "any"},
    {"name": "organic_web_info", "type": "any"},
]

HEARTBEAT_PATH = "player-events.heartbeat"
VIEW_TIME_PER_HEARTBEAT = 30

ONTO_LIST_BAOBAB_PATHS = [
    "$page.$top.$result.carousel.showcase.item.thumb",
    "$main.$top.$result.carousel.showcase.item.thumb",
]

ONTO_LIST_PATHS = [
    "/parallel/result/wiz/entity_search/upper/showcase/item/thumb",
    "/snippet/entity_search/upper/showcase/item/thumb"
]


def get_real_shown_onto_ids(row):
    result = []
    for elem in row["CarouselItemShowEvents"]:
        result.append(elem["Id"])

    return result


def calc_view_time(row, only_non_muted=True):
    heartbeat_count = 0
    for player_event_info in row["PlayerEvents"]:
        path = player_event_info["Path"]

        if path == HEARTBEAT_PATH:
            heartbeat_count += not only_non_muted or not player_event_info["Mute"]

    return heartbeat_count * VIEW_TIME_PER_HEARTBEAT


def get_clicks(row):
    results = []
    for click in row["Clicks"]:
        click_info = {}
        click_info["is_dynamic"] = click["IsDynamic"]
        click_info["dwell_time"] = click["DwellTime"]

        if click.get("Timestamp"):
            click_info["ts"] = click["Timestamp"]
        else:
            click_info["ts"] = click["ClientTimestamp"] // 1000

        if click.get("Baobab"):
            object_id = click.get("Attributes", {}).get("externalId", {}).get("id")
            path = click["Baobab"]
            click_info["path"] = path
            click_info["IsBaobab"] = True

            if object_id:
                click_info["id"] = object_id

            if click["Baobab"] in ONTO_LIST_BAOBAB_PATHS:
                click_info["type"] = "onto_click"
            else:
                click_info["type"] = "other"
        elif click.get("ConvertedPath"):
            object_id = click.get("Vars", {}).get("Id")
            path = click["ConvertedPath"]
            click_info["path"] = path

            if object_id:
                click_info["id"] = object_id

            if path in ONTO_LIST_PATHS:
                click["type"] = "onto_click"
            else:
                click_info["type"] = "other"
        else:
            click_info["type"] = "unknown"

        results.append(click_info)
    return results


def get_video_wizard_info(row):
    for result in row["Results"]:
        if result["IsWizard"] and result["Name"] == "video":
            return {"dwell_time": result["DwellTime"], "pos": result["Position"]}


def get_organic_web_info(row):
    web_info = []
    for result in row["Results"]:
        if not result["IsWizard"]:
            web_info.append({"dwell_time": result["DwellTime"], "pos": result["Position"]})
    return web_info


class ActionsSqueezerObjectAnswer(sq_common.ActionsSqueezer):
    """
    1: initial version
    """
    VERSION = 1

    YT_SCHEMA = OBJECT_ANSWER_YT_SCHEMA
    USE_LIBRA = False

    def __init__(self) -> None:
        super(ActionsSqueezerObjectAnswer, self).__init__()

    def get_actions(self, args: sq_common.ActionSqueezerArguments) -> None:
        """
        :type args: sq_common.ActionSqueezerArguments
        """

        assert not args.result_actions

        for row in args.container:
            """:type row: dict[str]"""

            squeezed = {
                "yuid" : row["UID"],
                "ts": row["Timestamp"],
                "reqid": row["ReqId"],
                "query": row["Query"],
                "ui": row["UI"],
                "service_dom_region": row["ServiceDomRegion"],
                "puid": row["PassportUID"],
                "browser": row["BrowserName"],
                "is_ento": row["Ento"] is not None,
                "subscription": row["UserSubscription"],
                "real_onto_id": row["RealOntoID"],
                "real_lst_onto_id": row["RealLstOntoID"],
                "shown_onto_ids": row["RealShowListOntoIDs"],
                "is_film_list": row["IsFilmList"] and row["IsFilmList"] > 5,
            }

            squeezed["real_shown_onto_ids"] = get_real_shown_onto_ids(row)
            squeezed["view_time_non_muted"] = calc_view_time(row)
            squeezed["view_time"] = calc_view_time(row, False)

            entity_search = row.get("EntitySearch")
            if entity_search:
                squeezed["object_type"] = entity_search["OType"]
                squeezed["object_sub_type"] = entity_search["OSubType"]
                squeezed["log"] = entity_search["Log"]

            squeezed["clicks"] = get_clicks(row)
            squeezed["video_wizard_info"] = get_video_wizard_info(row)
            squeezed["organic_web_info"] = get_organic_web_info(row)
            testids = row["TestID"]

            exp_bucket = self.check_experiments_by_testids(args, testids)
            squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket
            args.result_actions.append(squeezed)
