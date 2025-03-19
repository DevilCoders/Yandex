from dataclasses import dataclass
from datetime import datetime
from enum import Enum
import typing

from cloud.ai.lib.python.datetime import format_datetime, parse_datetime
from cloud.ai.lib.python.serialization import YsonSerializable
from cloud.ai.speechkit.stt.lib.data.model.common.id import generate_id
from cloud.ai.speechkit.stt.lib.data.model.common.hash import crc32
from .common import HashVersion
from .common_markup import MarkupData, MarkupStep
from .records_bits import RecordBit, RecordBitAudioParams


class AudioObfuscationVersions(Enum):
    SOX_EFFECTS = 'sox-effects'
    SOX_EFFECTS_V2 = 'sox-effects-v2'


@dataclass
class SoxEffectsObfuscationData:
    sox_effects: str

    def to_yson(self) -> typing.Dict:
        return {
            'version': AudioObfuscationVersions.SOX_EFFECTS.value,
            'sox_effects': self.sox_effects,
        }

    @staticmethod
    def from_yson(fields: typing.Dict) -> 'SoxEffectsObfuscationData':
        return SoxEffectsObfuscationData(
            sox_effects=fields['sox_effects'],
        )


@dataclass
class SoxEffectsObfuscationDataV2:
    convert_cmd: str
    pitch: int
    dbfs_before: float
    dbfs_after: float
    reduce_volume_cmd: typing.Optional[str]
    dbfs_after_reduced: typing.Optional[float]

    def to_yson(self) -> dict:
        y = {
            'version': AudioObfuscationVersions.SOX_EFFECTS_V2.value,
            'convert_cmd': self.convert_cmd,
            'pitch': self.pitch,
            'dbfs_before': self.dbfs_before,
            'dbfs_after': self.dbfs_after,
        }
        if self.reduce_volume_cmd is not None:
            y['reduce_volume_cmd'] = self.reduce_volume_cmd
            y['dbfs_after_reduced'] = self.dbfs_after_reduced
        return y

    @staticmethod
    def from_yson(fields: dict) -> 'SoxEffectsObfuscationDataV2':
        return SoxEffectsObfuscationDataV2(
            convert_cmd=fields['convert_cmd'],
            pitch=fields['pitch'],
            dbfs_before=fields['dbfs_before'],
            dbfs_after=fields['dbfs_after'],
            reduce_volume_cmd=fields.get('reduce_volume_cmd'),
            dbfs_after_reduced=fields.get('dbfs_after_reduced'),
        )

    @staticmethod
    def create(convert_cmd: str, bit_obfuscation_data: dict, reduce_volume_cmd: str) -> 'SoxEffectsObfuscationDataV2':
        return SoxEffectsObfuscationDataV2(
            convert_cmd=convert_cmd,
            pitch=bit_obfuscation_data['pitch'],
            dbfs_before=bit_obfuscation_data['dbfs_before'],
            dbfs_after=bit_obfuscation_data['dbfs_after'],
            reduce_volume_cmd=reduce_volume_cmd
            if bit_obfuscation_data.get('dbfs_after_reduced') is not None
            else None,
            dbfs_after_reduced=bit_obfuscation_data.get('dbfs_after_reduced'),
        )


AudioObfuscationData = typing.Union[SoxEffectsObfuscationData, SoxEffectsObfuscationDataV2]


class RecordBitMarkupValidationDataType(Enum):
    FEEDBACK_LOOP = 'feedback-loop'


@dataclass
class RecordBitMarkupFeedbackLoopValidationData(YsonSerializable):
    checks_ids: typing.List[str]  # RecordBitMarkup.id list which check this one

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'type': RecordBitMarkupValidationDataType.FEEDBACK_LOOP.value}


RecordBitMarkupValidationData = typing.Union[RecordBitMarkupFeedbackLoopValidationData]


def get_record_bit_markup_validation_data(fields: dict) -> RecordBitMarkupValidationData:
    try:
        return {
            RecordBitMarkupValidationDataType.FEEDBACK_LOOP: RecordBitMarkupFeedbackLoopValidationData,
        }[RecordBitMarkupValidationDataType(fields['type'])].from_yson(fields)
    except (ValueError, IndexError):
        raise ValueError(f'unexpected record bit markup validation data: {fields}')


@dataclass
class RecordBitMarkup:
    id: str
    bit_id: str
    record_id: str
    audio_obfuscation_data: AudioObfuscationData
    audio_params: RecordBitAudioParams
    markup_id: str
    markup_step: MarkupStep
    pool_id: str
    assignment_id: str
    markup_data: MarkupData
    validation_data: typing.Optional[RecordBitMarkupValidationData]
    received_at: datetime
    other: typing.Any

    def __lt__(self, other: 'RecordBitMarkup'):
        if self.record_id == other.record_id:
            if self.bit_id == other.bit_id:
                return self.id < other.id
            else:
                return self.bit_id < other.bit_id
        else:
            return self.record_id < other.record_id

    def to_yson(self) -> dict:
        return {
            'id': self.id,
            'bit_id': self.bit_id,
            'record_id': self.record_id,
            'audio_obfuscation_data': self.audio_obfuscation_data.to_yson(),
            'audio_params': self.audio_params.to_yson(),
            'markup_id': self.markup_id,
            'markup_step': self.markup_step.value,
            'pool_id': self.pool_id,
            'assignment_id': self.assignment_id,
            'markup_data': self.markup_data.to_yson(),
            'validation_data': None if self.validation_data is None else self.validation_data.to_yson(),
            'received_at': format_datetime(self.received_at),
            'other': self.other,
        }

    @staticmethod
    def from_yson(fields: dict) -> 'RecordBitMarkup':
        return RecordBitMarkup(
            id=fields['id'],
            bit_id=fields['bit_id'],
            record_id=fields['record_id'],
            audio_obfuscation_data=get_audio_obfuscation_data(fields['audio_obfuscation_data']),
            audio_params=RecordBitAudioParams.from_yson(fields['audio_params']),
            markup_id=fields['markup_id'],
            markup_step=MarkupStep(fields['markup_step']),
            pool_id=fields['pool_id'],
            assignment_id=fields['assignment_id'],
            markup_data=MarkupData.from_yson(fields['markup_data']),
            validation_data=None if fields['validation_data'] is None
            else get_record_bit_markup_validation_data(fields['validation_data']),
            received_at=parse_datetime(fields['received_at']),
            other=fields['other'],
        )

    @staticmethod
    def create(
        record_bit: RecordBit,
        audio_obfuscation_data: AudioObfuscationData,
        markup_id: str,
        markup_step: MarkupStep,
        pool_id: str,
        assignment_id: str,
        markup_data: MarkupData,
        received_at: datetime,
        validation_data: RecordBitMarkupValidationData = None,
    ) -> 'RecordBitMarkup':
        return RecordBitMarkup(
            id='%s/%s' % (record_bit.id, generate_id()),
            bit_id=record_bit.id,
            record_id=record_bit.record_id,
            audio_obfuscation_data=audio_obfuscation_data,
            audio_params=record_bit.audio_params,
            markup_id=markup_id,
            markup_step=markup_step,
            pool_id=pool_id,
            assignment_id=assignment_id,
            markup_data=markup_data,
            validation_data=validation_data,
            received_at=received_at,
            other=None,
        )


def get_audio_obfuscation_data(fields: dict) -> AudioObfuscationData:
    version = fields['version']
    if version == AudioObfuscationVersions.SOX_EFFECTS.value:
        return SoxEffectsObfuscationData.from_yson(fields)
    elif version == AudioObfuscationVersions.SOX_EFFECTS_V2.value:
        return SoxEffectsObfuscationDataV2.from_yson(fields)
    else:
        raise ValueError(f'Unsupported audio obfuscation data: {fields}')


@dataclass
class RecordBitMarkupAudio:
    bit_id: str
    audio: bytes
    hash: bytes
    hash_version: HashVersion

    def to_yson(self) -> dict:
        return {
            'bit_id': self.bit_id,
            'audio': self.audio,
            'hash': self.hash,
            'hash_version': self.hash_version.value,
        }

    def __lt__(self, other: 'RecordBitMarkupAudio'):
        return self.bit_id < other.bit_id

    @staticmethod
    def create(record_bit: RecordBit, audio: bytes) -> 'RecordBitMarkupAudio':
        return RecordBitMarkupAudio(
            bit_id=record_bit.id,
            audio=audio,
            hash=crc32(audio),
            hash_version=HashVersion.CRC_32_BZIP2,
        )
