--!syntax_v1

$endTimestamp = 1611878400; -- 2021-01-29

select configPath, commitId from `main/ConfigHistory` where CAST(JSON_VALUE(creationInfo, '$.created.seconds') as Int64) < $endTimestamp and status != 'NOT_CI' limit 1000;
select configPath, commitId from `main/ConfigHistory` where CAST(JSON_VALUE(creationInfo, '$.created.seconds') as Int64) < $endTimestamp and status != 'NOT_CI' limit 1000 offset 1000;
select configPath, commitId from `main/ConfigHistory` where CAST(JSON_VALUE(creationInfo, '$.created.seconds') as Int64) < $endTimestamp and status != 'NOT_CI' limit 1000 offset 2000;
select configPath, commitId from `main/ConfigHistory` where CAST(JSON_VALUE(creationInfo, '$.created.seconds') as Int64) < $endTimestamp and status != 'NOT_CI' limit 1000 offset 3000;
select configPath, commitId from `main/ConfigHistory` where CAST(JSON_VALUE(creationInfo, '$.created.seconds') as Int64) < $endTimestamp and status != 'NOT_CI' limit 1000 offset 4000;
-- select configPath, commitId from `main/ConfigHistory` where CAST(JSON_VALUE(creationInfo, '$.created.seconds') as Int64) < $endTimestamp and status != 'NOT_CI' limit 1000 offset 5000;
-- select configPath, commitId from `main/ConfigHistory` where CAST(JSON_VALUE(creationInfo, '$.created.seconds') as Int64) < $endTimestamp and status != 'NOT_CI' limit 1000 offset 6000;
-- select configPath, commitId from `main/ConfigHistory` where CAST(JSON_VALUE(creationInfo, '$.created.seconds') as Int64) < $endTimestamp and status != 'NOT_CI' limit 1000 offset 7000;
-- select configPath, commitId from `main/ConfigHistory` where CAST(JSON_VALUE(creationInfo, '$.created.seconds') as Int64) < $endTimestamp and status != 'NOT_CI' limit 1000 offset 8000;
-- select configPath, commitId from `main/ConfigHistory` where CAST(JSON_VALUE(creationInfo, '$.created.seconds') as Int64) < $endTimestamp and status != 'NOT_CI' limit 1000 offset 9000;
-- select configPath, commitId from `main/ConfigHistory` where CAST(JSON_VALUE(creationInfo, '$.created.seconds') as Int64) < $endTimestamp and status != 'NOT_CI' limit 1000 offset 10000;