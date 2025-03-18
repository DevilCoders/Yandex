$log = "logs/antiadb-cryprox-log/stream/5min/";
$list_of_names = AsList("{{ '", "'.join(list_of_tables) }}");
$list_of_tables = ListMap($list_of_names, ($x) -> { RETURN $log || $x;});

SELECT
    `cfg_version`,
    `service_id`,
    `action`,
    `request_tags`,
    `http_host`,
    `host`,
    `source_uri`,
    `timestamp`,
    `iso_eventtime`,
    `duration`,
    `url`,
    `http_code`,
    `request_id` as request_id
FROM
    EACH($list_of_tables)
WHERE
    {{ query_predicate }};
