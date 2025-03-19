import unittest

from cloud.ai.speechkit.stt.lib.experiments.records_utterances_split import (
    split_record_by_utterances, Fragment
)


class TestSplitByUtterances(unittest.TestCase):
    def test_all_cases(self):
        self.assertEqual(
            [],
            split_record_by_utterances(
                words=[],
                min_ms_between_utterances=3000,
            ),
        )
        self.assertEqual(
            [
                Fragment(
                    text='алло',
                    start_time_ms=50,
                    end_time_ms=1000,
                ),
            ],
            split_record_by_utterances(
                words=[
                    Fragment(
                        text='алло',
                        start_time_ms=50,
                        end_time_ms=1000,
                    ),
                ],
                min_ms_between_utterances=3000,
            ),
        )
        self.assertEqual(
            [
                Fragment(
                    text='алло добрый день',
                    start_time_ms=100,
                    end_time_ms=3000,
                ),
            ],
            split_record_by_utterances(
                words=[
                    Fragment(
                        text='алло',
                        start_time_ms=100,
                        end_time_ms=900,
                    ),
                    Fragment(
                        text='добрый',
                        start_time_ms=1000,
                        end_time_ms=2200,
                    ),
                    Fragment(
                        text='день',
                        start_time_ms=2400,
                        end_time_ms=3000,
                    ),
                ],
                min_ms_between_utterances=3000,
            ),
        )

        words = [
            Fragment(
                text='да',
                start_time_ms=50,
                end_time_ms=300,
            ),
            # 500ms
            Fragment(
                text='алло',
                start_time_ms=800,
                end_time_ms=1300,
            ),
            # 2000ms
            Fragment(
                text='добрый',
                start_time_ms=3300,
                end_time_ms=3900,
            ),
            # 300ms
            Fragment(
                text='день',
                start_time_ms=4200,
                end_time_ms=4700,
            ),
            # 4000ms
            Fragment(
                text='не',
                start_time_ms=8700,
                end_time_ms=9000,
            ),
            # 300ms
            Fragment(
                text='знаю',
                start_time_ms=9300,
                end_time_ms=9700,
            ),
            # 700ms
            Fragment(
                text='о',
                start_time_ms=10400,
                end_time_ms=10600,
            ),
            # 200ms
            Fragment(
                text='чем',
                start_time_ms=10800,
                end_time_ms=11100,
            ),
            # 200ms
            Fragment(
                text='вы',
                start_time_ms=11300,
                end_time_ms=11500,
            ),
        ]

        self.assertEqual(
            [
                Fragment(
                    text='да алло добрый день не знаю о чем вы',
                    start_time_ms=50,
                    end_time_ms=11500,
                ),
            ],
            split_record_by_utterances(
                words=words,
                min_ms_between_utterances=5000,
            ),
        )
        self.assertEqual(
            [
                Fragment(
                    text='да алло добрый день',
                    start_time_ms=50,
                    end_time_ms=4700,
                ),
                Fragment(
                    text='не знаю о чем вы',
                    start_time_ms=8700,
                    end_time_ms=11500
                ),
            ],
            split_record_by_utterances(
                words=words,
                min_ms_between_utterances=3500,
            )
        )
        self.assertEqual(
            [
                Fragment(
                    text='да алло',
                    start_time_ms=50,
                    end_time_ms=1300,
                ),
                Fragment(
                    text='добрый день',
                    start_time_ms=3300,
                    end_time_ms=4700,
                ),
                Fragment(
                    text='не знаю о чем вы',
                    start_time_ms=8700,
                    end_time_ms=11500
                ),
            ],
            split_record_by_utterances(
                words=words,
                min_ms_between_utterances=1500,
            )
        )
        self.assertEqual(
            [
                Fragment(
                    text='да',
                    start_time_ms=50,
                    end_time_ms=300,
                ),
                Fragment(
                    text='алло',
                    start_time_ms=800,
                    end_time_ms=1300,
                ),
                Fragment(
                    text='добрый день',
                    start_time_ms=3300,
                    end_time_ms=4700,
                ),
                Fragment(
                    text='не знаю',
                    start_time_ms=8700,
                    end_time_ms=9700
                ),
                Fragment(
                    text='о чем вы',
                    start_time_ms=10400,
                    end_time_ms=11500
                ),
            ],
            split_record_by_utterances(
                words=words,
                min_ms_between_utterances=400,
            )
        )
