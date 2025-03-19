import datetime
import unittest
from mock import patch, Mock

import dataforge.pool as pool_config
import toloka.client as toloka

from cloud.ai.speechkit.stt.lib.data.model.dao import MarkupStep, MarkupPriorities
import cloud.ai.speechkit.stt.lib.data_pipeline.markup_params as params
import cloud.ai.speechkit.stt.lib.data_pipeline.transcript_feedback_loop as fb_loop_params


class TestMarkupParams(unittest.TestCase):
    def test_values(self):
        self.assertEqual(18, params.transcript_tasks_in_assignment)
        self.assertEqual(97, params.transcript_assignment_fraud_duration_threshold_seconds)
        self.assertEqual(291, params.transcript_assignment_poor_duration_threshold_seconds)
        self.assertEqual(3888, params.transcript_assignment_max_duration_seconds)

        self.assertEqual(4, params.transcript_fb_loop_tasks_in_assignment)
        self.assertEqual(21, params.transcript_fb_loop_assignment_fraud_duration_threshold_seconds)
        self.assertEqual(64, params.transcript_fb_loop_assignment_poor_duration_threshold_seconds)
        self.assertEqual(864, params.transcript_fb_loop_assignment_max_duration_seconds)

        self.assertEqual(27, params.check_transcript_tasks_in_assignment)
        self.assertEqual(72, params.check_transcript_assignment_fraud_duration_threshold_seconds)
        self.assertEqual(218, params.check_transcript_assignment_poor_duration_threshold_seconds)
        self.assertEqual(2916, params.check_transcript_assignment_max_duration_seconds)

    @patch('cloud.ai.speechkit.stt.lib.data_pipeline.markup_params.markup_params.get_month_from_now')
    def test_transcript_ru_pool_params(self, get_month_from_now):
        get_month_from_now.return_value = datetime.datetime(year=1999, month=12, day=31)
        expected_pool = toloka.Pool(
            project_id='36508',
            private_name='11eeff (transcript) FOO:bar, CLIENT:kek',
            may_contain_adult_content=True,
            will_expire=datetime.datetime(year=1999, month=12, day=31),
            reward_per_assignment=0.14,
            assignment_max_duration_seconds=3888,
            auto_accept_solutions=False,
            auto_accept_period_day=7,
            assignments_issuing_config=toloka.Pool.AssignmentsIssuingConfig(
                issue_task_suites_in_creation_order=False,
            ),
            priority=20,
            defaults=toloka.Pool.Defaults(
                default_overlap_for_new_tasks=1,
                default_overlap_for_new_task_suites=1,
            ),
        )
        expected_pool.set_mixer_config(
            real_tasks_count=15,
            golden_tasks_count=3,
            training_tasks_count=0,
        )
        expected_pool.set_training_requirement(toloka.pool.QualityControl.TrainingRequirement(
            training_pool_id='29377413',
            training_passing_skill_value=60,
        ))
        expected_pool.set_captcha_frequency(toloka.pool.QualityControl.CaptchaFrequency.LOW)
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.Captcha(history_size=10),
            conditions=[toloka.conditions.StoredResultsCount >= 5, toloka.conditions.SuccessRate <= 30.0],
            action=toloka.actions.RestrictionV2(
                private_comment='Капча',
                duration=1,
                duration_unit=toloka.user_restriction.DurationUnit.DAYS,
                scope=toloka.UserRestriction.PROJECT,
            )
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.SkippedInRowAssignments(),
            conditions=[toloka.conditions.SkippedInRowCount >= 5],
            action=toloka.actions.RestrictionV2(
                private_comment='Пропущенные подряд задания',
                duration=1,
                duration_unit=toloka.user_restriction.DurationUnit.DAYS,
                scope=toloka.UserRestriction.PROJECT,
            )
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.AssignmentSubmitTime(
                fast_submit_threshold_seconds=97,
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
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.AssignmentSubmitTime(
                fast_submit_threshold_seconds=291,
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
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.AnswerCount(),
            conditions=[toloka.conditions.AssignmentsAcceptedCount >= 8],
            action=toloka.actions.RestrictionV2(
                private_comment='Выполнил достаточное количество заданий, отправили отдыхать.',
                duration=5,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.GoldenSet(history_size=3),
            conditions=[
                toloka.conditions.TotalAnswersCount >= 3,
                toloka.conditions.CorrectAnswersRate <= 60.0,
                toloka.conditions.CorrectAnswersRate > 30.0,
            ],
            action=toloka.actions.RestrictionV2(
                private_comment='Контрольные задания (1 из 3 верно)',
                duration=1,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.GoldenSet(history_size=3),
            conditions=[
                toloka.conditions.TotalAnswersCount >= 3,
                toloka.conditions.CorrectAnswersRate <= 30.0,
            ],
            action=toloka.actions.RestrictionV2(
                private_comment='Контрольные задания (0 из 3 верно)',
                duration=8,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )
        expected_pool.filter = (
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

        actual_pool = params.get_pool_params(
            lang='ru-RU',
            markup_id='11eeff',
            markup_step=MarkupStep.TRANSCRIPT,
            tags=['FOO:bar', 'CLIENT:kek'],
            priority=MarkupPriorities.INTERNAL,
            override={},
        )

        self.assertEqual(expected_pool, actual_pool)

    @patch.object(fb_loop_params.pool_config, 'datetime', Mock(wraps=datetime))
    def test_transcript_feedback_loop_ru_pool_params(self):
        fb_loop_params.pool_config.datetime.datetime.today.return_value = datetime.datetime(year=2000, month=11, day=30)
        fb_loop_params.pool_config.datetime.datetime.utcnow.return_value = \
            datetime.datetime(year=2000, month=11, day=30)

        expected_pool = toloka.Pool(
            project_id='36508',
            private_name='11eeff (transcript) FOO:bar, CLIENT:kek',
            may_contain_adult_content=True,
            will_expire=datetime.datetime(year=2000, month=12, day=30),
            reward_per_assignment=0.03,
            assignment_max_duration_seconds=1080,
            auto_accept_solutions=False,
            auto_accept_period_day=7,
            assignments_issuing_config=toloka.Pool.AssignmentsIssuingConfig(
                issue_task_suites_in_creation_order=False,
            ),
            priority=20,
            defaults=toloka.Pool.Defaults(
                default_overlap_for_new_tasks=1,
                default_overlap_for_new_task_suites=1,
            ),
        )
        expected_pool.set_mixer_config(
            real_tasks_count=4,
            golden_tasks_count=0,
            training_tasks_count=0,
        )
        expected_pool.set_training_requirement(toloka.pool.QualityControl.TrainingRequirement(
            training_pool_id='29377413',
            training_passing_skill_value=60,
        ))
        expected_pool.set_captcha_frequency(toloka.pool.QualityControl.CaptchaFrequency.LOW)
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.Captcha(history_size=10),
            conditions=[toloka.conditions.StoredResultsCount >= 5, toloka.conditions.SuccessRate <= 30.0],
            action=toloka.actions.RestrictionV2(
                private_comment='Captcha',
                duration=8,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            )
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.SkippedInRowAssignments(),
            conditions=[toloka.conditions.SkippedInRowCount >= 5],
            action=toloka.actions.RestrictionV2(
                private_comment='Skipped assignments',
                duration=8,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            )
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.AssignmentSubmitTime(
                fast_submit_threshold_seconds=21,
                history_size=10,
            ),
            conditions=[toloka.conditions.TotalSubmittedCount >= 1, toloka.conditions.FastSubmittedCount >= 1],
            action=toloka.actions.RestrictionV2(
                private_comment='Fast submits, <= 0.1',
                duration=1 * 24 * 60,
                duration_unit=toloka.user_restriction.DurationUnit.MINUTES,
                scope=toloka.UserRestriction.PROJECT,
            )
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.AssignmentSubmitTime(
                fast_submit_threshold_seconds=64,
                history_size=10,
            ),
            conditions=[toloka.conditions.TotalSubmittedCount >= 1, toloka.conditions.FastSubmittedCount >= 1],
            action=toloka.actions.RestrictionV2(
                private_comment='Fast submits, 0.1 < time <= 0.3',
                duration=8 * 60,
                duration_unit=toloka.user_restriction.DurationUnit.MINUTES,
                scope=toloka.UserRestriction.PROJECT,
            )
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.AnswerCount(),
            conditions=[toloka.conditions.AssignmentsAcceptedCount >= 33],
            action=toloka.actions.RestrictionV2(
                private_comment='Completed many assignments, vacation',
                duration=5,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )

        expected_pool.filter = toloka.filter.FilterAnd(
            [
                # 18 years ago from mock
                toloka.filter.DateOfBirth < int(datetime.datetime(1982, 11, 30).timestamp()),
                toloka.filter.FilterAnd(
                    [
                        toloka.filter.Languages.in_('RU'),
                        (
                            toloka.filter.RegionByPhone.in_(225) |  # RU
                            toloka.filter.RegionByPhone.in_(187) |  # UA
                            toloka.filter.RegionByPhone.in_(149)  # BY
                        ),
                        (
                            (toloka.filter.ClientType == toloka.filter.ClientType.ClientType.BROWSER) |
                            (toloka.filter.ClientType == toloka.filter.ClientType.ClientType.TOLOKA_APP)
                        )
                    ]
                )
            ]
        )

        actual_pool = pool_config.create_pool_params(fb_loop_params.get_pool_config(
            lang='ru-RU',
            markup_id='11eeff',
            markup_step=MarkupStep.TRANSCRIPT,
            tags=['FOO:bar', 'CLIENT:kek'],
            priority=MarkupPriorities.INTERNAL,
        ))
        self.assertEqual(expected_pool, actual_pool)

    @patch('cloud.ai.speechkit.stt.lib.data_pipeline.markup_params.markup_params.get_month_from_now')
    def test_check_asr_transcript_ru_pool_params(self, get_month_from_now):
        get_month_from_now.return_value = datetime.datetime(year=2000, month=12, day=31)
        expected_pool = toloka.Pool(
            project_id='37615',
            private_name='aa00dd (check-asr-transcript) FOO:bar, CLIENT:kek',
            may_contain_adult_content=True,
            will_expire=datetime.datetime(year=2000, month=12, day=31),
            reward_per_assignment=0.03,
            assignment_max_duration_seconds=2916,
            auto_accept_solutions=False,
            auto_accept_period_day=7,
            assignments_issuing_config=toloka.Pool.AssignmentsIssuingConfig(
                issue_task_suites_in_creation_order=False,
            ),
            priority=50,
            defaults=toloka.Pool.Defaults(
                default_overlap_for_new_tasks=3,
                default_overlap_for_new_task_suites=3,
            ),
        )
        expected_pool.set_mixer_config(
            real_tasks_count=24,
            golden_tasks_count=3,
            training_tasks_count=0,
        )
        expected_pool.set_training_requirement(toloka.pool.QualityControl.TrainingRequirement(
            training_pool_id='26996102',
            training_passing_skill_value=85,
        ))
        expected_pool.set_captcha_frequency(toloka.pool.QualityControl.CaptchaFrequency.LOW)
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.Captcha(history_size=10),
            conditions=[toloka.conditions.StoredResultsCount >= 5, toloka.conditions.SuccessRate <= 30.0],
            action=toloka.actions.RestrictionV2(
                private_comment='Капча',
                duration=1,
                duration_unit=toloka.user_restriction.DurationUnit.DAYS,
                scope=toloka.UserRestriction.PROJECT,
            )
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.SkippedInRowAssignments(),
            conditions=[toloka.conditions.SkippedInRowCount >= 5],
            action=toloka.actions.RestrictionV2(
                private_comment='Пропущенные подряд задания',
                duration=1,
                duration_unit=toloka.user_restriction.DurationUnit.DAYS,
                scope=toloka.UserRestriction.PROJECT,
            )
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.AssignmentSubmitTime(
                fast_submit_threshold_seconds=72,
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
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.AssignmentSubmitTime(
                fast_submit_threshold_seconds=218,
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
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.AnswerCount(),
            conditions=[toloka.conditions.AssignmentsAcceptedCount >= 8],
            action=toloka.actions.RestrictionV2(
                private_comment='Выполнил достаточное количество заданий, отправили отдыхать.',
                duration=5,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.GoldenSet(history_size=3),
            conditions=[
                toloka.conditions.TotalAnswersCount >= 3,
                toloka.conditions.CorrectAnswersRate <= 60.0,
                toloka.conditions.CorrectAnswersRate > 30.0,
            ],
            action=toloka.actions.RestrictionV2(
                private_comment='контрольные задания (1 из 3 верно)',
                duration=1,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.GoldenSet(history_size=3),
            conditions=[
                toloka.conditions.TotalAnswersCount >= 3,
                toloka.conditions.CorrectAnswersRate <= 30.0,
            ],
            action=toloka.actions.RestrictionV2(
                private_comment='контрольные задания (0 из 3 верно)',
                duration=8,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )
        expected_pool.filter = (
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

        actual_pool = params.get_pool_params(
            lang='ru-RU',
            markup_id='aa00dd',
            markup_step=MarkupStep.CHECK_ASR_TRANSCRIPT,
            tags=['FOO:bar', 'CLIENT:kek'],
            priority=MarkupPriorities.CLIENT_NORMAL,
            override={},
        )

        self.assertEqual(expected_pool, actual_pool)

    @patch.object(fb_loop_params.pool_config, 'datetime', Mock(wraps=datetime))
    def test_check_transcript_ru_pool_params(self):
        fb_loop_params.pool_config.datetime.datetime.today.return_value = datetime.datetime(year=2000, month=11, day=30)
        fb_loop_params.pool_config.datetime.datetime.utcnow.return_value = \
            datetime.datetime(year=2000, month=11, day=30)
        expected_pool = toloka.Pool(
            project_id='37615',
            private_name='aa00dd (check-transcript) FOO:bar, CLIENT:kek',
            may_contain_adult_content=True,
            will_expire=datetime.datetime(year=2000, month=12, day=30),
            reward_per_assignment=0.03,
            assignment_max_duration_seconds=3645,
            auto_accept_solutions=False,
            auto_accept_period_day=7,
            assignments_issuing_config=toloka.Pool.AssignmentsIssuingConfig(
                issue_task_suites_in_creation_order=False,
            ),
            priority=50,
            defaults=toloka.Pool.Defaults(
                default_overlap_for_new_tasks=2,
                default_overlap_for_new_task_suites=2,
            ),
        )
        expected_pool.set_mixer_config(
            real_tasks_count=24,
            golden_tasks_count=3,
            training_tasks_count=0,
        )
        expected_pool.set_training_requirement(toloka.pool.QualityControl.TrainingRequirement(
            training_pool_id='26996102',
            training_passing_skill_value=85,
        ))
        expected_pool.set_captcha_frequency(toloka.pool.QualityControl.CaptchaFrequency.LOW)
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.Captcha(history_size=10),
            conditions=[toloka.conditions.StoredResultsCount >= 5, toloka.conditions.SuccessRate <= 30.0],
            action=toloka.actions.RestrictionV2(
                private_comment='Captcha',
                duration=8,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            )
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.SkippedInRowAssignments(),
            conditions=[toloka.conditions.SkippedInRowCount >= 5],
            action=toloka.actions.RestrictionV2(
                private_comment='Skipped assignments',
                duration=8,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            )
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.AssignmentSubmitTime(
                fast_submit_threshold_seconds=72,
                history_size=10,
            ),
            conditions=[toloka.conditions.TotalSubmittedCount >= 1, toloka.conditions.FastSubmittedCount >= 1],
            action=toloka.actions.RestrictionV2(
                private_comment='Fast submits, <= 0.1',
                duration=1 * 60 * 24,
                duration_unit=toloka.user_restriction.DurationUnit.MINUTES,
                scope=toloka.UserRestriction.PROJECT,
            )
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.AssignmentSubmitTime(
                fast_submit_threshold_seconds=218,
                history_size=10,
            ),
            conditions=[toloka.conditions.TotalSubmittedCount >= 1, toloka.conditions.FastSubmittedCount >= 1],
            action=toloka.actions.RestrictionV2(
                private_comment='Fast submits, 0.1 < time <= 0.3',
                duration=8 * 60,
                duration_unit=toloka.user_restriction.DurationUnit.MINUTES,
                scope=toloka.UserRestriction.PROJECT,
            )
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.GoldenSet(history_size=3),
            conditions=[
                toloka.conditions.TotalAnswersCount >= 3,
                toloka.conditions.CorrectAnswersRate < (1 / 3) * 100,
            ],
            action=toloka.actions.RestrictionV2(
                private_comment='Control tasks: [0, 1) done correctly',
                duration=8 * 60,
                duration_unit=toloka.user_restriction.DurationUnit.MINUTES,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.GoldenSet(history_size=3),
            conditions=[
                toloka.conditions.TotalAnswersCount >= 3,
                toloka.conditions.CorrectAnswersRate >= (1 / 3) * 100,
                toloka.conditions.CorrectAnswersRate < (2 / 3) * 100,
            ],
            action=toloka.actions.RestrictionV2(
                private_comment='Control tasks: [1, 2) done correctly',
                duration=1 * 60,
                duration_unit=toloka.user_restriction.DurationUnit.MINUTES,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.AnswerCount(),
            conditions=[toloka.conditions.AssignmentsAcceptedCount >= 9],
            action=toloka.actions.RestrictionV2(
                private_comment='Completed many assignments, vacation',
                duration=5,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )
        expected_pool.filter = toloka.filter.FilterAnd(
            [
                # 18 years ago from mock
                toloka.filter.DateOfBirth < int(datetime.datetime(1982, 11, 30).timestamp()),
                toloka.filter.FilterAnd(
                    [
                        toloka.filter.Languages.in_('RU'),
                        (
                            toloka.filter.RegionByPhone.in_(225) |  # RU
                            toloka.filter.RegionByPhone.in_(187) |  # UA
                            toloka.filter.RegionByPhone.in_(149)  # BY
                        ),
                        (
                            (toloka.filter.ClientType == toloka.filter.ClientType.ClientType.BROWSER) |
                            (toloka.filter.ClientType == toloka.filter.ClientType.ClientType.TOLOKA_APP)
                        )
                    ]
                )
            ]
        )

        actual_pool = pool_config.create_pool_params(fb_loop_params.get_pool_config(
            lang='ru-RU',
            markup_id='aa00dd',
            markup_step=MarkupStep.CHECK_TRANSCRIPT,
            tags=['FOO:bar', 'CLIENT:kek'],
            priority=MarkupPriorities.CLIENT_NORMAL,
        ))
        self.assertEqual(expected_pool, actual_pool)

    @patch('cloud.ai.speechkit.stt.lib.data_pipeline.markup_params.markup_params.get_month_from_now')
    def test_quality_evaluation_kk_pool_params(self, get_month_from_now):
        get_month_from_now.return_value = datetime.datetime(year=2000, month=12, day=31)
        expected_pool = toloka.Pool(
            project_id='47136',
            private_name='aa00dd (quality-evaluation) FOO:bar, CLIENT:kek',
            may_contain_adult_content=True,
            will_expire=datetime.datetime(year=2000, month=12, day=31),
            reward_per_assignment=0.06,
            assignment_max_duration_seconds=2916,
            auto_accept_solutions=False,
            auto_accept_period_day=7,
            assignments_issuing_config=toloka.Pool.AssignmentsIssuingConfig(
                issue_task_suites_in_creation_order=False,
            ),
            priority=50,
            defaults=toloka.Pool.Defaults(
                default_overlap_for_new_tasks=5,
                default_overlap_for_new_task_suites=5,
            ),
        )
        expected_pool.set_mixer_config(
            real_tasks_count=21,
            golden_tasks_count=6,
            training_tasks_count=0,
        )
        expected_pool.set_captcha_frequency(toloka.pool.QualityControl.CaptchaFrequency.LOW)
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.Captcha(history_size=10),
            conditions=[toloka.conditions.StoredResultsCount >= 5, toloka.conditions.SuccessRate <= 30.0],
            action=toloka.actions.RestrictionV2(
                private_comment='Капча',
                duration=1,
                duration_unit=toloka.user_restriction.DurationUnit.DAYS,
                scope=toloka.UserRestriction.PROJECT,
            )
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.SkippedInRowAssignments(),
            conditions=[toloka.conditions.SkippedInRowCount >= 5],
            action=toloka.actions.RestrictionV2(
                private_comment='Пропущенные подряд задания',
                duration=1,
                duration_unit=toloka.user_restriction.DurationUnit.DAYS,
                scope=toloka.UserRestriction.PROJECT,
            )
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.AssignmentSubmitTime(
                fast_submit_threshold_seconds=72,
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
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.AssignmentSubmitTime(
                fast_submit_threshold_seconds=218,
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
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.AnswerCount(),
            conditions=[toloka.conditions.AssignmentsAcceptedCount >= 21],
            action=toloka.actions.RestrictionV2(
                private_comment='Выполнил достаточное количество заданий, отправили отдыхать.',
                duration=5,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.GoldenSet(history_size=6),
            conditions=[
                toloka.conditions.TotalAnswersCount >= 6,
                toloka.conditions.CorrectAnswersRate <= 80.0,
                toloka.conditions.CorrectAnswersRate > 40.0,
            ],
            action=toloka.actions.RestrictionV2(
                private_comment='контрольные задания (4 из 6 или 3 из 6 верно)',
                duration=1,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )
        expected_pool.quality_control.add_action(
            collector=toloka.collectors.GoldenSet(history_size=6),
            conditions=[
                toloka.conditions.TotalAnswersCount >= 6,
                toloka.conditions.CorrectAnswersRate <= 40.0,
            ],
            action=toloka.actions.RestrictionV2(
                private_comment='контрольные задания (2 из 6 или 1 из 6 или 0 из 6 верно)',
                duration=8,
                duration_unit=toloka.user_restriction.DurationUnit.HOURS,
                scope=toloka.UserRestriction.PROJECT,
            ),
        )
        expected_pool.filter = (
            toloka.filter.Languages.in_('KK') &
            (toloka.filter.DateOfBirth <= 1041368400) &  # 2003-01-01
            (
                (toloka.filter.ClientType == toloka.filter.ClientType.ClientType.BROWSER) |
                (toloka.filter.ClientType == toloka.filter.ClientType.ClientType.TOLOKA_APP)
            )
        )

        actual_pool = params.get_pool_params(
            lang='kk-KK',
            markup_id='aa00dd',
            markup_step=MarkupStep.QUALITY_EVALUATION,
            tags=['FOO:bar', 'CLIENT:kek'],
            priority=MarkupPriorities.CLIENT_NORMAL,
            override={},
        )

        self.assertEqual(expected_pool, actual_pool)
