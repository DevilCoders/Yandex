import typing
from dataclasses import dataclass
from datetime import datetime
from enum import Enum

from cloud.ai.lib.python.datetime import (
    format_datetime,
    parse_datetime,
    format_datetime_for_s3_subpath,
)
from cloud.ai.lib.python.serialization import YsonSerializable, OrderedYsonSerializable
from cloud.ai.speechkit.stt.lib.data.model.common.hash import crc32
from .common import Mark, HashVersion, S3Object
from .records import Record, RecordRequestParams
from ..common import s3_consts


class SplitVersions(Enum):
    FIXED_LENGTH_OFFSET = 'fixed-length-offset'


@dataclass
class SplitDataFixedLengthOffset(YsonSerializable):
    length_ms: int
    offset_ms: int
    split_cmd: str

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': SplitVersions.FIXED_LENGTH_OFFSET.value}


@dataclass
class BitDataTimeInterval(YsonSerializable):
    start_ms: int  # inclusive
    end_ms: int  # exclusive
    index: int
    channel: int

    @classmethod
    def from_yson(cls, fields) -> 'BitDataTimeInterval':
        return BitDataTimeInterval(
            start_ms=fields['start_ms'],
            end_ms=fields['end_ms'],
            index=fields['index'],
            channel=fields.get('channel', 1),  # TODO: add bit_data.channel to old data
        )

    def get_duration_seconds(self) -> float:
        return (self.end_ms - self.start_ms) / 1000.0


class ConvertVersions(Enum):
    CMD = 'cmd'


@dataclass
class ConvertDataCmd(YsonSerializable):
    cmd: str

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': ConvertVersions.CMD.value}


@dataclass
class RecordBitAudioParams(YsonSerializable):
    language_code: str
    sample_rate_hertz: int
    acoustic: str

    def to_record_request_params(self) -> 'RecordRequestParams':
        return RecordRequestParams(
            recognition_spec={
                'audio_encoding': 1,  # is always .wav
                'language_code': self.language_code,
                'sample_rate_hertz': self.sample_rate_hertz,
            },
        )


SplitData = typing.Union[SplitDataFixedLengthOffset]
BitData = typing.Union[BitDataTimeInterval]
ConvertData = typing.Union[ConvertDataCmd]


@dataclass
class RecordBit(OrderedYsonSerializable):
    id: str
    record_id: str
    split_id: str
    split_data: SplitData
    bit_data: BitData
    convert_data: ConvertData
    mark: Mark
    s3_obj: S3Object
    audio_params: RecordBitAudioParams
    received_at: datetime
    other: typing.Any

    @staticmethod
    def primary_key() -> typing.List[str]:
        return ['record_id', 'split_id', 'id']

    @staticmethod
    def from_yson(fields: dict) -> 'RecordBit':
        split_data, bit_data = get_split_and_bit_data(fields)

        convert_version = fields['convert_data']['version']
        if convert_version == ConvertVersions.CMD.value:
            convert_data = ConvertDataCmd.from_yson(fields['convert_data'])
        else:
            raise ValueError('Unsupported convert data: %s' % fields['convert_data'])

        return RecordBit(
            id=fields['id'],
            record_id=fields['record_id'],
            split_id=fields['split_id'],
            split_data=split_data,
            bit_data=bit_data,
            convert_data=convert_data,
            mark=Mark(fields['mark']),
            s3_obj=S3Object.from_yson(fields['s3_obj']),
            audio_params=RecordBitAudioParams.from_yson(fields['audio_params']),
            received_at=parse_datetime(fields['received_at']),
            other=fields['other'],
        )

    @staticmethod
    def create(
        record: Record,
        split_data: typing.Union[SplitDataFixedLengthOffset],
        received_at: datetime,
        bit_data: typing.Union[BitDataTimeInterval],
        convert_data: typing.Union[ConvertDataCmd],
        sample_rate_hertz_native: int,
    ) -> 'RecordBit':
        split_id = f'{record.id}/{format_datetime(received_at)}'
        if isinstance(bit_data, BitDataTimeInterval):
            id = f'{split_id}/{bit_data.channel:02d}/{bit_data.index:06d}'
        else:
            raise ValueError(f'Unsupported bit data: {bit_data}')

        s3_obj = S3Object(
            endpoint=s3_consts.cloud_endpoint,
            bucket=s3_consts.data_bucket,
            key=f'Speechkit/STT/Bits/{format_datetime_for_s3_subpath(received_at)}/{id}.wav',
        )

        return RecordBit(
            id=id,
            record_id=record.id,
            split_id=split_id,
            split_data=split_data,
            bit_data=bit_data,
            convert_data=convert_data,
            mark=record.mark,
            s3_obj=s3_obj,
            audio_params=RecordBitAudioParams(
                language_code=record.req_params.get_language_code(),
                sample_rate_hertz=sample_rate_hertz_native,
                acoustic=record.audio_params.acoustic,
            ),
            received_at=received_at,
            other=None,
        )


def get_split_and_bit_data(fields: dict) -> typing.Tuple[SplitData, BitData]:
    split_version = fields['split_data']['version']
    if split_version == SplitVersions.FIXED_LENGTH_OFFSET.value:
        split_data = SplitDataFixedLengthOffset.from_yson(fields['split_data'])
        bit_data = BitDataTimeInterval.from_yson(fields['bit_data'])
        return split_data, bit_data
    else:
        raise ValueError(f'Unsupported split data: {fields["split_data"]}')


@dataclass
class RecordBitAudio(OrderedYsonSerializable):
    bit_id: str
    audio: bytes
    hash: bytes
    hash_version: HashVersion

    @staticmethod
    def primary_key() -> typing.List[str]:
        return ['bit_id']

    @staticmethod
    def create(record_bit: RecordBit, audio: bytes) -> 'RecordBitAudio':
        return RecordBitAudio(
            bit_id=record_bit.id,
            audio=audio,
            hash=crc32(audio),
            hash_version=HashVersion.CRC_32_BZIP2,
        )
