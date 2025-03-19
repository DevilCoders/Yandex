import unittest
from datetime import datetime

from mock import patch

from cloud.ai.speechkit.stt.lib.data.model.dao import (
    JoinDataOKCheckTranscriptMV,
    JoinDataNotOKCheckTranscriptNoSpeechMV,
    JoinDataNoSpeechTranscriptMV,
    JoinDataNoSpeechAllBits,
    JoinDataDpM3NaiveDev,
    MajorityVote,
    SimpleStaticOverlapStrategy,
    MultiChannelJoinData,
    MultiChannelRecognition,
    ChannelJoinData,
    ChannelRecognition,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.join import (
    group_records_bits_markups,
    RecordMarkupData,
    RecordChannelMarkupData,
    RecordBitMarkupData,
    produce_majority_vote,
    produce_majority_votes_joins_and_filter_joiner_input,
    filter_records_bits_to_transcript,
    marshal_input_for_markup_joiner,
    unmarshal_output_of_markup_joiner,
    join_markups,
    RecordJoin,
    RecognitionPlainTranscript,
    MajorityVoteResult,
)
from .data import (
    BitDataTimeInterval,
    check_transcript_markups_with_bits_data,
    MarkupDataVersions,
    markups_with_bits_data,
    record1_channel1_bit1_markup1,
    record1_channel1_bit1_markup2,
    record1_channel1_bit1_markup3,
    record1_channel1_bit1_markup4,
    record1_channel1_bit2_markup1,
    record1_channel1_bit2_markup2,
    record1_channel1_bit3_markup1,
    record1_channel1_bit3_markup2,
    record1_channel1_bit3_markup3,
    record1_channel1_bit3_markup4,
    record1_channel1_bit3_markup5,
    record1_channel1_bit3_markup6,
    record1_channel1_bit4_markup1,
    record1_channel1_bit4_markup2,
    record1_channel1_bit4_markup3,
    record1_channel1_bit5_markup1,
    record1_channel1_bit5_markup2,
    record1_channel1_bit5_markup3,
    record1_channel1_bit5_markup4,
    record1_channel1_bit5_markup5,
    record1_channel1_bit6_markup1,
    record1_channel1_bit6_markup2,
    record1_channel1_bit6_markup3,
    record1_channel1_bit6_markup4,
    record1_channel1_bit6_markup5,
    record1_channel1_bit6_markup6,
    record1_channel1_bit6_markup7,
    record1_channel2_bit1_markup1,
    record1_channel2_bit2_markup1,
    record1_channel2_bit2_markup2,
    record1_channel2_bit2_markup3,
    record1_channel2_bit2_markup4,
    record1_channel2_bit2_markup5,
    record1_channel2_bit3_markup1,
    record1_channel2_bit4_markup1,
    record1_channel2_bit4_markup2,
    record1_channel2_bit4_markup3,
    record1_channel2_bit5_markup1,
    record1_channel2_bit5_markup2,
    record1_channel2_bit6_markup1,
    record1_channel2_bit6_markup2,
    record1_channel2_bit6_markup3,
    record1_id,
    record2_channel1_bit1_markup1,
    record2_id,
    record3_channel1_bit1_markup1,
    record3_channel1_bit1_markup2,
    record3_channel1_bit1_markup3,
    record3_channel1_bit2_markup1,
    record3_channel1_bit2_markup2,
    record3_channel1_bit3_markup1,
    record3_channel1_bit3_markup2,
    record3_channel1_bit3_markup3,
    record3_channel1_bit3_markup4,
    record3_channel2_bit1_markup1,
    record3_channel2_bit1_markup2,
    record3_channel2_bit1_markup3,
    record3_channel2_bit2_markup1,
    record3_channel2_bit2_markup2,
    record3_channel2_bit3_markup1,
    record3_id,
    record4_channel1_bit1_markup1,
    record4_channel1_bit1_markup2,
    record4_channel1_bit1_markup3,
    record4_channel2_bit1_markup1,
    record4_channel2_bit1_markup2,
    record4_channel2_bit1_markup3,
    record4_channel2_bit1_markup4,
    record4_channel2_bit1_markup5,
    record4_id,
    record5_channel1_bit1_markup1,
    record5_channel1_bit1_markup2,
    record5_id,
)


class TestMarkupsJoin(unittest.TestCase):
    def test_produce_majority_vote(self):
        for markups, expected_result in [
            (
                [record1_channel1_bit1_markup1, record1_channel1_bit1_markup2, record1_channel1_bit1_markup3],
                None,
            ),
            (
                [record1_channel1_bit1_markup4],
                MajorityVoteResult(
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=1, overall=1),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
            ),
            (
                [record1_channel1_bit2_markup1, record1_channel1_bit2_markup2],
                None,
            ),
            (
                [record1_channel1_bit3_markup1, record1_channel1_bit3_markup2],
                None,
            ),
            (
                [record1_channel1_bit3_markup3, record1_channel1_bit3_markup4, record1_channel1_bit3_markup5,
                 record1_channel1_bit3_markup6],
                None,
            ),
            (
                [record1_channel1_bit4_markup1, record1_channel1_bit4_markup2, record1_channel1_bit4_markup3],
                MajorityVoteResult(
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
            ),
            (
                [
                    record1_channel1_bit5_markup1,
                    record1_channel1_bit5_markup2,
                    record1_channel1_bit5_markup3,
                    record1_channel1_bit5_markup4,
                    record1_channel1_bit5_markup5,
                ],
                MajorityVoteResult(
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=3, overall=5),
                        overlap_strategy=None
                    ),
                    text='с оплатой интернета',
                ),
            ),
            (
                [record1_channel1_bit6_markup1, record1_channel1_bit6_markup2, record1_channel1_bit6_markup3],
                None,
            ),
            (
                [record1_channel1_bit6_markup4, record1_channel1_bit6_markup5, record1_channel1_bit6_markup6,
                 record1_channel1_bit6_markup7],
                None,
            ),
            (
                [record1_channel2_bit1_markup1],
                MajorityVoteResult(
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=1, overall=1),
                        overlap_strategy=None,
                    ),
                    text='меня зовут павел чем могу помочь',
                ),
            ),
            (
                [record1_channel2_bit2_markup1, record1_channel2_bit2_markup2],
                None,
            ),
            (
                [record1_channel2_bit2_markup3, record1_channel2_bit2_markup4, record1_channel2_bit2_markup5],
                MajorityVoteResult(
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
            ),
            (
                [record1_channel2_bit3_markup1],
                MajorityVoteResult(
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=1, overall=1),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
            ),
            (
                [record1_channel2_bit4_markup1, record1_channel2_bit4_markup2, record1_channel2_bit4_markup3],
                None,
            ),
            (
                [record1_channel2_bit5_markup1, record1_channel2_bit5_markup2],
                MajorityVoteResult(
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=2, overall=2),
                        overlap_strategy=None,
                    ),
                    text='счета для проверки',
                ),
            ),
            (
                [record1_channel2_bit6_markup1, record1_channel2_bit6_markup2, record1_channel2_bit6_markup3],
                MajorityVoteResult(
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
            ),
            (
                [record2_channel1_bit1_markup1],
                None,
            ),
            (
                [record3_channel1_bit1_markup1, record3_channel1_bit1_markup2, record3_channel1_bit1_markup3],
                MajorityVoteResult(
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
            ),
            (
                [record3_channel1_bit2_markup1, record3_channel1_bit2_markup2],
                MajorityVoteResult(
                    join_data=JoinDataNotOKCheckTranscriptNoSpeechMV(
                        votes=MajorityVote(majority=2, overall=2),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
            ),
            (
                [record3_channel1_bit3_markup1, record3_channel1_bit3_markup2, record3_channel1_bit3_markup3,
                 record3_channel1_bit3_markup4],
                MajorityVoteResult(
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=3, overall=4),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
            ),
            (
                [record3_channel2_bit1_markup1, record3_channel2_bit1_markup2],
                None,
            ),
            (
                [record3_channel2_bit1_markup3],
                None,
            ),
            (
                [record3_channel2_bit2_markup1, record3_channel2_bit2_markup2],
                None,
            ),
            (
                [record3_channel2_bit3_markup1],
                MajorityVoteResult(
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=1, overall=1),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
            ),
            (
                [record4_channel1_bit1_markup1, record4_channel1_bit1_markup2, record4_channel1_bit1_markup3],
                MajorityVoteResult(
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
            ),
            (
                [record4_channel2_bit1_markup1, record4_channel2_bit1_markup2],
                None,
            ),
            (
                [record4_channel2_bit1_markup3, record4_channel2_bit1_markup4, record4_channel2_bit1_markup5],
                MajorityVoteResult(
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
            ),
            (
                [record5_channel1_bit1_markup1, record5_channel1_bit1_markup2],
                MajorityVoteResult(
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=2, overall=2),
                        overlap_strategy=None,
                    ),
                    text='',
                )
            )
        ]:
            self.assertEqual(expected_result, produce_majority_vote(markups, None))

        self.assertEqual(
            MajorityVoteResult(
                join_data=JoinDataNoSpeechTranscriptMV(
                    votes=MajorityVote(majority=2, overall=3),
                    overlap_strategy=SimpleStaticOverlapStrategy(min_majority=2, overall=3),
                ),
                text='',
            ),
            produce_majority_vote(
                [record3_channel1_bit1_markup1, record3_channel1_bit1_markup2, record3_channel1_bit1_markup3],
                SimpleStaticOverlapStrategy(min_majority=2, overall=3),
            )
        )

        self.assertEqual(
            None,
            produce_majority_vote(
                [record3_channel1_bit1_markup1, record3_channel1_bit1_markup2, record3_channel1_bit1_markup3],
                SimpleStaticOverlapStrategy(min_majority=3, overall=3),
            )
        )

        with self.assertRaises(ValueError):
            # different markup versions
            produce_majority_vote(
                [record1_channel1_bit6_markup1, record1_channel1_bit6_markup2, record1_channel1_bit6_markup3,
                 record1_channel1_bit6_markup4],
                None,
            )

        with self.assertRaises(ValueError):
            # markups for different bits
            produce_majority_vote([record1_channel1_bit4_markup3, record1_channel1_bit5_markup1], None)

    def test_group_records_bits_markups(self):
        self.assertEqual(
            [
                RecordMarkupData(
                    record_id=record1_id,
                    channels=[
                        RecordChannelMarkupData(
                            channel=1,
                            bits_markups=[
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=0, end_ms=9000, index=0, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record1_channel1_bit1_markup1,
                                            record1_channel1_bit1_markup2,
                                            record1_channel1_bit1_markup3,
                                        ],
                                        MarkupDataVersions.PLAIN_TRANSCRIPT: [
                                            record1_channel1_bit1_markup4,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=3000, end_ms=12000, index=1, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
                                            record1_channel1_bit2_markup1,
                                            record1_channel1_bit2_markup2,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=6000, end_ms=15000, index=2, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record1_channel1_bit3_markup1,
                                            record1_channel1_bit3_markup2,
                                        ],
                                        MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
                                            record1_channel1_bit3_markup3,
                                            record1_channel1_bit3_markup4,
                                            record1_channel1_bit3_markup5,
                                            record1_channel1_bit3_markup6,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=9000, end_ms=18000, index=3, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record1_channel1_bit4_markup1,
                                            record1_channel1_bit4_markup2,
                                            record1_channel1_bit4_markup3,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=12000, end_ms=21000, index=4, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record1_channel1_bit5_markup1,
                                            record1_channel1_bit5_markup2,
                                            record1_channel1_bit5_markup3,
                                            record1_channel1_bit5_markup4,
                                            record1_channel1_bit5_markup5,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=15000, end_ms=22300, index=5, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record1_channel1_bit6_markup1,
                                            record1_channel1_bit6_markup2,
                                            record1_channel1_bit6_markup3,
                                        ],
                                        MarkupDataVersions.PLAIN_TRANSCRIPT: [
                                            record1_channel1_bit6_markup4,
                                            record1_channel1_bit6_markup5,
                                            record1_channel1_bit6_markup6,
                                            record1_channel1_bit6_markup7,
                                        ],
                                    },
                                ),
                            ],
                        ),
                        RecordChannelMarkupData(
                            channel=2,
                            bits_markups=[
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=0, end_ms=9000, index=0, channel=2),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record1_channel2_bit1_markup1,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=3000, end_ms=12000, index=1, channel=2),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record1_channel2_bit2_markup1,
                                            record1_channel2_bit2_markup2,
                                        ],
                                        MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
                                            record1_channel2_bit2_markup3,
                                            record1_channel2_bit2_markup4,
                                            record1_channel2_bit2_markup5,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=6000, end_ms=15000, index=2, channel=2),
                                    version_to_markups={
                                        MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
                                            record1_channel2_bit3_markup1,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=9000, end_ms=18000, index=3, channel=2),
                                    version_to_markups={
                                        MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
                                            record1_channel2_bit4_markup1,
                                            record1_channel2_bit4_markup2,
                                            record1_channel2_bit4_markup3,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=12000, end_ms=21000, index=4, channel=2),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record1_channel2_bit5_markup1,
                                            record1_channel2_bit5_markup2,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=15000, end_ms=22300, index=5, channel=2),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record1_channel2_bit6_markup1,
                                            record1_channel2_bit6_markup2,
                                            record1_channel2_bit6_markup3,
                                        ]
                                    }
                                )
                            ],
                        ),
                    ],
                ),
                RecordMarkupData(
                    record_id=record2_id,
                    channels=[
                        RecordChannelMarkupData(
                            channel=1,
                            bits_markups=[
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=0, end_ms=3100, index=0, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
                                            record2_channel1_bit1_markup1,
                                        ],
                                    },
                                ),
                            ],
                        ),
                    ],
                ),
                RecordMarkupData(
                    record_id=record3_id,
                    channels=[
                        RecordChannelMarkupData(
                            channel=1,
                            bits_markups=[
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=0, end_ms=9000, index=0, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
                                            record3_channel1_bit1_markup1,
                                            record3_channel1_bit1_markup2,
                                            record3_channel1_bit1_markup3,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=3000, end_ms=12000, index=1, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record3_channel1_bit2_markup1,
                                            record3_channel1_bit2_markup2,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=6000, end_ms=12300, index=2, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.PLAIN_TRANSCRIPT: [
                                            record3_channel1_bit3_markup1,
                                            record3_channel1_bit3_markup2,
                                            record3_channel1_bit3_markup3,
                                            record3_channel1_bit3_markup4,
                                        ],
                                    },
                                ),
                            ],
                        ),
                        RecordChannelMarkupData(
                            channel=2,
                            bits_markups=[
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=0, end_ms=9000, index=0, channel=2),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record3_channel2_bit1_markup1,
                                            record3_channel2_bit1_markup2,
                                        ],
                                        MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
                                            record3_channel2_bit1_markup3,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=3000, end_ms=12000, index=1, channel=2),
                                    version_to_markups={
                                        MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
                                            record3_channel2_bit2_markup1,
                                            record3_channel2_bit2_markup2,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=6000, end_ms=12300, index=2, channel=2),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record3_channel2_bit3_markup1,
                                        ],
                                    },
                                ),
                            ],
                        ),
                    ],
                ),
                RecordMarkupData(
                    record_id=record4_id,
                    channels=[
                        RecordChannelMarkupData(
                            channel=1,
                            bits_markups=[
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=0, end_ms=4400, index=0, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record4_channel1_bit1_markup1,
                                            record4_channel1_bit1_markup2,
                                            record4_channel1_bit1_markup3,
                                        ],
                                    },
                                ),
                            ],
                        ),
                        RecordChannelMarkupData(
                            channel=2,
                            bits_markups=[
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=0, end_ms=4400, index=0, channel=2),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record4_channel2_bit1_markup1,
                                            record4_channel2_bit1_markup2,
                                        ],
                                        MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
                                            record4_channel2_bit1_markup3,
                                            record4_channel2_bit1_markup4,
                                            record4_channel2_bit1_markup5,
                                        ],
                                    },
                                )
                            ],
                        ),
                    ],
                ),
                RecordMarkupData(
                    record_id=record5_id,
                    channels=[
                        RecordChannelMarkupData(
                            channel=1,
                            bits_markups=[
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=0, end_ms=1200, index=0, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.PLAIN_TRANSCRIPT: [
                                            record5_channel1_bit1_markup1,
                                            record5_channel1_bit1_markup2,
                                        ]
                                    }
                                )
                            ],
                        ),
                    ],
                ),
            ],
            group_records_bits_markups(markups_with_bits_data),
        )

    def test_filter_records_bits_to_transcript(self):
        self.assertEqual(
            {
                'record1_1_0-9000.wav': 'record1/1/bit1.wav',
                'record1_2_3000-12000.wav': 'record1/2/bit2.wav',
                'record1_1_6000-15000.wav': 'record1/1/bit3.wav',
                'record1_1_15000-22300.wav': 'record1/1/bit6.wav',
                'record3_2_0-9000.wav': 'record3/2/bit1.wav',
                'record4_2_0-4400.wav': 'record4/2/bit1.wav',
            },
            filter_records_bits_to_transcript(
                check_transcript_markups_with_bits_data,
                {
                    'record1_1_0-9000.wav': 'record1/1/bit1.wav',
                    'record1_2_0-9000.wav': 'record1/2/bit1.wav',
                    'record1_2_3000-12000.wav': 'record1/2/bit2.wav',
                    'record1_1_6000-15000.wav': 'record1/1/bit3.wav',
                    'record1_1_9000-18000.wav': 'record1/1/bit4.wav',
                    'record1_1_12000-21000.wav': 'record1/1/bit5.wav',
                    'record1_2_12000-21000.wav': 'record1/2/bit5.wav',
                    'record1_1_15000-22300.wav': 'record1/1/bit6.wav',
                    'record1_2_15000-22300.wav': 'record1/2/bit6.wav',
                    'record3_1_3000-12000.wav': 'record3/1/bit2.wav',
                    'record3_2_0-9000.wav': 'record3/2/bit1.wav',
                    'record3_2_6000-12300.wav': 'record3/2/bit3.wav',
                    'record4_1_0-4400.wav': 'record4/1/bit1.wav',
                    'record4_2_0-4400.wav': 'record4/2/bit1.wav',
                },
                None,
            ),
        )

    def test_produce_majority_votes_joins_and_filter_joiner_input(self):
        records_markups_data = group_records_bits_markups(markups_with_bits_data)
        (
            filtered_records_markups_data,
            bit_id_to_majority_vote_result,
        ) = produce_majority_votes_joins_and_filter_joiner_input(records_markups_data, {})

        self.assertEqual(
            [
                RecordMarkupData(
                    record_id=record1_id,
                    channels=[
                        RecordChannelMarkupData(
                            channel=1,
                            bits_markups=[
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=3000, end_ms=12000, index=1, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
                                            record1_channel1_bit2_markup1,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=6000, end_ms=15000, index=2, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
                                            record1_channel1_bit3_markup3,
                                            record1_channel1_bit3_markup6,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=12000, end_ms=21000, index=4, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record1_channel1_bit5_markup1,
                                            record1_channel1_bit5_markup2,
                                            record1_channel1_bit5_markup3,
                                            record1_channel1_bit5_markup4,
                                            record1_channel1_bit5_markup5,
                                        ]
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=15000, end_ms=22300, index=5, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.PLAIN_TRANSCRIPT: [
                                            record1_channel1_bit6_markup5,
                                            record1_channel1_bit6_markup6,
                                            record1_channel1_bit6_markup7,
                                        ],
                                    },
                                ),
                            ],
                        ),
                        RecordChannelMarkupData(
                            channel=2,
                            bits_markups=[
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=0, end_ms=9000, index=0, channel=2),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record1_channel2_bit1_markup1,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=9000, end_ms=18000, index=3, channel=2),
                                    version_to_markups={
                                        MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
                                            record1_channel2_bit4_markup2,
                                            record1_channel2_bit4_markup3,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=12000, end_ms=21000, index=4, channel=2),
                                    version_to_markups={
                                        MarkupDataVersions.CHECK_TRANSCRIPT: [
                                            record1_channel2_bit5_markup1,
                                            record1_channel2_bit5_markup2,
                                        ],
                                    },
                                ),
                            ],
                        ),
                    ],
                ),
                RecordMarkupData(
                    record_id=record2_id,
                    channels=[
                        RecordChannelMarkupData(
                            channel=1,
                            bits_markups=[
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=0, end_ms=3100, index=0, channel=1),
                                    version_to_markups={
                                        MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
                                            record2_channel1_bit1_markup1,
                                        ],
                                    },
                                ),
                            ],
                        ),
                    ],
                ),
                RecordMarkupData(
                    record_id=record3_id,
                    channels=[
                        RecordChannelMarkupData(
                            channel=2,
                            bits_markups=[
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=0, end_ms=9000, index=0, channel=2),
                                    version_to_markups={
                                        MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
                                            record3_channel2_bit1_markup3,
                                        ],
                                    },
                                ),
                                RecordBitMarkupData(
                                    bit_data=BitDataTimeInterval(start_ms=3000, end_ms=12000, index=1, channel=2),
                                    version_to_markups={
                                        MarkupDataVersions.TRANSCRIPT_AND_TYPE: [
                                            record3_channel2_bit2_markup1,
                                            record3_channel2_bit2_markup2,
                                        ],
                                    },
                                ),
                            ],
                        ),
                    ],
                )
            ],
            filtered_records_markups_data,
        )

        self.assertEqual(
            {
                'record1/1/bit1': MajorityVoteResult(
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=1, overall=1),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
                'record1/1/bit4': MajorityVoteResult(
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
                'record1/1/bit5': MajorityVoteResult(
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=3, overall=5),
                        overlap_strategy=None,
                    ),
                    text='с оплатой интернета',
                ),
                'record1/2/bit1': MajorityVoteResult(
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=1, overall=1),
                        overlap_strategy=None,
                    ),
                    text='меня зовут павел чем могу помочь',
                ),
                'record1/2/bit2': MajorityVoteResult(
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
                'record1/2/bit3': MajorityVoteResult(
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=1, overall=1),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
                'record1/2/bit5': MajorityVoteResult(
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=2, overall=2),
                        overlap_strategy=None,
                    ),
                    text='счета для проверки',
                ),
                'record1/2/bit6': MajorityVoteResult(
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
                'record3/1/bit1': MajorityVoteResult(
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
                'record3/1/bit2': MajorityVoteResult(
                    join_data=JoinDataNotOKCheckTranscriptNoSpeechMV(
                        votes=MajorityVote(majority=2, overall=2),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
                'record3/1/bit3': MajorityVoteResult(
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=3, overall=4),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
                'record3/2/bit3': MajorityVoteResult(
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=1, overall=1),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
                'record4/1/bit1': MajorityVoteResult(
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
                'record4/2/bit1': MajorityVoteResult(
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    text='',
                ),
                'record5/1/bit1': MajorityVoteResult(
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=2, overall=2),
                        overlap_strategy=None,
                    ),
                    text='',
                )
            },
            bit_id_to_majority_vote_result,
        )

    def test_marshal_input_for_markup_joiner(self):
        records_markups_data = group_records_bits_markups(markups_with_bits_data)
        filtered_records_markups_data, _ = produce_majority_votes_joins_and_filter_joiner_input(records_markups_data, {})

        self.assertEqual(
            """
1 record1/1/bit2
0 у меня такая
2 record1/1/bit3
0 такая проблемка
0 макая ракетку
5 record1/1/bit5
0 с оплатой интернета
0 с оплатой интернета
0 с оплатой интернета
0 с оплатой интернета
0 с оплатой интернета
3 record1/1/bit6
0 так
0 -
0 как
11 record1__01
3000 у меня такая
6000 такая проблемка
6000 макая ракетку
12000 с оплатой интернета
12000 с оплатой интернета
12000 с оплатой интернета
12000 с оплатой интернета
12000 с оплатой интернета
15000 так
15000 -
15000 как
1 record1/2/bit1
0 меня зовут павел чем могу помочь
2 record1/2/bit4
0 подскажите номер лицевого лота
0 подскажите номер ? счета
2 record1/2/bit5
0 счета для проверки
0 счета для проверки
5 record1__02
0 меня зовут павел чем могу помочь
9000 подскажите номер лицевого лота
9000 подскажите номер ? счета
12000 счета для проверки
12000 счета для проверки
1 record2/1/bit1
0 спасибо не надо
1 record2__01
0 спасибо не надо
1 record3/2/bit1
0 алло
2 record3/2/bit2
0 скажите пожалуйста
0 покажите ?
3 record3__02
0 алло
3000 скажите пожалуйста
3000 покажите ?
""".strip(),
            marshal_input_for_markup_joiner(filtered_records_markups_data),
        )

    def test_unmarshal_output_of_markup_joiner(self):
        self.assertEqual(
            {
                'record1/1/bit1': {1: 'здравствуйте добрый'},
                'record1/1/bit2': {1: 'день'},
                'record1/2/bit1': {1: 'чем могу помочь'},
                'record1/2/bit2': {1: 'помочь вам'},
                'record1': {1: 'здравствуйте добрый день', 2: 'чем могу помочь вам'},
                'record3/2/bit1': {1: 'алло'},
                'record3/2/bit2': {1: 'покажите пожалуйста'},
                'record3': {2: 'алло покажите пожалуйста'},
            },
            unmarshal_output_of_markup_joiner(
                """
record1/1/bit1 здравствуйте добрый
record1/1/bit2 день
record1/2/bit1 чем могу помочь
record1/2/bit2 помочь вам
record1__01 здравствуйте добрый день
record1__02 чем могу помочь вам
record3/2/bit1 алло
record3/2/bit2 покажите пожалуйста
record3__02 алло покажите пожалуйста
"""
            ),
        )

    @patch('cloud.ai.speechkit.stt.lib.data_pipeline.join.join.call_markups_joiner')
    def test_join_markups(self, call_markups_joiner):
        call_markups_joiner.return_value = {
            'record1/1/bit2': {1: 'у меня такая'},
            'record1/1/bit3': {1: 'такая проблемка'},
            'record1/1/bit5': {1: 'с оплатой интернета'},
            'record1/1/bit6': {1: 'как'},
            'record1/2/bit1': {1: 'меня зовут павел чем могу помочь'},
            'record1/2/bit4': {1: 'подскажите номер лицевого счета'},
            'record1/2/bit5': {1: 'счета для проверки'},
            'record1': {
                1: 'у меня такая проблемка с оплатой интернета как',
                2: 'меня зовут павел чем могу помочь подскажите номер лицевого счета для проверки',
            },
            'record2/1/bit1': {1: 'спасибо не надо'},
            'record2': {1: 'спасибо не надо'},
            'record3/2/bit1': {1: 'алло'},
            'record3/2/bit2': {1: 'покажите пожалуйста'},
            'record3': {2: 'алло покажите пожалуйста'},
        }

        received_at = datetime.fromisoformat('2019-12-12T00:00:00.000+03:00')

        joins = join_markups(
            markups_with_bit_data=markups_with_bits_data,
            markup_step_to_overlap_strategy={},
            markup_joiner_executable_path='./join',
            external_records_bits_joins=[],
            bit_offset=3000,
            received_at=received_at,
        )

        joins = sorted(joins, key=lambda join: join.record_id)

        self.assertEqual(
            [
                RecordJoin(
                    id='record1/2019-12-11T21:00:00+00:00',
                    record_id='record1',
                    recognition=MultiChannelRecognition(
                        channels=[
                            ChannelRecognition(
                                channel=1,
                                recognition=RecognitionPlainTranscript(
                                    text='у меня такая проблемка с оплатой интернета как',
                                ),
                            ),
                            ChannelRecognition(
                                channel=2,
                                recognition=RecognitionPlainTranscript(
                                    text='меня зовут павел чем могу помочь '
                                         'подскажите номер лицевого счета для проверки',
                                ),
                            ),
                        ],
                    ),
                    join_data=MultiChannelJoinData(
                        channels=[
                            ChannelJoinData(
                                channel=1,
                                join_data=JoinDataDpM3NaiveDev(),
                            ),
                            ChannelJoinData(
                                channel=2,
                                join_data=JoinDataDpM3NaiveDev(),
                            ),
                        ],
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record1/1/bit1/2019-12-11T21:00:00+00:00',
                    record_id='record1/1/bit1',
                    recognition=RecognitionPlainTranscript(text=''),
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=1, overall=1),
                        overlap_strategy=None,
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record1/1/bit2/2019-12-11T21:00:00+00:00',
                    record_id='record1/1/bit2',
                    recognition=RecognitionPlainTranscript(text='у меня такая'),
                    join_data=JoinDataDpM3NaiveDev(),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record1/1/bit3/2019-12-11T21:00:00+00:00',
                    record_id='record1/1/bit3',
                    recognition=RecognitionPlainTranscript(text='такая проблемка'),
                    join_data=JoinDataDpM3NaiveDev(),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record1/1/bit4/2019-12-11T21:00:00+00:00',
                    record_id='record1/1/bit4',
                    recognition=RecognitionPlainTranscript(text=''),
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record1/1/bit5/2019-12-11T21:00:00+00:00',
                    record_id='record1/1/bit5',
                    recognition=RecognitionPlainTranscript(text='с оплатой интернета'),
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=3, overall=5),
                        overlap_strategy=None,
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record1/1/bit6/2019-12-11T21:00:00+00:00',
                    record_id='record1/1/bit6',
                    recognition=RecognitionPlainTranscript(text='как'),
                    join_data=JoinDataDpM3NaiveDev(),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record1/2/bit1/2019-12-11T21:00:00+00:00',
                    record_id='record1/2/bit1',
                    recognition=RecognitionPlainTranscript(text='меня зовут павел чем могу помочь'),
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=1, overall=1),
                        overlap_strategy=None,
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record1/2/bit2/2019-12-11T21:00:00+00:00',
                    record_id='record1/2/bit2',
                    recognition=RecognitionPlainTranscript(text=''),
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record1/2/bit3/2019-12-11T21:00:00+00:00',
                    record_id='record1/2/bit3',
                    recognition=RecognitionPlainTranscript(text=''),
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=1, overall=1),
                        overlap_strategy=None,
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record1/2/bit4/2019-12-11T21:00:00+00:00',
                    record_id='record1/2/bit4',
                    recognition=RecognitionPlainTranscript(text='подскажите номер лицевого счета'),
                    join_data=JoinDataDpM3NaiveDev(),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record1/2/bit5/2019-12-11T21:00:00+00:00',
                    record_id='record1/2/bit5',
                    recognition=RecognitionPlainTranscript(text='счета для проверки'),
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=2, overall=2),
                        overlap_strategy=None,
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record1/2/bit6/2019-12-11T21:00:00+00:00',
                    record_id='record1/2/bit6',
                    recognition=RecognitionPlainTranscript(text=''),
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record2/2019-12-11T21:00:00+00:00',
                    record_id='record2',
                    recognition=RecognitionPlainTranscript(text='спасибо не надо'),
                    join_data=JoinDataDpM3NaiveDev(),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record2/1/bit1/2019-12-11T21:00:00+00:00',
                    record_id='record2/1/bit1',
                    recognition=RecognitionPlainTranscript(text='спасибо не надо'),
                    join_data=JoinDataDpM3NaiveDev(),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record3/2019-12-11T21:00:00+00:00',
                    record_id='record3',
                    recognition=MultiChannelRecognition(
                        channels=[
                            ChannelRecognition(
                                channel=1,
                                recognition=RecognitionPlainTranscript(text=''),
                            ),
                            ChannelRecognition(
                                channel=2,
                                recognition=RecognitionPlainTranscript(text='алло покажите пожалуйста'),
                            ),
                        ],
                    ),
                    join_data=MultiChannelJoinData(
                        channels=[
                            ChannelJoinData(
                                channel=1,
                                join_data=JoinDataNoSpeechAllBits(),
                            ),
                            ChannelJoinData(
                                channel=2,
                                join_data=JoinDataDpM3NaiveDev(),
                            ),
                        ],
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record3/1/bit1/2019-12-11T21:00:00+00:00',
                    record_id='record3/1/bit1',
                    recognition=RecognitionPlainTranscript(text=''),
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record3/1/bit2/2019-12-11T21:00:00+00:00',
                    record_id='record3/1/bit2',
                    recognition=RecognitionPlainTranscript(text=''),
                    join_data=JoinDataNotOKCheckTranscriptNoSpeechMV(
                        votes=MajorityVote(majority=2, overall=2),
                        overlap_strategy=None,
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record3/1/bit3/2019-12-11T21:00:00+00:00',
                    record_id='record3/1/bit3',
                    recognition=RecognitionPlainTranscript(text=''),
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=3, overall=4),
                        overlap_strategy=None,
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record3/2/bit1/2019-12-11T21:00:00+00:00',
                    record_id='record3/2/bit1',
                    recognition=RecognitionPlainTranscript(text='алло'),
                    join_data=JoinDataDpM3NaiveDev(),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record3/2/bit2/2019-12-11T21:00:00+00:00',
                    record_id='record3/2/bit2',
                    recognition=RecognitionPlainTranscript(text='покажите пожалуйста'),
                    join_data=JoinDataDpM3NaiveDev(),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record3/2/bit3/2019-12-11T21:00:00+00:00',
                    record_id='record3/2/bit3',
                    recognition=RecognitionPlainTranscript(text=''),
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=1, overall=1),
                        overlap_strategy=None,
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record4/2019-12-11T21:00:00+00:00',
                    record_id='record4',
                    recognition=MultiChannelRecognition(
                        channels=[
                            ChannelRecognition(
                                channel=1,
                                recognition=RecognitionPlainTranscript(text=''),
                            ),
                            ChannelRecognition(
                                channel=2,
                                recognition=RecognitionPlainTranscript(text=''),
                            ),
                        ],
                    ),
                    join_data=MultiChannelJoinData(
                        channels=[
                            ChannelJoinData(
                                channel=1,
                                join_data=JoinDataNoSpeechAllBits(),
                            ),
                            ChannelJoinData(
                                channel=2,
                                join_data=JoinDataNoSpeechAllBits(),
                            ),
                        ],
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record4/1/bit1/2019-12-11T21:00:00+00:00',
                    record_id='record4/1/bit1',
                    recognition=RecognitionPlainTranscript(text=''),
                    join_data=JoinDataOKCheckTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record4/2/bit1/2019-12-11T21:00:00+00:00',
                    record_id='record4/2/bit1',
                    recognition=RecognitionPlainTranscript(text=''),
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=2, overall=3),
                        overlap_strategy=None,
                    ),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record5/2019-12-11T21:00:00+00:00',
                    record_id='record5',
                    recognition=RecognitionPlainTranscript(text=''),
                    join_data=JoinDataNoSpeechAllBits(),
                    received_at=received_at,
                    other=None,
                ),
                RecordJoin(
                    id='record5/1/bit1/2019-12-11T21:00:00+00:00',
                    record_id='record5/1/bit1',
                    recognition=RecognitionPlainTranscript(text=''),
                    join_data=JoinDataNoSpeechTranscriptMV(
                        votes=MajorityVote(majority=2, overall=2),
                        overlap_strategy=None,
                    ),
                    received_at=received_at,
                    other=None,
                ),
            ],
            joins,
        )

        records_markups_data = group_records_bits_markups(markups_with_bits_data)
        filtered_records_markups_data, _ = produce_majority_votes_joins_and_filter_joiner_input(records_markups_data, {})

        call_markups_joiner.assert_called_with(
            './join',
            3000,
            filtered_records_markups_data,
        )
