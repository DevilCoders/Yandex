import unittest

from cloud.ai.speechkit.stt.lib.eval.metrics.calc import (
    get_metric_wer,
    get_metric_mer_1_0,
    calculate_metric,
    preprocess_text,
    CalculateMetricResult,
)
from cloud.ai.speechkit.stt.lib.eval.metrics.mer import MERData
from cloud.ai.speechkit.stt.lib.eval.metrics.wer import WERData, AlignmentElement, AlignmentAction
from cloud.ai.speechkit.stt.lib.eval.model import EvaluationTarget


class TestMetricsCalculation(unittest.TestCase):
    evaluation_targets = [
        EvaluationTarget(
            record_id='r1',
            channel=1,
            hypothesis='мама мыла красивую раму',
            reference='мама мыла раму',
            slices=[],
        ),
        EvaluationTarget(
            record_id='r2',
            channel=1,
            hypothesis='мама мыла раму',
            reference='мама мыла красивую раму',
            slices=[],
        ),
        EvaluationTarget(
            record_id='r3',
            channel=1,
            hypothesis='мама мыла красивую раму',
            reference='мама мыла ужасную раму',
            slices=['fancy'],
        ),
        EvaluationTarget(
            record_id='r2',
            channel=2,
            hypothesis='да хорошо',
            reference='да хорошо',
            slices=['yes'],
        ),
        EvaluationTarget(
            record_id='r4',
            channel=1,
            hypothesis='моя мама мыла ужасную раму',
            reference='мама мыла красивую раму',
            slices=['nice'],
        ),
        EvaluationTarget(
            record_id='r5',
            channel=1,
            hypothesis='нет мама мыла не раму',
            reference='да мама мыла раму',
            slices=['yes', 'nice'],
        ),
        EvaluationTarget(
            record_id='r2',
            channel=3,
            hypothesis='алиса',
            reference='',
            slices=[],
        ),
        EvaluationTarget(
            record_id='r6',
            channel=1,
            hypothesis='  красивая  хм елка',
            reference='?  ? Красивая ёлка  эээ ',
            slices=['fancy'],
        ),
        EvaluationTarget(
            record_id='r3',
            channel=2,
            hypothesis='угу спасибо портом',
            reference='угу нет спасибо потом',
            slices=['yes', 'fancy'],
        )
    ]
    tag_to_records_ids = {
        't1': ['r1', 'r2', 'r3', 'r4', 'r5'],
        't2': ['r1', 'r3', 'r5'],
        't3': ['r4'],
        't4': ['r2', 'r4', 'r5'],
        't5': ['r6'],
    }

    def test_calculate_wer(self):
        self.assertEqual(
            CalculateMetricResult(
                record_id_channel_to_metric_data={
                    ('r1', 1): WERData(
                        errors_count=1,
                        ref_words_count=3,
                        hyp_words_count=4,
                        diff_hyp='мама мыла красивую раму',
                        diff_ref='мама мыла ******** раму',
                        alignment=[
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='мама',
                                hyp_word='мама',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='мыла',
                                hyp_word='мыла',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.INSERTION,
                                ref_word='',
                                hyp_word='красивую',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='раму',
                                hyp_word='раму',
                            ),
                        ],
                    ),
                    ('r2', 1): WERData(
                        errors_count=1,
                        ref_words_count=4,
                        hyp_words_count=3,
                        diff_hyp='мама мыла ******** раму',
                        diff_ref='мама мыла красивую раму',
                        alignment=[
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='мама',
                                hyp_word='мама',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='мыла',
                                hyp_word='мыла',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.DELETION,
                                ref_word='красивую',
                                hyp_word='',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='раму',
                                hyp_word='раму',
                            ),
                        ],
                    ),
                    ('r3', 1): WERData(
                        errors_count=1,
                        ref_words_count=4,
                        hyp_words_count=4,
                        diff_hyp='мама мыла КРАСИВУЮ раму',
                        diff_ref='мама мыла УЖАСНУЮ  раму',
                        alignment=[
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='мама',
                                hyp_word='мама',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='мыла',
                                hyp_word='мыла',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.SUBSTITUTION,
                                ref_word='ужасную',
                                hyp_word='красивую',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='раму',
                                hyp_word='раму',
                            ),
                        ],
                    ),
                    ('r2', 2): WERData(
                        errors_count=0,
                        ref_words_count=2,
                        hyp_words_count=2,
                        diff_hyp='да хорошо',
                        diff_ref='да хорошо',
                        alignment=[
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='да',
                                hyp_word='да',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='хорошо',
                                hyp_word='хорошо',
                            ),
                        ],
                    ),
                    ('r4', 1): WERData(
                        errors_count=2,
                        ref_words_count=4,
                        hyp_words_count=5,
                        diff_hyp='моя мама мыла УЖАСНУЮ  раму',
                        diff_ref='*** мама мыла КРАСИВУЮ раму',
                        alignment=[
                            AlignmentElement(
                                action=AlignmentAction.INSERTION,
                                ref_word='',
                                hyp_word='моя',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='мама',
                                hyp_word='мама',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='мыла',
                                hyp_word='мыла',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.SUBSTITUTION,
                                ref_word='красивую',
                                hyp_word='ужасную',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='раму',
                                hyp_word='раму',
                            ),
                        ],
                    ),
                    ('r5', 1): WERData(
                        errors_count=2,
                        ref_words_count=4,
                        hyp_words_count=5,
                        diff_hyp='НЕТ мама мыла не раму',
                        diff_ref='ДА  мама мыла ** раму',
                        alignment=[
                            AlignmentElement(
                                action=AlignmentAction.SUBSTITUTION,
                                ref_word='да',
                                hyp_word='нет',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='мама',
                                hyp_word='мама',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='мыла',
                                hyp_word='мыла',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.INSERTION,
                                ref_word='',
                                hyp_word='не',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='раму',
                                hyp_word='раму',
                            ),
                        ],
                    ),
                    ('r2', 3): WERData(
                        errors_count=1,
                        ref_words_count=0,
                        hyp_words_count=1,
                        diff_hyp='алиса',
                        diff_ref='*****',
                        alignment=[
                            AlignmentElement(
                                action=AlignmentAction.INSERTION,
                                ref_word='',
                                hyp_word='алиса',
                            ),
                        ],
                    ),
                    ('r6', 1): WERData(
                        errors_count=0,
                        ref_words_count=2,
                        hyp_words_count=2,
                        diff_hyp='красивая елка',
                        diff_ref='красивая елка',
                        alignment=[
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='красивая',
                                hyp_word='красивая',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='елка',
                                hyp_word='елка',
                            ),
                        ],
                    ),
                    ('r3', 2): WERData(
                        errors_count=2,
                        ref_words_count=4,
                        hyp_words_count=3,
                        diff_hyp='угу *** спасибо ПОРТОМ',
                        diff_ref='угу нет спасибо ПОТОМ ',
                        alignment=[
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='угу',
                                hyp_word='угу',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.DELETION,
                                ref_word='нет',
                                hyp_word='',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.COINCIDENCE,
                                ref_word='спасибо',
                                hyp_word='спасибо',
                            ),
                            AlignmentElement(
                                action=AlignmentAction.SUBSTITUTION,
                                ref_word='потом',
                                hyp_word='портом',
                            ),
                        ],
                    )
                },
                record_id_channel_to_metric_value={
                    ('r1', 1): 0.25,
                    ('r2', 1): 0.25,
                    ('r3', 1): 0.25,
                    ('r2', 2): 0.0,
                    ('r4', 1): 0.4,
                    ('r5', 1): 0.4,
                    ('r2', 3): 1.0,
                    ('r6', 1): 0.0,
                    ('r3', 2): 0.5,
                },
                tag_slice_to_metric_aggregates={
                    ('t1', None): {
                        'mean': (0.25 + 0.25 + 0.25 + 0.0 + 0.4 + 0.4 + 1.0 + 0.5) / 8,
                        'total': (1 + 1 + 1 + 0 + 2 + 2 + 1 + 2) / max(
                            3 + 4 + 4 + 2 + 4 + 4 + 0 + 4,
                            4 + 3 + 4 + 2 + 5 + 5 + 1 + 3,
                        ),
                    },
                    ('t1', 'yes'): {
                        'mean': (0.0 + 0.5 + 0.4) / 3,
                        'total': (0 + 2 + 2) / max(
                            2 + 3 + 5,
                            2 + 4 + 4,
                        ),
                    },
                    ('t1', 'fancy'): {
                        'mean': (0.25 + 0.5) / 2,
                        'total': (1 + 2) / max(
                            4 + 3,
                            4 + 4,
                        ),
                    },
                    ('t1', 'nice'): {
                        'mean': (0.4 + 0.4) / 2,
                        'total': (2 + 2) / max(
                            5 + 5,
                            4 + 4,
                        ),
                    },
                    ('t2', None): {
                        'mean': (0.25 + 0.25 + 0.4 + 0.5) / 4,
                        'total': (1 + 1 + 2 + 2) / max(
                            3 + 4 + 4 + 4,
                            4 + 4 + 5 + 3,
                        ),
                    },
                    ('t2', 'yes'): {
                        'mean': (0.5 + 0.4) / 2,
                        'total': (2 + 2) / max(
                            3 + 5,
                            4 + 4,
                        ),
                    },
                    ('t2', 'fancy'): {
                        'mean': (0.25 + 0.5) / 2,
                        'total': (1 + 2) / max(
                            4 + 3,
                            4 + 4,
                        ),
                    },
                    ('t2', 'nice'): {
                        'mean': 0.4,
                        'total': 0.4,
                    },
                    ('t3', None): {
                        'mean': 0.4,
                        'total': 2 / max(4, 5),
                    },
                    ('t3', 'nice'): {
                        'mean': 0.4,
                        'total': 0.4,
                    },
                    ('t4', None): {
                        'mean': (0.25 + 0.0 + 0.4 + 0.4 + 1.0) / 5,
                        'total': (1 + 0 + 2 + 2 + 1) / max(
                            4 + 2 + 4 + 4 + 0,
                            3 + 2 + 5 + 5 + 1,
                        )
                    },
                    ('t4', 'yes'): {
                        'mean': (0.0 + 0.4) / 2,
                        'total': (0 + 2) / max(
                            2 + 5,
                            2 + 4,
                        ),
                    },
                    ('t4', 'nice'): {
                        'mean': (0.4 + 0.4) / 2,
                        'total': (2 + 2) / max(
                            5 + 5,
                            4 + 4,
                        ),
                    },
                    ('t5', None): {
                        'mean': 0.0,
                        'total': 0.0,
                    },
                    ('t5', 'fancy'): {
                        'mean': 0.0,
                        'total': 0.0,
                    },
                },
            ),
            calculate_metric(
                metric=get_metric_wer(),
                evaluation_targets=self.evaluation_targets,
                tag_to_records_ids=self.tag_to_records_ids,
                stop_words={'?', 'хм', 'эээ'},
            ),
        )

    def test_calculate_mer(self):
        self.assertEqual(
            CalculateMetricResult(
                record_id_channel_to_metric_data={
                    ('r1', 1): MERData(score=0.3402489094339534),
                    ('r2', 1): MERData(score=0.3402489094339534),
                    ('r3', 1): MERData(score=0.3438066345963348),
                    ('r2', 2): MERData(score=0.14928124007283444),
                    ('r4', 1): MERData(score=0.4698320789455751),
                    ('r5', 1): MERData(score=0.4647707453143987),
                    ('r2', 3): MERData(score=0.878831633007543),
                    ('r6', 1): MERData(score=0.5545356733398293),
                    ('r3', 2): MERData(score=0.56263543823034),
                },
                record_id_channel_to_metric_value={
                    ('r1', 1): 0.3402489094339534,
                    ('r2', 1): 0.3402489094339534,
                    ('r3', 1): 0.3438066345963348,
                    ('r2', 2): 0.14928124007283444,
                    ('r4', 1): 0.4698320789455751,
                    ('r5', 1): 0.4647707453143987,
                    ('r2', 3): 0.878831633007543,
                    ('r6', 1): 0.5545356733398293,
                    ('r3', 2): 0.56263543823034,
                },
                tag_slice_to_metric_aggregates={
                    ('t1', None): {'mean': (
                                               0.3402489094339534 +
                                               0.3402489094339534 +
                                               0.3438066345963348 +
                                               0.14928124007283444 +
                                               0.4698320789455751 +
                                               0.4647707453143987 +
                                               0.878831633007543 +
                                               0.56263543823034
                                           ) / 8},
                    ('t1', 'yes'): {'mean': (
                                                0.14928124007283444 +
                                                0.56263543823034 +
                                                0.4647707453143987
                                            ) / 3},
                    ('t1', 'fancy'): {'mean': (
                                                  0.56263543823034 +
                                                  0.3438066345963348
                                              ) / 2},
                    ('t1', 'nice'): {'mean': (
                                                 0.4698320789455751 +
                                                 0.4647707453143987
                                             ) / 2},
                    ('t2', None): {'mean': (
                                               0.3402489094339534 +
                                               0.3438066345963348 +
                                               0.4647707453143987 +
                                               0.56263543823034
                                           ) / 4},
                    ('t2', 'yes'): {'mean': (
                                                0.56263543823034 +
                                                0.4647707453143987
                                            ) / 2},
                    ('t2', 'fancy'): {'mean': (
                                                  0.56263543823034 +
                                                  0.3438066345963348
                                              ) / 2},
                    ('t2', 'nice'): {'mean': 0.4647707453143987},
                    ('t3', None): {'mean': 0.4698320789455751},
                    ('t3', 'nice'): {'mean': 0.4698320789455751},
                    ('t4', None): {'mean': (
                                               0.3402489094339534 +
                                               0.14928124007283444 +
                                               0.4698320789455751 +
                                               0.4647707453143987 +
                                               0.878831633007543
                                           ) / 5},
                    ('t4', 'yes'): {'mean': (
                                                0.14928124007283444 +
                                                0.4647707453143987
                                            ) / 2},
                    ('t4', 'nice'): {'mean': (
                                                 0.4698320789455751 +
                                                 0.4647707453143987
                                             ) / 2},
                    ('t5', None): {'mean': 0.5545356733398293},
                    ('t5', 'fancy'): {'mean': 0.5545356733398293},
                },
            ),
            calculate_metric(
                metric=get_metric_mer_1_0(),
                evaluation_targets=self.evaluation_targets,
                tag_to_records_ids=self.tag_to_records_ids,
                stop_words=set(),
            ),
        )

    def test_preprocess_text(self):
        self.assertEqual(
            'красивая елка',
            preprocess_text('   Красивая  ?  ёлка  '),
        )
        self.assertEqual(
            '? красивая елка',
            preprocess_text(' эээ ?  красивая ёлка  хм', stop_words={'хм', 'эээ'}),
        )
        self.assertEqual(
            'ёлка',
            preprocess_text('ёлка', replace_yo=False),
        )
