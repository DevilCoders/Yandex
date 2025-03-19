import os
import argparse
from collections import defaultdict
import typing
import multiprocessing

import yt.wrapper as yt

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data.model.dao import Record, RecordSourceCloud, RecordTag, RecordTagType, RecordTagData
from cloud.ai.speechkit.stt.lib.data.model.registry.tags import folder_id_to_client_slug
from cloud.ai.speechkit.stt.lib.data.ops.yt import table_records_meta, table_records_tags_meta


def _parse_args():
    parser = argparse.ArgumentParser(
        description='Add missing client tags after client tags registry update',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument('-from_table', default='0000-01-01')
    parser.add_argument('-to_table', default='9999-12-31')
    parser.add_argument(
        '-dry_run',
        action='store_true',
        help='Dry run: do not append to YT tables, only log changes.',
    )
    parser.add_argument(
        '-verbose',
        action='store_true',
        help='Verbose output',
    )
    return parser.parse_args()


def main():
    run(**vars(_parse_args()))


def run(from_table: str, to_table: str, dry_run: bool, verbose: bool):
    valid_table = lambda table_name: from_table <= table_name <= to_table
    args = [(table_name, dry_run, verbose) for table_name in yt.list(table_records_meta.dir_path) if valid_table(table_name)]

    print(f'cores={os.cpu_count()}')
    with multiprocessing.Pool(os.cpu_count()) as pool:
        res = pool.starmap(process_tables, args)

    new_client_slug_to_count = defaultdict(int)
    for new_client_slug_to_count_chunk in res:
        for client_slug, count in new_client_slug_to_count_chunk.items():
            new_client_slug_to_count[client_slug] += count

    print('New client tags stats:')
    for client_slug, count in new_client_slug_to_count.items():
        print(f'{client_slug}: {count}')


# Important: script is based on invariant that records tags of type CLIENT for records are always located
# within same table name, because CLIENT tags are always created with records in the same time.
def process_tables(table_name: str, dry_run: bool, verbose: bool) -> typing.Dict[str, int]:
    print(f'Process table {table_name}', flush=True)

    client = yt.YtClient(proxy=yt.config['proxy']['url'], token=yt.config['token'])

    table_records = Table(meta=table_records_meta, name=table_name, client=client)
    table_records_tags = Table(meta=table_records_tags_meta, name=table_name, client=client)

    records_ids_with_client_tags = set([])
    if client.exists(table_records_tags.path):
        for record_tag_row in client.read_table(table_records_tags.path):
            record_tag = RecordTag.from_yson(record_tag_row)
            if record_tag.data.type == RecordTagType.CLIENT:
                records_ids_with_client_tags.add(record_tag.record_id)

    new_record_tags = []
    new_client_slug_to_count = defaultdict(int)

    for record_row in client.read_table(table_records.path):
        record = Record.from_yson(record_row)
        if not isinstance(record.source, RecordSourceCloud):
            if verbose:
                print(f'SKP: record {record.id} is not Cloud source')
            continue
        if record.id in records_ids_with_client_tags:
            if verbose:
                print(f'SKP: record {record.id} is already with client tag')
            continue

        folder_id = record.source.folder_id
        client_slug = folder_id_to_client_slug.get(folder_id)
        if client_slug is None:
            if verbose:
                print(f'SKP: folder ID {folder_id} of record {record.id} without registered client slug')
            continue

        if verbose:
            print(f'ADD: client slug {client_slug} for record {record.id}')
        new_client_slug_to_count[client_slug] += 1
        new_record_tags.append(
            RecordTag.add(
                record=record,
                data=RecordTagData(type=RecordTagType.CLIENT, value=client_slug),
                received_at=record.received_at,
            )
        )

    if not dry_run:
        table_records_tags.append_objects(new_record_tags)

    return new_client_slug_to_count
