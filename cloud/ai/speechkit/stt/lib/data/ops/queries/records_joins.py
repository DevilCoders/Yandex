import os
import typing
from dataclasses import dataclass
from datetime import datetime

from cloud.ai.lib.python.datetime import parse_datetime, format_datetime
from cloud.ai.speechkit.stt.lib.data.model.dao import (
    RecordRequestParams,
    S3Object,
    Mark,
    Recognition,
    JoinData,
    RecordBitAudioParams,
    get_recognition,
    get_join_data,
    get_split_and_bit_data,
)
from cloud.ai.speechkit.stt.lib.data.model.tags import RecordTagDataRequest
from .records_tags import tag_conjunctions_to_yql_query
from .select import run_yql_select_query
from ..yt import table_records_meta, table_records_joins_meta, table_records_bits_meta


@dataclass
class SelectRecordsJoinsByTagsRow:
    record_id: str
    s3_obj: S3Object
    req_params: RecordRequestParams
    tags: typing.List[str]
    joins: typing.List[typing.Tuple[Recognition, JoinData, datetime]]
    duration: float


def get_records_with_joins_yql(
    join_received_after: typing.Optional[datetime],
    join_received_before: typing.Optional[datetime],
) -> str:
    return f"""
$records_with_joins = (
    SELECT
        records_with_specified_tags.`record_id` as record_id,
        AGGREGATE_LIST(records_joins.`recognition`) AS recognition_list,
        AGGREGATE_LIST(records_joins.`join_data`) AS join_data_list,
        AGGREGATE_LIST(records_joins.`received_at`) AS join_received_at_list
    FROM
        $records_with_specified_tags AS records_with_specified_tags
    INNER JOIN
        LIKE(`{table_records_joins_meta.dir_path}`, "%") AS records_joins
    ON
        records_with_specified_tags.`record_id` = records_joins.`record_id`
{get_join_received_filter(join_received_after, join_received_before)}
    GROUP BY
        records_with_specified_tags.`record_id`
);
"""


def select_records_joins_by_tags(
    tag_conjunctions: typing.List[typing.Set[RecordTagDataRequest]],
    mark: typing.Optional[Mark],
    join_received_after: typing.Optional[datetime],
    join_received_before: typing.Optional[datetime],
) -> typing.List[SelectRecordsJoinsByTagsRow]:
    yql_query = f"""
{tag_conjunctions_to_yql_query(tag_conjunctions, os.environ['YT_PROXY'])}

{get_records_with_joins_yql(join_received_after, join_received_before)}

SELECT
    records_with_joins.`record_id` AS record_id,
    records_with_joins.`recognition_list` AS recognition_list,
    records_with_joins.`join_data_list` AS join_data_list,
    records_with_joins.`join_received_at_list` AS join_received_at_list,
    records_with_specified_tags.`tags` AS tags,
    records.`s3_obj` AS s3_obj,
    records.`req_params` AS req_params,
    records.`audio_params` AS audio_params
FROM
    $records_with_joins AS records_with_joins
INNER JOIN
    LIKE(`{table_records_meta.dir_path}`, "%") AS records
ON
    records_with_joins.`record_id` = records.`id`
INNER JOIN
    $records_with_specified_tags AS records_with_specified_tags
ON
    records_with_joins.`record_id` = records_with_specified_tags.`record_id`
{get_mark_filter(mark, 'records')};
"""

    records_joins_data = []
    tables, _ = run_yql_select_query(yql_query)
    rows = tables[0]
    for row in rows:
        records_joins_data.append(
            SelectRecordsJoinsByTagsRow(
                record_id=row['record_id'],
                tags=row['tags'],
                joins=get_joins_from_row(row),
                s3_obj=S3Object.from_yson(row['s3_obj']),
                req_params=RecordRequestParams.from_yson(row['req_params']),
                duration=row['audio_params']['duration_seconds'],
            )
        )

    return records_joins_data


def select_records_bits_joins_by_tags(
    tag_conjunctions: typing.List[typing.Set[RecordTagDataRequest]],
    mark: typing.Optional[Mark],
    join_received_after: typing.Optional[datetime],
    join_received_before: typing.Optional[datetime],
) -> typing.List[SelectRecordsJoinsByTagsRow]:
    yql_query = f"""
{tag_conjunctions_to_yql_query(tag_conjunctions, os.environ['YT_PROXY'], bits=True)}

{get_records_with_joins_yql(join_received_after, join_received_before)}

SELECT
    records_with_joins.`record_id` AS record_id,
    records_with_joins.`recognition_list` AS recognition_list,
    records_with_joins.`join_data_list` AS join_data_list,
    records_with_joins.`join_received_at_list` AS join_received_at_list,
    records_with_specified_tags.`tags` AS tags,
    records_bits.`s3_obj` AS s3_obj,
    records_bits.`audio_params` AS audio_params,
    records_bits.`split_data` AS split_data,
    records_bits.`bit_data` AS bit_data
FROM
    $records_with_joins AS records_with_joins
INNER JOIN
    LIKE(`{table_records_bits_meta.dir_path}`, "%") AS records_bits
ON
    records_with_joins.`record_id` = records_bits.`id`
INNER JOIN
    $records_with_specified_tags AS records_with_specified_tags
ON
    records_with_joins.`record_id` = records_with_specified_tags.`record_id`
{get_mark_filter(mark, 'records_bits')};
"""

    records_joins_data = []
    tables, _ = run_yql_select_query(yql_query)
    rows = tables[0]
    for row in rows:
        _, bit_data = get_split_and_bit_data(row)
        records_joins_data.append(
            SelectRecordsJoinsByTagsRow(
                record_id=row['record_id'],
                tags=row['tags'],
                joins=get_joins_from_row(row),
                s3_obj=S3Object.from_yson(row['s3_obj']),
                req_params=RecordBitAudioParams.from_yson(row['audio_params']).to_record_request_params(),
                duration=bit_data.get_duration_seconds(),
            )
        )

    return records_joins_data


def get_joins_from_row(row: dict) -> typing.List[typing.Tuple[Recognition, JoinData, datetime]]:
    recognition_list = row['recognition_list']
    join_data_list = row['join_data_list']
    join_received_at_list = row['join_received_at_list']
    assert len(recognition_list) == len(join_data_list) == len(join_received_at_list)
    joins = []
    for i in range(len(recognition_list)):
        joins.append(
            (
                get_recognition(recognition_list[i]),
                get_join_data(join_data_list[i]),
                parse_datetime(join_received_at_list[i]),
            )
        )
    return joins


def get_mark_filter(mark: typing.Optional[Mark], table_name: str):
    if mark is None:
        return ''

    return f"""
    WHERE
        {table_name}.`mark` = '{mark.value}'
    """


def get_join_received_filter(before: typing.Optional[datetime], after: typing.Optional[datetime]):
    def get_single_filter(dt: datetime, left_border: bool):
        return f'records_joins.`received_at` {">=" if left_border else "<="} "{format_datetime(dt)}"'

    filters = []
    if before is not None:
        filters.append(get_single_filter(before, left_border=True))
    if after is not None:
        filters.append(get_single_filter(after, left_border=False))

    if len(filters) == 0:
        return ''

    return f"""
    WHERE
        {' AND '.join(filters)}
"""
