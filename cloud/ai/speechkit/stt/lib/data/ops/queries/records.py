import os
import typing

from cloud.ai.speechkit.stt.lib.data.model.dao import Record, Mark
from cloud.ai.speechkit.stt.lib.data.model.tags import (
    parse_tag_conjunctions,
    validate_tag_conjunctions,
    prepare_tags_for_view,
)
from .records_tags import generate_subqueries_for_tags_conjunctions_with_limits_from_table
from .records_tags import tag_conjunctions_to_yql_query
from .select import run_yql_select_query
from ..yt import table_records_meta, table_records_bits_markups_meta


def get_uuid_range_predicate(this_session: int, session_count: int) -> str:
    """Return str predicate for 1st symbol for this_session out of session_count total sessions"""
    assert session_count in [2, 4, 8, 16]
    assert 0 < this_session <= session_count

    full_range = list(map(str, range(10))) + ['a', 'b', 'c', 'd', 'e', 'f']
    start = 16 // session_count * (this_session - 1)
    end = 16 // session_count * this_session - 1
    return f'SUBSTRING(records.`id`, 0, 1) BETWEEN "{full_range[start]}" AND "{full_range[end]}"'


# Returns records with its tags, for records with specified tags and without markups.
def select_records_without_markups_by_tags(
    tags: typing.List[str],
    tags_limits: typing.List[int],
    mark: typing.Optional[Mark],
    table_name_pattern: str = '%',
    min_duration_seconds: float = 0,
    max_duration_seconds: float = 0,
    concurrent_session: typing.Optional[str] = None
) -> typing.List[typing.Tuple[Record, typing.Tuple[str]]]:
    tag_conjunctions = parse_tag_conjunctions(tags)
    validate_tag_conjunctions(tag_conjunctions)

    predicates = [
        'records_bits_markups.`id` IS NULL',
    ]

    if min_duration_seconds > 0:
        predicates.append(f'Yson::LookupDouble(records.`audio_params`, "duration_seconds") >= {min_duration_seconds}')
    if max_duration_seconds > 0:
        predicates.append(f'Yson::LookupDouble(records.`audio_params`, "duration_seconds") <= {max_duration_seconds}')
    if mark is not None:
        predicates.append(f'records.`mark` = "{mark.value}"')
    if concurrent_session:
        this_session_str, session_count_str = concurrent_session.split('/')
        this_session = int(this_session_str)
        session_count = int(session_count_str)
        predicates.append(get_uuid_range_predicate(this_session, session_count))

    where_clause = '\nAND\n'.join(f'({predicate})' for predicate in predicates)

    yql_query = f"""
{tag_conjunctions_to_yql_query(tag_conjunctions, os.environ['YT_PROXY'])}

$unmarked_records = (
    SELECT
        records.`id` AS id,
        records.`s3_obj` AS s3_obj,
        records.`mark` AS mark,
        records.`source` AS source,
        records.`req_params` AS req_params,
        records.`audio_params` AS audio_params,
        records.`received_at` AS received_at,
        records.`other` AS other,
        records_with_specified_tags.`tags` AS tags
    FROM
        $records_with_specified_tags AS records_with_specified_tags
    INNER JOIN
        LIKE(`{table_records_meta.dir_path}`, "{table_name_pattern}") AS records
    ON
        records_with_specified_tags.`record_id` = records.`id`
    LEFT JOIN
        LIKE(`{table_records_bits_markups_meta.dir_path}`, "{table_name_pattern}") AS records_bits_markups
    ON
        records_with_specified_tags.`record_id` = records_bits_markups.`record_id`
    WHERE
        {where_clause}
);

{generate_subqueries_for_tags_conjunctions_with_limits_from_table(
        tag_conjunctions,
        tags_limits, '$unmarked_records'
    )}
"""

    records_with_tags = []
    records_ids = set([])
    tables, _ = run_yql_select_query(yql_query)
    for table in tables:
        for row in table:
            record = Record.from_yson(row)
            if record.id in records_ids:
                print(f'WARN: intersecting tag conjunctions for record {record.id}')
                continue
            records_ids.add(record.id)
            record_tags = prepare_tags_for_view(row['tags'], compose=False)
            records_with_tags.append((record, tuple(record_tags)))
    return records_with_tags
