$log = "logs/bs-hit-log/stream/5min/";
$list_of_names = AsList("{{ '", "'.join(list_of_tables) }}");
$list_of_tables = ListMap($list_of_names, ($x) -> { RETURN $log || $x;});

SELECT
    `adbbits` as log_id,
    `pageid`,
    `hitlogid`,
    `eventid`,
    `placeid`,
    `bannercount`,
    `iso_eventtime`,
    `eventtime` as `timestamp`
FROM
    EACH($list_of_tables)
WHERE
    CAST(CAST(`adbbits` as UINT64) >> 1 AS STRING) in ({{ query_predicate }});
