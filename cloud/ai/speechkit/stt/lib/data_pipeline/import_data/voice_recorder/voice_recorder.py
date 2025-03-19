import typing
from dataclasses import dataclass
from datetime import datetime, timedelta
from multiprocessing.pool import ThreadPool
from functools import partial
from enum import Enum

from cloud.ai.speechkit.stt.lib.data_pipeline.import_data.records import RecordFileData
from cloud.ai.speechkit.stt.lib.data.ops.collect_records import assign_mark
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    Record,
    RecordAudio,
    S3Object,
    HashVersion,
    RecordSourceImport,
    VoiceRecorderImportDataV1,
    VoiceRecorderMarkupData,
    VoiceRecorderEvaluationData,
    RecordRequestParams,
    RecordAudioParams,
    RecordTag,
    RecordTagType,
    RecordTagData,
    RecordJoin,
    RecognitionPlainTranscript,
    RecognitionRawText,
    ImportSource,
    JoinDataVoiceRecorder,
)
from cloud.ai.speechkit.stt.lib.data.model.common.hash import fast_crc32
from cloud.ai.speechkit.stt.lib.data.model.common import s3_consts
from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data.ops.yt import (
    table_records_meta,
    table_records_audio_meta,
    table_records_tags_meta,
    table_records_joins_meta,
)
from cloud.ai.speechkit.stt.lib.text.re import clean as clean_text


class AcousticType(Enum):
    VoiceRecorder = 'voice-recorder'
    Phone = 'phone'


@dataclass
class VoiceRecorderData:
    assignment_id: str
    id: str
    asr_mark: str
    hypothesis: str
    metric_name: str
    metric_value: float
    text: str
    text_transformations: str
    uuid: str
    record_data: RecordFileData
    worker_id: str
    acoustic_type: AcousticType
    toloka_region: str
    dataset: str


def make_record(
    vr_data: VoiceRecorderData,
    lang: str,
    mark: str,
    received_at:
    datetime, s3,
    is_test_run: bool,
) -> typing.Tuple[Record, RecordAudio]:
    record_data = vr_data.record_data

    s3_obj = S3Object(
        endpoint=s3_consts.cloud_endpoint,
        bucket=s3_consts.data_bucket,
        key=f'Speechkit/STT/Data/{received_at.year}/{received_at.month:02d}/{received_at.day:02d}/{record_data.record_id}.raw',
    )

    mark_probs = {
        'TRAIN': (1, 0),
        'TEST': (0, 1),
        'VAL': (0, 0),
        'AUTO': (0.9, 0.05)
    }[mark]

    record = Record(
        id=record_data.record_id,
        s3_obj=s3_obj,
        mark=assign_mark(*mark_probs),
        source=RecordSourceImport.create_voice_recorder(
            VoiceRecorderImportDataV1(
                markup_data=VoiceRecorderMarkupData(
                    assignment_id=vr_data.assignment_id,
                    worker_id=vr_data.worker_id,
                    task_id=vr_data.id,
                ),
                evaluation_data=VoiceRecorderEvaluationData(
                    hypothesis=vr_data.hypothesis,
                    metric_name=vr_data.metric_name,
                    metric_value=vr_data.metric_value,
                    text_transformations=vr_data.text_transformations,
                ),
                duration=record_data.duration_seconds,
                asr_mark=vr_data.asr_mark,
                text=vr_data.text,
                uuid=vr_data.uuid,
            ),
        ),
        req_params=RecordRequestParams(
            recognition_spec={
                'audio_encoding': 1,
                'language_code': lang,
                'sample_rate_hertz': record_data.sample_rate_hertz
            },
        ),
        audio_params=RecordAudioParams(
            acoustic=vr_data.acoustic_type.value,
            duration_seconds=record_data.duration_seconds,
            size_bytes=len(record_data.raw_data),
            channel_count=1,
        ),
        received_at=received_at,
        other=None,
    )
    record_audio = RecordAudio(
        record_id=record_data.record_id,
        audio=None,
        hash=fast_crc32(record_data.raw_data),
        hash_version=HashVersion.CRC_32,
    )

    if not is_test_run:
        print('S3 PUT %s' % record.s3_obj.to_https_url(), flush=True)
        s3.put_object(Bucket=record.s3_obj.bucket, Key=record.s3_obj.key, Body=record_data.raw_data)

    return record, record_audio


def process_chunk(voice_recorder_data: typing.List[VoiceRecorderData], lang: str, mark: str,
                  received_at: datetime, s3, is_test_run: bool):
    print(f'Start chunk processing: {datetime.now()}', flush=True)

    pool = ThreadPool(32)

    records_with_audio = pool.map(
        partial(make_record, lang=lang, mark=mark, received_at=received_at, s3=s3, is_test_run=is_test_run),
        voice_recorder_data,
    )
    records = [r[0] for r in records_with_audio]
    records_audio = [r[1] for r in records_with_audio]

    print(f'Batch was uploaded to S3, updating YT tables: {datetime.now()}', flush=True)

    records_tags = []
    records_joins = []

    # Create Record and RecordTag objects, upload .raw to S3
    for vr_data, record in zip(voice_recorder_data, records):
        tags_data_list = [
            RecordTagData(
                type=RecordTagType.IMPORT,
                value=ImportSource.VOICE_RECORDER_V1.value
            ),
            RecordTagData(
                type=RecordTagType.ACOUSTIC,
                value=vr_data.acoustic_type.value,
            ),
            RecordTagData(
                type=RecordTagType.DATASET,
                value=RecordTag.sanitize_value(vr_data.dataset),
            ),
            RecordTagData.create_lang(lang),
            RecordTagData.create_period(received_at),
            RecordTagData(
                type=RecordTagType.REGION,
                value=vr_data.toloka_region
            )
        ]

        for tag_data in tags_data_list:
            records_tags.append(RecordTag.add(record=record, data=tag_data, received_at=received_at))

        raw_text = vr_data.text.decode('utf-8')
        text_without_punctuation, _ = clean_text(raw_text, lang)

        records_joins.append(
            RecordJoin.create_by_record(
                record=record,
                recognition=RecognitionRawText(text=raw_text),
                join_data=JoinDataVoiceRecorder(),
                received_at=received_at,
                other=None,
            )
        )

        records_joins.append(
            RecordJoin.create_by_record(
                record=record,
                recognition=RecognitionPlainTranscript(text=text_without_punctuation),
                join_data=JoinDataVoiceRecorder(),
                received_at=received_at + timedelta(milliseconds=1),
                other=None,
            )
        )

    table_name = Table.get_name(received_at)
    table_records = Table(meta=table_records_meta, name=table_name)
    table_records_audio = Table(meta=table_records_audio_meta, name=table_name)
    table_records_tags = Table(meta=table_records_tags_meta, name=table_name)
    table_records_joins = Table(meta=table_records_joins_meta, name=table_name)

    table_records.append_objects(records)
    table_records_audio.append_objects(records_audio)
    table_records_tags.append_objects(records_tags)
    table_records_joins.append_objects(records_joins)
