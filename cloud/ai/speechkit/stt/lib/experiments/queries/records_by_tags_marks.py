import os
from typing import Iterable, List, Dict
from dataclasses import dataclass

from cloud.ai.speechkit.stt.lib.data.ops.yt import table_records_meta, table_records_tags_meta
from cloud.ai.speechkit.stt.lib.data.model.dao import RecordRequestParams, S3Object, RecordTagData, Mark
from cloud.ai.speechkit.stt.lib.data.ops.queries import get_tags_filter, run_yql_select_query

from .records_filters import get_marks_filter


@dataclass
class SelectRecordsByTagsMarksRow:
    record_id: str
    s3_obj: S3Object
    req_params: RecordRequestParams
    tag_data: RecordTagData

    @staticmethod
    def from_yson(fields: Dict) -> 'SelectRecordsByTagsMarksRow':
        return SelectRecordsByTagsMarksRow(
            record_id=fields['record_id'],
            s3_obj=S3Object.from_yson(fields['s3_obj']),
            req_params=RecordRequestParams.from_yson(fields['req_params']),
            tag_data=RecordTagData.from_yson(fields['tag_data']),
        )


def select_records_by_tags_marks(
    tags: Iterable[RecordTagData], marks: Iterable[Mark]
) -> List[SelectRecordsByTagsMarksRow]:

    yql_query = f"""
    USE {os.environ['YT_PROXY']};

    SELECT
        records.`id` as record_id,
        records.`s3_obj` as s3_obj,
        records.`req_params` AS req_params,
        records_tags.`data` AS tag_data,
        records.`mark` as mark
    FROM
        RANGE(`{table_records_tags_meta.dir_path}`, "%") AS records_tags
    INNER JOIN
        RANGE(`{table_records_meta.dir_path}`, "%") AS records
    ON
        records_tags.`record_id` = records.`id`
    WHERE
    (
    {get_marks_filter(marks)}
    )
    AND
    (
    {get_tags_filter(tags)}
    );
    """

    records_data = []
    tables, _ = run_yql_select_query(yql_query)
    rows = tables[0]
    for row in rows:
        records_data.append(SelectRecordsByTagsMarksRow.from_yson(row))

    return records_data
