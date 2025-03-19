import json
import typing
from dataclasses import dataclass
from datetime import timedelta

import toloka.client as toloka

import dataforge.base as base
import dataforge.control as control
import dataforge.mapping as mapping
import dataforge.feedback_loop as feedback_loop
import dataforge.pool as pool_config

import cloud.ai.speechkit.stt.lib.data.model.dao as dao
from cloud.ai.lib.python.serialization import YsonSerializable


# Differs from usual Audio, has user_url field, which contains temporary public link to Audio for Toloka tasks.
# TODO: it would be better to have two separate lists of Audio instead ot this class
@dataclass
class TolokaAudio(YsonSerializable, base.Object):
    url: str
    user_url: str


@dataclass
class Text(YsonSerializable, base.Object):
    text: str


class AudioClass(base.Class):
    SPEECH = 'sp'
    SILENCE = 'si'
    UNCLEAR = 'mis'


audio_mapping = mapping.ObjectMapping(
    obj_cls=TolokaAudio,
    obj_task_fields=[('url', 'url'), ('user_url', 'user_url')])

text_mapping = mapping.ObjectMapping(
    obj_cls=Text,
    obj_task_fields=[('text', 'text')])

audio_class_mapping = mapping.ObjectMapping(
    obj_cls=AudioClass,
    obj_task_fields=[('value', 'cls')])

binary_evaluation_mapping = mapping.ObjectMapping(
    obj_cls=base.BinaryEvaluation,
    obj_task_fields=[('ok', 'ok')])

input_objects_mapping = [audio_mapping]
markup_objects_mapping = [text_mapping, audio_class_mapping]
check_objects_mapping = [binary_evaluation_mapping, audio_class_mapping]

markup_task_mapping, check_task_mapping = mapping.generate_feedback_loop_mapping(
    input_objects_mapping, markup_objects_mapping, check_objects_mapping,
)


def task_to_input_objects(task: dict, users_urls_map: typing.Dict[str, str]) -> mapping.Objects:
    return [TolokaAudio(
        url=task['input_values']['url'],
        user_url=users_urls_map[task['input_values']['url']],
    )]


def task_to_control_objects(task: dict, users_urls_map: typing.Dict[str, str]) -> mapping.TaskSingleSolution:
    assert len(task['known_solutions']) == 1
    return list(task_to_input_objects(task, users_urls_map)) + [
        Text(text=task['input_values']['text']),
        AudioClass(cls=task['input_values']['cls']),
    ], [
        base.BinaryEvaluation(ok=task['known_solutions'][0]['output_values']['ok']),
        AudioClass(cls=task['known_solutions'][0]['output_values']['cls']),
    ]


def construct_feedback_loop(
    inputs: typing.Any,
    params: typing.Any,
    users_urls_map: typing.Dict[str, str],
    lang: str,
) -> feedback_loop.FeedbackLoop:
    with open(inputs.get('params.json')) as f:
        fb_loop_params = feedback_loop.Params.from_yson(json.load(f))

    # It would be better to pass objects instead of tasks, but we use tasks for compatibility with
    # old transcript pipelines
    with open(inputs.get('tasks.json')) as f:
        pool_input_objects = [task_to_input_objects(t, users_urls_map) for t in json.load(f)]

    # Yang does not supported, exception will be raised
    toloka_environment = toloka.TolokaClient.Environment[params.get('toloka-environment')]
    toloka_token: str = params.get('toloka-token')

    toloka_client = toloka.TolokaClient(
        token=toloka_token,
        environment=toloka_environment,
        retries=100,
        timeout=(60.0, 240.0),
    )

    return feedback_loop.FeedbackLoop(
        pool_input_objects=pool_input_objects,
        input_objects_mapping=input_objects_mapping,
        markup_objects_mapping=markup_objects_mapping,
        check_objects_mapping=check_objects_mapping,
        params=fb_loop_params,
        client=toloka_client,
        lang=lang)


@dataclass
class RecordBitJoins(YsonSerializable):
    record_bit: dao.RecordBit
    joins: typing.List[dao.RecordJoin]  # sorted by quality, first is best


lang_to_check_transcript_project_id = {'ru-RU': '37615', 'kk-KK': '47136'}
lang_to_transcript_project_id = {'ru-RU': '36508', 'kk-KK': '42621'}
lang_to_transcript_training_pool_id = {'ru-RU': '29377413', 'kk-KK': None}
lang_to_check_transcript_training_pool_id = {'ru-RU': '26996102', 'kk-KK': None}

transcript_fb_loop_tasks_in_assignment = 4
max_record_duration = timedelta(seconds=9)

transcript_prior_ratio = 6
check_transcript_prior_ratio = 3

fb_loop_check_overlap = 2
check_transcript_real_tasks_in_assignment = 24
check_transcript_honeypots_in_assignment = 3
check_transcript_min_correct_honeypot_solutions = 2


def get_reward_per_assignment(check: bool, lang: str) -> float:
    if check:
        base_reward = 0.03
    else:
        base_reward = 0.03

    multiplier = {
        'kk-KK': 2.,
    }.get(lang, 1.)
    return base_reward * multiplier


lang_to_vacation_multiplier = {'kk-KK': 2.5}


def get_markup_pool_config(private_name: str, lang: str, priority: int = 20) -> pool_config.MarkupConfig:
    training_pool_id = lang_to_transcript_training_pool_id[lang]
    training_requirement = None
    if training_pool_id is not None:
        training_requirement = toloka.pool.QualityControl.TrainingRequirement(
            training_pool_id=training_pool_id,
            training_passing_skill_value=60,
        )
    task_duration_hint = max_record_duration * transcript_prior_ratio
    ratio_rand, ratio_poor = .1, .3
    return pool_config.MarkupConfig(
        project_id=lang_to_transcript_project_id[lang],
        private_name=private_name,
        lang=lang[-2:],
        reward_per_assignment=get_reward_per_assignment(check=False, lang=lang),
        auto_accept_period_day=7,
        real_tasks_count=transcript_fb_loop_tasks_in_assignment,
        training_requirement=training_requirement,
        work_duration_before_vacation=timedelta(hours=2) * lang_to_vacation_multiplier.get(lang, 1.),
        task_duration_hint=task_duration_hint,
        priority=priority,
        control_params=control.Control(control.RuleBuilder().add_static_reward(0.5).add_speed_control(
            task_duration_hint, ratio_rand, ratio_poor).build())
    )


def get_check_pool_config(private_name: str, lang: str, priority: int = 20) -> pool_config.ClassificationConfig:
    training_pool_id = lang_to_check_transcript_training_pool_id[lang]
    training_requirement = None
    if training_pool_id is not None:
        training_requirement = toloka.pool.QualityControl.TrainingRequirement(
            training_pool_id=training_pool_id,
            training_passing_skill_value=85,
        )
    task_duration_hint = max_record_duration * check_transcript_prior_ratio
    ratio_rand, ratio_poor = .1, .3
    return pool_config.ClassificationConfig(
        project_id=lang_to_check_transcript_project_id[lang],
        private_name=private_name,
        lang=lang[-2:],
        reward_per_assignment=get_reward_per_assignment(check=True, lang=lang),
        auto_accept_period_day=7,
        real_tasks_count=check_transcript_real_tasks_in_assignment,
        training_requirement=training_requirement,
        work_duration_before_vacation=timedelta(hours=2) * lang_to_vacation_multiplier.get(lang, 1.),
        task_duration_hint=task_duration_hint,
        overlap=fb_loop_check_overlap,
        control_tasks_count=check_transcript_honeypots_in_assignment,
        priority=priority,
        control_params=control.Control(control.RuleBuilder().add_static_reward(0.5).add_speed_control(
            task_duration_hint, ratio_rand, ratio_poor).add_control_task_control(
            check_transcript_honeypots_in_assignment, 1, check_transcript_min_correct_honeypot_solutions).build()),
    )


def get_pool_config(
    lang: str,
    markup_id: str,
    markup_step: dao.MarkupStep,
    tags: typing.Iterable[str],
    priority: dao.MarkupPriorities,
) -> pool_config.BaseConfig:
    private_name = f'{markup_id} ({markup_step.value}) {", ".join(tags)}'
    if len(private_name) > 200:
        private_name = f'{private_name[:200]}…'

    if markup_step == dao.MarkupStep.TRANSCRIPT:
        pool = get_markup_pool_config(
            lang=lang, private_name=private_name, priority=priority.get_toloka_pool_priority()
        )
    elif markup_step == dao.MarkupStep.CHECK_TRANSCRIPT:
        pool = get_check_pool_config(
            lang=lang, private_name=private_name, priority=priority.get_toloka_pool_priority()
        )
    else:
        raise ValueError(f'Unexpected markup step: {markup_step}')
    return pool
