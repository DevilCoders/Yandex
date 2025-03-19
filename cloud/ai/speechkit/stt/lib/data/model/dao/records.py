import typing
from dataclasses import dataclass
from datetime import datetime
from enum import Enum

from cloud.ai.lib.python.datetime import parse_datetime
from cloud.ai.lib.python.serialization import YsonSerializable, OrderedYsonSerializable
from .common import Mark, HashVersion, S3Object


class AudioEncoding(Enum):
    LPCM = 1
    OGG_OPUS = 2


@dataclass
class RecordRequestParams(YsonSerializable):
    recognition_spec: dict

    def get_sample_rate_hertz(self) -> int:
        return int(self.recognition_spec['sample_rate_hertz'])

    def get_language_code(self) -> str:
        return self.recognition_spec['language_code']

    def get_audio_encoding(self) -> AudioEncoding:
        return AudioEncoding(int(self.recognition_spec.get('audio_encoding', 2)))

    def fix_lang(self):
        if self.recognition_spec.get('language_code', '') == '':
            self.recognition_spec['language_code'] = 'ru-RU'


@dataclass
class RecordAudioParams(YsonSerializable):
    acoustic: str
    duration_seconds: float
    size_bytes: int
    channel_count: int


class RecordSourceType(Enum):
    CLOUD = 'cloud'
    EXEMPLAR = 'exemplar'
    IMPORT = 'import'
    MARKUP = 'markup'


@dataclass
class RecordSourceCloudMethod(YsonSerializable):
    name: str
    version: str


@dataclass
class RecordSourceCloud(YsonSerializable):
    folder_id: str
    method: RecordSourceCloudMethod

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': RecordSourceType.CLOUD.value}


@dataclass
class RecordSourceExemplar(YsonSerializable):
    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': RecordSourceType.EXEMPLAR.value}


class MarkupPriorities(Enum):
    INTERNAL = 'internal'
    CLIENT_NORMAL = 'client-normal'
    CLIENT_HIGH = 'client-high'

    def get_toloka_pool_priority(self) -> int:
        return {
            MarkupPriorities.INTERNAL: 20,
            MarkupPriorities.CLIENT_NORMAL: 50,
            MarkupPriorities.CLIENT_HIGH: 80,
        }[self]


@dataclass
class RecordSourceMarkup(YsonSerializable):
    source_file_name: str
    folder_id: str
    billing_units: int

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': RecordSourceType.MARKUP.value}


class ImportSource(Enum):
    VOICETABLE = 'voicetable'
    YANDEX_CALL_CENTER = 'yandex-call-center'
    YT_TABLE = 'yt-table'
    DATASPHERE = 'datasphere'
    FILES = 'files'
    VOICE_RECORDER_V1 = 'voice-recorder-v1'
    S3_BUCKET = 's3-bucket'


@dataclass
class VoiceRecorderMarkupData(YsonSerializable):
    assignment_id: str
    worker_id: str
    task_id: str


@dataclass
class VoiceRecorderEvaluationData(YsonSerializable):
    hypothesis: str
    metric_name: str
    metric_value: float
    text_transformations: str


@dataclass
class VoiceRecorderImportDataV1(YsonSerializable):
    markup_data: VoiceRecorderMarkupData
    evaluation_data: VoiceRecorderEvaluationData
    duration: float
    asr_mark: str
    text: str
    uuid: str


@dataclass
class VoicetableImportData(YsonSerializable):
    other_info: dict
    original_info: dict
    language: str
    text_ru: str
    text_orig: str
    application: typing.Optional[str]
    acoustic: typing.Optional[str]
    date: str
    dataset_id: typing.Optional[str]
    speaker_info: str
    duration: float
    uttid: str
    mark: str


@dataclass
class YandexCallCenterImportData(YsonSerializable):
    yt_table_path: str
    source_audio_url: str
    channel: int
    call_reason: str
    phone: str
    call_time: str
    call_duration: int
    call_status: str
    create_time: datetime


@dataclass
class YTTableImportData(YsonSerializable):
    yt_table_path: str
    ticket: str
    folder_id: typing.Optional[str]
    original_uuid: typing.Optional[str]


@dataclass
class DatasphereImportData(YsonSerializable):
    source_archive_s3_obj: S3Object
    folder_id: str
    dataset_name: str
    wav_file: str


@dataclass
class FilesImportData(YsonSerializable):
    ticket: str
    source_file_name: str
    folder_id: typing.Optional[str]


@dataclass
class S3BucketImportData(YsonSerializable):
    source_s3_obj: S3Object
    last_modified: datetime
    tag: str


ImportData = typing.Union[
    YandexCallCenterImportData, VoicetableImportData, YTTableImportData, FilesImportData, VoiceRecorderImportDataV1,
    S3BucketImportData, DatasphereImportData
]


@dataclass
class RecordSourceImport(YsonSerializable):
    source: ImportSource
    data: ImportData

    @classmethod
    def from_yson(cls, fields: dict) -> 'RecordSourceImport':
        source = ImportSource(fields['source'])
        if source == ImportSource.YANDEX_CALL_CENTER:
            data = YandexCallCenterImportData.from_yson(fields['data'])
        elif source == ImportSource.YT_TABLE:
            data = YTTableImportData.from_yson(fields['data'])
        elif source == ImportSource.DATASPHERE:
            data = DatasphereImportData.from_yson(fields['data'])
        elif source == ImportSource.FILES:
            data = FilesImportData.from_yson(fields['data'])
        elif source == ImportSource.VOICETABLE:
            data = VoicetableImportData.from_yson(fields['data'])
        elif source == ImportSource.VOICE_RECORDER_V1:
            data = VoiceRecorderImportDataV1.from_yson(fields['data'])
        elif source == ImportSource.S3_BUCKET:
            data = S3BucketImportData.from_yson(fields['data'])
        else:
            raise NotImplementedError(f'Unmarshal for source {source.value} is not implemented')
        return RecordSourceImport(source=source, data=data)

    @staticmethod
    def create_yandex_call_center(import_data: YandexCallCenterImportData):
        return RecordSourceImport(
            source=ImportSource.YANDEX_CALL_CENTER,
            data=import_data,
        )

    @staticmethod
    def create_voicetable(import_data: VoicetableImportData):
        return RecordSourceImport(
            source=ImportSource.VOICETABLE,
            data=import_data,
        )

    @staticmethod
    def create_voice_recorder(import_data: VoiceRecorderImportDataV1):
        return RecordSourceImport(
            source=ImportSource.VOICE_RECORDER_V1,
            data=import_data,
        )

    @staticmethod
    def create_yt_table(import_data: YTTableImportData):
        return RecordSourceImport(
            source=ImportSource.YT_TABLE,
            data=import_data,
        )

    @staticmethod
    def create_datasphere(import_data: DatasphereImportData):
        return RecordSourceImport(
            source=ImportSource.DATASPHERE,
            data=import_data,
        )

    @staticmethod
    def create_files(import_data: FilesImportData):
        return RecordSourceImport(
            source=ImportSource.FILES,
            data=import_data,
        )

    @staticmethod
    def create_s3_bucket(import_data: S3BucketImportData):
        return RecordSourceImport(
            source=ImportSource.S3_BUCKET,
            data=import_data,
        )

    @staticmethod
    def serialization_additional_fields() -> typing.Dict[str, typing.Any]:
        return {'version': RecordSourceType.IMPORT.value}


RecordSource = typing.Union[RecordSourceCloud, RecordSourceExemplar, RecordSourceImport, RecordSourceMarkup]


@dataclass
class Record(OrderedYsonSerializable):
    id: str
    s3_obj: S3Object
    mark: Mark
    source: RecordSource
    req_params: RecordRequestParams
    audio_params: RecordAudioParams
    received_at: datetime
    other: typing.Optional[dict]

    @staticmethod
    def primary_key() -> typing.List[str]:
        return ['id']

    def download_audio(self, s3) -> bytes:
        return s3.get_object(Bucket=self.s3_obj.bucket, Key=self.s3_obj.key)['Body'].read()

    @classmethod
    def from_yson(cls, fields: dict) -> 'Record':
        source_version = fields['source']['version']
        if source_version == RecordSourceType.CLOUD.value:
            source = RecordSourceCloud.from_yson(fields['source'])
        elif source_version == RecordSourceType.EXEMPLAR.value:
            source = RecordSourceExemplar()
        elif source_version == RecordSourceType.IMPORT.value:
            source = RecordSourceImport.from_yson(fields['source'])
        elif source_version == RecordSourceType.MARKUP.value:
            source = RecordSourceMarkup.from_yson(fields['source'])
        else:
            raise ValueError(f'Unsupported source: {fields["source"]}')

        return Record(
            id=fields['id'],
            s3_obj=S3Object.from_yson(fields['s3_obj']),
            mark=Mark(fields['mark']),
            source=source,
            req_params=RecordRequestParams.from_yson(fields['req_params']),
            audio_params=RecordAudioParams.from_yson(fields['audio_params']),
            received_at=parse_datetime(fields['received_at']),
            other=fields['other'],
        )


@dataclass
class RecordAudio(OrderedYsonSerializable):
    record_id: str
    audio: typing.Optional[bytes]
    hash: bytes
    hash_version: HashVersion

    @staticmethod
    def primary_key() -> typing.List[str]:
        return ['record_id']
