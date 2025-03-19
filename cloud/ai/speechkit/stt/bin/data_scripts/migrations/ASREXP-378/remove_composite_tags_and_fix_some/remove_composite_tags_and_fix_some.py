import argparse

import yt.wrapper as yt
import yt.yson as yson

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data.model.dao import RecordTag, RecordTagData, RecordTagType
from cloud.ai.speechkit.stt.lib.data.ops.yt import table_records_meta, table_records_tags_meta


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
1. Remove composite tags
2. Tag renames:
EXEMPLAR:just-ai-megafon-411-test-2020-05-08 -> OTHER:just-ai_megafon_411_test_2020-05-08
OTHER:call-center-288-test-2020-01 -> OTHER:yandex-call-center_288_test_2020-01
"""


def process_tables(table_name: str, dry_run: bool):
    print(f'Processing tables {table_name}')
    table_records_tags = Table(meta=table_records_tags_meta, name=table_name)

    records_tags = []
    for record_tag_row in yt.read_table(table_records_tags.path, format=yt.YsonFormat(encoding=None)):
        records_tags.append(RecordTag.from_yson(record_tag_row))

    remove_types = [RecordTagType.CLIENT_PERIOD, RecordTagType.CLIENT_ACTUAL, RecordTagType.IMPORT_PERIOD]

    def generate_fixed_tag(tag: RecordTag, new_data: RecordTagData) -> RecordTag:
        return RecordTag(
            id=tag.id,
            action=tag.action,
            record_id=tag.record_id,
            data=new_data,
            received_at=tag.received_at,
        )

    has_changes = False

    fixed_records_tags = []
    for record_tag in records_tags:
        if record_tag.data.type in remove_types:
            print(f'[del] tag {record_tag.data.to_str()} for record {record_tag.record_id}')
            has_changes = True
        elif record_tag.data.to_str() == 'EXEMPLAR:just-ai-megafon-411-test-2020-05-08':
            fixed_tag = generate_fixed_tag(
                record_tag, RecordTagData(type=RecordTagType.OTHER, value='just-ai_megafon_411_test_2020-05-08')
            )
            fixed_records_tags.append(fixed_tag)
            print(f'[fix] tag {record_tag.data.to_str()} -> {fixed_tag.data.to_str()}')
            has_changes = True
        elif record_tag.data.to_str() == 'OTHER:call-center-288-test-2020-01':
            fixed_tag = generate_fixed_tag(
                record_tag, RecordTagData(type=RecordTagType.OTHER, value='yandex-call-center_288_test_2020-01')
            )
            fixed_records_tags.append(fixed_tag)
            print(f'[fix] tag {record_tag.data.to_str()} -> {fixed_tag.data.to_str()}')
            has_changes = True
        else:
            fixed_records_tags.append(record_tag)

    if not has_changes:
        print(f'[skp] table {table_name} has no changes')
        return

    if dry_run:
        return

    records_tags_table_path = f'{table_records_tags_meta.dir_path}/{table_name}'

    yt.remove(records_tags_table_path)
    yt.create('table', records_tags_table_path, attributes=yson.json_to_yson(table_records_tags_meta.attrs))
    yt.write_table(records_tags_table_path, (t.to_yson() for t in sorted(fixed_records_tags)))
