## Греп логов testenv тасок
Расположены в YT.
Найти нужный task_id можно [тут](https://testenv.yandex-team.ru/?screen=engine_tasks_status) - по названию проекта и типу задачи, также id таски присутствует во контексте всех задач sandox, запущенных тестенвом, в поле testenv_task_id - это таска, непосредственно запустившая задачу sandox

(Пример)
```
SELECT
    `message`,
    `context`,
    `_logfeller_timestamp`
FROM CONCAT(
    arnold.`logs/deploy-logs/1d/2021-06-17`
    arnold.`logs/deploy-logs/1d/2021-06-18`
    
)
WHERE stage = 'testenv-prod' and Yson::ConvertToString(Yson::YPath(Yson::Parse(context), '/task_id')) = '737921767'
ORDER BY `_logfeller_timestamp`

```

### Греп логов testenv тасок на хостах

## Греп логов балансера
Расположены в YT в `logs/balancer-dynbalancer-https-log/`
В логах балансера нет payload-ов запросов - ничего дельного кроме времени, http статуса и отправителя выцепить трудно. При этом фильтрация происходит в большнстве случаев по ручке и времени - они в предыдущем пункте.

(пример)

```
SELECT
    `iso_eventtime`,
    `source`,
    `spuid`,
    `timestamp`,
    `start_time`,
    `spuid_ses_type`,
    `puid_ses_type`,
    `p2uid_ses_type`,
    `duration`,
    `p2uid`,
    `puid`,
    `processing_tree`,
    `subkey`,
    `referrer`,
    `url`,
    `timezone`,
    `host_header`,
    `_logfeller_timestamp`,
    `source_uri`,
    `_logfeller_index_bucket`,
    `_stbx`
FROM hahn.`logs/balancer-dynbalancer-https-log/1d/2021-03-01`
WHERE `host_header` LIKE "%testenv%" AND `url` LIKE "%/handlers/cleanupDb%" AND `iso_eventtime` LIKE "2021-03-05 13:20%"
LIMIT 100;
```

## Почему отфильтровалась/не отфильтровалась ревизия:

```
SELECT
    `message`,
    `context`,
    `_logfeller_timestamp`
FROM arnold.`logs/deploy-logs/1d/2021-06-17`
where stage="testenv-prod" AND message LIKE '%should_check_revision_item 8274467 news-trunk COMPARE_ANNOTATOR_RESPONSES%'
```
Из записи получаем context.task_id и получаем полный лог применяя греп по task_id (указан выше)
