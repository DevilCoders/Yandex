import collections.abc
import typing
from dataclasses import dataclass
from datetime import datetime, timedelta

import dataforge.feedback_loop as feedback_loop
import dataforge.classification as classification
import dataforge.classification_loop as classification_loop
from dataforge.control import RuleBuilder
import toloka.client as toloka

from cloud.ai.speechkit.stt.lib.data.model.dao import (
    MarkupStep,
    MarkupPriorities,
    OverlapStrategy,
    SimpleStaticOverlapStrategy,
)

max_record_duration_seconds = 9.0

# our expectations about how long average toloker will perform assignment in relation to assignment audio duration sum
transcript_prior_ratio = 6.0
check_transcript_prior_ratio = 3.0
assignment_duration_ratio_thresholds = (0.1, 0.3, 4.0)  # (fraud, poor, max allowed) assignment durations

transcript_real_tasks_in_assignment = 15
transcript_honeypot_tasks_in_assignment = 3
transcript_min_correct_honeypot_solutions = 2
transcript_tasks_in_assignment = transcript_real_tasks_in_assignment + transcript_honeypot_tasks_in_assignment
transcript_fb_loop_tasks_in_assignment = 4

transcript_assignment_completion_time_ratios = [
    transcript_prior_ratio * t for t in assignment_duration_ratio_thresholds
]
transcript_assignment_fraud_duration_threshold_seconds, \
transcript_assignment_poor_duration_threshold_seconds, \
transcript_assignment_max_duration_seconds = \
    (int(transcript_tasks_in_assignment * max_record_duration_seconds * r) for r in
     transcript_assignment_completion_time_ratios)

transcript_fb_loop_assignment_fraud_duration_threshold_seconds, \
transcript_fb_loop_assignment_poor_duration_threshold_seconds, \
transcript_fb_loop_assignment_max_duration_seconds = \
    (int(transcript_fb_loop_tasks_in_assignment * max_record_duration_seconds * r) for r in
     transcript_assignment_completion_time_ratios)

check_transcript_real_tasks_in_assignment = 24
check_transcript_honeypots_is_assignment = 3
check_transcript_min_correct_honeypot_solutions = 2
check_transcript_tasks_in_assignment = check_transcript_real_tasks_in_assignment + check_transcript_honeypots_is_assignment
check_transcript_overlap = 3
check_transcript_overlap_strategy = SimpleStaticOverlapStrategy(min_majority=3, overall=3)

check_transcript_assignment_completion_time_ratios = [
    check_transcript_prior_ratio * t for t in assignment_duration_ratio_thresholds
]
check_transcript_assignment_fraud_duration_threshold_seconds, \
check_transcript_assignment_poor_duration_threshold_seconds, \
check_transcript_assignment_max_duration_seconds = \
    (int(check_transcript_tasks_in_assignment * max_record_duration_seconds * r) for r in
     check_transcript_assignment_completion_time_ratios)

quality_evaluation_real_tasks_in_assignment = 21
quality_evaluation_honeypots_is_assignment = 6
quality_evaluation_min_correct_honeypot_solutions = 5
quality_evaluation_tasks_in_assignment = quality_evaluation_real_tasks_in_assignment + quality_evaluation_honeypots_is_assignment
quality_evaluation_overlap = 5
quality_evaluation_overlap_strategy = SimpleStaticOverlapStrategy(min_majority=4, overall=5)

quality_evaluation_assignment_fraud_duration_threshold_seconds, \
quality_evaluation_assignment_poor_duration_threshold_seconds, \
quality_evaluation_assignment_max_duration_seconds = \
    (int(quality_evaluation_tasks_in_assignment * max_record_duration_seconds * r) for r in
     check_transcript_assignment_completion_time_ratios)

markup_step_to_overlap_strategy = {
    MarkupStep.CHECK_ASR_TRANSCRIPT: check_transcript_overlap_strategy,
    MarkupStep.QUALITY_EVALUATION: quality_evaluation_overlap_strategy,
}
markup_step_to_min_correct_honeypot_solutions = {
    MarkupStep.TRANSCRIPT: transcript_min_correct_honeypot_solutions,
    MarkupStep.CHECK_ASR_TRANSCRIPT: check_transcript_min_correct_honeypot_solutions,
    MarkupStep.QUALITY_EVALUATION: quality_evaluation_min_correct_honeypot_solutions,
}


def get_reward_per_assignment(check: bool, fb_loop: bool, lang: str) -> float:
    if check:
        base_reward = 0.03
    else:
        if fb_loop:
            base_reward = 0.03
        else:
            base_reward = 0.14
    multiplier = {
        'kk-KK': 2.,
    }.get(lang, 1.)
    return base_reward * multiplier


def get_feedback_loop_params(lang: str) -> feedback_loop.Params:
    return feedback_loop.Params(
        control_markup=feedback_loop.Control(
            rules=RuleBuilder().add_static_reward(threshold=.5).build()),
        control_check=feedback_loop.Control(
            rules=RuleBuilder().add_static_reward(threshold=.5).build()),
        evaluation=feedback_loop.Evaluation(
            aggregation_algorithm=classification.AggregationAlgorithm.MAJORITY_VOTE,
            overlap=classification_loop.StaticOverlap(overlap=2),
            assignment_check_sample=None),
        stop_criteria=feedback_loop.StopCriteria(
            object_markup_attempts=3,
            ok_object_ratio=.9),
        quality=feedback_loop.Quality(confidence_threshold=.5),
    )


def set_ru_users_filter(pool: toloka.Pool):
    pool.filter = (
        toloka.filter.Languages.in_('RU') &
        (
            toloka.filter.RegionByPhone.in_(225) |  # RU
            toloka.filter.RegionByPhone.in_(187) |  # UA
            toloka.filter.RegionByPhone.in_(149)  # BY
        ) &
        (toloka.filter.DateOfBirth <= 1041368400) &  # 2003-01-01
        (
            (toloka.filter.ClientType == toloka.filter.ClientType.ClientType.BROWSER) |
            (toloka.filter.ClientType == toloka.filter.ClientType.ClientType.TOLOKA_APP)
        )
    )


def set_kk_users_filter(pool: toloka.Pool):
    pool.filter = (
        toloka.filter.Languages.in_('KK') &
        (toloka.filter.DateOfBirth <= 1041368400) &  # 2003-01-01
        (
            (toloka.filter.ClientType == toloka.filter.ClientType.ClientType.BROWSER) |
            (toloka.filter.ClientType == toloka.filter.ClientType.ClientType.TOLOKA_APP)
        )
    )


def add_captcha(pool: toloka.Pool):
    pool.set_captcha_frequency(toloka.pool.QualityControl.CaptchaFrequency.LOW)
    pool.quality_control.add_action(
        collector=toloka.collectors.Captcha(history_size=10),
        conditions=[toloka.conditions.StoredResultsCount >= 5, toloka.conditions.SuccessRate <= 30.0],
        action=toloka.actions.RestrictionV2(
            private_comment='Капча',
            duration=1,
            duration_unit=toloka.user_restriction.DurationUnit.DAYS,
            scope=toloka.UserRestriction.PROJECT,
        )
    )


def add_skipped_assignments(pool: toloka.Pool):
    pool.quality_control.add_action(
        collector=toloka.collectors.SkippedInRowAssignments(),
        conditions=[toloka.conditions.SkippedInRowCount >= 5],
        action=toloka.actions.RestrictionV2(
            private_comment='Пропущенные подряд задания',
            duration=1,
            duration_unit=toloka.user_restriction.DurationUnit.DAYS,
            scope=toloka.UserRestriction.PROJECT,
        )
    )


def add_fast_submits(pool: toloka.Pool, fraud_threshold_seconds: int, poor_threshold_seconds: int):
    pool.quality_control.add_action(
        collector=toloka.collectors.AssignmentSubmitTime(
            fast_submit_threshold_seconds=fraud_threshold_seconds,
            history_size=10,
        ),
        conditions=[toloka.conditions.TotalSubmittedCount > 0, toloka.conditions.FastSubmittedCount >= 1],
        action=toloka.actions.RestrictionV2(
            private_comment='Очень быстрые ответы',
            duration=7,
            duration_unit=toloka.user_restriction.DurationUnit.DAYS,
            scope=toloka.UserRestriction.PROJECT,
        )
    )
    pool.quality_control.add_action(
        collector=toloka.collectors.AssignmentSubmitTime(
            fast_submit_threshold_seconds=poor_threshold_seconds,
            history_size=10,
        ),
        conditions=[toloka.conditions.TotalSubmittedCount >= 5, toloka.conditions.FastSubmittedCount > 3],
        action=toloka.actions.RestrictionV2(
            private_comment='Быстрые ответы',
            duration=5,
            duration_unit=toloka.user_restriction.DurationUnit.HOURS,
            scope=toloka.UserRestriction.PROJECT,
        )
    )


def add_increase_overlap_on_reject(pool: toloka.Pool):
    pool.quality_control.add_action(
        collector=toloka.collectors.AssignmentsAssessment(),
        conditions=[toloka.conditions.AssessmentEvent == toloka.conditions.AssessmentEvent.Type.REJECT],
        action=toloka.actions.ChangeOverlap(delta=1, open_pool=True),
    )


lang_to_vacation_multiplier = {'kk-KK': 2.5}


def add_vacation(pool: toloka.Pool, lang: str, check: bool = False):
    assert pool.mixer_config is not None
    total_tasks_count = pool.mixer_config.real_tasks_count + \
                        pool.mixer_config.golden_tasks_count + \
                        pool.mixer_config.training_tasks_count

    check_multiplier = 1.5 if check else 1.

    completed_assignments_threshold = int(round(
        150. / total_tasks_count * check_multiplier * lang_to_vacation_multiplier.get(lang, 1.)
    ))

    pool.quality_control.add_action(
        collector=toloka.collectors.AnswerCount(),
        conditions=[toloka.conditions.AssignmentsAcceptedCount >= completed_assignments_threshold],
        action=toloka.actions.RestrictionV2(
            private_comment='Выполнил достаточное количество заданий, отправили отдыхать.',
            duration=5,
            duration_unit=toloka.user_restriction.DurationUnit.HOURS,
            scope=toloka.UserRestriction.PROJECT,
        ),
    )


def get_pool_params(
    lang: str,
    markup_id: str,
    markup_step: MarkupStep,
    tags: typing.Iterable[str],
    priority: MarkupPriorities,
    override: dict,
    for_feedback_loop: bool = False,
) -> toloka.Pool:
    private_name = f'{markup_id} ({markup_step.value}) {", ".join(tags)}'
    if len(private_name) > 200:
        private_name = f'{private_name[:200]}…'

    if markup_step == MarkupStep.TRANSCRIPT:
        pool = get_transcript_pool_params(lang, for_feedback_loop)
    elif markup_step == MarkupStep.CHECK_ASR_TRANSCRIPT:
        pool = get_check_transcript_pool_params(lang, for_feedback_loop=False)
    elif markup_step == MarkupStep.CHECK_TRANSCRIPT:
        pool = get_check_transcript_pool_params(lang, for_feedback_loop=for_feedback_loop)
    elif markup_step == MarkupStep.QUALITY_EVALUATION:
        pool = get_check_transcript_pool_params(lang, quality_evaluation=True)
    else:
        raise ValueError(f'Unexpected markup step: {markup_step}')

    if lang == 'ru-RU':
        set_ru_users_filter(pool)
    elif lang == 'kk-KK':
        set_kk_users_filter(pool)
    else:
        raise ValueError(f'Unexpected language: {lang}')

    pool.private_name = private_name
    pool.priority = priority.get_toloka_pool_priority()

    if override:
        pool_params = pool.unstructure()
        dict_merge(pool_params, override)
        pool = toloka.Pool.structure(pool_params)

    return pool


lang_to_transcript_project_id = {'ru-RU': '36508', 'kk-KK': '42621'}
lang_to_transcript_training_pool_id = {'ru-RU': '29377413', 'kk-KK': None}


def get_month_from_now() -> datetime:
    return datetime.utcnow() + timedelta(days=30)


def get_transcript_pool_params(lang: str, for_feedback_loop: bool = False) -> toloka.Pool:
    pool = toloka.Pool(
        project_id=lang_to_transcript_project_id[lang],
        may_contain_adult_content=True,
        will_expire=get_month_from_now(),
        reward_per_assignment=get_reward_per_assignment(check=False, fb_loop=for_feedback_loop, lang=lang),
        assignment_max_duration_seconds=transcript_fb_loop_assignment_max_duration_seconds if for_feedback_loop else transcript_assignment_max_duration_seconds,
        auto_accept_solutions=False,
        auto_accept_period_day=7,
        assignments_issuing_config=toloka.Pool.AssignmentsIssuingConfig(
            issue_task_suites_in_creation_order=False,
        ),
        defaults=toloka.Pool.Defaults(
            default_overlap_for_new_tasks=1,
            default_overlap_for_new_task_suites=1,
        ),
    )

    pool.set_mixer_config(
        real_tasks_count=transcript_fb_loop_tasks_in_assignment if for_feedback_loop else transcript_real_tasks_in_assignment,
        golden_tasks_count=0 if for_feedback_loop else transcript_honeypot_tasks_in_assignment,
        training_tasks_count=0,
    )

    training_pool_id = lang_to_transcript_training_pool_id[lang]

    if training_pool_id is not None:
        pool.set_training_requirement(toloka.pool.QualityControl.TrainingRequirement(
            training_pool_id=training_pool_id,
            training_passing_skill_value=60,
        ))

    add_captcha(pool)
    add_skipped_assignments(pool)

    add_fast_submits(
        pool,
        fraud_threshold_seconds=transcript_fb_loop_assignment_fraud_duration_threshold_seconds if for_feedback_loop else transcript_assignment_fraud_duration_threshold_seconds,
        poor_threshold_seconds=transcript_fb_loop_assignment_poor_duration_threshold_seconds if for_feedback_loop else transcript_assignment_poor_duration_threshold_seconds,
    )

    add_vacation(pool, lang)

    if lang == 'kk-KK':
        # temporary, while we have problems with transcript honeypots and lack of tolokers
        return pool

    if for_feedback_loop:
        return pool

    condition_assignment_answers = toloka.conditions.TotalAnswersCount >= transcript_honeypot_tasks_in_assignment
    condition_one_correct_answer = [
        condition_assignment_answers,
        toloka.conditions.CorrectAnswersRate <= 60.0,
        toloka.conditions.CorrectAnswersRate > 30.0,
    ]
    condition_no_correct_answers = [
        condition_assignment_answers,
        toloka.conditions.CorrectAnswersRate <= 30.0,
    ]
    pool.quality_control.add_action(
        collector=toloka.collectors.GoldenSet(history_size=transcript_honeypot_tasks_in_assignment),
        conditions=condition_one_correct_answer,
        action=toloka.actions.RestrictionV2(
            private_comment='Контрольные задания (1 из 3 верно)',
            duration=1,
            duration_unit=toloka.user_restriction.DurationUnit.HOURS,
            scope=toloka.UserRestriction.PROJECT,
        ),
    )
    pool.quality_control.add_action(
        collector=toloka.collectors.GoldenSet(history_size=transcript_honeypot_tasks_in_assignment),
        conditions=condition_no_correct_answers,
        action=toloka.actions.RestrictionV2(
            private_comment='Контрольные задания (0 из 3 верно)',
            duration=8,
            duration_unit=toloka.user_restriction.DurationUnit.HOURS,
            scope=toloka.UserRestriction.PROJECT,
        ),
    )

    return pool


lang_to_check_transcript_project_id = {'ru-RU': '37615', 'kk-KK': '47136'}
lang_to_check_transcript_training_pool_id = {'ru-RU': '26996102', 'kk-KK': None}


def get_check_transcript_pool_params(lang: str, for_feedback_loop: bool = False,
                                     quality_evaluation: bool = False) -> toloka.Pool:
    assert not for_feedback_loop or not quality_evaluation

    if quality_evaluation:
        overlap = quality_evaluation_overlap
    else:
        if for_feedback_loop:
            overlap = get_feedback_loop_params(lang).evaluation.overlap.min_overlap()
        else:
            overlap = check_transcript_overlap

    pool = toloka.Pool(
        project_id=lang_to_check_transcript_project_id[lang],
        may_contain_adult_content=True,
        will_expire=get_month_from_now(),
        reward_per_assignment=get_reward_per_assignment(check=True, fb_loop=for_feedback_loop, lang=lang),
        assignment_max_duration_seconds=quality_evaluation_assignment_max_duration_seconds if quality_evaluation else check_transcript_assignment_max_duration_seconds,
        auto_accept_solutions=False,
        auto_accept_period_day=7,
        assignments_issuing_config=toloka.Pool.AssignmentsIssuingConfig(
            issue_task_suites_in_creation_order=False,
        ),
        defaults=toloka.Pool.Defaults(
            default_overlap_for_new_tasks=overlap,
            default_overlap_for_new_task_suites=overlap,
        ),
    )

    real_tasks_count = quality_evaluation_real_tasks_in_assignment if quality_evaluation else check_transcript_real_tasks_in_assignment
    golden_tasks_count = quality_evaluation_honeypots_is_assignment if quality_evaluation else check_transcript_honeypots_is_assignment
    pool.set_mixer_config(
        real_tasks_count=real_tasks_count,
        golden_tasks_count=golden_tasks_count,
        training_tasks_count=0,
    )

    training_pool_id = lang_to_check_transcript_training_pool_id[lang]

    if training_pool_id is not None:
        pool.set_training_requirement(toloka.pool.QualityControl.TrainingRequirement(
            training_pool_id=training_pool_id,
            training_passing_skill_value=85,
        ))

    add_captcha(pool)
    add_skipped_assignments(pool)

    add_fast_submits(
        pool,
        fraud_threshold_seconds=quality_evaluation_assignment_fraud_duration_threshold_seconds if quality_evaluation else check_transcript_assignment_fraud_duration_threshold_seconds,
        poor_threshold_seconds=quality_evaluation_assignment_poor_duration_threshold_seconds if quality_evaluation else check_transcript_assignment_poor_duration_threshold_seconds,
    )

    add_vacation(pool, lang, check=True)

    if quality_evaluation:
        condition_assignment_answers = toloka.conditions.TotalAnswersCount >= quality_evaluation_honeypots_is_assignment
        condition_three_or_four_correct_answers = [
            condition_assignment_answers,
            toloka.conditions.CorrectAnswersRate <= 80.0,
            toloka.conditions.CorrectAnswersRate > 40.0,
        ]
        condition_two_or_less_correct_answers = [
            condition_assignment_answers,
            toloka.conditions.CorrectAnswersRate <= 40.0,
        ]
        pool.quality_control.add_action(
            collector=toloka.collectors.GoldenSet(history_size=golden_tasks_count),
            conditions=condition_three_or_four_correct_answers,
            action=toloka.actions.RestrictionV2(
                private_comment='контрольные задания (4 из 6 или 3 из 6 верно)',
                duration=1,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )
        pool.quality_control.add_action(
            collector=toloka.collectors.GoldenSet(history_size=golden_tasks_count),
            conditions=condition_two_or_less_correct_answers,
            action=toloka.actions.RestrictionV2(
                private_comment='контрольные задания (2 из 6 или 1 из 6 или 0 из 6 верно)',
                duration=8,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )
    else:
        condition_assignment_answers = toloka.conditions.TotalAnswersCount >= check_transcript_honeypots_is_assignment

        condition_two_or_more_correct_answers = [
            condition_assignment_answers,
            toloka.conditions.CorrectAnswersRate > 60.0,
        ]
        condition_one_correct_answer = [
            condition_assignment_answers,
            toloka.conditions.CorrectAnswersRate <= 60.0,
            toloka.conditions.CorrectAnswersRate > 30.0,
        ]
        condition_one_or_less_correct_answers = [
            condition_assignment_answers,
            toloka.conditions.CorrectAnswersRate <= 60.0,
        ]
        condition_no_correct_answers = [
            condition_assignment_answers,
            toloka.conditions.CorrectAnswersRate <= 30.0,
        ]

        pool.quality_control.add_action(
            collector=toloka.collectors.GoldenSet(history_size=golden_tasks_count),
            conditions=condition_one_correct_answer,
            action=toloka.actions.RestrictionV2(
                private_comment='контрольные задания (1 из 3 верно)',
                duration=1,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )
        pool.quality_control.add_action(
            collector=toloka.collectors.GoldenSet(history_size=golden_tasks_count),
            conditions=condition_no_correct_answers,
            action=toloka.actions.RestrictionV2(
                private_comment='контрольные задания (0 из 3 верно)',
                duration=8,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )

        if for_feedback_loop:
            add_increase_overlap_on_reject(pool)
            pool.quality_control.add_action(
                collector=toloka.collectors.GoldenSet(history_size=golden_tasks_count),
                conditions=condition_two_or_more_correct_answers,
                action=toloka.actions.ApproveAllAssignments(),
            )
            pool.quality_control.add_action(
                collector=toloka.collectors.GoldenSet(history_size=golden_tasks_count),
                conditions=condition_one_or_less_correct_answers,
                action=toloka.actions.RejectAllAssignments(public_comment='мало правильных ответов'),
            )

    return pool


# DEPRECATED, for old projects
@dataclass
class PoolParams:
    base_pool_id: str
    real_tasks_per_assignment: int
    honeypot_tasks_per_assignment: int
    min_correct_honeypot_solutions: int
    reward_per_assignment: float
    overlap_strategy: typing.Optional[OverlapStrategy]


compare_meaning_ru_pool_params = PoolParams(
    base_pool_id='19379777',
    real_tasks_per_assignment=30,
    honeypot_tasks_per_assignment=5,
    min_correct_honeypot_solutions=4,
    reward_per_assignment=0.02,
    overlap_strategy=SimpleStaticOverlapStrategy(min_majority=6, overall=10),
)


# DEPRECATED, for old projects
def produce_pool_params(
    markup_id: str,
    markup_step: MarkupStep,
    tags: typing.Iterable[str],
    pool_params: PoolParams,
    priority: MarkupPriorities,
) -> dict:
    private_name = f'{markup_id} ({markup_step.value}) {", ".join(tags)}'
    if len(private_name) > 200:
        private_name = f'{private_name[:200]}…'
    toloka_params = {
        'mixer_config': {
            'real_tasks_count': pool_params.real_tasks_per_assignment,
            'golden_tasks_count': pool_params.honeypot_tasks_per_assignment,
            'training_tasks_count': 0,
            'force_last_assignment': True,
        },
        'private_name': private_name,
        'reward_per_assignment': pool_params.reward_per_assignment,
        'priority': priority.get_toloka_pool_priority(),
    }
    if pool_params.overlap_strategy is not None:
        toloka_params['defaults'] = {
            'default_overlap_for_new_task_suites': pool_params.overlap_strategy.overall,
        }
    return toloka_params


def dict_merge(dct, merge_dct):
    for k, v in merge_dct.items():
        if k in dct and isinstance(dct[k], dict) and isinstance(merge_dct[k], collections.abc.Mapping):
            dict_merge(dct[k], merge_dct[k])
        else:
            dct[k] = merge_dct[k]
