import yt.wrapper as yt

from cloud.ai.lib.python.datasource.yt.ops import configure_yt_wrapper, TableMeta, Table
from cloud.ai.speechkit.stt.lib.data.ops.yt import (
    table_metrics_eval_records_meta,
    table_metrics_eval_tags_meta,
    table_markups_assignments_meta,
)

fix_cr = {
    "descriptors": [
        {
            "lang": "ru-RU",
            "revision": 7704884,
            "source": "arcadia",
            "topic": "common"
        },
        {
            "lang": "ru-RU",
            "revision": 7704884,
            "source": "arcadia",
            "topic": "cars"
        },
        {
            "lang": "ru-RU",
            "revision": 7704884,
            "source": "arcadia",
            "topic": "legacy"
        }
    ],
    "merge_strategy": "skip"
}


def main():
    configure_yt_wrapper()
    for table_name in yt.list(table_metrics_eval_tags_meta.dir_path):
        print(f'{table_name} eval tags')
        process_metrics_eval_table(table_name, table_metrics_eval_tags_meta)
    for table_name in yt.list(table_metrics_eval_records_meta.dir_path):
        print(f'{table_name} eval metrics')
        process_metrics_eval_table(table_name, table_metrics_eval_records_meta)
    for table_name in yt.list(table_markups_assignments_meta.dir_path):
        print(f'{table_name} assignments')
        process_markups_assignments_table(table_name)


def process_metrics_eval_table(table_name: str, table_meta: TableMeta):
    table = Table(meta=table_meta, name=table_name)
    has_changes = False
    rows = []
    for row in table.read_rows():
        if row['calc_data'] is not None:
            if 'cr' in row['calc_data']:
                has_changes = True
                if 'descriptors' not in row['calc_data']['cr']:
                    row['calc_data']['cr'] = fix_cr
                else:
                    for d in row['calc_data']['cr']['descriptors']:
                        d['lang'] = 'ru-RU'
            if 'stop_words' in row['calc_data']:
                has_changes = True
                row['calc_data']['stop_words']['lang'] = 'ru-RU'
        if 'tag' in row:
            # CLIENT:fromtech-qnd MODE:stream PERIOD:2020-10 ->
            # CLIENT:fromtech-qnd MODE:stream LANG:ru-RU PERIOD:2020-10
            tag = row['tag']
            if tag.startswith('CLIENT') and 'LANG' not in tag:
                has_changes = True
                period_pos = tag.index('PERIOD')
                row['tag'] = f'{tag[:period_pos]}LANG:ru-RU {tag[period_pos:]}'
        rows.append(row)
    if has_changes:
        table.remove()
        table.append_rows(rows)


def process_markups_assignments_table(table_name: str):
    table = Table(meta=table_markups_assignments_meta, name=table_name)
    has_changes = False
    rows = []
    for row in table.read_rows():
        for validation_entry in row['validation_data']:
            for solution in validation_entry['correct_solutions'] + validation_entry['incorrect_solutions']:
                for known_solution in solution['known_solutions']:
                    for diff in known_solution['diff_list']:
                        if 'extra' in diff and diff['extra'] is not None:
                            if 'cr' in diff['extra']:
                                has_changes = True
                                if 'descriptors' not in diff['extra']['cr']:
                                    diff['extra']['cr'] = fix_cr
                                else:
                                    for d in diff['extra']['cr']['descriptors']:
                                        d['lang'] = 'ru-RU'
                            if 'stop_words' in diff['extra']:
                                has_changes = True
                                diff['extra']['stop_words']['lang'] = 'ru-RU'
        rows.append(row)
    if has_changes:
        table.remove()
        table.append_rows(rows)
