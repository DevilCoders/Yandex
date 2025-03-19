import io
from pydub import AudioSegment
import typing
from datetime import datetime
from datetime import timedelta
from multiprocessing.pool import ThreadPool
from functools import partial

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    Record,
    RecordAudio,
    S3Object,
    HashVersion,
    RecordSourceImport,
    VoicetableImportData,
    RecordRequestParams,
    RecordAudioParams,
    RecordTag,
    RecordTagType,
    RecordTagData,
    RecordJoin,
    RecognitionPlainTranscript,
    ImportSource,
    JoinDataVoicetableReferenceRu,
    JoinDataVoicetableReferenceOrig,
)
from cloud.ai.speechkit.stt.lib.data.model.common.id import generate_id
from cloud.ai.speechkit.stt.lib.data.model.common.hash import crc32
from cloud.ai.speechkit.stt.lib.data.model.common import s3_consts
from cloud.ai.speechkit.stt.lib.data.ops.collect_records import assign_mark
from cloud.ai.speechkit.stt.lib.data.ops.yt import (
    table_records_meta,
    table_records_audio_meta,
    table_records_tags_meta,
    table_records_joins_meta,
)


class VoicetableData:
    def __init__(
        self,
        other_info,
        original_info,
        language,
        text_orig,
        text_ru,
        acoustic,
        application,
        dataset_id,
        date,
        speaker_info,
        wav,
        duration,
        uttid,
        mark,
    ):
        self.other_info = other_info
        self.original_info = original_info
        self.language = language
        self.text_orig = text_orig
        self.text_ru = text_ru
        self.acoustic = acoustic
        self.application = application
        self.dataset_id = dataset_id
        self.date = date
        self.speaker_info = speaker_info
        self.wav = wav
        self.duration = float(duration)
        self.uttid = uttid
        self.mark = mark


def make_record(vt_data: VoicetableData, received_at: datetime, s3, is_test_run) -> typing.Tuple[Record, RecordAudio]:
    record_id = generate_id()
    duration_seconds = vt_data.duration

    audio = AudioSegment.from_wav(io.BytesIO(vt_data.wav)).raw_data

    s3_obj = S3Object(
        endpoint=s3_consts.cloud_endpoint,
        bucket=s3_consts.data_bucket,
        key=f'Speechkit/STT/Data/{received_at.year}/{received_at.month:02d}/{received_at.day:02d}/{record_id}.raw',
    )

    record = Record(
        id=record_id,
        s3_obj=s3_obj,
        mark=assign_mark(),
        source=RecordSourceImport.create_voicetable(
            VoicetableImportData(
                original_info=vt_data.original_info,
                other_info=vt_data.other_info,
                language=vt_data.language,
                text_ru=vt_data.text_ru,
                text_orig=vt_data.text_orig,
                application=vt_data.application,
                acoustic=vt_data.acoustic,
                date=vt_data.date,
                dataset_id=vt_data.dataset_id,
                speaker_info=vt_data.speaker_info,
                duration=vt_data.duration,
                uttid=vt_data.uttid,
                mark=vt_data.mark,
            ),
        ),
        req_params=RecordRequestParams(
            recognition_spec={'audio_encoding': 1, 'language_code': vt_data.language, 'sample_rate_hertz': 16000},
        ),
        audio_params=RecordAudioParams(
            acoustic='unknown' if vt_data.acoustic is None else RecordTag.sanitize_value(vt_data.acoustic),
            duration_seconds=duration_seconds,
            size_bytes=len(audio),
            channel_count=1,
        ),
        received_at=received_at,
        other=None,
    )
    record_audio = RecordAudio(
        record_id=record_id,
        audio=None,
        hash=crc32(audio),
        hash_version=HashVersion.CRC_32_BZIP2,
    )

    if not is_test_run:
        print('S3 PUT %s' % record.s3_obj.to_https_url())
        s3.put_object(Bucket=record.s3_obj.bucket, Key=record.s3_obj.key, Body=audio)

    return record, record_audio


def process_chunk(voicetable_data: typing.List[VoicetableData], received_at: datetime, s3, is_test_run):
    pool = ThreadPool(32)

    records_with_audio = pool.map(
        partial(make_record, received_at=received_at, s3=s3, is_test_run=is_test_run),
        voicetable_data,
    )
    records = [r[0] for r in records_with_audio]
    records_audio = [r[1] for r in records_with_audio]

    records_tags = []
    records_joins = []

    # Create Record and RecordTag objects, upload .raw to S3
    for vt_data, record in zip(voicetable_data, records):
        tags_data_list = [
            RecordTagData(type=RecordTagType.IMPORT, value=ImportSource.VOICETABLE.value),
            RecordTagData(
                type=RecordTagType.APP,
                value='none' if vt_data.application is None else RecordTag.sanitize_value(vt_data.application),
            ),
            RecordTagData(
                type=RecordTagType.ACOUSTIC,
                value='none' if vt_data.acoustic is None else RecordTag.sanitize_value(vt_data.acoustic),
            ),
            RecordTagData(
                type=RecordTagType.DATASET,
                value='none' if vt_data.dataset_id is None else RecordTag.sanitize_value(vt_data.dataset_id),
            ),
            # we import only russian now
            RecordTagData.create_lang_ru(),
            RecordTagData.create_period(received_at),
        ]

        for tag_data in tags_data_list:
            records_tags.append(RecordTag.add(record=record, data=tag_data, received_at=received_at))

        records_joins.append(
            RecordJoin.create_by_record(
                record=record,
                recognition=RecognitionPlainTranscript(text=vt_data.text_ru),
                join_data=JoinDataVoicetableReferenceRu(),
                received_at=received_at + timedelta(milliseconds=1),
                other=None,
            )
        )

        records_joins.append(
            RecordJoin.create_by_record(
                record=record,
                recognition=RecognitionPlainTranscript(text=vt_data.text_orig),
                join_data=JoinDataVoicetableReferenceOrig(),
                received_at=received_at,
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
