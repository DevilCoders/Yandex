$log = "logs/antiadb-nginx-log/stream/5min/";
$list_of_names = AsList("{{ '", "'.join(list_of_tables) }}");
$list_of_tables = ListMap($list_of_names, ($x) -> { RETURN $log || $x;});

SELECT
    `http_host`,
    `iso_eventtime`,
    `method`,
    `http_referer`,
    `clientip`,
    `uri_path`,
    `source_uri`,
    `service_id`,
    `user_browser_name`,
    `request_time` as duration,
    `response_code`,
    `timestamp`,
    `bamboozled`,
    `host`,
    `request_id` as request_id
FROM
    EACH($list_of_tables)
WHERE
    {{ query_predicate }};
