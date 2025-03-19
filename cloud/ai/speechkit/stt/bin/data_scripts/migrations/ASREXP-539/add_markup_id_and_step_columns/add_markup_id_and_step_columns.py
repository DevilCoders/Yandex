import typing
import yt.wrapper as yt
import yt.yson as yson

from cloud.ai.lib.python.datasource.yt.ops import TableMeta
from cloud.ai.speechkit.stt.lib.data.ops.yt import (
    table_records_bits_markups_meta,
    table_markups_assignments_meta,
    table_metrics_markup_meta,
)
from cloud.ai.speechkit.stt.lib.data.model.dao import MarkupDataVersions, MarkupStep

markup_data_to_step = {
    MarkupDataVersions.PLAIN_TRANSCRIPT.value: MarkupStep.TRANSCRIPT.value,
    MarkupDataVersions.TRANSCRIPT_AND_TYPE.value: MarkupStep.TRANSCRIPT.value,
    MarkupDataVersions.CHECK_TRANSCRIPT.value: MarkupStep.CHECK_TRANSCRIPT.value,
}


def main():
    table_name_to_records_bits_markups = read_tables(table_records_bits_markups_meta)
    table_name_to_markups_assignments = read_tables(table_markups_assignments_meta)
    table_name_to_metrics_markups = read_tables(table_metrics_markup_meta)

    pool_id_to_markup_id = {}
    for metrics_markups in table_name_to_metrics_markups.values():
        for metric in metrics_markups:
            markup_id = metric['markup_id']
            for step_metrics in metric['step_to_metrics'].values():
                pool_id = step_metrics['pool_id']
                pool_id_to_markup_id[pool_id] = markup_id
            metric['accuracy'] = None
            metric['not_evaluated_records_ratio'] = None
            metric['quality_evaluation_duration_minutes'] = None
            metric['quality_evaluation_cost'] = None
            metric['quality_evaluation_toloker_speed_mean'] = None
            metric['quality_evaluation_all_assignments_honeypots_quality_mean'] = None
            metric['quality_evaluation_accepted_assignments_honeypots_quality_mean'] = None

    for records_bits_markups in table_name_to_records_bits_markups.values():
        for markup in records_bits_markups:
            pool_id = markup['pool_id']
            markup_id = pool_id_to_markup_id[pool_id]
            markup['markup_id'] = markup_id

            markup_data_version = markup['markup_data']['version']
            markup_step = markup_data_to_step[markup_data_version]
            markup['markup_step'] = markup_step

    for markups_assignments in table_name_to_markups_assignments.values():
        for assignment in markups_assignments:
            pool_id = assignment['pool_id']
            markup_id = pool_id_to_markup_id[pool_id]
            assignment['markup_id'] = markup_id

            markup_data_version = assignment['tasks'][0]['version']
            markup_step = markup_data_to_step[markup_data_version]
            assignment['markup_step'] = markup_step

    for table_name, records_bits_markups in table_name_to_records_bits_markups.items():
        rewrite_table(table_records_bits_markups_meta, table_name, records_bits_markups)
    for table_name, markups_assignments in table_name_to_markups_assignments.items():
        rewrite_table(table_markups_assignments_meta, table_name, markups_assignments)
    for table_name, metrics_markups in table_name_to_metrics_markups.items():
        rewrite_table(table_metrics_markup_meta, table_name, metrics_markups)


def read_tables(table_meta: TableMeta) -> typing.Dict[str, typing.List[dict]]:
    table_name_to_rows = {}
    for table_name in yt.list(table_meta.dir_path):
        rows = []
        for row in yt.read_table(f'{table_meta.dir_path}/{table_name}'):
            rows.append(row)
        table_name_to_rows[table_name] = rows
    return table_name_to_rows


def rewrite_table(table_meta: TableMeta, table_name: str, rows: typing.List[dict]):
    table_path = f'{table_meta.dir_path}/{table_name}'
    yt.remove(table_path)
    yt.create('table', table_path, attributes=yson.json_to_yson(table_meta.attrs))
    yt.write_table(table_path, rows)
