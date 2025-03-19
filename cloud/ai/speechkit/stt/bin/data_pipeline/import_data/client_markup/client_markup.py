#!/usr/bin/python3

import json
import typing
from io import BytesIO
from multiprocessing.pool import ThreadPool

import nirvana.job_context as nv

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.lib.python.datetime import now
from cloud.ai.speechkit.stt.lib.data.model.common import s3_consts
from cloud.ai.speechkit.stt.lib.data.model.common.hash import crc32
from cloud.ai.speechkit.stt.lib.data.model.common.id import generate_id
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    Record,
    RecordAudio,
    S3Object,
    HashVersion,
    Mark,
    RecordSourceMarkup,
    RecordRequestParams,
    RecordAudioParams,
    RecordTag,
    RecordTagData,
    RecordTagType,
)
from cloud.ai.speechkit.stt.lib.data.ops.yt import (
    table_records_meta,
    table_records_audio_meta,
    table_records_tags_meta,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.import_data.records import process_file, RecordFileData
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_cost import calculate_final_billing_units
from cloud.ai.speechkit.stt.lib.data_pipeline.markup_params import transcript_real_tasks_in_assignment
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client


def main():
    op_ctx = nv.context()

    outputs = op_ctx.outputs
    params = op_ctx.parameters

    result_path = outputs.get('result.json')

    records_s3_bucket: str = params.get('records-s3-bucket')
    records_s3_dir_key: str = params.get('records-s3-dir-key')
    folder_id: str = params.get('folder-id')
    bit_length: int = params.get('bit-length')
    bit_offset: int = params.get('bit-offset')
    chunk_overlap: int = params.get('chunk-overlap')
    edge_bits_full_overlap: bool = params.get('edge-bits-full-overlap')

    s3 = create_client()

    if not records_s3_dir_key.endswith('/'):
        records_s3_dir_key += '/'

    continuation_token = None
    records_s3_keys = []
    while True:
        args = {}
        if continuation_token is not None:
            args['ContinuationToken'] = continuation_token

        resp = s3.list_objects_v2(Bucket=records_s3_bucket, Prefix=records_s3_dir_key, **args)

        for content in resp['Contents']:
            if content['Size'] > 0:  # filter dir itself
                records_s3_keys.append(content['Key'])

        if resp['IsTruncated']:
            continuation_token = resp['NextContinuationToken']
        else:
            break

    received_at = now()
    table_name = Table.get_name(received_at)

    record_tag_data = RecordTagData(
        type=RecordTagType.MARKUP,
        value=f'{folder_id}_{int(received_at.timestamp())}_{len(records_s3_keys)}',
    )

    def upload_and_process_record(
        s3_key: str,
    ) -> typing.Tuple[str, typing.Optional[RecordFileData], typing.Optional[Exception]]:
        file_name = s3_key.split('/')[-1]
        audio = s3.get_object(Bucket=s3_consts.data_bucket, Key=s3_key)['Body'].read()
        try:
            return file_name, process_file(file_name, BytesIO(audio)), None
        except Exception as e:
            return file_name, None, e

    pool = ThreadPool(processes=16)
    process_audio_results = pool.map(upload_and_process_record, records_s3_keys)

    process_errors = []
    for file_name, _, process_error in process_audio_results:
        if process_error:
            process_errors.append((file_name, process_error))

    if len(process_errors) > 0:
        with open(result_path, 'w') as f:
            f.write(
                json.dumps(
                    {
                        'errors': [
                            f'invalid file {file_name}: {process_error}' for file_name, process_error in process_errors
                        ],
                    }
                )
            )
        return

    billing_units = calculate_final_billing_units(
        records_durations=[int(d[1].duration_seconds * 1000) for d in process_audio_results],
        records_channel_counts=[d[1].channel_count for d in process_audio_results],
        bit_length=bit_length,
        bit_offset=bit_offset,
        chunk_overlap=chunk_overlap,
        edge_bits_full_overlap=edge_bits_full_overlap,
        real_tasks_per_assignment=transcript_real_tasks_in_assignment,
    )

    records = []
    records_audio = []
    records_tags = []
    file_name_to_record_id = {}

    for file_name, audio_data, _ in process_audio_results:
        record_id = generate_id()
        file_name_to_record_id[file_name] = record_id

        s3_obj = S3Object(
            endpoint=s3_consts.cloud_endpoint,
            bucket=s3_consts.data_bucket,
            key=f'Speechkit/STT/Data/{table_name.replace("-", "/")}/{record_id}.raw',
        )

        s3.put_object(Bucket=s3_obj.bucket, Key=s3_obj.key, Body=audio_data.raw_data)

        record = Record(
            id=record_id,
            s3_obj=s3_obj,
            mark=Mark.TEST,
            source=RecordSourceMarkup(
                source_file_name=file_name,
                folder_id=folder_id,
                billing_units=billing_units,
            ),
            req_params=RecordRequestParams(
                recognition_spec={
                    'audio_encoding': 1,
                    'language_code': 'ru-RU',
                    'sample_rate_hertz': audio_data.sample_rate_hertz,
                },
            ),
            audio_params=RecordAudioParams(
                acoustic='unknown',
                duration_seconds=audio_data.duration_seconds,
                size_bytes=len(audio_data.raw_data),
                channel_count=audio_data.channel_count,
            ),
            received_at=received_at,
            other=None,
        )
        record_audio = RecordAudio(
            record_id=record_id,
            audio=None,
            hash=crc32(audio_data.raw_data),
            hash_version=HashVersion.CRC_32_BZIP2,
        )
        records.append(record)
        records_audio.append(record_audio)

        records_tags.append(RecordTag.add(record=record, data=record_tag_data, received_at=received_at))
        records_tags.append(
            RecordTag.add(record=record, data=RecordTagData.create_period(received_at), received_at=received_at)
        )
        records_tags.append(RecordTag.add(record=record, data=RecordTagData.create_lang_ru(), received_at=received_at))

    table_records = Table(meta=table_records_meta, name=table_name)
    table_records_audio = Table(meta=table_records_audio_meta, name=table_name)
    table_records_tags = Table(meta=table_records_tags_meta, name=table_name)

    table_records.append_objects(records)
    table_records_audio.append_objects(records_audio)
    table_records_tags.append_objects(records_tags)

    with open(result_path, 'w') as f:
        f.write(
            json.dumps(
                {
                    'tag': record_tag_data.to_str(),
                    'billing_units': billing_units,
                    'file_name_to_record_id': file_name_to_record_id,
                }
            )
        )
