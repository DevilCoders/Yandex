import os
import pydub
from datetime import datetime
from io import BytesIO
import typing
from multiprocessing.pool import ThreadPool
from random import shuffle

import nirvana.job_context as nv

from cloud.ai.speechkit.stt.lib.data.model.common import s3_consts
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client
from cloud.ai.speechkit.stt.lib.data_pipeline.import_data.records import process_file_wav, process_file, RecordFileData
from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data.ops.collect_records import assign_mark
from cloud.ai.speechkit.stt.lib.data.model.common.hash import fast_crc32
from cloud.ai.lib.python.datetime import now, parse_datetime
from cloud.ai.speechkit.stt.lib.data.model.common.id import generate_id
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    Record,
    RecordAudio,
    S3Object,
    HashVersion,
    RecordSourceImport,
    S3BucketImportData,
    RecordRequestParams,
    RecordAudioParams,
    RecordTag,
    RecordTagData,
    RecordTagType,
    RecordSourceType,
    ImportSource,
)
from cloud.ai.speechkit.stt.lib.data.ops.yt import (
    table_records_meta,
    table_records_audio_meta,
    table_records_tags_meta,
)
from cloud.ai.lib.python.datasource.yql import run_yql_select_query


def main():
    op_ctx = nv.context()

    params = op_ctx.parameters

    s3 = create_client()

    source_endpoint, source_endpoint_url = s3_consts.mds_endpoint, s3_consts.mds_url
    source_bucket = params.get('source-bucket')
    source_s3 = create_client(
        endpoint_url=source_endpoint_url,
        access_key_id=params.get('source-aws-access-key-id'),
        secret_access_key=params.get('source-aws-secret-access-key'),
    )

    tag = params.get('tag')
    assert tag in ['taxi-p2p', 'khural']

    imported_s3_keys, from_ts = get_imported_s3_keys_and_latest_record_ts(tag)
    print(f'{len(imported_s3_keys)} records are already imported')
    print(f'fetching records which uploaded after {from_ts}')

    continuation_token = None
    records_data = []
    while True:
        args = {}
        if continuation_token is not None:
            args['ContinuationToken'] = continuation_token

        resp = source_s3.list_objects_v2(Bucket=source_bucket, **args)

        for content in resp['Contents']:
            if content['Size'] > 0:  # filter dir itself
                if from_ts is None or content['LastModified'] > from_ts:
                    records_data.append(content)

        if resp['IsTruncated']:
            continuation_token = resp['NextContinuationToken']
        else:
            break

    # There is too much new calls, we will import only sample of it.
    sample_size = params.get('sample-size')
    print(f'sample records: {sample_size}/{len(records_data)}')
    records_data = sample_records(records_data, sample_size)

    sampled_s3_keys = {x['Key'] for x in records_data}
    if len(sampled_s3_keys.intersection(imported_s3_keys)) > 0:
        raise ValueError('Invalid last modified sample: s3 keys intersection')

    def download_record(record_data: dict) -> typing.Optional[RecordFileData]:
        record_key = record_data['Key']
        audio = source_s3.get_object(Bucket=source_bucket, Key=record_key)['Body'].read()
        try:
            if tag == 'taxi-p2p':
                # Keys in taxi bucket has no file extensions, but they all are WAV
                return process_file_wav(BytesIO(audio))
            else:
                return process_file(record_key, BytesIO(audio))
        except (pydub.exceptions.CouldntDecodeError, ValueError) as e:
            print(f'failed to decode record {record_key}:\n{e}')
            return None

    pool = ThreadPool(processes=16)
    downloaded_records = pool.map(download_record, records_data)

    if len(downloaded_records) == 0:
        print(f'no newly uploaded records')
        return

    decode_errors = len([x for x in downloaded_records if x is None])
    if decode_errors > 0:
        print(f'{decode_errors}/{len(downloaded_records)} records are failed to decode')

    if decode_errors / len(downloaded_records) > 0.5:
        raise RuntimeError('Too much records are failed to decode')

    received_at = now()

    table_name = Table.get_name(received_at)
    s3_dir = f'Speechkit/STT/Data/{table_name.replace("-", "/")}'

    record_tag_data = RecordTagData(type=RecordTagType.IMPORT, value=tag)

    records = []
    records_audio = []
    records_tags = []

    if tag == 'taxi-p2p':
        lang = 'kk-KK'  # TODO: get lang from source s3 object headers
    elif tag == 'khural':
        lang = 'ru-RU'
    else:
        raise ValueError(f'Unexpected tag: {tag}')

    for record_data, record_file_data in zip(records_data, downloaded_records):
        if record_file_data is None:
            continue

        if tag == 'khural' and record_file_data.channel_count > 1:
            print(f'khural audio {record_data["Key"]} has {record_file_data.channel_count} channels, skip it')
            continue

        record_id = generate_id()

        source_s3_obj = S3Object(
            endpoint=source_endpoint,
            bucket=source_bucket,
            key=record_data['Key'],
        )
        s3_obj = S3Object(
            endpoint=s3_consts.cloud_endpoint,
            bucket=s3_consts.data_bucket,
            key=os.path.join(s3_dir, f'{record_id}.raw'),
        )
        s3.put_object(Bucket=s3_obj.bucket, Key=s3_obj.key, Body=record_file_data.raw_data)

        record = Record(
            id=record_id,
            s3_obj=s3_obj,
            mark=assign_mark(),
            source=RecordSourceImport.create_s3_bucket(
                S3BucketImportData(
                    source_s3_obj=source_s3_obj,
                    last_modified=record_data['LastModified'],
                    tag=tag,
                ),
            ),
            req_params=RecordRequestParams(
                recognition_spec={
                    'audio_encoding': 1,
                    'language_code': lang,
                    'sample_rate_hertz': record_file_data.sample_rate_hertz,
                },
            ),
            audio_params=RecordAudioParams(
                acoustic='unknown',
                duration_seconds=record_file_data.duration_seconds,
                size_bytes=len(record_file_data.raw_data),
                channel_count=record_file_data.channel_count,
            ),
            received_at=received_at,
            other=None,
        )
        records.append(record)
        records_audio.append(RecordAudio(
            record_id=record_id,
            audio=None,
            hash=fast_crc32(record_file_data.raw_data),
            hash_version=HashVersion.CRC_32,
        ))
        records_tags += [
            RecordTag.add(record=record, data=record_tag_data, received_at=received_at),
            RecordTag.add(record=record, data=RecordTagData.create_period(received_at), received_at=received_at),
            RecordTag.add(record=record, data=RecordTagData.create_lang(lang), received_at=received_at),
        ]

    print(f'imported {len(records)} records')

    table_records = Table(meta=table_records_meta, name=table_name)
    table_records_audio = Table(meta=table_records_audio_meta, name=table_name)
    table_records_tags = Table(meta=table_records_tags_meta, name=table_name)

    table_records.append_objects(records)
    table_records_audio.append_objects(records_audio)
    table_records_tags.append_objects(records_tags)


def get_imported_s3_keys_and_latest_record_ts(tag: str) -> typing.Tuple[typing.Set[str], typing.Optional[datetime]]:
    tables, _ = run_yql_select_query(f"""
USE {os.environ['YT_PROXY']};

$records = (
    SELECT
        Yson::YPathString(source, '/data/last_modified') AS last_modified,
        Yson::YPathString(source, '/data/source_s3_obj/key') AS s3_key
    FROM
        RANGE(`{table_records_meta.dir_path}`)
    WHERE
        Yson::LookupString(source, 'version') = '{RecordSourceType.IMPORT.value}' AND
        Yson::LookupString(source, 'source') = '{ImportSource.S3_BUCKET.value}' AND
        Yson::YPathString(source, '/data/tag') = '{tag}'
);

SELECT
    s3_key
FROM
    $records;

SELECT
    last_modified
FROM
    $records
ORDER BY
    last_modified DESC
LIMIT
    1;
""")
    s3_keys_rows, last_modified_rows = tables
    s3_keys = {x['s3_key'] for x in s3_keys_rows}
    last_modified = parse_datetime(last_modified_rows[0]['last_modified']) if len(last_modified_rows) > 0 else None
    return s3_keys, last_modified


def sample_records(records_data: typing.List[dict], sample_size: typing.Optional[int]) -> typing.List[dict]:
    if sample_size is None or sample_size < 1:
        return records_data
    records_data = sorted(records_data, key=lambda x: x['LastModified'])
    last_record = records_data.pop()  # always get last record to remember last modified
    shuffle(records_data)
    return records_data[:sample_size - 1] + [last_record]
