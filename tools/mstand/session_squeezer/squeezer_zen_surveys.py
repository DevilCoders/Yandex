import itertools
import session_squeezer.squeezer_common as sq_common

type_v3_op_list_int64 = {"type_name": "optional", "item": {"type_name": "list", "item": "int64"}}
type_v3_op_list_op_int64 = {"type_name": "optional", "item": {"type_name": "list", "item": {"type_name": "optional", "item": "int64"}}}
type_v3_op_list_string = {"type_name": "optional", "item": {"type_name": "list", "item": {"type_name": "optional", "item": "string"}}}

ZEN_SURVEYS_YT_SCHEMA = [
    {"name": "yuid", "type": "string"},
    {"name": "ts", "type": "int64"},
    {"name": "action_index", "type": "int64"},
    {"name": "bucket", "type": "int64"},
    {"name": "is_match", "type": "boolean"},
    {"name": "testid", "type": "string"},
    {"name": "servicetype", "type": "string"},

    {"name": "age", "type": "string"},
    {"name": "answer_option_ids", "type": "any", "type_v3": type_v3_op_list_int64},
    {"name": "answer_option_labels", "type": "any", "type_v3": type_v3_op_list_string},
    {"name": "answer_timestamp", "type": "uint64"},
    {"name": "answer_value", "type": "int32"},
    {"name": "gender", "type": "string"},
    {"name": "income_5", "type": "string"},
    {"name": "interview_id", "type": "string"},
    {"name": "is_multiple_choice", "type": "boolean"},
    {"name": "prob_age", "type": "double"},
    {"name": "prob_gender", "type": "double"},
    {"name": "prob_income_5", "type": "double"},
    {"name": "question_id", "type": "int64"},
    {"name": "question_label", "type": "string"},
    {"name": "question_option_ids", "type": "any", "type_v3": type_v3_op_list_int64},
    {"name": "question_option_labels", "type": "any", "type_v3": type_v3_op_list_string},
    {"name": "question_option_values", "type": "any", "type_v3": type_v3_op_list_op_int64},
    {"name": "question_timestamp", "type": "uint64"},
    {"name": "question_type", "type": "string"},
    {"name": "strongest_id", "type": "string"},
    {"name": "strongest_id_prefix", "type": "string"},
    {"name": "user_temperature", "type": "string"},
    {"name": "yandex_loyalty", "type": "double"},
    {"name": "history_user_temperature", "type": "string"},
    {"name": "yandexuid", "type": "uint64"},
    {"name": "integration", "type": "string"},
    {"name": "partner", "type": "string"},
    {"name": "product", "type": "string"},
]

COLUMN_NAMES = {item["name"] for item in ZEN_SURVEYS_YT_SCHEMA}


class ActionsSqueezerZenSurveys(sq_common.ActionsSqueezer):
    """
    1: initial version
    2: change the way of splitting of data by days (MSTAND-2113)
    3: migrate to the new source table and add a few fields (MSTAND-2137)
    """
    VERSION = 3

    YT_SCHEMA = ZEN_SURVEYS_YT_SCHEMA
    USE_LIBRA = False

    def __init__(self):
        super(ActionsSqueezerZenSurveys, self).__init__()

    def get_actions(self, args: sq_common.ActionSqueezerArguments):
        assert not args.result_actions

        extra_data = dict(
            history_user_temperature=None,
            integration=None,
            partner=None,
            product=None,
            user_temperature=None,
        )

        first_group_type = None
        for group_type, group in itertools.groupby(args.container, key=lambda x: x["table_index"]):
            if first_group_type is None:
                first_group_type = group_type
            if group_type == first_group_type:
                for row in group:
                    for name, value in extra_data.items():
                        if not value:
                            extra_data[name] = row[name]

            elif group_type == first_group_type + 1:
                for row in group:
                    """:type row: dict[str]"""
                    squeezed = {key: value for key, value in row.items() if key in COLUMN_NAMES}
                    squeezed.update(extra_data)
                    exp_bucket = self.check_experiments_fake(args)
                    squeezed[sq_common.EXP_BUCKET_FIELD] = exp_bucket
                    args.result_actions.append(squeezed)

            else:
                raise Exception("There was a problem with the source group types!")
