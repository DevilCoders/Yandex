import typing
from dataclasses import dataclass
from datetime import datetime
from enum import Enum

from cloud.ai.lib.python.datetime import format_datetime
from cloud.ai.lib.python.serialization import YsonSerializable, OrderedYsonSerializable
from cloud.ai.speechkit.stt.lib.data.model.common.id import generate_id
from .records import Record


class RecognitionVersions(Enum):
    PLAIN_TRANSCRIPT = 'plain-transcript'
    RAW_TEXT = 'raw-text'


# Lower-cased text without punctuation with words separated by whitespaces.
@dataclass
class RecognitionPlainTranscript(YsonSerializable):
    text: str

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': RecognitionVersions.PLAIN_TRANSCRIPT.value}


# Raw text (may contain any characters).
@dataclass
class RecognitionRawText(YsonSerializable):
    text: str

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': RecognitionVersions.RAW_TEXT.value}


MonoChannelRecognition = typing.Union[RecognitionPlainTranscript, RecognitionRawText]


def get_mono_channel_recognition(fields: dict) -> MonoChannelRecognition:
    try:
        return {
            RecognitionVersions.PLAIN_TRANSCRIPT: RecognitionPlainTranscript,
            RecognitionVersions.RAW_TEXT: RecognitionRawText,
        }[RecognitionVersions(fields['version'])].from_yson(fields)
    except (KeyError, ValueError):
        raise ValueError(f'unsupported mono channel recognition: {fields}')


@dataclass
class ChannelRecognition(YsonSerializable):
    channel: int
    recognition: MonoChannelRecognition

    @staticmethod
    def deserialization_overrides() -> typing.Dict[str, typing.Callable]:
        return {
            'recognition': get_mono_channel_recognition,
        }


@dataclass
class MultiChannelRecognition(YsonSerializable):
    channels: typing.List[ChannelRecognition]

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'multichannel': True}


Recognition = typing.Union[MonoChannelRecognition, MultiChannelRecognition]


def get_recognition(fields: dict) -> Recognition:
    if fields.get('multichannel', False):
        return MultiChannelRecognition.from_yson(fields)
    else:
        return get_mono_channel_recognition(fields)


class JoinDataVersions(Enum):
    # ground truth
    EXEMPLAR = 'exemplar'

    # dynamic programming, expecting [bit_length / bit_offset := 3], equal edge weights
    # (not depend from markup worker), develop version without tests
    DP_M3_NAIVE_DEV = 'dp-m3-naive:dev'

    # text is evaluated in feedback loop v0.0
    FEEDBACK_LOOP_V_0_0 = 'feedback-loop-v0.0'

    # majority of record bit transcript markups indicated lack of speech
    NO_SPEECH_TRANSCRIPT_MV = 'no-speech-ts-mv'

    # DEPRECATED: now "no speech" decisions can be made from different markup versions, not only transcript.
    # all bits of record have majority votes for lack of speech due to transcript markups
    NO_SPEECH_TRANSCRIPT_ALL_BITS = 'no-speech-ts-ab'

    # all bits of record have majority votes for lack of speech
    NO_SPEECH_ALL_BITS = 'no-speech-ab'

    # majority of record bit transcript markups indicated that transcript is valid (either empty or with text)
    CHECK_TRANSCRIPT_OK_MV = 'ct-ok-mv'

    # majority of record bit transcript markups indicated that transcript is not valid and it has no speech
    CHECK_TRANSCRIPT_NOT_OK_NO_SPEECH_MV = 'ct-not-ok-no-speech-mv'

    # imported text from voicetable (converted from original)
    VOICETABLE_REFERENCE_RU = 'voicetable-ref-ru'

    # imported original text from voicetable
    VOICETABLE_REFERENCE_ORIG = 'voicetable-ref-orig'

    # imported text from voice recorder audios
    VOICE_RECORDER = 'voice-recorder'

    # some external source of text, may be bad quality or not properly processed texts
    EXTERNAL = 'external'


@dataclass
class JoinDataVoiceRecorder(YsonSerializable):
    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': JoinDataVersions.VOICE_RECORDER.value}


@dataclass
class JoinDataVoicetableReferenceRu(YsonSerializable):
    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': JoinDataVersions.VOICETABLE_REFERENCE_RU.value}


@dataclass
class JoinDataVoicetableReferenceOrig(YsonSerializable):
    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': JoinDataVersions.VOICETABLE_REFERENCE_ORIG.value}


@dataclass
class JoinDataExemplar(YsonSerializable):
    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': JoinDataVersions.EXEMPLAR.value}


@dataclass
class JoinDataDpM3NaiveDev(YsonSerializable):
    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': JoinDataVersions.DP_M3_NAIVE_DEV.value}


@dataclass
class JoinDataFeedbackLoop(YsonSerializable):
    confidence: typing.Optional[float]
    assignment_accuracy: float
    assignment_evaluation_recall: float
    attempts: int  # how many attempts for this bit was performed; this join data corresponds to single attempt

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': JoinDataVersions.FEEDBACK_LOOP_V_0_0.value}

    def __lt__(self, other: 'JoinDataFeedbackLoop'):
        # TODO: sort logic differs from one in feedback loop: text without evaluations are pessimized
        if self.confidence is None and other.confidence is not None:
            return True
        if self.confidence is not None and other.confidence is None:
            return False
        if self.confidence == other.confidence:
            def f1(p, r):
                return 2 * (p * r) / (p + r)

            return f1(self.assignment_accuracy, self.assignment_evaluation_recall) < \
                   f1(other.assignment_accuracy, other.assignment_evaluation_recall)
        else:
            return self.confidence < other.confidence


@dataclass
class JoinDataExternal(YsonSerializable):
    comment: str

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': JoinDataVersions.EXTERNAL.value}


@dataclass
class MajorityVote(YsonSerializable):
    majority: int
    overall: int


class OverlapType(Enum):
    STATIC = 'static'
    DYNAMIC = 'dynamic'


class OverlapStrategyType(Enum):
    SIMPLE = 'simple'


@dataclass
class SimpleStaticOverlapStrategy(YsonSerializable):
    min_majority: int
    overall: int

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {
            'overlap_type': OverlapType.STATIC.value,
            'strategy': OverlapStrategyType.SIMPLE.value,
        }

    def check_majority_vote(self, mv: MajorityVote) -> bool:
        if mv.overall > self.overall:
            # TODO: it can happen because of redundant overlap due to assignments construction; handle this properly
            print(f'result overlap {mv.overall} is greater then target {self.overall}')
        elif mv.overall < self.overall:
            # TODO: investigate why this happening
            print(f'WARN: result overlap {mv.overall} is LESS then target {self.overall}')
            return False
        return mv.majority >= self.min_majority


OverlapStrategy = typing.Union[SimpleStaticOverlapStrategy]


def get_overlap_strategy(fields: dict) -> typing.Optional[OverlapStrategy]:
    overlap_strategy = fields.get('overlap_strategy')
    if overlap_strategy is None:
        return None
    overlap_type = OverlapType(overlap_strategy['overlap_type'])
    strategy = OverlapStrategyType(overlap_strategy['strategy'])
    if overlap_type == OverlapType.STATIC and strategy == OverlapStrategyType.SIMPLE:
        return SimpleStaticOverlapStrategy(
            min_majority=overlap_strategy['min_majority'],
            overall=overlap_strategy['overall'],
        )
    else:
        raise ValueError(f'Unexpected overlap strategy with {overlap_type} overlap and {strategy} strategy')


@dataclass
class JoinDataNoSpeechTranscriptMV(YsonSerializable):
    votes: MajorityVote
    overlap_strategy: typing.Optional[OverlapStrategy]

    def to_yson(self) -> dict:
        fields = {
            'votes': self.votes.to_yson(),
            'version': JoinDataVersions.NO_SPEECH_TRANSCRIPT_MV.value,
        }
        if self.overlap_strategy is not None:
            fields['overlap_strategy'] = self.overlap_strategy.to_yson()
        return fields

    @classmethod
    def from_yson(cls, fields: dict) -> 'JoinDataNoSpeechTranscriptMV':
        return JoinDataNoSpeechTranscriptMV(
            votes=MajorityVote.from_yson(fields['votes']),
            overlap_strategy=get_overlap_strategy(fields),
        )


@dataclass
class JoinDataOKCheckTranscriptMV(YsonSerializable):
    votes: MajorityVote
    overlap_strategy: typing.Optional[OverlapStrategy]

    def to_yson(self) -> dict:
        fields = {
            'votes': self.votes.to_yson(),
            'version': JoinDataVersions.CHECK_TRANSCRIPT_OK_MV.value,
        }
        if self.overlap_strategy is not None:
            fields['overlap_strategy'] = self.overlap_strategy.to_yson()
        return fields

    @classmethod
    def from_yson(cls, fields: dict) -> 'JoinDataOKCheckTranscriptMV':
        return JoinDataOKCheckTranscriptMV(
            votes=MajorityVote.from_yson(fields['votes']),
            overlap_strategy=get_overlap_strategy(fields),
        )


@dataclass
class JoinDataNotOKCheckTranscriptNoSpeechMV(YsonSerializable):
    votes: MajorityVote
    overlap_strategy: typing.Optional[OverlapStrategy]

    def to_yson(self) -> dict:
        fields = {
            'votes': self.votes.to_yson(),
            'version': JoinDataVersions.CHECK_TRANSCRIPT_NOT_OK_NO_SPEECH_MV.value,
        }
        if self.overlap_strategy is not None:
            fields['overlap_strategy'] = self.overlap_strategy.to_yson()
        return fields

    @classmethod
    def from_yson(cls, fields: dict) -> 'JoinDataNotOKCheckTranscriptNoSpeechMV':
        return JoinDataNotOKCheckTranscriptNoSpeechMV(
            votes=MajorityVote.from_yson(fields['votes']),
            overlap_strategy=get_overlap_strategy(fields),
        )


# DEPRECATED
@dataclass
class JoinDataNoSpeechTranscriptAllBits(YsonSerializable):
    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': JoinDataVersions.NO_SPEECH_TRANSCRIPT_ALL_BITS.value}


@dataclass
class JoinDataNoSpeechAllBits(YsonSerializable):
    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': JoinDataVersions.NO_SPEECH_ALL_BITS.value}


MonoChannelJoinData = typing.Union[
    JoinDataExemplar,
    JoinDataDpM3NaiveDev,
    JoinDataNoSpeechTranscriptMV,
    JoinDataOKCheckTranscriptMV,
    JoinDataNotOKCheckTranscriptNoSpeechMV,
    JoinDataNoSpeechTranscriptAllBits,  # DEPRECATED
    JoinDataNoSpeechAllBits,
    JoinDataVoicetableReferenceRu,
    JoinDataVoicetableReferenceOrig,
    JoinDataVoiceRecorder,
    JoinDataFeedbackLoop,
]


def get_mono_channel_join_data(fields: dict) -> MonoChannelJoinData:
    try:
        return {
            JoinDataVersions.EXEMPLAR: JoinDataExemplar,
            JoinDataVersions.EXTERNAL: JoinDataExternal,
            JoinDataVersions.DP_M3_NAIVE_DEV: JoinDataDpM3NaiveDev,
            JoinDataVersions.NO_SPEECH_TRANSCRIPT_MV: JoinDataNoSpeechTranscriptMV,
            JoinDataVersions.CHECK_TRANSCRIPT_OK_MV: JoinDataOKCheckTranscriptMV,
            JoinDataVersions.CHECK_TRANSCRIPT_NOT_OK_NO_SPEECH_MV: JoinDataNotOKCheckTranscriptNoSpeechMV,
            JoinDataVersions.NO_SPEECH_TRANSCRIPT_ALL_BITS: JoinDataNoSpeechTranscriptAllBits,
            JoinDataVersions.NO_SPEECH_ALL_BITS: JoinDataNoSpeechAllBits,
            JoinDataVersions.VOICETABLE_REFERENCE_RU: JoinDataVoicetableReferenceRu,
            JoinDataVersions.VOICETABLE_REFERENCE_ORIG: JoinDataVoicetableReferenceOrig,
            JoinDataVersions.VOICE_RECORDER: JoinDataVoiceRecorder,
            JoinDataVersions.FEEDBACK_LOOP_V_0_0: JoinDataFeedbackLoop,
        }[JoinDataVersions(fields['version'])].from_yson(fields)
    except (KeyError, ValueError):
        raise ValueError(f'unsupported mono channel join data: {fields}')


@dataclass
class ChannelJoinData(YsonSerializable):
    channel: int
    join_data: MonoChannelJoinData

    @staticmethod
    def deserialization_overrides() -> typing.Dict[str, typing.Callable]:
        return {
            'join_data': get_mono_channel_join_data,
        }


@dataclass
class MultiChannelJoinData(YsonSerializable):
    channels: typing.List[ChannelJoinData]

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'multichannel': True}


JoinData = typing.Union[MonoChannelJoinData, MultiChannelJoinData]


def get_join_data(fields: dict) -> JoinData:
    if fields.get('multichannel', False):
        return MultiChannelJoinData.from_yson(fields)
    else:
        return get_mono_channel_join_data(fields)


@dataclass
class RecordJoin(OrderedYsonSerializable):
    id: str
    record_id: str
    recognition: Recognition
    join_data: JoinData
    received_at: datetime
    other: typing.Optional[dict]

    @staticmethod
    def primary_key() -> typing.List[str]:
        return ['record_id', 'id']

    @staticmethod
    def deserialization_overrides() -> typing.Dict[str, typing.Callable]:
        return {
            'recognition': get_recognition,
            'join_data': get_join_data,
        }

    @staticmethod
    def create_by_record(
        record: Record, recognition: Recognition, join_data: JoinData, received_at: datetime, other: typing.Any
    ) -> 'RecordJoin':
        return RecordJoin.create_by_record_id(record.id, recognition, join_data, received_at, other)

    @staticmethod
    def create_by_record_id(
        record_id: str, recognition: Recognition, join_data: JoinData, received_at: datetime, other: typing.Any,
        multiple_per_moment: bool = False,
    ) -> 'RecordJoin':
        id = f'{record_id}/{format_datetime(received_at)}'
        if multiple_per_moment:
            # multiple joins with same received_at; we have to add something to distinguish
            id = f'{id}/{generate_id()}'
        return RecordJoin(
            id=id,
            record_id=record_id,
            recognition=recognition,
            join_data=join_data,
            received_at=received_at,
            other=other,
        )

    def has_better_quality(self, other: 'RecordJoin') -> typing.Optional[bool]:
        if all(isinstance(j.join_data, JoinDataFeedbackLoop) for j in (self, other)):
            return self.join_data > other.join_data
        return None


def get_channels_texts(recognition: Recognition) -> typing.List[str]:
    if isinstance(recognition, MultiChannelRecognition):
        # channel recognitions should be already sorted by channel, but let's sort them just in case
        channels = sorted(recognition.channels, key=lambda r: r.channel)
        return [
            channel.recognition.text for channel in channels
        ]
    else:
        return [recognition.text]
