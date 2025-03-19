import typing
import unittest
from dataclasses import dataclass

from cloud.ai.speechkit.stt.lib.data.model.dao import (
    MarkupAssignment,
    MarkupData,
    MarkupDataVersions,
    MarkupSolution,
    MarkupSolutionCheckTranscript,
    MarkupSolutionTranscriptAndType,
    MarkupTranscriptType,
    KnownSolution,
    SolutionFieldDiff,
    SolutionAcceptanceResult,
    SolutionHoneypotAcceptanceData,
    HoneypotsAcceptanceValidationData,
    HoneypotAcceptanceStrategy,
    TextComparisonStopWordsArcadiaSource,
    ClusterReferencesInfo,
    ClusterReferencesArcadia,
    ClusterReferenceMergeStrategy,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_quality import (
    assignment_honeypot_acceptance,
    validate_and_get_honeypot_markup_data,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_quality.strategies import (
    default_honeypot_acceptance_strategy,
    transcript_acceptance_strategy_v1,
    TextComparisonStopWords,
    TextValidationToolkit,
)
from cloud.ai.speechkit.stt.lib.eval.metrics.calc import get_metric_wer


class TestAcceptance(unittest.TestCase):
    def setUp(self):
        # TODO: dirty hack, delete after calculate_wer begins to accept config as json argument
        with open('cluster_references.json', 'w+', encoding='utf-8') as f:
            self.cluster_references_file = f
            self.cluster_references_file.write(
                """
            {
                "алло": [
                    "алле",
                    "аллё",
                    "ало",
                    "але",
                    "алё"
                ],
                "здравствуйте": [
                    "здрасьте",
                    "здрасте",
                    "здрасити"
                ],
            }"""
            )

        self.text_validation_toolkit = TextValidationToolkit(
            wer=get_metric_wer(self.cluster_references_file.name),
            cluster_references_info=ClusterReferencesInfo(
                descriptors=[
                    ClusterReferencesArcadia(
                        topic='common',
                        lang='ru-RU',
                        revision=332211,
                    ),
                ],
                merge_strategy=ClusterReferenceMergeStrategy.SKIP,
            ),
            stop_words=TextComparisonStopWords(
                source=TextComparisonStopWordsArcadiaSource(
                    topic='general',
                    lang='ru-RU',
                    revision=123456,
                ),
                words={
                    '?', 'м', 'ммм', 'э',
                }
            )
        )

    def test_validate_honeypot_markup_data(self):
        with self.assertRaises(AssertionError):
            validate_and_get_honeypot_markup_data(gen_markup_data(
                expected_solutions=[
                    MarkupSolutionCheckTranscript(
                        ok=True,
                        type=MarkupTranscriptType.SPEECH,
                    ),
                    MarkupSolutionTranscriptAndType(
                        type=MarkupTranscriptType.SPEECH,
                        text='аало',
                    ),
                ],
                actual_solution=MarkupSolutionCheckTranscript(
                    ok=True,
                    type=MarkupTranscriptType.SPEECH,
                ),
            ))

    def test_default_honeypot_acceptance_strategy(self):
        self.assertEqual(
            SolutionAcceptanceResult(
                accepted=False,
                diff_list=[
                    SolutionFieldDiff(
                        field_name='ok',
                        expected_value=True,
                        actual_value=False,
                        extra=None,
                    ),
                    SolutionFieldDiff(
                        field_name='type',
                        expected_value='speech',
                        actual_value='unclear',
                        extra=None,
                    ),
                ]
            ),
            default_honeypot_acceptance_strategy(
                expected_solution=MarkupSolutionCheckTranscript(
                    ok=True,
                    type=MarkupTranscriptType.SPEECH,
                ),
                actual_solution=MarkupSolutionCheckTranscript(
                    ok=False,
                    type=MarkupTranscriptType.UNCLEAR,
                ),
            ),
        )
        self.assertEqual(
            SolutionAcceptanceResult(
                accepted=False,
                diff_list=[
                    SolutionFieldDiff(
                        field_name='text',
                        expected_value='алло',
                        actual_value='привет',
                        extra=None,
                    ),
                ]
            ),
            default_honeypot_acceptance_strategy(
                expected_solution=MarkupSolutionTranscriptAndType(
                    text='алло',
                    type=MarkupTranscriptType.SPEECH,
                ),
                actual_solution=MarkupSolutionTranscriptAndType(
                    text='привет',
                    type=MarkupTranscriptType.SPEECH,
                ),
            ),
        )
        self.assertEqual(
            SolutionAcceptanceResult(
                accepted=True,
                diff_list=[],
            ),
            default_honeypot_acceptance_strategy(
                expected_solution=MarkupSolutionCheckTranscript(
                    ok=True,
                    type=MarkupTranscriptType.SPEECH,
                ),
                actual_solution=MarkupSolutionCheckTranscript(
                    ok=True,
                    type=MarkupTranscriptType.SPEECH,
                ),
            ),
        )

        # Different attr sets
        with self.assertRaises(AssertionError):
            expected_solution = MarkupSolutionCheckTranscript(
                ok=True,
                type=MarkupTranscriptType.SPEECH,
            )
            actual_solution = MarkupSolutionCheckTranscript(
                ok=True,
                type=MarkupTranscriptType.SPEECH,
            )
            actual_solution.foo = 'bar'
            default_honeypot_acceptance_strategy(
                expected_solution=expected_solution,
                actual_solution=actual_solution,
            )

        # Different attr types
        with self.assertRaises(AssertionError):
            expected_solution = MarkupSolutionCheckTranscript(
                ok=True,
                type=MarkupTranscriptType.SPEECH,
            )
            actual_solution = MarkupSolutionCheckTranscript(
                ok=True,
                type=MarkupTranscriptType.SPEECH,
            )
            actual_solution.ok = 42
            default_honeypot_acceptance_strategy(
                expected_solution=expected_solution,
                actual_solution=actual_solution,
            )

        @dataclass
        class MarkupSolutionFoo:
            foo: list

        # list field type is not supported
        with self.assertRaises(AssertionError):
            # noinspection PyTypeChecker
            default_honeypot_acceptance_strategy(
                expected_solution=MarkupSolutionFoo(foo=[]),
                actual_solution=MarkupSolutionFoo(foo=[])
            )

    def test_transcript_acceptance_strategy_v1(self):
        self.assertEqual(
            SolutionAcceptanceResult(
                accepted=True,
                diff_list=[],
            ),
            transcript_acceptance_strategy_v1(
                expected_solution=MarkupSolutionTranscriptAndType(
                    text='алло здравствуйте',
                    type=MarkupTranscriptType.SPEECH,
                ),
                actual_solution=MarkupSolutionTranscriptAndType(
                    text='алло здравствуйте',
                    type=MarkupTranscriptType.SPEECH,
                ),
                text_validation_toolkit=self.text_validation_toolkit,
            ),
        )
        self.assertEqual(
            SolutionAcceptanceResult(
                accepted=False,
                diff_list=[
                    SolutionFieldDiff(
                        field_name='type',
                        expected_value='unclear',
                        actual_value='no-speech',
                        extra=None,
                    ),
                ],
            ),
            transcript_acceptance_strategy_v1(
                expected_solution=MarkupSolutionTranscriptAndType(
                    text='',
                    type=MarkupTranscriptType.UNCLEAR,
                ),
                actual_solution=MarkupSolutionTranscriptAndType(
                    text='',
                    type=MarkupTranscriptType.NO_SPEECH,
                ),
                text_validation_toolkit=self.text_validation_toolkit,
            ),
        )
        self.assertEqual(
            SolutionAcceptanceResult(
                accepted=True,
                diff_list=[
                    SolutionFieldDiff(
                        field_name='text',
                        expected_value='хочу ммм взять  кредит',
                        actual_value='? хочу  э взять ?  кредит ',
                        extra={
                            'stop-words': {
                                'source': 'arcadia',
                                'topic': 'general',
                                'lang': 'ru-RU',
                                'revision': 123456,
                            },
                            'cr': {
                                'descriptors': [
                                    {
                                        'topic': 'common',
                                        'lang': 'ru-RU',
                                        'revision': 332211,
                                        'source': 'arcadia',
                                    }
                                ],
                                'merge_strategy': 'skip',
                            },
                        },
                    ),
                ],
            ),
            transcript_acceptance_strategy_v1(
                expected_solution=MarkupSolutionTranscriptAndType(
                    text='хочу ммм взять  кредит',
                    type=MarkupTranscriptType.SPEECH,
                ),
                actual_solution=MarkupSolutionTranscriptAndType(
                    text='? хочу  э взять ?  кредит ',
                    type=MarkupTranscriptType.SPEECH,
                ),
                text_validation_toolkit=self.text_validation_toolkit,
            ),
        )
        self.assertEqual(
            SolutionAcceptanceResult(
                accepted=False,
                diff_list=[
                    SolutionFieldDiff(
                        field_name='text',
                        expected_value='хочу взять кредит',
                        actual_value='? хочу  вот взять ?  кредит ',
                        extra={
                            'stop-words': {
                                'source': 'arcadia',
                                'topic': 'general',
                                'lang': 'ru-RU',
                                'revision': 123456,
                            },
                            'cr': {
                                'descriptors': [
                                    {
                                        'topic': 'common',
                                        'lang': 'ru-RU',
                                        'revision': 332211,
                                        'source': 'arcadia',
                                    }
                                ],
                                'merge_strategy': 'skip',
                            },
                        },
                    ),
                ],
            ),
            transcript_acceptance_strategy_v1(
                expected_solution=MarkupSolutionTranscriptAndType(
                    text='хочу взять кредит',
                    type=MarkupTranscriptType.SPEECH,
                ),
                actual_solution=MarkupSolutionTranscriptAndType(
                    text='? хочу  вот взять ?  кредит ',
                    type=MarkupTranscriptType.SPEECH,
                ),
                text_validation_toolkit=self.text_validation_toolkit,
            ),
        )
        self.assertEqual(
            SolutionAcceptanceResult(
                accepted=False,
                diff_list=[
                    SolutionFieldDiff(
                        field_name='type',
                        expected_value='speech',
                        actual_value='unclear',
                        extra=None,
                    ),
                    SolutionFieldDiff(
                        field_name='text',
                        expected_value='да',
                        actual_value='нет',
                        extra={
                            'stop-words': {
                                'source': 'arcadia',
                                'topic': 'general',
                                'lang': 'ru-RU',
                                'revision': 123456,
                            },
                            'cr': {
                                'descriptors': [
                                    {
                                        'topic': 'common',
                                        'lang': 'ru-RU',
                                        'revision': 332211,
                                        'source': 'arcadia',
                                    }
                                ],
                                'merge_strategy': 'skip',
                            },
                        },
                    ),
                ],
            ),
            transcript_acceptance_strategy_v1(
                expected_solution=MarkupSolutionTranscriptAndType(
                    text='да',
                    type=MarkupTranscriptType.SPEECH,
                ),
                actual_solution=MarkupSolutionTranscriptAndType(
                    text='нет',
                    type=MarkupTranscriptType.UNCLEAR,
                ),
                text_validation_toolkit=self.text_validation_toolkit,
            ),
        )
        self.assertEqual(
            SolutionAcceptanceResult(
                accepted=True,
                diff_list=[
                    SolutionFieldDiff(
                        field_name='text',
                        expected_value=' ало да здрасьте спасибо ммм не   надо',
                        actual_value='  алле да здрасте  э спасибо не надо ? ',
                        extra={
                            'stop-words': {
                                'source': 'arcadia',
                                'topic': 'general',
                                'lang': 'ru-RU',
                                'revision': 123456,
                            },
                            'cr': {
                                'descriptors': [
                                    {
                                        'topic': 'common',
                                        'lang': 'ru-RU',
                                        'revision': 332211,
                                        'source': 'arcadia',
                                    }
                                ],
                                'merge_strategy': 'skip',
                            },
                        },
                    ),
                ],
            ),
            transcript_acceptance_strategy_v1(
                expected_solution=MarkupSolutionTranscriptAndType(
                    text=' ало да здрасьте спасибо ммм не   надо',
                    type=MarkupTranscriptType.SPEECH,
                ),
                actual_solution=MarkupSolutionTranscriptAndType(
                    text='  алле да здрасте  э спасибо не надо ? ',
                    type=MarkupTranscriptType.SPEECH,
                ),
                text_validation_toolkit=self.text_validation_toolkit,
            ),
        )

    def test_assignment_honeypot_acceptance(self):
        markup_data_1 = gen_markup_data(
            expected_solutions=[
                MarkupSolutionCheckTranscript(
                    ok=False,
                    type=MarkupTranscriptType.UNCLEAR,
                ),
                MarkupSolutionCheckTranscript(
                    ok=True,
                    type=MarkupTranscriptType.SPEECH,
                ),
            ],
            actual_solution=MarkupSolutionCheckTranscript(
                ok=True,
                type=MarkupTranscriptType.SPEECH,
            ),
            task_id='task-1',
        )

        markup_data_2 = gen_markup_data(
            expected_solutions=[
                MarkupSolutionCheckTranscript(
                    ok=True,
                    type=MarkupTranscriptType.SPEECH,
                ),
                MarkupSolutionCheckTranscript(
                    ok=False,
                    type=MarkupTranscriptType.NO_SPEECH,
                ),
            ],
            actual_solution=MarkupSolutionCheckTranscript(
                ok=True,
                type=MarkupTranscriptType.UNCLEAR,
            ),
            task_id='task-2',
        )

        markup_data_3 = gen_markup_data(
            expected_solutions=[
                MarkupSolutionCheckTranscript(
                    ok=False,
                    type=MarkupTranscriptType.NO_SPEECH,
                ),
            ],
            actual_solution=MarkupSolutionCheckTranscript(
                ok=False,
                type=MarkupTranscriptType.NO_SPEECH,
            ),
            task_id='task-3',
        )

        assignment = gen_markup_assignment([markup_data_1, markup_data_2, markup_data_3])

        for min_correct_solution, expected_accepted, expected_rejection_comment in [
            (2, True, None),
            (3, False, 'Проверьте задания под номерами: 2. Если вы не нашли в них ошибок, мы рассмотрим вашу '
                       'апелляцию. Подробнее в разделе “Проверка задания” инструкции проекта.'),
        ]:
            self.assertEqual(
                HoneypotsAcceptanceValidationData(
                    accepted=expected_accepted,
                    correct_solutions=[
                        SolutionHoneypotAcceptanceData(
                            task_id='task-1',
                            known_solutions=[
                                SolutionAcceptanceResult(
                                    accepted=False,
                                    diff_list=[
                                        SolutionFieldDiff(
                                            field_name='ok',
                                            expected_value=False,
                                            actual_value=True,
                                            extra=None,
                                        ),
                                        SolutionFieldDiff(
                                            field_name='type',
                                            expected_value='unclear',
                                            actual_value='speech',
                                            extra=None,
                                        ),
                                    ]
                                ),
                                SolutionAcceptanceResult(
                                    accepted=True,
                                    diff_list=[],
                                ),
                            ],
                        ),
                        SolutionHoneypotAcceptanceData(
                            task_id='task-3',
                            known_solutions=[
                                SolutionAcceptanceResult(
                                    accepted=True,
                                    diff_list=[],
                                )
                            ],
                        )
                    ],
                    incorrect_solutions=[
                        SolutionHoneypotAcceptanceData(
                            task_id='task-2',
                            known_solutions=[
                                SolutionAcceptanceResult(
                                    accepted=False,
                                    diff_list=[
                                        SolutionFieldDiff(
                                            field_name='type',
                                            expected_value='speech',
                                            actual_value='unclear',
                                            extra=None,
                                        ),
                                    ],
                                ),
                                SolutionAcceptanceResult(
                                    accepted=False,
                                    diff_list=[
                                        SolutionFieldDiff(
                                            field_name='ok',
                                            expected_value=False,
                                            actual_value=True,
                                            extra=None,
                                        ),
                                        SolutionFieldDiff(
                                            field_name='type',
                                            expected_value='no-speech',
                                            actual_value='unclear',
                                            extra=None,
                                        ),
                                    ],
                                ),
                            ],
                        )
                    ],
                    min_correct_solutions=min_correct_solution,
                    acceptance_strategy=HoneypotAcceptanceStrategy.DEFAULT,
                    rejection_comment=expected_rejection_comment,
                ),
                assignment_honeypot_acceptance(
                    assignment, min_correct_solutions=min_correct_solution, lang='ru-RU',
                ),
            )

        markup_data_4 = gen_markup_data(
            expected_solutions=[
                MarkupSolutionTranscriptAndType(
                    text='добрый день',
                    type=MarkupTranscriptType.SPEECH,
                ),
                MarkupSolutionTranscriptAndType(
                    text='',
                    type=MarkupTranscriptType.UNCLEAR,
                ),
            ],
            actual_solution=MarkupSolutionTranscriptAndType(
                text='э добрый  день ?',
                type=MarkupTranscriptType.SPEECH,
            ),
            task_id='task-4',
            version=MarkupDataVersions.TRANSCRIPT_AND_TYPE,
        )

        markup_data_5 = gen_markup_data(
            expected_solutions=[
                MarkupSolutionTranscriptAndType(
                    text='',
                    type=MarkupTranscriptType.NO_SPEECH,
                ),
            ],
            actual_solution=MarkupSolutionTranscriptAndType(
                text='',
                type=MarkupTranscriptType.NO_SPEECH,
            ),
            task_id='task-5',
            version=MarkupDataVersions.TRANSCRIPT_AND_TYPE,
        )

        markup_data_6 = gen_markup_data(
            expected_solutions=[
                MarkupSolutionTranscriptAndType(
                    text='нет спасибо',
                    type=MarkupTranscriptType.SPEECH,
                ),
                MarkupSolutionTranscriptAndType(
                    text='не спасибо',
                    type=MarkupTranscriptType.SPEECH,
                ),
            ],
            actual_solution=MarkupSolutionTranscriptAndType(
                text='да спасибо',
                type=MarkupTranscriptType.SPEECH,
            ),
            task_id='task-6',
            version=MarkupDataVersions.TRANSCRIPT_AND_TYPE,
        )

        assignment = gen_markup_assignment([markup_data_4, markup_data_5, markup_data_6])

        for min_correct_solution, expected_accepted, expected_rejection_comment in [
            (2, True, None),
            (3, False, 'Проверьте задания под номерами: 3. Если вы не нашли в них ошибок, мы рассмотрим вашу '
                       'апелляцию. Подробнее в разделе “Проверка задания” инструкции проекта.'),
        ]:
            self.assertEqual(
                HoneypotsAcceptanceValidationData(
                    accepted=expected_accepted,
                    correct_solutions=[
                        SolutionHoneypotAcceptanceData(
                            task_id='task-4',
                            known_solutions=[
                                SolutionAcceptanceResult(
                                    accepted=True,
                                    diff_list=[
                                        SolutionFieldDiff(
                                            field_name='text',
                                            expected_value='добрый день',
                                            actual_value='э добрый  день ?',
                                            extra={
                                                'stop-words': {
                                                    'source': 'arcadia',
                                                    'topic': 'general',
                                                    'lang': 'ru-RU',
                                                    'revision': 123456,
                                                },
                                                'cr': {
                                                    'descriptors': [
                                                        {
                                                            'topic': 'common',
                                                            'lang': 'ru-RU',
                                                            'revision': 332211,
                                                            'source': 'arcadia',
                                                        }
                                                    ],
                                                    'merge_strategy': 'skip',
                                                },
                                            },
                                        ),
                                    ],
                                ),
                                SolutionAcceptanceResult(
                                    accepted=False,
                                    diff_list=[
                                        SolutionFieldDiff(
                                            field_name='type',
                                            expected_value='unclear',
                                            actual_value='speech',
                                            extra=None,
                                        ),
                                        SolutionFieldDiff(
                                            field_name='text',
                                            expected_value='',
                                            actual_value='э добрый  день ?',
                                            extra={
                                                'stop-words': {
                                                    'source': 'arcadia',
                                                    'topic': 'general',
                                                    'lang': 'ru-RU',
                                                    'revision': 123456,
                                                },
                                                'cr': {
                                                    'descriptors': [
                                                        {
                                                            'topic': 'common',
                                                            'lang': 'ru-RU',
                                                            'revision': 332211,
                                                            'source': 'arcadia',
                                                        }
                                                    ],
                                                    'merge_strategy': 'skip',
                                                },
                                            },
                                        ),
                                    ]
                                )
                            ],
                        ),
                        SolutionHoneypotAcceptanceData(
                            task_id='task-5',
                            known_solutions=[
                                SolutionAcceptanceResult(
                                    accepted=True,
                                    diff_list=[],
                                ),
                            ],
                        ),
                    ],
                    incorrect_solutions=[
                        SolutionHoneypotAcceptanceData(
                            task_id='task-6',
                            known_solutions=[
                                SolutionAcceptanceResult(
                                    accepted=False,
                                    diff_list=[
                                        SolutionFieldDiff(
                                            field_name='text',
                                            expected_value='нет спасибо',
                                            actual_value='да спасибо',
                                            extra={
                                                'stop-words': {
                                                    'source': 'arcadia',
                                                    'topic': 'general',
                                                    'lang': 'ru-RU',
                                                    'revision': 123456,
                                                },
                                                'cr': {
                                                    'descriptors': [
                                                        {
                                                            'topic': 'common',
                                                            'lang': 'ru-RU',
                                                            'revision': 332211,
                                                            'source': 'arcadia',
                                                        }
                                                    ],
                                                    'merge_strategy': 'skip',
                                                },
                                            },
                                        ),
                                    ],
                                ),
                                SolutionAcceptanceResult(
                                    accepted=False,
                                    diff_list=[
                                        SolutionFieldDiff(
                                            field_name='text',
                                            expected_value='не спасибо',
                                            actual_value='да спасибо',
                                            extra={
                                                'stop-words': {
                                                    'source': 'arcadia',
                                                    'topic': 'general',
                                                    'lang': 'ru-RU',
                                                    'revision': 123456,
                                                },
                                                'cr': {
                                                    'descriptors': [
                                                        {
                                                            'topic': 'common',
                                                            'lang': 'ru-RU',
                                                            'revision': 332211,
                                                            'source': 'arcadia',
                                                        }
                                                    ],
                                                    'merge_strategy': 'skip',
                                                },
                                            },
                                        ),
                                    ],
                                ),
                            ],
                        ),
                    ],
                    min_correct_solutions=min_correct_solution,
                    acceptance_strategy=HoneypotAcceptanceStrategy.TRANSCRIPT_V1,
                    rejection_comment=expected_rejection_comment,
                ),
                assignment_honeypot_acceptance(
                    assignment, min_correct_solutions=min_correct_solution, lang='ru-RU',
                    text_validation_toolkit=self.text_validation_toolkit,
                ),
            )


# noinspection PyTypeChecker
def gen_markup_data(
    expected_solutions: typing.List[MarkupSolution],
    actual_solution: MarkupSolution,
    task_id: str = '',
    version: MarkupDataVersions = None,
) -> MarkupData:
    return MarkupData(
        version=version,
        input=None,
        solution=actual_solution,
        known_solutions=[
            KnownSolution(solution=expected_solution, correctness_weight=1)
            for expected_solution in expected_solutions
        ],
        task_id=task_id,
        overlap=None,
        raw_data=None,
        created_at=None
    )


# noinspection PyTypeChecker
def gen_markup_assignment(
    tasks: typing.List[MarkupData],
) -> MarkupAssignment:
    return MarkupAssignment(
        id=None,
        markup_id=None,
        markup_step=None,
        pool_id='pool_id',
        data=None,
        tasks=tasks,
        validation_data=[],
        received_at=None,
        other=None
    )
