from dataclasses import dataclass
from datetime import datetime
from enum import Enum
import typing

from cloud.ai.lib.python.serialization import YsonSerializable, OrderedYsonSerializable
from cloud.ai.lib.python.datasource.yt.model import TableMeta, generate_attrs


class AudioSourceType(Enum):
    # Заранее подготовленная корзинка аудио, для каждого аудио известно, что с ним не так.
    BUCKET = 'bucket'

    # Синтез из продакшен-модели, модели из эксперимента, и т.д.
    SYNTH = 'synth'


@dataclass
class BucketAudioSource(YsonSerializable):
    category: str
    damage_amount: str

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'type': AudioSourceType.BUCKET.value}


@dataclass
class SynthAudioSource(YsonSerializable):
    model_id: str

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'type': AudioSourceType.SYNTH.value}


AudioSource = typing.Union[BucketAudioSource, SynthAudioSource]


def get_audio_source(fields: dict) -> AudioSource:
    try:
        return {
            AudioSourceType.BUCKET: BucketAudioSource,
            AudioSourceType.SYNTH: SynthAudioSource,
        }[AudioSourceType(fields['type'])].from_yson(fields)
    except (KeyError, ValueError):
        raise ValueError(f'unsupported audio source: {fields}')


# Flatten fields and timestamps for DataLens.
# In case of new audio sources and you need them in DataLens, add their fields here.
# IMPORTANT: DataLens field names from different audio sources should be unique.
audio_annotation_datalens_fields = [
    {
        'name': 'audio_category',
        'type': 'string',
    },
    {
        'name': 'audio_damage_amount',
        'type': 'string',
    },
    {
        'name': 'audio_model_id',
        'type': 'string',
    },
    {
        'name': 'audio_source_type',
        'type': 'string',
    },
    {
        'name': 'received_at_ts',
        'type': 'int64',
        'required': True,
    },
]


def add_datalens_audio_source_field(
    yson: dict,
    datalens_field_name: str,
    audio_source: AudioSource,
    field_prefix: str = 'audio_',
):
    if datalens_field_name[len(field_prefix):] in audio_source.__dict__:
        # flatten this audio source
        yson[datalens_field_name] = audio_source.__dict__[datalens_field_name[len(field_prefix):]]
    else:
        # nulls for other audio sources
        yson[datalens_field_name] = None


@dataclass
class AudioAnnotation(OrderedYsonSerializable):
    id: str
    assignment_id: str
    audio_id: str
    question: str
    answer: bool
    audio_source: AudioSource  # TODO: temporary field, until we have OR model
    received_at: datetime

    @staticmethod
    def primary_key():
        return ['audio_id', 'id']

    def to_yson(self) -> dict:
        yson = super(AudioAnnotation, self).to_yson()
        for datalens_field_name in [field['name'] for field in audio_annotation_datalens_fields]:
            if datalens_field_name == 'received_at_ts':
                yson[datalens_field_name] = int(self.received_at.timestamp())
            elif datalens_field_name == 'audio_source_type':
                yson[datalens_field_name] = self.audio_source.to_yson()['type']
            else:
                add_datalens_audio_source_field(yson, datalens_field_name, self.audio_source)
        return yson

    @staticmethod
    def deserialization_overrides() -> typing.Dict[str, typing.Callable]:
        return {
            'audio_source': get_audio_source,
        }


table_audio_annotations_meta = TableMeta(
    dir_path='//home/mlcloud/speechkit/tts/audio_annotation_experiment/audio_annotations',
    attrs=generate_attrs(AudioAnnotation, required={'received_at'}),
)
table_audio_annotations_meta.attrs['schema']['$value'] += audio_annotation_datalens_fields


class SbSChoice(Enum):
    LEFT = 'left'
    RIGHT = 'right'


# Flatten fields and timestamps for DataLens.
# In case of new audio sources and you need them in DataLens, add their fields here.
# IMPORTANT: DataLens field names from different audio sources should be unique.
audio_sbs_choice_datalens_fields = [
    {
        'name': 'audio_left_category',
        'type': 'string',
    },
    {
        'name': 'audio_left_damage_amount',
        'type': 'string',
    },
    {
        'name': 'audio_left_model_id',
        'type': 'string',
    },
    {
        'name': 'audio_left_source_type',
        'type': 'string',
    },
    {
        'name': 'audio_right_category',
        'type': 'string',
    },
    {
        'name': 'audio_right_damage_amount',
        'type': 'string',
    },
    {
        'name': 'audio_right_model_id',
        'type': 'string',
    },
    {
        'name': 'audio_right_source_type',
        'type': 'string',
    },
    {
        'name': 'received_at_ts',
        'type': 'int64',
        'required': True,
    },
]


@dataclass
class AudioSbSChoice(OrderedYsonSerializable):
    id: str
    assignment_id: str
    audio_left_id: str
    audio_right_id: str
    choice: SbSChoice
    audio_left_source: AudioSource  # TODO: temporary field, until we have OR model
    audio_right_source: AudioSource  # TODO: temporary field, until we have OR model
    received_at: datetime

    @staticmethod
    def primary_key():
        return ['audio_left_id', 'audio_right_id', 'id']

    def to_yson(self) -> dict:
        yson = super(AudioSbSChoice, self).to_yson()
        for datalens_field_name in [field['name'] for field in audio_sbs_choice_datalens_fields]:
            if datalens_field_name == 'received_at_ts':
                yson[datalens_field_name] = int(self.received_at.timestamp())
            elif datalens_field_name == 'audio_left_source_type':
                yson[datalens_field_name] = self.audio_left_source.to_yson()['type']
            elif datalens_field_name == 'audio_right_source_type':
                yson[datalens_field_name] = self.audio_right_source.to_yson()['type']
            elif datalens_field_name.startswith('audio_left_'):
                add_datalens_audio_source_field(yson, datalens_field_name, self.audio_left_source, 'audio_left_')
            else:
                add_datalens_audio_source_field(yson, datalens_field_name, self.audio_right_source, 'audio_right_')
        return yson

    @staticmethod
    def deserialization_overrides() -> typing.Dict[str, typing.Callable]:
        return {
            'audio_left_source': get_audio_source,
            'audio_right_source': get_audio_source,
        }


table_audio_sbs_choices_meta = TableMeta(
    dir_path='//home/mlcloud/speechkit/tts/audio_annotation_experiment/audio_sbs_choices',
    attrs=generate_attrs(AudioSbSChoice, required={'received_at'}),
)
table_audio_sbs_choices_meta.attrs['schema']['$value'] += audio_sbs_choice_datalens_fields
