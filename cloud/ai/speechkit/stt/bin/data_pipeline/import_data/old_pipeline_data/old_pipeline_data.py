import argparse
import io
import json
import os
import typing
from datetime import datetime

import boto3
import pytz
import requests
import yt.wrapper as yt
from botocore.exceptions import ClientError
from pydub import AudioSegment
from pydub.exceptions import CouldntDecodeError

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data.model.common import s3_consts
from cloud.ai.speechkit.stt.lib.data.model.common.hash import crc32
from cloud.ai.speechkit.stt.lib.data.model.common.id import generate_id
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    MarkupAssignmentStatus,
    Record,
    S3Object,
    HashVersion,
    RecordSourceImport,
    YandexCallCenterImportData,
    RecordBit,
    SplitDataFixedLengthOffset,
    BitDataTimeInterval,
    ConvertDataCmd,
    RecordRequestParams,
    RecordAudioParams,
    RecordBitMarkup,
    SoxEffectsObfuscationData,
    Mark,
    RecordTagType,
    RecordTag,
    RecordTagData,
    MarkupStep,
)
from cloud.ai.speechkit.stt.lib.data.ops.collect_records import RecordData, create_record_from_data, assign_mark
from cloud.ai.speechkit.stt.lib.data.ops.yt import (
    table_records_meta,
    table_records_tags_meta,
    table_records_bits_meta,
    table_records_bits_markups_meta,
    table_markups_assignments_meta,
    table_records_joins_meta,
)
from cloud.ai.speechkit.stt.lib.data_pipeline.files import get_name_for_record_bit_audio_file_from_record_bit
from cloud.ai.speechkit.stt.lib.data_pipeline.join import join_markups
from cloud.ai.speechkit.stt.lib.data_pipeline.toloka import TolokaClient


def _parse_args():
    parser = argparse.ArgumentParser(
        description='Import records, its bit, markups and joins, created by old '
                    'flow https://nirvana.yandex-team.ru/flow/13a82851-cdc3-46d5-8049-9249eaf97f59. '
                    'Run with environment variables '
                    'YT_PROXY, YT_TOKEN, YQL_TOKEN, AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY, TOLOKA_OAUTH_TOKEN.',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        '-records_table_path',
        help='Path to Hahn YT table with source records',
        type=str,
    )
    parser.add_argument(
        '-records_limit',
        help='Limit of records to import (for test purposes)',
        type=int,
        default=0,
    )
    parser.add_argument(
        '-store_to_hume',
        action='store_true',
        help='Create result YT tables in Hume cluster (for test purposes)',
    )
    parser.add_argument(
        '-source',
        help='Records source: Ural Innovations, call center, RZD, ...',
        type=str,
    )
    parser.add_argument(
        '-toloka_pool_id',
        help='ID of Toloka pool with markup results',
        type=str,
    )
    parser.add_argument(
        '-result_tables_name',
        help='Name of result YT table with format YYYY-MM-DD',
        type=str,
    )
    parser.add_argument(
        '-convert_sox_effects',
        help='Additional sox effects during conversion from source format',
        type=str,
        default='',
    )
    parser.add_argument(
        '-tags',
        help='Additional tags for records, comma-separated list of <TAG_TYPE>:<TAG_VALUE>, '
             'i.e "CONVERT:old-pipeline,IMPORT:call-center"',
        type=str,
    )
    parser.add_argument(
        '-markup_joiner_executable',
        help='path to markup joiner executable',
        required=True,
    )
    parser.add_argument(
        '-test_basket_file_names_path',
        help='Path to file names which will be marked as TEST and will be tagged as OTHER:<-test_basket_tag_value>',
        default='',
    )
    parser.add_argument(
        '-test_basket_tag_value',
        help='Value of tag with type OTHER for test basket',
        default='',
    )
    parser.add_argument(
        '-dry_run',
        action='store_true',
        help='Dry run: do not upload S3 objects',
    )
    return parser.parse_args()


def main():
    run(**vars(_parse_args()))


source_ui = 'ui'
source_cc = 'cc'

call_center_chosen_channel = 1


# Currently works only for Ural Innovations data
def run(
    records_table_path,
    records_limit,
    store_to_hume,
    source,
    toloka_pool_id,
    result_tables_name,
    convert_sox_effects,
    tags,
    markup_joiner_executable,
    test_basket_file_names_path,
    test_basket_tag_value,
    dry_run,
):
    check_tmp_dir_status = os.system('ls records')
    tmp_dirs = 'records records_bits records_bits_up_sampled records_bits_obfuscated records_bits_obfuscated_up_sampled'
    if check_tmp_dir_status == 0:
        os.system('rm -r ' + tmp_dirs)
    os.system('mkdir ' + tmp_dirs)

    assert source in [source_ui, source_cc]

    audio_url_column_name = {
        source_ui: 'url',
        source_cc: 'call_url',
    }[source]

    # Read records urls from YT table
    urls = []
    read_count = 0
    for row in yt.read_table(records_table_path):
        if 0 < records_limit <= read_count:
            break
        urls.append(row[audio_url_column_name])
        read_count += 1

    if store_to_hume:
        print('Creating YT tables in Hume')
        yt.config["proxy"]["url"] = "hume"

    if len(test_basket_file_names_path) > 0:
        if source != source_cc:
            raise ValueError
        with open(test_basket_file_names_path) as f:
            test_basket_file_names = set(json.loads(f.read()))
        print(
            'Test basket will be tagged as OTHER:%s and contain files %s'
            % (test_basket_tag_value, test_basket_file_names)
        )
    else:
        test_basket_file_names = set([])

    # Calc records duration with pydub, optionally
    # take raw data if it's .wav
    source_file_to_audio_data = {}
    not_decoded_source_records = 0
    for url in urls:
        source_file = url.rsplit('/', 1)[1]
        audio_data = requests.get(url).content
        path = 'records/%s' % source_file
        with open(path, 'wb') as f:
            f.write(audio_data)
        try:
            if source == source_ui:
                audio_segment = AudioSegment.from_wav(path)
                assert audio_segment.channels == 1
                assert audio_segment.sample_width == 2
                assert audio_segment.frame_rate == 8000
            elif source == source_cc:
                audio_segment = AudioSegment.from_mp3(path)
                assert audio_segment.channels == 2
                assert audio_segment.sample_width == 2
                assert audio_segment.frame_rate == 8000
                target_path = 'records/conv_%s' % source_file
                sox_command = 'sox -t mp3 {source_file} -r 8k -t wav {target_file} {sox_effects}'.format(
                    source_file=path,
                    target_file=target_path,
                    sox_effects=convert_sox_effects,
                )
                exit_code = os.system(sox_command)
                if exit_code != 0:
                    raise ValueError('sox returned %d exit code for source file %s' % (exit_code, source_file))
                audio_segment = AudioSegment.from_wav(target_path)
                assert audio_segment.channels == 1
                assert audio_segment.sample_width == 2
                assert audio_segment.frame_rate == 8000
            else:
                raise RuntimeError
        except CouldntDecodeError:
            print('Source record %s couldn\'t be decoded, skip' % source_file)
            not_decoded_source_records += 1
            continue
        audio_duration_ms = audio_segment.__len__()
        if float(audio_duration_ms) > (4.95 * 60 * 1000):
            if source_file in test_basket_file_names:
                raise ValueError('Test basket file %s is longer than max duration' % source_file)
            print('Skip source record %s because it\'s too long' % source_file)
            continue
        print('%s: %f' % (source_file, audio_duration_ms))
        source_file_to_audio_data[source_file] = {
            'raw_data': audio_segment.raw_data,
            'duration_seconds': float(audio_duration_ms) / 1000.0,
            'url': url,
        }

    print('Not decoded source records ratio: %.3f' % (float(not_decoded_source_records) / float(len(urls))))

    s3 = get_s3()

    source_file_to_record = {}
    records = []
    records_tags = []

    year, month, day = (int(x) for x in result_tables_name.split('-'))
    old_pipeline_run_at = datetime(year=year, month=month, day=day, tzinfo=pytz.UTC)

    tags_data_list = create_tags_data_list(tags)

    # Create Record and RecordTag objects, upload .raw to S3
    for source_file, audio_data in source_file_to_audio_data.items():
        record_id = generate_id()
        duration_seconds = audio_data['duration_seconds']
        audio = audio_data['raw_data']
        url = audio_data['url']

        s3_obj = S3Object(
            endpoint=s3_consts.cloud_endpoint,
            bucket=s3_consts.data_bucket,
            key='Speechkit/STT/Data/%s/%s.raw' % (result_tables_name.replace('-', '/'), record_id),
        )

        if source == source_ui:
            record_data = RecordData(
                id=record_id,
                s3_obj=s3_obj,
                hash=crc32(audio),
                hash_version=HashVersion.CRC_32_BZIP2,
                req_params=RecordRequestParams(
                    recognition_spec={
                        'audio_encoding': 1,
                        'language_code': 'ru-RU',
                        'model': 'general',
                        'partial_results': True,
                        'sample_rate_hertz': 8000,
                    }
                ),
                duration_seconds=duration_seconds,
                size_bytes=len(audio),
                folder_id='b1g0gu61j6fvcih43l9l',
                method='speechkit.stt_stream',
                received_at=old_pipeline_run_at,
                audio_channel_count=1,
            )
            record, _, record_tags = create_record_from_data(record_data)
        elif source == source_cc:
            if source_file in test_basket_file_names:
                print('Place %s into test basket' % source_file)
                mark = Mark.TEST
                tag_data = RecordTagData(type=RecordTagType.OTHER, value=test_basket_tag_value)
            else:
                mark = assign_mark()
                tag_data = None
            record = Record(
                id=record_id,
                s3_obj=s3_obj,
                mark=mark,
                source=RecordSourceImport.create_yandex_call_center(
                    YandexCallCenterImportData(
                        yt_table_path=records_table_path,
                        source_audio_url=url,
                        channel=call_center_chosen_channel,
                    ),
                ),
                req_params=RecordRequestParams(
                    recognition_spec={'audio_encoding': 1, 'language_code': 'ru-RU', 'sample_rate_hertz': 8000},
                ),
                audio_params=RecordAudioParams(
                    acoustic='phone',
                    duration_seconds=duration_seconds,
                    size_bytes=len(audio),
                    channel_count=1,
                ),
                received_at=old_pipeline_run_at,
                other=None,
            )
            if tag_data is None:
                record_tags = []
            else:
                record_tags = [RecordTag.add(record, tag_data, old_pipeline_run_at)]
        else:
            raise RuntimeError

        print('S3 PUT %s' % record.s3_obj.to_https_url())
        if not dry_run:
            s3.put_object(Bucket=record.s3_obj.bucket, Key=record.s3_obj.key, Body=audio)

        records.append(record)
        for tag in record_tags:
            records_tags.append(tag)
        source_file_to_record[source_file] = record

        for tag_data in tags_data_list:
            records_tags.append(RecordTag.add(record=record, data=tag_data, received_at=old_pipeline_run_at))

    print('Source audio count: %d' % len(source_file_to_audio_data))

    records_bits = []

    bit_id_to_up_sampled_obfuscated_audio = {}
    old_pipeline_filename_to_bit_id = {}

    if source == source_ui:
        bit_s3_key_prefix = 'Data/Speechkit/toloka/private/ural_innovations/%s'
        obfuscated_bit_s3_key_prefix = 'Data/Speechkit/toloka/ural_innovations/%s'
    elif source == source_cc:
        bit_s3_key_prefix = 'Data/Speechkit/toloka/private/call_center/%s'
        obfuscated_bit_s3_key_prefix = 'Data/Speechkit/toloka/call_center/%s'
    else:
        raise RuntimeError

    records_with_incomplete_bits_data = 0

    # Download bits .wav from S3, get bit range, up-sample .wav to 16KHz, create RecordBit objects
    for source_file, record in source_file_to_record.items():
        source_file_without_ext = source_file.rsplit('.', 1)[0]

        # Bit key is like
        # Data/Speechkit/toloka/private/ural_innovations/0053592b-9aaa-412b-9c05-06b1a2393aa6_4fb4ef41-e516-4705-a0c3-a8d340a8105a_3_0-9000.wav
        # Data/Speechkit/toloka/private/call_center/...
        #
        # Obfuscated bit key is like
        # Data/Speechkit/toloka/ural_innovations/0053592b-9aaa-412b-9c05-06b1a2393aa6_4fb4ef41-e516-4705-a0c3-a8d340a8105a_3_0-9000.wav
        # Data/Speechkit/toloka/call_center/...

        bits_resp = s3.list_objects_v2(Bucket=s3_consts.data_bucket, Prefix=bit_s3_key_prefix % source_file_without_ext)

        if 'Contents' not in bits_resp or len(bits_resp['Contents']) == 0:
            print('No bits in S3 for record %s' % source_file)
            continue

        obfuscated_bits_resp = s3.list_objects_v2(
            Bucket=s3_consts.data_bucket, Prefix=obfuscated_bit_s3_key_prefix % source_file_without_ext
        )

        if 'Contents' not in bits_resp or len(bits_resp['Contents']) == 0:
            raise ValueError('No obfuscated audio bits in S3 for record %s' % record.id)

        def process_bits(s3_list_resp, source_bits_dir, up_sampled_bits_dir):
            bit_data_list = []
            for item in s3_list_resp['Contents']:
                bit_key = item['Key']
                bit_range = bit_key.split('_')[-1].split('.')[0]
                bit_left, bit_right = (int(x) for x in bit_range.split('-'))
                bit_wav_data = s3.get_object(Bucket=s3_consts.data_bucket, Key=bit_key)['Body'].read()
                path = '%s/%s_%s.wav' % (source_bits_dir, source_file_without_ext, bit_range)
                with open(path, 'wb') as f:
                    f.write(bit_wav_data)

                audio_segment = AudioSegment.from_wav(path)
                audio_segment = audio_segment.set_frame_rate(16000)
                up_sampled_path = '%s/%s_%s.wav' % (up_sampled_bits_dir, source_file_without_ext, bit_range)
                audio_segment.export(up_sampled_path, 'wav')

                with open(up_sampled_path, 'rb') as f:
                    bit_wav_up_sampled_data = f.read()

                bit_filename = bit_key.split('/')[-1]
                bit_data_list.append((bit_left, bit_right, bit_wav_up_sampled_data, bit_filename))
                bit_data_list = sorted(bit_data_list, key=lambda x: x[0])

            return bit_data_list

        bit_data_list = process_bits(bits_resp, 'records_bits', 'records_bits_up_sampled')
        obfuscated_bit_data_list = process_bits(
            obfuscated_bits_resp, 'records_bits_obfuscated', 'records_bits_obfuscated_up_sampled'
        )

        obfuscated_filename_to_up_sampled_data = {x[3]: x[2] for x in obfuscated_bit_data_list}

        bits_without_associated_obfuscated_audio = 0

        for bit_index, bit_data in enumerate(bit_data_list):
            bit_left, bit_right, bit_wav_data, bit_filename = bit_data

            if bit_filename not in obfuscated_filename_to_up_sampled_data:
                bits_without_associated_obfuscated_audio += 1
                print('Bit %s has no associated obfuscated audio in S3' % bit_filename)
                continue

            record_bit, _ = RecordBit.create(
                record=record,
                split_data=SplitDataFixedLengthOffset(
                    length_ms=9000,
                    offset_ms=3000,
                    split_cmd='pydub-0.23.1',
                ),
                received_at=old_pipeline_run_at,
                bit_data=BitDataTimeInterval(
                    start_ms=bit_left,
                    end_ms=bit_right,
                    index=bit_index,
                ),
                convert_data=ConvertDataCmd(
                    '%spydub-0.23.1 AudioSegment.set_frame_rate(16000)'
                    % ('' if len(convert_sox_effects) == 0 else ('%s; ' % convert_sox_effects))
                ),
                audio=bit_wav_data,
                sample_rate_hertz_native=16000,
            )

            print('S3 PUT %s' % record_bit.s3_obj.to_https_url())
            if not dry_run:
                s3.put_object(
                    Bucket=record_bit.s3_obj.bucket,
                    Key=record_bit.s3_obj.key,
                    Body=bit_wav_data,
                )

            bit_id_to_up_sampled_obfuscated_audio[record_bit.id] = obfuscated_filename_to_up_sampled_data[bit_filename]
            old_pipeline_filename_to_bit_id[bit_filename] = record_bit.id

            records_bits.append(record_bit)

        if bits_without_associated_obfuscated_audio > 0:
            print(
                'Record %s has %d bits without associated obfuscated audio, so data about it is incomplete'
                % (record.id, bits_without_associated_obfuscated_audio)
            )
            records_with_incomplete_bits_data += 1

    if records_with_incomplete_bits_data > 0:
        print(
            'There are %d records with incomplete bit data (because some bits not associated with obfuscated audio)'
            % records_with_incomplete_bits_data
        )

    id_to_bit = {r.id: r for r in records_bits}

    markup_assignments = TolokaClient(
        oauth_token=os.environ['TOLOKA_OAUTH_TOKEN'],
    ).get_assignments(pool_id=toloka_pool_id, markup_step=MarkupStep.TRANSCRIPT)

    records_bits_markups = []
    records_bits_markups_with_bit_data = []

    print('Old pipeline filenames of bits: %s' % old_pipeline_filename_to_bit_id.keys())

    print(
        'Accepted assignments ratio: %.3f'
        % (float(len(markup_assignments)) / float(len([a for a in markup_assignments if a.data.status == 'ACCEPTED'])))
    )

    for assignment in markup_assignments:
        for task in assignment.tasks:
            obfuscated_audio_s3_key = task.input.audio_s3_obj.key
            old_pipeline_filename = obfuscated_audio_s3_key.split('/')[-1]
            if old_pipeline_filename not in old_pipeline_filename_to_bit_id:
                if records_limit < 1:
                    print(
                        'Skip task with input URL %s, perhaps it is honeypot' % task.input.audio_s3_obj.to_https_url()
                    )
                continue
            bit_id = old_pipeline_filename_to_bit_id[old_pipeline_filename]
            bit = id_to_bit[bit_id]
            up_sampled_obfuscated_audio = bit_id_to_up_sampled_obfuscated_audio[bit_id]

            print(
                'Found matching bit %s for Toloka task with input URL %s and transcription "%s"'
                % (bit_id, task.input.audio_s3_obj.to_https_url(), task.solution.text)
            )

            task.input.audio_s3_obj = S3Object(
                endpoint=s3_consts.cloud_endpoint,
                bucket=s3_consts.data_bucket,
                key='Speechkit/STT/Toloka/Assignments/%s/%s'
                    % (result_tables_name.replace('-', '/'), get_name_for_record_bit_audio_file_from_record_bit(bit)),
            )

            record_bit_markup = RecordBitMarkup.create(
                record_bit=bit,
                audio_obfuscation_data=SoxEffectsObfuscationData(
                    sox_effects='sox -t wav {old_filename} -t wav {new_filename} --norm pitch -300'
                ),
                pool_id=toloka_pool_id,
                assignment_id=assignment.id,
                markup_data=task,
                received_at=old_pipeline_run_at,
            )

            try:
                s3.head_object(Bucket=task.input.audio_s3_obj.bucket, Key=task.input.audio_s3_obj.key)
            except ClientError:
                print('S3 PUT %s' % task.input.audio_s3_obj.to_https_url())
                if not dry_run:
                    audio_io = io.BytesIO(up_sampled_obfuscated_audio)
                    s3.upload_fileobj(
                        Bucket=task.input.audio_s3_obj.bucket,
                        Key=task.input.audio_s3_obj.key,
                        Fileobj=audio_io,
                        ExtraArgs={'ACL': 'public-read'},
                    )
            else:
                print('Obfuscated audio %s is already uploaded to S3' % task.input.audio_s3_obj.to_https_url())

            if assignment.data.status == MarkupAssignmentStatus.ACCEPTED:
                records_bits_markups.append(record_bit_markup)
                records_bits_markups_with_bit_data.append((record_bit_markup, bit.bit_data))

    records_joins = join_markups(
        records_bits_markups_with_bit_data, markup_joiner_executable, 3000, old_pipeline_run_at
    )

    table_name = Table.get_name(old_pipeline_run_at)
    table_records = Table(meta=table_records_meta, name=table_name)
    table_records_tags = Table(meta=table_records_tags_meta, name=table_name)
    table_records_bits = Table(meta=table_records_bits_meta, name=table_name)
    table_records_bits_markups = Table(meta=table_records_bits_markups_meta, name=table_name)
    table_markups_assignments = Table(meta=table_markups_assignments_meta, name=table_name)
    table_records_joins = Table(meta=table_records_joins_meta, name=table_name)

    table_records.append_objects(records)
    table_records_tags.append_objects(records_tags)
    table_records_bits.append_objects(records_bits)
    table_records_bits_markups.append_objects(records_bits_markups)
    table_markups_assignments.append_objects(markup_assignments)
    table_records_joins.append_objects(records_joins)


def get_s3():
    session = boto3.session.Session()
    return session.client(
        service_name='s3',
        endpoint_url=s3_consts.cloud_url,
    )


def create_tags_data_list(tags_str: typing.Optional[str]) -> typing.List[RecordTagData]:
    if tags_str is None or len(tags_str) == 0:
        return []
    tags_data_list = []
    for tag_str in tags_str.split(','):
        tags_data_list.append(RecordTagData.from_str(tag_str))
    return tags_data_list
