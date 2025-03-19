import io
import typing
from multiprocessing.pool import ThreadPool

import nirvana.job_context as nv
import yt.wrapper as yt

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.lib.python.datetime import now
from cloud.ai.speechkit.stt.lib.data.model.common import s3_consts
from cloud.ai.speechkit.stt.lib.data.model.common.hash import fast_crc32
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    Mark,
    Record,
    RecordAudio,
    HashVersion,
    RecordRequestParams,
    RecordAudioParams,
    S3Object,
    RecordSourceImport,
    YTTableImportData,
    RecordJoin,
    RecognitionPlainTranscript,
    JoinDataExemplar,
    RecordTag,
    RecordTagData,
    RecordTagType,
    ImportSource,
)
from cloud.ai.speechkit.stt.lib.data.ops.collect_records import assign_mark
from cloud.ai.speechkit.stt.lib.data.ops.yt import (
    table_records_meta,
    table_records_audio_meta,
    table_records_tags_meta,
    table_records_joins_meta,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.import_data.records import process_file_wav
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client
from cloud.ai.speechkit.stt.lib.text.re import validate as validate_text
from cloud.ai.speechkit.stt.lib.text.re import clean as clean_text


def main():
    params = nv.context().parameters
    if params.get('mark') == 'AUTO':
        mark = None
    else:
        mark = Mark(params.get('mark'))
    folder_id = params.get('folder_id')
    if folder_id is not None and len(folder_id) == 0:
        folder_id = None
    run(
        params.get('yt_table_path'),
        params.get('tag_suffix'),
        params.get('ticket'),
        folder_id,
        params.get('lang'),
        mark,
        params.get('has_texts'),
        params.get('store_to_hume'),
        params.get('additional_tags')
    )


def process_batch(
    batch: typing.List[dict],
    yt_table_path: str,
    tag_suffix: str,
    ticket: str,
    folder_id: typing.Optional[str],
    language: str,
    mark: typing.Optional[Mark],
    has_texts: bool,
    store_to_hume: bool,
    additional_tags: typing.List[str]
):
    records_data = []
    for row in batch:
        wav = row[b'wav']
        original_uuid = row.get(b'uuid')
        if original_uuid is not None:
            original_uuid = original_uuid.decode('utf-8')

        record_file_data = process_file_wav(io.BytesIO(wav))

        record_data = {
            'record_file_data': record_file_data,
            'original_uuid': original_uuid,
        }

        if has_texts:
            record_data['text'], _ = clean_text(row[b'text'].decode('utf-8'), language)

        records_data.append(record_data)

    print(f'{len(records_data)} records read from {yt_table_path}')

    if store_to_hume:
        print('Creating YT tables in Hume')
        yt.config["proxy"]["url"] = "hume"

    received_at = now()

    table_name = Table.get_name(received_at)

    s3 = create_client()

    pool = ThreadPool(processes=16)

    audio_s3_dir = table_name.replace('-', '/')

    def get_s3_key(record_id: str):
        return f'Speechkit/STT/Data/{audio_s3_dir}/{record_id}.raw'

    def put_audio_to_s3(record_data: dict):
        s3.put_object(
            Bucket=s3_consts.data_bucket,
            Key=get_s3_key(record_data['record_file_data'].record_id),
            Body=record_data['record_file_data'].raw_data,
        )

    pool.map(put_audio_to_s3, records_data)

    records = []
    records_audio = []
    records_tags = []
    records_joins = []

    tag_data_list = [
        RecordTagData(
            type=RecordTagType.IMPORT,
            value=f'{ImportSource.YT_TABLE.value}_{tag_suffix}',
        ),
        RecordTagData.create_period(received_at),
        RecordTagData.create_lang(language)
    ]
    if additional_tags:
        tag_data_list.extend([
            RecordTagData.from_str(tag) for tag in additional_tags
        ])

    for record_data in records_data:
        record_file_data = record_data['record_file_data']
        record_id = record_file_data.record_id
        audio = record_file_data.raw_data
        record = Record(
            id=record_id,
            s3_obj=S3Object(
                endpoint=s3_consts.cloud_endpoint,
                bucket=s3_consts.data_bucket,
                key=get_s3_key(record_id),
            ),
            mark=mark or assign_mark(),
            source=RecordSourceImport.create_yt_table(
                YTTableImportData(
                    yt_table_path=yt_table_path,
                    ticket=ticket,
                    folder_id=folder_id,
                    original_uuid=record_data['original_uuid'],
                ),
            ),
            req_params=RecordRequestParams(
                recognition_spec={
                    'audio_encoding': 1,
                    'language_code': language,
                    'sample_rate_hertz': record_file_data.sample_rate_hertz,
                },
            ),
            audio_params=RecordAudioParams(
                acoustic='unknown',
                duration_seconds=record_file_data.duration_seconds,
                size_bytes=len(audio),
                channel_count=record_file_data.channel_count,
            ),
            received_at=received_at,
            other=None,
        )
        record_audio = RecordAudio(
            record_id=record_id,
            audio=None,
            hash=fast_crc32(audio),
            hash_version=HashVersion.CRC_32,
        )
        records.append(record)
        records_audio.append(record_audio)

        if has_texts:
            records_joins.append(
                RecordJoin.create_by_record(
                    record=record,
                    recognition=RecognitionPlainTranscript(text=record_data['text']),
                    join_data=JoinDataExemplar(),
                    received_at=received_at,
                    other=None,
                )
            )

        records_tags.extend(
            RecordTag.add(record=record, data=tag_data, received_at=received_at) for tag_data in tag_data_list
        )

    table_records = Table(meta=table_records_meta, name=table_name)
    table_records_audio = Table(meta=table_records_audio_meta, name=table_name)
    table_records_tags = Table(meta=table_records_tags_meta, name=table_name)
    table_records_joins = Table(meta=table_records_joins_meta, name=table_name)

    table_records.append_objects(records)
    table_records_audio.append_objects(records_audio)
    table_records_tags.append_objects(records_tags)
    if has_texts:
        table_records_joins.append_objects(records_joins)

    print(f'Tag: {tag_data_list[0].to_str()}')


def run(
    yt_table_path: str,
    tag_suffix: str,
    ticket: str,
    folder_id: typing.Optional[str],
    language: str,
    mark: typing.Optional[Mark],
    has_texts: bool,
    store_to_hume: bool,
    additional_tags: typing.List[str]
):
    assert len(tag_suffix) > 5
    assert folder_id is None or folder_id.startswith('b1g')

    if has_texts:
        for row in yt.read_table(yt_table_path, format=yt.YsonFormat(encoding=None)):
            text = row[b'text'].decode('utf-8')
            try:
                validate_text(text, language)
            except ValueError as e:
                print(e)

    batch = []  # split in batches to fit big tables in memory
    for row in yt.read_table(yt_table_path, format=yt.YsonFormat(encoding=None)):
        batch.append(row)
        if len(batch) >= 100000:
            process_batch(
                batch,
                yt_table_path,
                tag_suffix,
                ticket,
                folder_id,
                language,
                mark,
                has_texts,
                store_to_hume,
                additional_tags
            )
            batch = []

    if len(batch) > 0:
        process_batch(
            batch,
            yt_table_path,
            tag_suffix,
            ticket,
            folder_id,
            language,
            mark,
            has_texts,
            store_to_hume,
            additional_tags
        )
