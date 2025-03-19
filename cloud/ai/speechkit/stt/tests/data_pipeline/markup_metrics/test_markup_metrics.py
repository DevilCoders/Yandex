import unittest

from cloud.ai.speechkit.stt.lib.data.model.dao import (
    RecordBitMarkup, MarkupSolution, MarkupSolutionCheckTranscript, MarkupData, MarkupTranscriptType,
    SimpleStaticOverlapStrategy,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_metrics import (
    get_majority_solution,
    calculate_quality_evaluation_values,
)


class TestMarkupStatistics(unittest.TestCase):
    def test_calculate_quality_evaluation_values(self):
        markups = [
            # no majority vote
            self.gen_markup('1', MarkupSolutionCheckTranscript(ok=False, type=MarkupTranscriptType.NO_SPEECH)),
            self.gen_markup('1', MarkupSolutionCheckTranscript(ok=False, type=MarkupTranscriptType.NO_SPEECH)),
            self.gen_markup('1', MarkupSolutionCheckTranscript(ok=False, type=MarkupTranscriptType.UNCLEAR)),

            # ok
            self.gen_markup('2', MarkupSolutionCheckTranscript(ok=True, type=MarkupTranscriptType.NO_SPEECH)),
            self.gen_markup('2', MarkupSolutionCheckTranscript(ok=True, type=MarkupTranscriptType.NO_SPEECH)),
            self.gen_markup('2', MarkupSolutionCheckTranscript(ok=True, type=MarkupTranscriptType.NO_SPEECH)),

            # not ok
            self.gen_markup('3', MarkupSolutionCheckTranscript(ok=False, type=MarkupTranscriptType.SPEECH)),
            self.gen_markup('3', MarkupSolutionCheckTranscript(ok=False, type=MarkupTranscriptType.SPEECH)),
            self.gen_markup('3', MarkupSolutionCheckTranscript(ok=False, type=MarkupTranscriptType.SPEECH)),

            # ok
            self.gen_markup('4', MarkupSolutionCheckTranscript(ok=True, type=MarkupTranscriptType.SPEECH)),
            self.gen_markup('4', MarkupSolutionCheckTranscript(ok=True, type=MarkupTranscriptType.SPEECH)),
            self.gen_markup('4', MarkupSolutionCheckTranscript(ok=True, type=MarkupTranscriptType.SPEECH)),

            # ok
            self.gen_markup('5', MarkupSolutionCheckTranscript(ok=True, type=MarkupTranscriptType.SPEECH)),
            self.gen_markup('5', MarkupSolutionCheckTranscript(ok=True, type=MarkupTranscriptType.SPEECH)),
            self.gen_markup('5', MarkupSolutionCheckTranscript(ok=True, type=MarkupTranscriptType.SPEECH)),
        ]
        self.assertEqual(
            (0.75, 0.2),
            calculate_quality_evaluation_values(markups, SimpleStaticOverlapStrategy(min_majority=3, overall=3)),
        )

        pass

    def test_majority_solution(self):
        markups = [
            self.gen_markup('1', MarkupSolutionCheckTranscript(ok=False, type=MarkupTranscriptType.NO_SPEECH)),
            self.gen_markup('1', MarkupSolutionCheckTranscript(ok=True, type=MarkupTranscriptType.SPEECH)),
            self.gen_markup('1', MarkupSolutionCheckTranscript(ok=False, type=MarkupTranscriptType.UNCLEAR)),
            self.gen_markup('1', MarkupSolutionCheckTranscript(ok=True, type=MarkupTranscriptType.SPEECH)),
            self.gen_markup('1', MarkupSolutionCheckTranscript(ok=False, type=MarkupTranscriptType.NO_SPEECH)),
            self.gen_markup('1', MarkupSolutionCheckTranscript(ok=True, type=MarkupTranscriptType.SPEECH)),
            self.gen_markup('1', MarkupSolutionCheckTranscript(ok=False, type=MarkupTranscriptType.UNCLEAR)),
        ]
        self.assertEqual(
            (MarkupSolutionCheckTranscript(ok=True, type=MarkupTranscriptType.SPEECH), 3),
            get_majority_solution(markups),
        )

    # noinspection PyTypeChecker
    @staticmethod
    def gen_markup(bit_id: str, solution: MarkupSolution) -> RecordBitMarkup:
        return RecordBitMarkup(
            record_id=None,
            bit_id=bit_id,
            id=None,
            audio_obfuscation_data=None,
            audio_params=None,
            markup_id=None,
            markup_step=None,
            pool_id=None,
            assignment_id=None,
            markup_data=MarkupData(
                version=None,
                input=None,
                solution=solution,
                known_solutions=None,
                task_id=None,
                overlap=None,
                raw_data=None,
                created_at=None,
            ),
            validation_data=None,
            received_at=None,
            other=None,
        )
