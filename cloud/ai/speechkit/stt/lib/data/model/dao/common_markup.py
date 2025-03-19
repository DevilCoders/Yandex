import typing
from datetime import datetime

from dataclasses import dataclass
from enum import Enum

from cloud.ai.lib.python.datetime import format_datetime, parse_datetime
from cloud.ai.lib.python.serialization import YsonSerializable
from .common import S3Object
from .recognitions import RecognitionEndpoint, Recognition
from .records_joins import get_recognition


# https://yandex.ru/dev/toloka/doc/concepts/result-docpage/


@dataclass
class MarkupSolutionPlainTranscript:
    text: str
    misheard: bool

    def to_yson(self) -> typing.Dict:
        return {
            'text': self.text,
            'misheard': self.misheard,
        }

    @staticmethod
    def from_yson(fields: typing.Dict) -> 'MarkupSolutionPlainTranscript':
        return MarkupSolutionPlainTranscript(
            text=fields['text'],
            misheard=fields['misheard'],
        )


class MarkupTranscriptType(Enum):
    SPEECH = 'speech'  # sure with speech
    UNCLEAR = 'unclear'  # possibly speech, but illegible
    NO_SPEECH = 'no-speech'  # sure no speech

    @staticmethod
    def from_toloka_output(output: str) -> 'MarkupTranscriptType':
        t = toloka_output_to_transcript_type.get(output)
        if t is None:
            raise ValueError(f'Unexpected output type value: {output}')
        return t

    def to_toloka_output(self) -> str:
        return transcript_type_to_toloka_output[self]


toloka_output_to_transcript_type = {
    'sp': MarkupTranscriptType.SPEECH,
    'mis': MarkupTranscriptType.UNCLEAR,
    'si': MarkupTranscriptType.NO_SPEECH,
}

transcript_type_to_toloka_output = {v: k for k, v in toloka_output_to_transcript_type.items()}


@dataclass
class MarkupSolutionTranscriptAndType:
    text: str
    type: MarkupTranscriptType

    def to_yson(self) -> dict:
        return {
            'text': self.text,
            'type': self.type.value,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'MarkupSolutionTranscriptAndType':
        return MarkupSolutionTranscriptAndType(text=fields['text'], type=MarkupTranscriptType(fields['type']))


@dataclass
class MarkupSolutionCheckTranscript:
    ok: bool
    type: MarkupTranscriptType

    def to_yson(self) -> dict:
        return {
            'ok': self.ok,
            'type': self.type.value,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'MarkupSolutionCheckTranscript':
        return MarkupSolutionCheckTranscript(
            ok=fields['ok'],
            type=MarkupTranscriptType(fields['type']),
        )


# compare single text to reference, TRUE if meaning preserved
@dataclass
class TextMeaningComparisonSolution:
    result: bool

    def to_yson(self) -> dict:
        return {
            'result': self.result,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'TextMeaningComparisonSolution':
        return TextMeaningComparisonSolution(
            result=fields['result'],
        )


class SbSChoice(Enum):
    LEFT = 'LEFT'
    RIGHT = 'RIGHT'


# compare pair of texts to reference, choice for text which preserves meaning better
@dataclass
class TextMeaningSbSComparisonSolution:
    choice: SbSChoice

    def to_yson(self) -> dict:
        return {
            'choice': self.choice.value,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'TextMeaningSbSComparisonSolution':
        return TextMeaningSbSComparisonSolution(
            choice=SbSChoice(fields['choice']),
        )


# tmp ASREXP-778 stuff
@dataclass
class AudioEuphonyComparisonSolution:
    chosen_audio_url: str

    def to_yson(self) -> dict:
        return {
            'chosen_audio_url': self.chosen_audio_url,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'AudioEuphonyComparisonSolution':
        return AudioEuphonyComparisonSolution(
            chosen_audio_url=fields['chosen_audio_url'],
        )


MarkupSolution = typing.Union[
    MarkupSolutionPlainTranscript,
    MarkupSolutionTranscriptAndType,
    MarkupSolutionCheckTranscript,
    TextMeaningComparisonSolution,
    TextMeaningSbSComparisonSolution,
    AudioEuphonyComparisonSolution,
]


@dataclass
class KnownSolution:
    solution: MarkupSolution
    correctness_weight: float

    def to_yson(self) -> typing.Dict:
        return {
            'solution': self.solution.to_yson(),
            'correctness_weight': self.correctness_weight,
        }


@dataclass
class AudioURLInput:
    audio_s3_obj: S3Object

    def to_yson(self) -> dict:
        return {
            'audio_s3_obj': self.audio_s3_obj.to_yson(),
        }

    @staticmethod
    def from_yson(fields: dict) -> 'AudioURLInput':
        return AudioURLInput(
            audio_s3_obj=S3Object.from_yson(fields['audio_s3_obj']),
        )


@dataclass
class TranscriptSourceASR:
    recognition: Recognition
    recognition_endpoint: RecognitionEndpoint

    def to_yson(self) -> dict:
        return {
            'type': TranscriptSourceType.ASR.value,
            'recognition': self.recognition.to_yson(),
            'recognition_endpoint': self.recognition_endpoint.to_yson(),
        }

    @staticmethod
    def from_yson(fields: dict) -> 'TranscriptSourceASR':
        return TranscriptSourceASR(
            recognition=get_recognition(fields['recognition']),
            recognition_endpoint=RecognitionEndpoint.from_yson(fields['recognition_endpoint']),
        )


@dataclass
class TranscriptSourceJoin(YsonSerializable):
    record_join_id: str

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'type': TranscriptSourceType.JOIN.value}


class TranscriptSourceHoneypot(YsonSerializable):
    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'type': TranscriptSourceType.HONEYPOT.value}


class TranscriptSourceMarkup(YsonSerializable):
    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'type': TranscriptSourceType.MARKUP.value}


class TranscriptSourceType(Enum):
    ASR = 'ASR'
    JOIN = 'join'
    HONEYPOT = 'honeypot'
    MARKUP = 'markup'


TranscriptSource = typing.Union[
    TranscriptSourceASR,
    TranscriptSourceJoin,
    TranscriptSourceHoneypot,
    TranscriptSourceMarkup,
]


def get_text_source(fields: dict) -> TranscriptSource:
    try:
        return {
            TranscriptSourceType.ASR: TranscriptSourceASR,
            TranscriptSourceType.JOIN: TranscriptSourceJoin,
            TranscriptSourceType.HONEYPOT: TranscriptSourceHoneypot,
            TranscriptSourceType.MARKUP: TranscriptSourceMarkup,
        }[TranscriptSourceType(fields['type'])].from_yson(fields)
    except (ValueError, IndexError):
        raise ValueError(f'Unsupported text source: {fields}')


@dataclass
class AudioURLAndTranscriptInput:
    audio_s3_obj: S3Object
    text: str
    text_source: TranscriptSource

    def to_yson(self) -> dict:
        return {
            'audio_s3_obj': self.audio_s3_obj.to_yson(),
            'text': self.text,
            'text_source': self.text_source.to_yson(),
        }

    @staticmethod
    def from_yson(fields: dict) -> 'AudioURLAndTranscriptInput':
        return AudioURLAndTranscriptInput(
            audio_s3_obj=S3Object.from_yson(fields['audio_s3_obj']),
            text=fields['text'],
            text_source=get_text_source(fields['text_source']),
        )


@dataclass
class TextMeaningComparisonInput:
    reference: str
    hypothesis: str

    def to_yson(self) -> dict:
        return {
            'reference': self.reference,
            'hypothesis': self.hypothesis,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'TextMeaningComparisonInput':
        return TextMeaningComparisonInput(
            reference=fields['reference'],
            hypothesis=fields['hypothesis'],
        )


# hypotheses will be shuffled by Toloka and real state
# will be selected during assignments download
@dataclass
class TextMeaningSbSComparisonInput:
    reference: str
    hypothesis_left: str
    hypothesis_right: str

    def to_yson(self) -> dict:
        return {
            'reference': self.reference,
            'hypothesis_left': self.hypothesis_left,
            'hypothesis_right': self.hypothesis_right,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'TextMeaningSbSComparisonInput':
        return TextMeaningSbSComparisonInput(
            reference=fields['reference'],
            hypothesis_left=fields['hypothesis_left'],
            hypothesis_right=fields['hypothesis_right'],
        )


# tmp ASREXP-778 stuff
@dataclass
class AudioEuphonyComparisonInput:
    left_audio_url: str
    right_audio_url: str

    def to_yson(self) -> dict:
        return {
            'left_audio_url': self.left_audio_url,
            'right_audio_url': self.right_audio_url,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'AudioEuphonyComparisonInput':
        return AudioEuphonyComparisonInput(
            left_audio_url=fields['left_audio_url'],
            right_audio_url=fields['right_audio_url'],
        )


MarkupInput = typing.Union[
    AudioURLInput,
    AudioURLAndTranscriptInput,
    TextMeaningComparisonInput,
    TextMeaningSbSComparisonInput,
    AudioEuphonyComparisonInput,
]


class MarkupDataVersions(Enum):
    PLAIN_TRANSCRIPT = 'plain-transcript'
    TRANSCRIPT_AND_TYPE = 'transcript-and-type'
    CHECK_TRANSCRIPT = 'check-transcript'
    COMPARE_TEXT_MEANING = 'compare-text-meaning'
    COMPARE_TEXT_MEANING_SBS = 'compare-text-meaning-sbs'
    COMPARE_AUDIO_EUPHONY = 'compare-audio-euphony'


# This enum contains conceptual markup step, while MarkupDataVersions
# corresponds to some Toloka project, which implements markup step.
class MarkupStep(Enum):
    TRANSCRIPT = 'transcript'
    CHECK_ASR_TRANSCRIPT = 'check-asr-transcript'
    CHECK_TRANSCRIPT = 'check-transcript'
    CLASSIFICATION = 'base'
    QUALITY_EVALUATION = 'quality-evaluation'
    COMPARE_TEXT_MEANING = 'compare-text-meaning'
    COMPARE_TEXT_MEANING_SBS = 'compare-text-meaning-sbs'
    COMPARE_AUDIO_EUPHONY = 'compare-audio-euphony'


active_transcript_markup_version = MarkupDataVersions.TRANSCRIPT_AND_TYPE


@dataclass
class MarkupData:
    version: MarkupDataVersions
    input: MarkupInput
    solution: MarkupSolution
    known_solutions: typing.List[KnownSolution]
    task_id: str
    overlap: int
    raw_data: typing.Optional[dict]  # optional because old markup data don't have this field
    created_at: datetime

    def to_yson(self) -> dict:
        return {
            'version': self.version.value,
            'input': self.input.to_yson(),
            'solution': self.solution.to_yson(),
            'known_solutions': [x.to_yson() for x in self.known_solutions],
            'task_id': self.task_id,
            'overlap': self.overlap,
            'raw_data': self.raw_data,
            'created_at': format_datetime(self.created_at),
        }

    @staticmethod
    def from_yson(fields: dict) -> 'MarkupData':
        version_field = fields['version']
        if version_field == MarkupDataVersions.PLAIN_TRANSCRIPT.value:
            version = MarkupDataVersions.PLAIN_TRANSCRIPT
            input_type = AudioURLInput
            solution_type = MarkupSolutionPlainTranscript
        elif version_field == MarkupDataVersions.TRANSCRIPT_AND_TYPE.value:
            version = MarkupDataVersions.TRANSCRIPT_AND_TYPE
            input_type = AudioURLInput
            solution_type = MarkupSolutionTranscriptAndType
        elif version_field == MarkupDataVersions.CHECK_TRANSCRIPT.value:
            version = MarkupDataVersions.CHECK_TRANSCRIPT
            input_type = AudioURLAndTranscriptInput
            solution_type = MarkupSolutionCheckTranscript
        elif version_field == MarkupDataVersions.COMPARE_TEXT_MEANING.value:
            version = MarkupDataVersions.COMPARE_TEXT_MEANING
            input_type = TextMeaningComparisonInput
            solution_type = TextMeaningComparisonSolution
        elif version_field == MarkupDataVersions.COMPARE_TEXT_MEANING_SBS.value:
            version = MarkupDataVersions.COMPARE_TEXT_MEANING_SBS
            input_type = TextMeaningSbSComparisonInput
            solution_type = TextMeaningSbSComparisonSolution
        elif version_field == MarkupDataVersions.COMPARE_AUDIO_EUPHONY.value:
            version = MarkupDataVersions.COMPARE_AUDIO_EUPHONY
            input_type = AudioEuphonyComparisonInput
            solution_type = AudioEuphonyComparisonSolution
        else:
            raise ValueError(f'Unsupported markup data: {fields["markup_data"]}')

        input = input_type.from_yson(fields['input'])
        solution = solution_type.from_yson(fields['solution'])
        known_solutions = [
            KnownSolution(solution=solution_type.from_yson(x['solution']), correctness_weight=x['correctness_weight'])
            for x in fields['known_solutions']
        ]

        return MarkupData(
            version=version,
            input=input,
            solution=solution,
            known_solutions=known_solutions,
            task_id=fields['task_id'],
            overlap=fields['overlap'],
            raw_data=fields.get('raw_data'),
            created_at=parse_datetime(fields['created_at']),
        )
