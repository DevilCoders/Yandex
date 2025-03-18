## В CI-backend непрерывно крутится крутилка (js падает с ошибкой)
Скорее всего, запрос не может найти тесты.
Проще всего [запустить CI Web Application](https://a.yandex-team.ru/arc_vcs/testenv/ci/backend#kak-zapustit-ci-backend-v-idea) и посмотреть на конкретные запросы.

Параметры запуска, которые позволят подключиться к проду: **-Dspring.config.additional-location=/Users/miroslav2/arc/arcadia/testenv/ci/backend/deploy/configuration/stable/config/application.properties,/Users/miroslav2/.ci/cib-stable-secrets.properties -Dci.api.base=http://ci-fake-testenv1.n.yandex-team.ru**

Секреты в `.ci/cib-stable-secrets.properties`:
```
app.datasource.clickhouse.all.username=web
app.datasource.clickhouse.all.password=...
ci.redis.password=...
```

В файле https://a.yandex-team.ru/arc_vcs/testenv/ci/backend/src/main/resources/logback-default.xml имеет смысл добавить блок с логирование запросов:
```xml
<logger name="org.springframework.jdbc.core" level="TRACE" additivity="false">
    <appender-ref ref="STDOUT" />
</logger>
```

После чего можно выполнять запросы и смотреть на них: `curl 'http://localhost:8080/api/v1.0/runs/grouped?only_important_diffs=0&offset=0&limit=20&check_id=4x0xl&diff_type=DELETED'`


## Работаем с Mongo
О чем нужно помнить:
1) Любая операция `update` по умолчанию выполняется без транзакций
2) Упавшая операция в CLI совершенно спокойно может продолжать выполняться (она сама по себе не отменится)
3) Смотреть на текущие операции можно командой `db.currentOp()`


## Как деактивировать тест

{% cut "Решение" %}

`ssh ci-clickhouse-xx.search.yandex.net`
`clickhouse-client -d testenv`
`ALTER TABLE last_test_runs UPDATE active = 0 WHERE str_id = 'f50bb45621d74a763cf337287fbf1f4b' and toolchain = 'default-linux-x86_64-release' and branch_id = 2;`
str_id - test id
toolchain - test toolchain

{% endcut %}


## Работа с Clickhouse

* Активные хосты ClickHouse указаны в [app.datasource.clickhouse.all.data-source-properties.jdbcUrl](https://a.yandex-team.ru/arc/trunk/arcadia/testenv/ci/backend/deploy/configuration/stable/config/application.properties#L17)
* Подключение к ClickHouse по ssh: `ssh root@ci-clickhouse-05.search.yandex.net`
* На хосте: `clickhouse-client --user web --password <password> -d testenv`.
  Пароль можно найти в секретнице в [testenv-ci_backend-clickhouse-prod](https://yav.yandex-team.ru/secret/sec-01d8wz4w20g27ztgpraxn5k7az/explore/versions).
* Для прекоммитных проверок уникальным идентификатором является `branch_id`, для посткоммитных проверок `branch_id + revision` (`branch_id` всегда равно 2).
* `branch_id` для прекоммитной проверки можно узнать из таблицы `branches`: https://ci.yandex-team.ru/check/<branch_name>.
* Описания всех таблиц можно посмотреть в [clickhouse.sql](https://a.yandex-team.ru/arc/trunk/arcadia/testenv/stream_processor/src/main/sql/clickhouse.sql)
* Для проверки доступности хоста <clickhouse-host> можно зайти по ssh на `ssh root@bootstrap-cibackend-2.man.yp-c.yandex.net` и выполнить команду:
  `curl 'http://<clickhouse-host>:8123'`
* Для проверки статуса реплики, необходимо зайти на хост и выполнить команду: `curl localhost:8123/replicas_status`. Если возвращается статус OK, значит отставания реплики нет.
* Узнать размер таблиц:

  {% cut "sql-запрос" %}
  ```sql
  SELECT database, table, round(sum(bytes) / 1024/1024/1024, 2) as size_gb FROM system.parts WHERE active GROUP BY database, table ORDER BY size_gb DESC;
  ```
  {% endcut %}



## MongoDB

* Mongo находится на ci-clickhouse-0[1-3].search.yandex.net хостах.
* Подключение к Mongo по ssh: `ssh root@ci-clickhouse-03.search.yandex.net`
* На хосте: `mongo -u observer -p password --port 27037 testenv`
* Если это SECONDARY хост, необходимо выполнить команду: `db.setSlaveOk()`
* Найти запись по id: `db.<collection>.find({_id:NumberLong(360069460)})` (Может быть задано регулярным выражением: `_id:/.*expr.*/`)


## Частые вопросы/Известные проблемы

* ### Как понять, какие результаты заслали в SP по конкретному тесту?
  1. Зайти в `Done` в `Configure/Build/Style/Small tests/Medium tests/Large tests` на плашке проверки.
  2. Выбрать, в зависимости от платформы теста, необходимую `AUTOCHECK_BUILD_PARENT_2` таску (на каждую платформу по 2 таски: левая без патча и правая с патчем).
  3. Зайти в `RESOURCES/AUTOCHECK_LOGS/`, там в `results2.json.tar` будут лежать все результаты, которые отправлялись в SP.
  4. В отличие от остальных платформ, в задачах LINUX есть разбиение на 6 партиций.

* ### Как узнать, из какой партиции пришли данные по тесту?
  1. В ClickHouse таблице `runs_by_revision` найти запись нужным `branch_id`, `test_id`.
  2. Взять `counter = \<counter\>` и найти в монго запись `db.change_events.find({_id:NumberLong(\<counter\>)}).pretty()`.
  3. В поле `pt` будет лежать номер партиции: [ссылка](https://a.yandex-team.ru/arc/trunk/arcadia/testenv/stream_processor/src/main/java/messages/changes/ChangeMessage.java#L27).

* ### У теста старый/неправильный `run_id`:
  У нас данные кэшируются, если у двух тестов совпадает набор `(testStrId, toolchain, status, errorType, owners, uid, size, tags, requirements)`, то `run_id` у них будет одинаковый.

* ### Где смотреть логи SP?
  1. На хостах в `/logs/stream_processor/*` <br>
     Primary processors хосты: [https://cisp11.n.yandex-team.ru/hosts](https://cisp11.n.yandex-team.ru/hosts)
     Secondary processors хосты: [https://cisp12.n.yandex-team.ru/hosts](https://cisp12.n.yandex-team.ru/hosts)
     Web хосты: [https://cisp10.n.yandex-team.ru/hosts](https://cisp10.n.yandex-team.ru/hosts)
  2. Или в [https://yt.yandex-team.ru/hahn/navigation?path=//home/logfeller/logs/sp-prod-server-log](https://yt.yandex-team.ru/hahn/navigation?path=//home/logfeller/logs/sp-prod-server-log)


* ### Графики в [https://datalens.yandex-team.ru/nx85qercjgq0k-autocheck-rt](https://datalens.yandex-team.ru/nx85qercjgq0k-autocheck-rt) показывают что-то не то:
  Проверить отставание реплик ClickHouse.

* ### Почему не было перезапуска?
  В таблице `event_stages` в поле `not_recheck_reason`.

* ### Какие таргеты были отправлены на перезапуск?
  В логах Primary processors искать запись "Initiate recheck event..." с `branch_name` прекоммита.

* ### Fat тесты не завершаются
  [Пример исследования](https://st.yandex-team.ru/DEVTOOLSSUPPORT-2221#5f0da0657fd8a90b194fd49d)

    ```javascript
    //  Дата последнего обработанного implicit diff
    let replaceTimestamp = (message) => { if (message.tm) { message.tm = (new Date(message.tm * 1000)).toISOString(); } return message; };
    db.getCollection('change_events').find({_id: db.getCollection('counters').find({_id: 'stream:change_events-processor:implicit_diffs'}).map(i => i.value)[0]}, {"tm": 1}).map(replaceTimestamp)[0]
    > { "_id" : NumberLong(605664038), "tm" : "2020-11-01T15:38:32.000Z" }
    ```

  1. Смотрит в таблицу `large_test_tasks`, запуски должны быть зарегистрированы
  2. В таблице `complete_test_events` проверяем наличие событий, об успешном завершении тестов
  3. События производятся SP сообщением `CheckCompleteDiffMessage`, [проверяем](https://st.yandex-team.ru/DEVTOOLSSUPPORT-2221#5f0f3aa55d018510a21fcf02)
     коллекцию `diff_events`
  4. Проверяем текущий счетчик обработанных сообщений в counters `stream:change_events-processor:implicit_diffs`, смотрим по id сообщения
     время последнего обработанного события.


* ### Тесты давно удалены, однако показываются как упавшие
  CI иногда пропускает удаление тестов в посткоммитах, это обычно связано с инцидентами в автосборке. Тест можно удалить руками, тогда он с личных / проектных дашбордов пропадет.
  ```bash
  ssh ci-clickhouse-xx.search.yandex.net
  clickhouse-client -d testenv
  ```

  ```sql
  ALTER TABLE last_test_runs
  DELETE WHERE str_id = 'f50bb45621d74a763cf337287fbf1f4b' and toolchain = 'default-linux-x86_64-release' and branch_id = 2;
  ```
  str_id - test id\
  toolchain - test toolchain

* ### Загорелся алерт http_check на awacs
См. https://st.yandex-team.ru/BALANCERSUPPORT-4918 и вывод
