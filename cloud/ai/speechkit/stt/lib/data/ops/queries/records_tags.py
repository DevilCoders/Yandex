from dataclasses import dataclass
import os
import typing

from ..yt import table_records_tags_meta, table_records_meta, table_records_joins_meta, table_records_bits_meta
from cloud.ai.speechkit.stt.lib.data.model.dao import RecordTagData, Mark
from cloud.ai.speechkit.stt.lib.data.model.tags import (
    prepare_tags_for_view,
    parse_tag_conjunctions,
    validate_tag_conjunctions,
    RecordTagDataRequest,
)
from .select import run_yql_select_query


def select_records_ids_by_tag(tag_data: RecordTagData):
    yql_query = f"""
USE {os.environ['YT_PROXY']};

SELECT records_tags.`record_id` as record_id
FROM
RANGE(`{table_records_tags_meta.dir_path}`, "%") AS records_tags
WHERE
Yson::LookupString(records_tags.`data`, "type") = '{tag_data.type.value}' AND
Yson::LookupString(records_tags.`data`, "value") = '{tag_data.value}'
"""
    records_ids = []
    tables, _ = run_yql_select_query(yql_query)
    rows = tables[0]
    for row in rows:
        records_ids.append(row['record_id'])
    return records_ids


@dataclass
class CalculateTagsStatisticsResult:
    tags: typing.List[str]
    records_count: int
    duration_hours: float
    duration_with_joins_hours: float
    duration_hist: str
    duration_percentiles: typing.Dict[int, float]

    @staticmethod
    def from_yson(fields: dict) -> 'CalculateTagsStatisticsResult':
        duration_percentiles = {}
        for key, value in fields.items():
            if key.startswith('duration_p'):
                percentile = int(key[len('duration_p'):])
                duration_percentiles[percentile] = value
        return CalculateTagsStatisticsResult(
            # TODO (ASREXP-468)
            # whitespace was used as tags delimiter before, switch back to it here and in YQL after tags repair
            tags=prepare_tags_for_view(fields['tags'].split('##'), compose=False),
            records_count=fields['records_count'],
            duration_hours=fields['duration_hours'],
            duration_with_joins_hours=fields['duration_with_joins_hours'],
            duration_hist=fields['duration_hist'],
            duration_percentiles=duration_percentiles,
        )


def calculate_tags_statistics() -> typing.List[CalculateTagsStatisticsResult]:
    yql_query = f"""
{tag_conjunctions_to_yql_query([], os.environ['YT_PROXY'])}

$records_with_combined_tags = (
    SELECT
        records_with_specified_tags.`record_id` AS record_id,
        String::JoinFromList(ListSort(records_with_specified_tags.`tags`), "##") AS tags
    FROM
        $records_with_specified_tags AS records_with_specified_tags
);

$records_with_has_joins = (
    SELECT
        records_with_combined_tags.`record_id` AS record_id,
        IF(ListLength(AGGREGATE_LIST(records_joins.`recognition`)) > 0, True, False) AS has_join
    FROM
        $records_with_combined_tags AS records_with_combined_tags
    LEFT JOIN
        RANGE(`{table_records_joins_meta.dir_path}`, "%") AS records_joins
    ON
        records_with_combined_tags.`record_id` = records_joins.`record_id`
    GROUP BY
        records_with_combined_tags.`record_id`
);

$records_with_durations = (
    SELECT
        records_with_combined_tags.`record_id` AS record_id,
        Yson::LookupDouble(records.`audio_params`, "duration_seconds") AS duration_seconds,
    FROM
        $records_with_combined_tags AS records_with_combined_tags
    INNER JOIN
        RANGE(`{table_records_meta.dir_path}`, "%") AS records
    ON
        records_with_combined_tags.`record_id` = records.`id`
);

SELECT
    records_with_combined_tags.`tags` AS tags,
    COUNT(*) AS records_count,
    SUM(
        records_with_durations.`duration_seconds`
    ) / 60.0 / 60.0 AS duration_hours,
    SUM(
        IF(records_with_has_joins.`has_join`, records_with_durations.`duration_seconds`, 0.0)
    ) / 60.0 / 60.0 AS duration_with_joins_hours,
    Histogram::Print(HISTOGRAM(records_with_durations.`duration_seconds`, 20)) AS duration_hist,
    PERCENTILE(records_with_durations.`duration_seconds`, 0.10) AS duration_p10,
    PERCENTILE(records_with_durations.`duration_seconds`, 0.25) AS duration_p25,
    PERCENTILE(records_with_durations.`duration_seconds`, 0.50) AS duration_p50,
    PERCENTILE(records_with_durations.`duration_seconds`, 0.75) AS duration_p75,
    PERCENTILE(records_with_durations.`duration_seconds`, 0.90) AS duration_p90,
    PERCENTILE(records_with_durations.`duration_seconds`, 0.95) AS duration_p95,
    PERCENTILE(records_with_durations.`duration_seconds`, 0.99) AS duration_p99
FROM
    $records_with_combined_tags AS records_with_combined_tags
INNER JOIN
    $records_with_has_joins AS records_with_has_joins
ON
    records_with_combined_tags.`record_id` = records_with_has_joins.`record_id`
INNER JOIN
    $records_with_durations AS records_with_durations
ON
    records_with_combined_tags.`record_id` = records_with_durations.`record_id`
GROUP BY
    records_with_combined_tags.`tags`;
"""

    tables, _ = run_yql_select_query(yql_query)
    rows = tables[0]

    results = []
    for row in rows:
        results.append(CalculateTagsStatisticsResult.from_yson(row))

    return results


def select_records_ids_for_evaluation_basket(
    tags: typing.List[str],
    tags_limits: typing.List[int],
) -> typing.Iterable[str]:
    tag_conjunctions = parse_tag_conjunctions(tags)
    validate_tag_conjunctions(tag_conjunctions)

    yql_query = f"""
{tag_conjunctions_to_yql_query(tag_conjunctions, os.environ['YT_PROXY'])}

$records_with_joins = (
    SELECT
        records_with_specified_tags.`record_id` as record_id,
    FROM
        $records_with_specified_tags AS records_with_specified_tags
    INNER JOIN
        LIKE(`{table_records_meta.dir_path}`, "%") AS records
    ON
        records_with_specified_tags.`record_id` = records.`id`
    INNER JOIN
        LIKE(`{table_records_joins_meta.dir_path}`, "%") AS records_joins
    ON
        records_with_specified_tags.`record_id` = records_joins.`record_id`
    WHERE
        records.`mark` = '{Mark.TEST.value}'
    GROUP BY
        records_with_specified_tags.`record_id`
);

$shuffled_records_with_joins = (
    SELECT
        *
    FROM
        $records_with_joins
    ORDER BY
        Digest::Crc64(`record_id` || CAST(RandomUuid(`record_id`) AS string))
);

$all_records = (
    SELECT
        shuffled_records_with_joins.`record_id` as record_id,
        records_with_specified_tags.`tags` as tags,
    FROM
        $shuffled_records_with_joins AS shuffled_records_with_joins
    INNER JOIN
        $records_with_specified_tags AS records_with_specified_tags
    ON
        shuffled_records_with_joins.`record_id` = records_with_specified_tags.`record_id`
);

{generate_subqueries_for_tags_conjunctions_with_limits_from_table(
        tag_conjunctions,
        tags_limits, '$all_records'
    )}
"""

    records_ids = set([])
    tables, _ = run_yql_select_query(yql_query)
    for table in tables:
        for row in table:
            record_id = row['record_id']
            if record_id in records_ids:
                print(f'WARN: intersecting tag conjunctions for record {record_id}')
                continue
            records_ids.add(record_id)
    return records_ids


def generate_subqueries_for_tags_conjunctions_with_limits_from_table(
    tag_conjunctions: typing.List[typing.Set[RecordTagDataRequest]],
    tags_limits: typing.List[int],
    table: str = '$records_with_specified_tags',
) -> str:
    def get_limit_yql(limit: int):
        if limit < 1:
            return ''
        else:
            return f"""
    LIMIT {limit}"""

    assert len(tag_conjunctions) == len(tags_limits)
    return '\n\n'.join(
        f"""
    SELECT *
    FROM {table}
    WHERE
    {get_tag_conjunction_yql_filter(tag_conjunction, tags_column='tags')}
    {get_limit_yql(tags_limits[i])};
"""
        for i, tag_conjunction in enumerate(tag_conjunctions)
    )


def tag_conjunctions_to_yql_query(
    tag_conjunctions: typing.List[typing.Set[RecordTagDataRequest]],
    yt_proxy: str,
    bits: bool = False,
) -> str:
    """
    Makes YQL query to filter records (or records bits) meet tag expression using tag conjunctions.
    """
    if bits:
        tags_column = 'records_bits_with_tags.`tags`'
    else:
        tags_column = 'records_with_tags.`tags`'

    if len(tag_conjunctions) > 0:
        tag_conjunction_filters = []
        for tag_conjunction in tag_conjunctions:
            tag_conjunction_filters.append(get_tag_conjunction_yql_filter(tag_conjunction, tags_column))
        tags_filter = '\nOR\n'.join(tag_conjunction_filters)
        where_clause = '\n'.join(('WHERE', tags_filter))
    else:
        where_clause = ''

    records_with_tags_yql = f"""
USE {yt_proxy};

PRAGMA yt.LookupJoinMaxRows = "500";  -- will slow down query, see YQL-12923

$tags_count_with_type = ($tags, $type) -> {{
    RETURN ListLength(
        ListFilter(
            $tags,
            ($tag) -> {{
                RETURN String::StartsWith($tag, $type)
            }}
        )
    );
}};

$records_with_tags = (
    SELECT
        records.`id` AS record_id,
        AGGREGATE_LIST(
            Yson::LookupString(records_tags.`data`, "type") || ":" || Yson::LookupString(records_tags.`data`, "value")
        ) AS tags
    FROM
        LIKE(`{table_records_meta.dir_path}`, "%") AS records
    INNER JOIN
        LIKE(`{table_records_tags_meta.dir_path}`, "%") AS records_tags
    ON
        records.`id` = records_tags.`record_id`
    GROUP BY
        records.`id`
);"""

    if bits:
        # Records bits tags are union of its records tags and its own tags.
        return f"""
{records_with_tags_yql}

$records_bits_with_records_tags = (
    SELECT
        records_bits.`id` AS record_bit_id,
        records_with_tags.`tags` AS tags
    FROM
        $records_with_tags AS records_with_tags
    INNER JOIN
        LIKE(`{table_records_bits_meta.dir_path}`, "%") AS records_bits
    ON
        records_with_tags.`record_id` = records_bits.`record_id`
);

$records_bits_with_records_bits_tags = (
    SELECT
        records_bits.`id` AS record_bit_id,
        AGGREGATE_LIST(
            Yson::LookupString(records_tags.`data`, "type") || ":" || Yson::LookupString(records_tags.`data`, "value")
        ) AS tags
    FROM
        LIKE(`{table_records_bits_meta.dir_path}`, "%") AS records_bits
    INNER JOIN
        LIKE(`{table_records_tags_meta.dir_path}`, "%") AS records_tags
    ON
        records_bits.`id` = records_tags.`record_id`
    GROUP BY
        records_bits.`id`
);

$records_bits_with_tags = (
    SELECT
        IF(
            records_bits_with_records_tags.`record_bit_id` IS NULL,
            records_bits_with_records_bits_tags.`record_bit_id`,
            records_bits_with_records_tags.`record_bit_id`
        ) AS record_id,
        ListUniq(ListExtend(
            IF(
                records_bits_with_records_tags.`tags` IS NULL,
                [],
                records_bits_with_records_tags.`tags`
            ),
            IF(
                records_bits_with_records_bits_tags.`tags` IS NULL,
                [],
                records_bits_with_records_bits_tags.`tags`
            )
        )) AS tags
    FROM
        $records_bits_with_records_tags AS records_bits_with_records_tags
    FULL JOIN
        $records_bits_with_records_bits_tags AS records_bits_with_records_bits_tags
    ON
        records_bits_with_records_tags.`record_bit_id` = records_bits_with_records_bits_tags.`record_bit_id`
);

$records_with_specified_tags = (
    SELECT
        *
    FROM
        $records_bits_with_tags AS records_bits_with_tags
{where_clause}
);
"""
    else:
        # If records_tags/ table contains only records tags, we could make just
        # select from it without join with records/ to construct $records_with_tags.
        # But records_tags/ can contain records bits tags, so join is required.
        return f"""
{records_with_tags_yql}

$records_with_specified_tags = (
    SELECT
        *
    FROM
        $records_with_tags AS records_with_tags
{where_clause}
);
"""


def get_tag_conjunction_yql_filter(
    tag_conjunction: typing.Set[RecordTagDataRequest],
    tags_column='records_with_tags.`tags`',
) -> str:
    tag_conjunction_filter = []
    for tag_query in sorted(tag_conjunction):
        tag_conjunction_filter.append(tag_query.to_yql_filter(tags_column))
    return ' AND '.join(tag_conjunction_filter)
