import yaqutils.misc_helpers as umisc
import session_squeezer.squeezer_common as sq_common

type_v3_list_int64 = {"type_name": "optional", "item": {"type_name": "list", "item": "int64"}}
type_v3_list_string = {"type_name": "optional", "item": {"type_name": "list", "item": {"type_name": "optional", "item": "string"}}}

SURVEYS_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "ad_group_type", "type": "string"},
    {"name": "age", "type": "string"},
    {"name": "answer_option_ids", "type": "any", "type_v3": type_v3_list_int64},
    {"name": "answer_option_labels", "type": "any", "type_v3": type_v3_list_string},
    {"name": "answer_text", "type": "string"},
    {"name": "answer_timestamp", "type": "uint64"},
    {"name": "gender", "type": "string"},
    {"name": "has_no_answer", "type": "boolean"},
    {"name": "has_no_opinion_answer", "type": "boolean"},
    {"name": "has_other_answer", "type": "boolean"},
    {"name": "has_other_answer_with_text", "type": "boolean"},
    {"name": "has_rotation", "type": "boolean"},
    {"name": "income_5", "type": "string"},
    {"name": "interview_id", "type": "string"},
    {"name": "is_multiple_choice", "type": "boolean"},
    {"name": "is_required", "type": "boolean"},
    {"name": "max_value", "type": "int64"},
    {"name": "min_value", "type": "int64"},
    {"name": "prob_age", "type": "double"},
    {"name": "prob_gender", "type": "double"},
    {"name": "prob_income_5", "type": "double"},
    {"name": "question_id", "type": "int32"},
    {"name": "question_label", "type": "string"},
    {"name": "question_option_ids", "type": "any", "type_v3": type_v3_list_int64},
    {"name": "question_option_labels", "type": "any", "type_v3": type_v3_list_string},
    {"name": "question_timestamp", "type": "uint64"},
    {"name": "question_type", "type": "string"},
    {"name": "referer", "type": "string"},
    {"name": "revision_id", "type": "uint32"},
    {"name": "slug", "type": "string"},
    {"name": "supplier_id", "type": "string"},
    {"name": "survey_id", "type": "uint32"},
    {"name": "user_agent", "type": "string"},
    {"name": "video", "type": "boolean"},
    {"name": "yandex_loyalty", "type": "double"},
    {"name": "yandexuid", "type": "uint64"},
]

COLUMN_NAMES = {item["name"] for item in SURVEYS_YT_SCHEMA}


class ActionsSqueezerSurveys(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: add type_v3 schema support (MSTAND-2017)
    3: change type for the question_id column (MSTAND-2024)
    4: add source for video adv experiments (MSTAND-2023)
    5: migrate to the new source table (MSTAND-2137)
    6: use enrich source table (MSTAND-2185)
    """
    VERSION = 6

    YT_SCHEMA = SURVEYS_YT_SCHEMA
    USE_LIBRA = False

    def __init__(self):
        super(ActionsSqueezerSurveys, self).__init__()

    def get_actions(self, args: sq_common.ActionSqueezerArguments):
        assert not args.result_actions

        for row in args.container:
            """:type row: dict[str]"""
            squeezed = {key: value for key, value in row.items() if key in COLUMN_NAMES}
            squeezed["question_id"] = umisc.optional_int(squeezed["question_id"])

            exp_bucket = self.check_experiments_fake(args)
            squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket
            args.result_actions.append(squeezed)
