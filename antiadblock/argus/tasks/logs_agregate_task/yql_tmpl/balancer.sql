$log = "logs/antiadb-balancer-log/stream/5min/";
$list_of_names = AsList("{{ '", "'.join(list_of_tables) }}");
$list_of_tables = ListMap($list_of_names, ($x) -> { RETURN $log || $x;});

SELECT
    `iso_eventtime`,
    `source`,
    `url`,
    `duration`,
    `referrer`,
    `host_header`,
    `processing_tree`,
    `source_uri`,
    `x_aab_requestid` as request_id,
    DateTime::ToSeconds(DateTime::MakeTimestamp(DateTime::ParseIso8601(`start_time`))) as `timestamp`
FROM
    EACH($list_of_tables)
WHERE
    {{ query_predicate }};
