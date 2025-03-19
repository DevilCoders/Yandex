import argparse

import yt.wrapper as yt

from cloud.ai.lib.python.datasource.yt.ops import configure_yt_wrapper, Table
from cloud.ai.speechkit.stt.lib.data.ops.yt import table_records_meta
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client

s3 = create_client()


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


def run(from_table, to_table, dry_run):
    configure_yt_wrapper()
    for table_name in yt.list(table_records_meta.dir_path):
        if from_table <= table_name <= to_table:
            process_table(table_name, dry_run)


def process_table(table_name, dry_run):
    print(f'Processing table {table_name}', flush=True)
    table = Table(meta=table_records_meta, name=table_name)

    rows = []
    for row in yt.read_table(table.path):
        rows.append(row)

    has_changes = False
    for row in rows:
        rid = row['id']

        if 'acoustic' not in row['audio_params'] or row['audio_params']['acoustic'] is None:
            row['audio_params']['acoustic'] = 'unknown'
            print(f'fix acoustic {rid}')
            has_changes = True
        if 'channel_count' not in row['audio_params']:
            row['audio_params']['channel_count'] = 1
            print(f'fix channel count {rid}')
            has_changes = True

        if row['source']['version'] == 'import':
            if row['source']['source'] == 'yt-table':
                if 'folder_id' not in row['source']['data']:
                    row['source']['data']['folder_id'] = None
                    print(f'fix source yt-table {rid}')
                    has_changes = True
                if 'original_uuid' not in row['source']['data']:
                    row['source']['data']['original_uuid'] = None
                    print(f'fix source yt-table {rid}')
                    has_changes = True
            elif row['source']['source'] == 'files':
                if 'folder_id' not in row['source']['data']:
                    row['source']['data']['folder_id'] = None
                    print(f'fix source files {rid}')
                    has_changes = True

        recognition_spec = row['req_params']['recognition_spec']
        s3_obj = row['s3_obj']

        is_ogg = 'audio_encoding' not in recognition_spec or recognition_spec['audio_encoding'] == 2
        s3_raw = s3_obj['key'].endswith('.raw')

        if is_ogg and s3_raw:
            old_s3_key = s3_obj['key']
            s3_obj['key'] = s3_obj['key'][:-4] + '.ogg'
            if not dry_run:
                audio = s3.get_object(Bucket=s3_obj['bucket'], Key=old_s3_key)['Body'].read()
                s3.put_object(Bucket=s3_obj['bucket'], Key=s3_obj['key'], Body=audio)
            print(f'fix ogg {rid}')
            has_changes = True

    if not has_changes:
        return

    if dry_run:
        return

    yt.remove(table.path)
    table.append_rows(rows)
