import os

import yt.wrapper as yt
import yt.yson as yson

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    SolutionHoneypotAcceptanceData, SolutionAcceptanceResult, SolutionFieldDiff,
)
from cloud.ai.speechkit.stt.lib.data.ops.yt import table_markups_assignments_meta


def main():
    yt.config['proxy']['url'] = os.environ['YT_PROXY']
    for table_name in yt.list(table_markups_assignments_meta.dir_path):
        if table_name >= '2021-02-17':
            continue
        process_table(table_name)


def process_table(table_name):
    print(f'Processing table {table_name}')
    table = Table(meta=table_markups_assignments_meta, name=table_name)

    rows = []
    for row in yt.read_table(table.path):
        rows.append(row)

    has_changes = False
    for row in rows:
        for validation_item in row['validation_data']:
            for solutions_field in ['correct_solutions', 'incorrect_solutions']:
                migrated_solutions = []
                for solution in validation_item[solutions_field]:
                    migrated_solutions.append(SolutionHoneypotAcceptanceData(
                        task_id=solution['task_id'],
                        known_solutions=[
                            SolutionAcceptanceResult(
                                diff_list=[SolutionFieldDiff.from_yson(i) for i in solution['diff_list']],
                                accepted=True if solutions_field == 'correct_solutions' else False,
                            ),
                        ],
                    ).to_yson())
                validation_item[solutions_field] = migrated_solutions
                has_changes = True

    if not has_changes:
        return

    yt.remove(table.path)
    yt.create('table', table.path, attributes=yson.json_to_yson(table.meta.attrs))
    yt.write_table(table.path, rows)
