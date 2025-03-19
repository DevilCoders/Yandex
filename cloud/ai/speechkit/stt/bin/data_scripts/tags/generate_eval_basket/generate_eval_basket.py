import typing

import nirvana.job_context as nv
import yt.wrapper as yt

from cloud.ai.lib.python.datasource.yt.ops import Table
from cloud.ai.lib.python.datetime import now
from cloud.ai.speechkit.stt.lib.data.model.dao import RecordTag, RecordTagType, RecordTagData
from cloud.ai.speechkit.stt.lib.data.ops.queries import select_records_ids_for_evaluation_basket
from cloud.ai.speechkit.stt.lib.data.ops.yt import table_records_tags_meta


def main():
    op_ctx = nv.context()

    params = op_ctx.parameters

    tags: typing.List[str] = params.get('tags')
    tags_limits: typing.List[int] = params.get('tags-limits')
    tag_suffix: str = params.get('basket-suffix')
    dry_run: bool = params.get('dry-run')

    received_at = now()

    records_ids = list(select_records_ids_for_evaluation_basket(tags, tags_limits))

    tag_value = f'{received_at.year}-{received_at.month:02d}_{len(records_ids)}_{tag_suffix}'
    tag_data = RecordTagData(type=RecordTagType.KPI, value=tag_value)

    records_tags = []
    for record_id in records_ids:
        records_tags.append(
            RecordTag.add_by_record_id(
                record_id=record_id,
                data=tag_data,
                received_at=received_at,
            )
        )

    if dry_run:
        print('Dry run: append to table in Hume')
        yt.config['proxy']['url'] = 'hume'

    date = Table.get_name(received_at)
    table_records_tags = Table(meta=table_records_tags_meta, name=date)
    table_records_tags.append_objects(records_tags)

    print(f'{len(records_tags)} records tagged as {tag_data.to_str()}')
