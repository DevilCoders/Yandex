import unittest

from cloud.ai.speechkit.stt.lib.data.model.dao import *  # noqa


class TestRecordsJoins(unittest.TestCase):
    def test_has_better_quality(self):
        for join_1, join_2, expected_result in [
            (
                RecordJoin(
                    id='1/abc',
                    record_id='1',
                    recognition=None,  # noqa
                    join_data=JoinDataFeedbackLoop(
                        confidence=0.5,
                        assignment_accuracy=0.6,
                        assignment_evaluation_recall=1.0,
                        attempts=2,
                    ),
                    received_at=None,  # noqa
                    other=None,
                ),
                RecordJoin(
                    id='1/abc',
                    record_id='1',
                    recognition=None,  # noqa
                    join_data=JoinDataFeedbackLoop(
                        confidence=0.5,
                        assignment_accuracy=0.6,
                        assignment_evaluation_recall=1.0,
                        attempts=2,
                    ),
                    received_at=None,  # noqa
                    other=None,
                ),
                False,  # equal quality
            ),
            (
                RecordJoin(
                    id='1/abc',
                    record_id='1',
                    recognition=None,  # noqa
                    join_data=JoinDataFeedbackLoop(
                        confidence=0.7,
                        assignment_accuracy=0.6,
                        assignment_evaluation_recall=1.0,
                        attempts=2,
                    ),
                    received_at=None,  # noqa
                    other=None,
                ),
                RecordJoin(
                    id='1/abc',
                    record_id='1',
                    recognition=None,  # noqa
                    join_data=JoinDataFeedbackLoop(
                        confidence=0.5,
                        assignment_accuracy=0.6,
                        assignment_evaluation_recall=1.0,
                        attempts=2,
                    ),
                    received_at=None,  # noqa
                    other=None,
                ),
                True,  # better by confidence
            ),
            (
                RecordJoin(
                    id='1/abc',
                    record_id='1',
                    recognition=None,  # noqa
                    join_data=JoinDataFeedbackLoop(
                        confidence=0.7,
                        assignment_accuracy=0.6,
                        assignment_evaluation_recall=1.0,
                        attempts=2,
                    ),
                    received_at=None,  # noqa
                    other=None,
                ),
                RecordJoin(
                    id='1/abc',
                    record_id='1',
                    recognition=None,  # noqa
                    join_data=JoinDataFeedbackLoop(
                        confidence=None,
                        assignment_accuracy=0.6,
                        assignment_evaluation_recall=1.0,
                        attempts=2,
                    ),
                    received_at=None,  # noqa
                    other=None,
                ),
                True,  # better by confidence (other is None)
            ),
            (
                RecordJoin(
                    id='1/abc',
                    record_id='1',
                    recognition=None,  # noqa
                    join_data=JoinDataFeedbackLoop(
                        confidence=0.5,
                        assignment_accuracy=0.5,
                        assignment_evaluation_recall=0.5,
                        attempts=2,
                    ),
                    received_at=None,  # noqa
                    other=None,
                ),
                RecordJoin(
                    id='1/abc',
                    record_id='1',
                    recognition=None,  # noqa
                    join_data=JoinDataFeedbackLoop(
                        confidence=0.5,
                        assignment_accuracy=0.01,
                        assignment_evaluation_recall=0.99,
                        attempts=2,
                    ),
                    received_at=None,  # noqa
                    other=None,
                ),
                True,  # better by f1
            ),
            (
                RecordJoin(
                    id='1/abc',
                    record_id='1',
                    recognition=None,  # noqa
                    join_data=JoinDataFeedbackLoop(
                        confidence=0.5,
                        assignment_accuracy=0.6,
                        assignment_evaluation_recall=1.0,
                        attempts=2,
                    ),
                    received_at=None,  # noqa
                    other=None,
                ),
                RecordJoin(
                    id='1/abc',
                    record_id='1',
                    recognition=None,  # noqa
                    join_data=JoinDataExemplar(),
                    received_at=None,  # noqa
                    other=None,
                ),
                None,  # one of joins is not feedback loop join
            ),
        ]:
            self.assertEqual(expected_result, join_1.has_better_quality(join_2))
