--!syntax_v1

$endTimestamp = 1611878400; -- 2021-01-29
select count(DISTINCT commitId) from `main/ConfigHistory` where CAST(JSON_VALUE(creationInfo, '$.created.seconds') as Int64) < $endTimestamp and status != 'NOT_CI';