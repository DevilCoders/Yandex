from collections import defaultdict

from typing import Tuple
from typing import Generator
from typing import List
from typing import Union
from yaqtypes import JsonDict

import session_squeezer.squeezer_common as sq_common

type_v3_list_tuple = {
    "type_name": "optional",
    "item": {
        "type_name": "list",
        "item": {
            "type_name": "tuple",
            "elements": [
                {
                    "type": "int32"
                },
                {
                    "type": {
                        "type_name": "list",
                        "item": "int32"
                    }
                }
            ]
        }
    }
}

WEB_SURVEYS_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "answer_id", "type": "int32"},
    {"name": "answers", "type": "any"},
    {"name": "multi_answers", "type": "any", "type_v3": type_v3_list_tuple},
    {"name": "block_id", "type": "string"},
    {"name": "block_name", "type": "string"},
    {"name": "comment_text", "type": "string"},
    {"name": "event_id", "type": "string"},
    {"name": "query_text", "type": "string"},
    {"name": "question_id", "type": "int32"},
    {"name": "questions", "type": "any"},
    {"name": "reqid", "type": "string"},
    {"name": "result_block_id", "type": "string"},
    {"name": "selected_option", "type": "int32"},
    {"name": "survey_id", "type": "string"},
    {"name": "survey_type", "type": "string"},
    {"name": "type", "type": "string"},
    {"name": "url", "type": "string"},
    {"name": "wizard_name", "type": "string"},
    {"name": "prism_segment", "type": "int64"},
    {"name": "dwelltime", "type": "int64"},
    {"name": "wizard_position", "type": "int64"},
]


class ActionsSqueezerWebSurveys(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: add query_text field (MSTAND-2045)
    3: add answers order and survey type (MSTAND-2059)
    4: Fix the answers list formation (MSTAND-2059)
    5: add questions column (MSTAND-2075)
    6: add url column (MSTAND-2088)
    7: fix the answer list formation (MSTAND-2127)
    8: update for new answer format: answer block with id, label and idQuestion attrs (MSTAND-2128)
    9: add ui, browser and visibility columns (MSTAND-2152)
    10: save comment id into question_id column (MSTAND-2154)
    """
    VERSION = 11

    YT_SCHEMA = WEB_SURVEYS_YT_SCHEMA

    def __init__(self) -> None:
        super(ActionsSqueezerWebSurveys, self).__init__()

    def get_actions(self, args: sq_common.ActionSqueezerArguments) -> None:
        for request in args.container.GetRequests():
            if not self.check_request(request):
                continue

            exp_bucket = self.check_experiments(args, request)
            collect_survey_actions(args.result_actions, exp_bucket, request)

    def check_request(self, request: "libra.TRequest") -> bool:
        return any((
            request.IsA("TYandexWebRequest"),
            request.IsA("TTouchYandexWebRequest"),
            request.IsA("TPadYandexWebRequest"),
            request.IsA("TMobileAppYandexWebRequest"),
        ))


def collect_survey_actions(
        known_actions: List[JsonDict],
        exp_bucket: sq_common.ExpBucketInfo,
        request: "libra.TRequest",
) -> None:
    base_data = {
        "ts": request.Timestamp,
        "reqid": request.ReqId,
        "query_text": request.CorrectedQuery,
        sq_common.EXP_BUCKET_FIELD: exp_bucket,
    }

    import baobab
    for block, joiner, tree in get_survey_blocks(request):
        result_block = baobab.common.get_ancestor_result_block(block)
        result_block_id = result_block.id if result_block else None
        wizard_name = result_block.get_attr("wizard_name") if result_block else None
        url = result_block.get_attr("documentUrl") if result_block else None

        request_data = dict(
            base_data,
            survey_id=block.get_attr("surveyId"),
            type="request",
            wizard_name=wizard_name,
            result_block_id=result_block_id,
            answers=get_answers(block),
            multi_answers=get_multi_answers(block),
            survey_type=get_survey_type(block),
            questions=block.get_attr("questions"),
            url=url,
        )

        known_actions.append(request_data)
        collect_events_data(known_actions, request_data, block, joiner, tree)


def get_answers(block: "baobab.common.Block") -> List[int]:
    import baobab
    name_filter = baobab.common.NameFilter("answer")
    return [
        get_answer_id(answer)
        for answer in baobab.common.bfs_iterator_filtered(block, name_filter)
    ]


def get_multi_answers(block: "baobab.common.Block") -> List[Tuple[int, List[int]]]:
    import baobab
    name_filter = baobab.common.NameFilter("answer")
    answers = defaultdict(list)
    for answer in baobab.common.bfs_iterator_filtered(block, name_filter):
        if "idQuestion" in answer.attrs:
            answers[answer.get_attr("idQuestion")].append(get_answer_id(answer))
    return list(answers.items())


def get_survey_type(block: "baobab.common.Block") -> str:
    import baobab
    name_filter = baobab.common.NameFilter("answer")
    for answer in baobab.common.bfs_iterator_filtered(block, name_filter):
        if "text" in answer.attrs:
            return "text"
        if "emoji_id" in answer.attrs:
            return "emoji"
    return "unknown"


def get_survey_blocks(
        request: "libra.TRequest",
) -> Generator[Union["baobab.common.Block", "baobab.common.ShowAndClicksJoiner", "baobab.common.Tree"], None, None]:
    import baobab

    if request.IsA("TBaobabProperties"):
        joiners = request.BaobabAllTrees()
        for joiner in joiners:
            tree = joiner.get_show().tree
            name_filter = baobab.common.NameFilter("user-survey")
            for block in baobab.common.bfs_iterator_filtered(tree.root, name_filter):
                yield block, joiner, tree


def get_answer_id(answer_block: "baobab.common.Block") -> str:
    return answer_block.get_attr("answer_id") if "answer_id" in answer_block.attrs else answer_block.get_attr("id")


def collect_events_data(
        known_actions: List[JsonDict],
        request_data: JsonDict,
        block: "baobab.common.Block",
        joiner: "baobab.common.ShowAndClicksJoiner",
        tree: "baobab.common.Tree",
) -> None:
    import baobab

    events_data = []
    for event in joiner.get_events_by_block_subtree(block):
        event_data = dict(
            request_data,
            ts=event.client_timestamp,
            event_id=event.event_id,
            block_id=event.block_id,
        )
        if isinstance(event, baobab.common.Tech):
            event_data["type"] = "techevent"
            event_data["comment_text"] = event.data.get("commentText")
            event_data["selected_option"] = event.data.get("selectedOption")
        elif isinstance(event, baobab.common.Click):
            click_block = tree.get_block_by_id(event.block_id)
            event_data["type"] = "click"
            event_data["block_name"] = click_block.name
            event_data["answer_id"] = get_answer_id(click_block)
            event_data["question_id"] = click_block.get_attr("idQuestion")

        events_data.append(event_data)

    events_data.sort(key=lambda x: x["ts"])
    known_actions.extend(events_data)
