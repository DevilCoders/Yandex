import unittest

from cloud.ai.lib.python.datetime import parse_datetime
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    RecognitionPlainTranscript,
    JoinDataDpM3NaiveDev,
    JoinDataExemplar,
    MultiChannelJoinData,
    MultiChannelRecognition,
    ChannelJoinData,
    ChannelRecognition,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.select_records_joins import choose_join, available_strategies


class TestJoinChoice(unittest.TestCase):
    def test_select_exemplar_join(self):
        self.assertEqual(
            ['exemplar join text'],
            choose_join(
                'rid',
                [
                    (
                        RecognitionPlainTranscript(text='exemplar join text'),
                        JoinDataExemplar(),
                        parse_datetime('2019-12-16T00:00:00+00:00'),
                    ),
                    (
                        RecognitionPlainTranscript(text='dp join text'),
                        JoinDataDpM3NaiveDev(),
                        parse_datetime('2020-12-16T00:00:00+00:00'),
                    ),
                ],
                [available_strategies['EXEMPLAR'], available_strategies['LATEST']]
            ),
        )

    def test_select_exemplar_multichannel_join(self):
        self.assertEqual(
            ['exemplar join text channel 1', 'exemplar join text channel 2'],
            choose_join(
                'rid',
                [
                    (
                        MultiChannelRecognition(
                            channels=[
                                ChannelRecognition(
                                    channel=1,
                                    recognition=RecognitionPlainTranscript(text='exemplar join text channel 1'),
                                ),
                                ChannelRecognition(
                                    channel=2,
                                    recognition=RecognitionPlainTranscript(text='exemplar join text channel 2'),
                                ),
                            ],
                        ),
                        MultiChannelJoinData(
                            channels=[
                                ChannelJoinData(
                                    channel=1,
                                    join_data=JoinDataExemplar(),
                                ),
                                ChannelJoinData(
                                    channel=2,
                                    join_data=JoinDataExemplar(),
                                ),
                            ],
                        ),
                        parse_datetime('2019-12-16T00:00:00+00:00'),
                    ),
                    (
                        RecognitionPlainTranscript(text='dp join text'),
                        JoinDataDpM3NaiveDev(),
                        parse_datetime('2020-12-16T00:00:00+00:00'),
                    ),
                ],
                [available_strategies['EXEMPLAR'], available_strategies['LATEST']]
            ),
        )

    def text_failure_on_multiple_exemplar_joins(self):
        with self.assertRaises(ValueError):
            choose_join(
                'rid',
                [
                    (
                        RecognitionPlainTranscript(text='exemplar join text'),
                        JoinDataExemplar(),
                        parse_datetime('2019-12-16T00:00:00+00:00'),
                    ),
                    (
                        RecognitionPlainTranscript(text='exemplar join text'),
                        JoinDataExemplar(),
                        parse_datetime('2019-12-16T00:00:00+00:00'),
                    ),
                ],
                [available_strategies['EXEMPLAR'], available_strategies['LATEST']]
            )

    def test_select_latest_join(self):
        self.assertEqual(
            ['text 2'],
            choose_join(
                'rid',
                [
                    (
                        RecognitionPlainTranscript(text='text 1'),
                        JoinDataDpM3NaiveDev(),
                        parse_datetime('2018-12-16T00:00:00+00:00'),
                    ),
                    (
                        RecognitionPlainTranscript(text='text 2'),
                        JoinDataDpM3NaiveDev(),
                        parse_datetime('2020-12-16T00:00:00+00:00'),
                    ),
                    (
                        RecognitionPlainTranscript(text='text 3'),
                        JoinDataDpM3NaiveDev(),
                        parse_datetime('2019-12-16T00:00:00+00:00'),
                    ),
                ],
                [available_strategies['EXEMPLAR'], available_strategies['LATEST']]
            ),
        )

    def test_select_earliest_join(self):
        self.assertEqual(
            ['text 1'],
            choose_join(
                'rid',
                [
                    (
                        RecognitionPlainTranscript(text='text 1'),
                        JoinDataDpM3NaiveDev(),
                        parse_datetime('2018-12-16T00:00:00+00:00'),
                    ),
                    (
                        RecognitionPlainTranscript(text='text 2'),
                        JoinDataDpM3NaiveDev(),
                        parse_datetime('2020-12-16T00:00:00+00:00'),
                    ),
                    (
                        RecognitionPlainTranscript(text='text 3'),
                        JoinDataDpM3NaiveDev(),
                        parse_datetime('2019-12-16T00:00:00+00:00'),
                    ),
                ],
                [available_strategies['EXEMPLAR'], available_strategies['EARLIEST']]
            ),
        )
