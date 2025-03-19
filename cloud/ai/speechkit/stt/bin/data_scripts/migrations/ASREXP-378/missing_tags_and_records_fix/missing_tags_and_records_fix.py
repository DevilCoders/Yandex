import argparse
from collections import defaultdict
from datetime import datetime

import yt.wrapper as yt
import yt.yson as yson
import pytz

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    Record,
    RecordTag,
    RecordTagType,
    RecordTagData,
    RecordSourceCloud,
    RecordSourceImport,
    YandexCallCenterImportData,
)
from cloud.ai.speechkit.stt.lib.data.ops.yt import table_records_meta, table_records_tags_meta
from cloud.ai.speechkit.stt.lib.data.model.registry.tags import folder_id_to_client_slug


def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-from_table', default='0000-01-01')
    parser.add_argument('-to_table', default='9999-12-31')
    parser.add_argument(
        '-dry_run',
        action='store_true',
        help='Dry run: do not change YT tables, only log changes.',
    )
    return parser.parse_args()


def main():
    run(**vars(_parse_args()))


def run(from_table: str, to_table: str, dry_run: bool):
    if dry_run:
        print('DRY RUN')
    for table_name in yt.list(table_records_meta.dir_path):
        if from_table <= table_name <= to_table:
            process_tables(table_name, dry_run)


"""
Important: script is based on invariant that records tags of type CLIENT for records are always located
within same table name, because CLIENT tags are always created with records in the same time.

1. Add LANG tags if record doesn't have one.
2. Add PERIOD tags if record doesn't have one.
3. Add MODE tags for CLIENT records if record doesn't have one.
4. Add missing data for old pipeline IMPORT:yandex-call-center records.
5. Fix version v1->v2 for 'stream' methods.
6. Fix CLIENT tags so that value always contains folder suffix.
"""


def process_tables(table_name: str, dry_run: bool):
    print(f'Processing tables {table_name}')
    table_records = Table(meta=table_records_meta, name=table_name)
    table_records_tags = Table(meta=table_records_tags_meta, name=table_name)

    records = []
    for record_row in yt.read_table(table_records.path, format=yt.YsonFormat(encoding=None)):
        records.append(Record.from_yson(record_row))

    records_tags = []
    for record_tag_row in yt.read_table(table_records_tags.path, format=yt.YsonFormat(encoding=None)):
        records_tags.append(RecordTag.from_yson(record_tag_row))

    record_id_to_tags = defaultdict(list)
    for record_tag in records_tags:
        record_id_to_tags[record_tag.record_id].append(record_tag)

    cc_records_to_fix = []

    for record in records:
        record.req_params.fix_lang()

        tags = record_id_to_tags[record.id]

        if all(tag.data.type != RecordTagType.LANG for tag in tags):
            tag_data = RecordTagData.create_lang(record.req_params.get_language_code())
            records_tags.append(
                RecordTag.add(
                    record=record,
                    data=tag_data,
                    received_at=record.received_at,
                )
            )
            print(f'[add] tag {tag_data.to_str()} for record {record.id}')
        else:
            print(f'[skp] record {record.id} already contains LANG tag')

        if all(tag.data.type != RecordTagType.PERIOD for tag in tags):
            tag_data = RecordTagData.create_period(record.received_at)
            records_tags.append(
                RecordTag.add(
                    record=record,
                    data=tag_data,
                    received_at=record.received_at,
                )
            )
            print(f'[add] tag {tag_data.to_str()} for record {record.id}')
        else:
            print(f'[skp] record {record.id} already contains PERIOD tag')

        if isinstance(record.source, RecordSourceCloud):
            if all(tag.data.type != RecordTagType.MODE for tag in tags):
                tag_data = RecordTagData(type=RecordTagType.MODE, value=record.source.method.name)
                records_tags.append(
                    RecordTag.add(
                        record=record,
                        data=tag_data,
                        received_at=record.received_at,
                    )
                )
                print(f'[add] tag {tag_data.to_str()} for record {record.id}')
            else:
                print(f'[skp] record {record.id} with cloud source already contains MODE tag')

            if record.source.method.name == 'stream':
                if record.source.method.version == 'v1':
                    record.source.method.version = 'v2'
                    print(f'[fix] record {record.id} method version v1->v2')
                else:
                    print(f'[skp] record {record.id} has right stream method version')

            folder_id = record.source.folder_id
            for tag in tags:
                if tag.data.type == RecordTagType.CLIENT:
                    expected_client_slug = folder_id_to_client_slug[folder_id]
                    actual_client_slug = tag.data.value
                    if expected_client_slug == actual_client_slug:
                        print(f'[skp] tag {tag.data.to_str()} is already correct')
                    else:
                        old_data = tag.data.to_str()
                        tag.data.value = expected_client_slug
                        print(f'[fix] fix tag {old_data} to {tag.data.to_str()}')

        if isinstance(record.source, RecordSourceImport) and isinstance(record.source.data, YandexCallCenterImportData):
            if record.source.data.call_reason == None:
                cc_records_to_fix.append(record)
            else:
                print(f'[skp] record {record.id} from call center has all fields')

    cc_table_paths = set([])
    for record in cc_records_to_fix:
        cc_table_paths.add(record.source.data.yt_table_path)

    yt_proxy = yt.config["proxy"]["url"]
    yt.config["proxy"]["url"] = 'hahn'

    cc_audio_url_to_data = {}
    for path in cc_table_paths:
        rows = yt.read_table(path)
        for row in rows:
            cc_audio_url_to_data[row['call_url']] = row

    yt.config["proxy"]["url"] = yt_proxy

    for record in cc_records_to_fix:
        data = cc_audio_url_to_data[record.source.data.source_audio_url]
        record.source.data.call_reason = data['call_reason']
        record.source.data.phone = data['phone']
        record.source.data.call_time = data['call_time']
        record.source.data.call_duration = data['call_duration']
        record.source.data.call_status = data['call_status']
        record.source.data.create_time = parse_datetime_from_unix_timestamp(data['create_time'])
        print(f'[fix] add missing fields for record {record.id} from call center')

    if dry_run:
        return

    records_table_path = f'{table_records_meta.dir_path}/{table_name}'

    yt.remove(records_table_path)
    yt.create('table', records_table_path, attributes=yson.json_to_yson(table_records_meta.attrs))
    yt.write_table(records_table_path, (r.to_yson() for r in sorted(records)))

    records_tags_table_path = f'{table_records_tags_meta.dir_path}/{table_name}'

    yt.remove(records_tags_table_path)
    yt.create('table', records_tags_table_path, attributes=yson.json_to_yson(table_records_tags_meta.attrs))
    yt.write_table(records_tags_table_path, (t.to_yson() for t in sorted(records_tags)))


def parse_datetime_from_unix_timestamp(timestamp):
    return pytz.utc.localize(datetime.fromtimestamp(timestamp / 1000))
