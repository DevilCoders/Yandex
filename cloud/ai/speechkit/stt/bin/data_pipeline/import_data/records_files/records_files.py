import argparse
import json
import os
import typing
from multiprocessing.pool import Pool, ThreadPool

import yt.wrapper as yt

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.lib.python.datetime import now
from cloud.ai.speechkit.stt.lib.data.model.common import s3_consts
from cloud.ai.speechkit.stt.lib.data.model.common.hash import crc32
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    Record,
    RecordAudio,
    S3Object,
    HashVersion,
    Mark,
    RecordSourceExemplar,
    RecordRequestParams,
    RecordAudioParams,
    RecordJoin,
    JoinDataVersions,
    JoinDataExemplar,
    JoinDataExternal,
    RecognitionPlainTranscript,
    RecordTag,
    RecordTagData,
    RecordTagType,
    RecordSourceImport,
    FilesImportData,
    ImportSource,
)
from cloud.ai.speechkit.stt.lib.data.ops.collect_records import assign_mark
from cloud.ai.speechkit.stt.lib.data.ops.yt import (
    table_records_meta,
    table_records_audio_meta,
    table_records_tags_meta,
    table_records_joins_meta,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.import_data.records import process_file
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client


def _parse_args():
    parser = argparse.ArgumentParser(
        description='Import records from audio files with optional references',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument('-audio_files_path', help='Path to directory with audio files (.wav or .mp3)')
    parser.add_argument('-references_path', help='(optional) Path to JSON file with references')
    parser.add_argument(
        '-tag_type',
        help='Tag type (IMPORT or EXEMPLAR)',
        type=str,
    )
    parser.add_argument(
        '-tag_value',
        help='Tag value (suffix for IMPORT type)',
        type=str,
    )
    parser.add_argument(
        '-ticket',
        help='Ticket, required for IMPORT tag type',
        type=str,
    )
    parser.add_argument(
        '-language',
        help='Records language',
        type=str,
    )
    parser.add_argument(
        '-text_type',
        help='(optional) Text type: exemplar|external',
        type=str,
        default='external',
    )
    parser.add_argument(
        '-external_text_comment',
        type=str,
        help='(optional) Comment for external text',
        default='',
    )
    parser.add_argument(
        '-mark',
        help='Records mark: TRAIN|VAL|TEST|AUTO',
        type=str,
    )
    parser.add_argument(
        '-folder_id',
        help='(optional) If records refer to specific partner folder, specify it',
        type=str,
    )
    return parser.parse_args()


def main():
    run(**vars(_parse_args()))


def calc_hash(data: typing.Tuple[str, bytes]) -> typing.Tuple[str, bytes]:
    file_name, raw_data = data
    return file_name, crc32(raw_data)


def run(
    audio_files_path,
    references_path,
    tag_type,
    tag_value,
    ticket,
    language,
    text_type,
    external_text_comment,
    mark,
    folder_id,
):
    yt.config['proxy']['url'] = 'hahn'

    tag_type = RecordTagType(tag_type)
    assert tag_type in [RecordTagType.EXEMPLAR, RecordTagType.IMPORT]

    if tag_type == RecordTagType.IMPORT:
        assert ticket is not None and len(ticket) > 0
        record_tag_data = RecordTagData(type=tag_type, value=f'{ImportSource.FILES.value}_{tag_value}')
    else:
        record_tag_data = RecordTagData(type=tag_type, value=tag_value)

    assert folder_id is None or folder_id.startswith('b1g')
    if folder_id is not None and len(folder_id) == 0:
        folder_id = None

    if mark == 'AUTO':
        mark = None
    else:
        mark = Mark(mark)

    join_data = None
    if references_path:
        join_version = JoinDataVersions(text_type)
        assert join_version in (JoinDataVersions.EXEMPLAR, JoinDataVersions.EXTERNAL)
        if join_version == JoinDataVersions.EXEMPLAR:
            join_data = JoinDataExemplar()
        else:
            comment = external_text_comment or ticket
            join_data = JoinDataExternal(comment=comment)

    print('Reading files from disk')

    file_name_to_record_data = {}
    for audio_file_name in os.listdir(audio_files_path):
        if audio_file_name.startswith('.'):
            continue
        record_file_data = process_file(os.path.join(audio_files_path, audio_file_name))
        file_name_to_record_data[audio_file_name] = record_file_data

    file_name_to_reference = {}
    if references_path:
        with open(references_path) as f:
            file_name_to_reference = json.load(f)
        assert file_name_to_record_data.keys() > file_name_to_reference.keys()

    received_at = now()

    table_name = Table.get_name(received_at)

    print('Calculating hashes')

    pool = Pool(processes=16)

    hash_data = pool.map(
        calc_hash,
        [(file_name, record_data.raw_data) for file_name, record_data in file_name_to_record_data.items()],
    )
    file_name_to_crc32 = {file_name: crc32_hash for file_name, crc32_hash in hash_data}

    print('Making objects')

    records = []
    records_audio = []
    records_tags = []
    records_joins = []

    s3_data_list = []

    for file_name in file_name_to_record_data.keys():
        record_data = file_name_to_record_data[file_name]

        record_id = record_data.record_id
        s3_obj = S3Object(
            endpoint=s3_consts.cloud_endpoint,
            bucket=s3_consts.data_bucket,
            key=f'Speechkit/STT/Data/{table_name.replace("-", "/")}/{record_id}.raw',
        )

        audio = record_data.raw_data

        s3_data_list.append((audio, s3_obj))

        if record_tag_data.type == RecordTagType.IMPORT:
            source = RecordSourceImport.create_files(
                FilesImportData(
                    ticket=ticket,
                    source_file_name=file_name,
                    folder_id=folder_id,
                )
            )
        else:
            source = RecordSourceExemplar()

        record = Record(
            id=record_id,
            s3_obj=s3_obj,
            mark=mark or assign_mark(),
            source=source,
            req_params=RecordRequestParams(
                recognition_spec={
                    'audio_encoding': 1,
                    'language_code': language,
                    'sample_rate_hertz': record_data.sample_rate_hertz,
                },
            ),
            audio_params=RecordAudioParams(
                acoustic='unknown',
                duration_seconds=record_data.duration_seconds,
                size_bytes=len(audio),
                channel_count=record_data.channel_count,
            ),
            received_at=received_at,
            other=None,
        )
        record_audio = RecordAudio(
            record_id=record_id,
            audio=None,
            hash=file_name_to_crc32[file_name],
            hash_version=HashVersion.CRC_32_BZIP2,
        )
        records.append(record)
        records_audio.append(record_audio)

        records_tags.append(
            RecordTag.add(record=record, data=record_tag_data, received_at=received_at)
        )
        records_tags.append(
            RecordTag.add(record=record, data=RecordTagData.create_period(received_at), received_at=received_at)
        )
        records_tags.append(
            RecordTag.add(record=record, data=RecordTagData.create_lang(language), received_at=received_at)
        )

        if file_name in file_name_to_reference:
            records_joins.append(
                RecordJoin.create_by_record(
                    record=record,
                    recognition=RecognitionPlainTranscript(text=file_name_to_reference[file_name]),
                    join_data=join_data,
                    received_at=received_at,
                    other=None,
                )
            )

    s3 = create_client()

    def put_audio_to_s3(audio_data: typing.Tuple[bytes, S3Object]):
        audio_bytes, s3_obj = audio_data
        s3.put_object(
            Bucket=s3_obj.bucket,
            Key=s3_obj.key,
            Body=audio_bytes,
        )

    print('Uploading to S3')

    pool = ThreadPool(processes=16)

    pool.map(put_audio_to_s3, s3_data_list)

    table_records = Table(meta=table_records_meta, name=table_name)
    table_records_audio = Table(meta=table_records_audio_meta, name=table_name)
    table_records_tags = Table(meta=table_records_tags_meta, name=table_name)
    table_records_joins = Table(meta=table_records_joins_meta, name=table_name)

    table_records.append_objects(records)
    table_records_audio.append_objects(records_audio)
    table_records_tags.append_objects(records_tags)
    if references_path:
        table_records_joins.append_objects(records_joins)
