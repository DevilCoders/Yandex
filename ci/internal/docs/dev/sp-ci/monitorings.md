# Мониторинги

## Stream processor
{% note warning %}

Следующие SP мониторинги наиболее критичные и требуют немедленной реакции:
* ci_sp-primary_processors-stopped
* ci_sp-primary_export_latency
* ci_stream_processor

{% endnote %}

[ci_stream_processor](https://juggler.yandex-team.ru/check_details/?host=solomon-alert&service=ci_stream_processor) - общий мониторинг времени активных стадий SP.

**Контроллеры SP в самогоне**
* [WEB](https://cisp10.n.yandex-team.ru/)
* [PRIMARY](https://cisp11.n.yandex-team.ru/)
* [SECONDARY](https://cisp12.n.yandex-team.ru/)

**ClickHouse в MDB**
* [ci-clickhouse-stable-runs](https://yc.yandex-team.ru/folders/fooo2f1maha60gvok8ml/managed-clickhouse/cluster/mdb5hfu4l1f0job8l8nt?secton=monitoring)
* [ci-clickhouse-stable-all-runs](https://yc.yandex-team.ru/folders/fooo2f1maha60gvok8ml/managed-clickhouse/cluster/mdbckcguevu91iu04akc?section=monitoring)

### Мониторинги по обработке SP
**Наиболее информативные дашборды**
* [SP 1](https://solomon.yandex-team.ru/?project=ci_sp&service=ci_sp_10&cluster=ci_sp_10&dashboard=ci-sp)
* [SP 2](https://solomon.yandex-team.ru/?project=ci_sp&cluster=ci_sp_10&service=ci_sp_10&dashboard=ci-sp2)

**Список мониторингов**
* [ci_sp-primary_processors-stopped](https://juggler.yandex-team.ru/check_details/?host=solomon-alert&service=ci_sp-primary_processors-stopped)
* [ci_sp-primary_processors-high_load](https://juggler.yandex-team.ru/check_details/?host=solomon-alert&service=ci_sp-primary_processors-high_load)
* [ci_sp-secondary_processors-stopped](https://juggler.yandex-team.ru/check_details/?host=solomon-alert&service=ci_sp-secondary_processors-stopped)
* [ci_sp-secondary_processors-high_load](https://juggler.yandex-team.ru/check_details/?host=solomon-alert&service=ci_sp-secondary_processors-high_load)

{% cut "Действия" %}

1. Понять какие процессы остановились - агрегат содержит в себе мониторинги разбитые по процессам.
2. Проверить логи процессора на соответствующем контроллере SP на предмет необычных ошибок, проверить когда последний раз катился SP - ошибки разбирать по ситуации, в случае недавнего деплоя прийти к автору релиза, узнать, что катилось, можно ли откатить.
3. Найти строку соответствующих процессу графиков на дашборде **SP 2**, посмотреть как они изменились при срабатывании мониторинга - если явно выросли графики таймингов операций в монге, вероятнее всего у какой-то из используемых при обработке коллекций индексы перестали влезать в память, можно добавить памяти, что-то почистить, за подробностями можно обратиться к (@andreevdm)[https://staff.yandex-team.ru/andreevdm].

**ci_sp-secondary_processors-stopped** - эти процессы терпят сутки-другие. Если остановился процесс implicit_diffs, то можно передать [@anmakon](https://staff.yandex-team.ru/anmakon). В остальных случаях смотреть по общей инструкции.

При срабатывании мониторинга типа **\*-high_load** нужно:
* Следить за основной стадией SP на дашборде дежурного автосборки и мониторингом **ci_stream_processor**, возможен рост времени обрботки из-за высокой нагрузки.
* Найти резко выросшие графики типа **\* Processing Latency**, понять растут ли они или уже начали падать. Чаще всего срабатывает при обработке большого числа **Explicit Diff** и само рассасывается.

{% endcut %}


### Мониторинги по отставанию SP
* [ci_sp-change2implicit_diff-implicit_diffs-delay-alert-lower](https://juggler.yandex-team.ru/check_details/?host=testenv-stream-processor-stable&service=ci_sp-change2implicit_diff-implicit_diffs-delay-alert-lower&last=1DAY) - отставание implicit diff-ов в SP Secondary (5 дней WARN, 20 дней ALERT). Если зажегся, то обработка implicit diff-ов отстает по какой-то причине; обычно из-за того, что SP Secondary просто падает.
* [ci_sp-change2test_properties-test_properties-delay-alert-lower](https://juggler.yandex-team.ru/check_details/?host=testenv-stream-processor-stable&service=ci_sp-change2test_properties-test_properties-delay-alert-lower&last=1DAY) - отставание обработки пропертей в SP Secondary, в т.ч. и в бранчах (2 дня WARN, 7 дней ALERT). Если зажегся, то обработка test ptoperties отстает; либо тормозит монга, либо SP постоянно падает из-за обработки implicit diff-ов и эти properties тоже постоянно перезапускаются.

### Мониторинги по MongoDB
**Список мониторингов**
* [sp-stable-sharded-cpu_wait*](https://juggler.yandex-team.ru/check_details/?host=mondgo-db_sp-stable-sharded&service=sp-stable-sharded-cpu_wait)
* [sp-stable-sharded-disk_free*](https://juggler.yandex-team.ru/check_details/?host=mondgo-db_sp-stable-sharded&service=sp-stable-sharded-disk_free)
* [sp-stable-sharded-replication_lag*](https://juggler.yandex-team.ru/check_details/?host=solomon-alert&service=sp-stable-sharded-replication_lag)
* [sp-stable*](https://juggler.yandex-team.ru/check_details/?host=solomon-alert&service=sp-stable)

**Базы в MDB**
* [sp-stable-sharded](https://yc.yandex-team.ru/folders/fooo2f1maha60gvok8ml/managed-mongodb/cluster/mdbab7786re5cltjf1r8?section=monitoring)
* [sp-stable](https://yc.yandex-team.ru/folders/fooo2f1maha60gvok8ml/managed-mongodb/cluster/mdbeh9aj6rspu96fakv4?section=monitoring)

{% cut "Действия" %}
1. Смотрим графики в MDB на вкладке "Мониторинг", проверяем статусы хостов, смотрим графики по хостам, пытаемся определить кто тормозит.
2. Если непонятно почему не реплицируется база, почему загружен хост и тд. нужно идти в mdb support.

**\*disk_free** - Кол-во свободного места на жестком диске. Необходимо проверить работу GC в Stream processor.
Проверить, что GC работает можно по логам процесса auxillary в **SECONDARY**. GC всегда пишет о кол-ве удаляемых старых записей с указанием коллекций.

{% note warning %}

GC работает не все время, а запускается по расписанию, указанному в конфиге в`gc.schedule`. Также максимальное непрерывное время работы GC ограничено в `gc.timeLimit`. При изучении логов это нужно учитывать.

{% endnote %}

GC чистит все коллекции последовательно в одном потоке, поэтому в особых случаях может не успевать очистить некоторые коллекции (из-за загруженности других).
* Если при работе GC случаются ошибки (кроме ошибки таймаута на время работы GC), то необходимо их пофиксить.
* Если GC много циклов (> 2 дней) не успевает добраться до чистки каких-то коллекций, необходимо разобраться почему. В качестве workaround можно закомментировать все GC идущие перед проблемной коллекцией и вернуть после очистки [здесь](https://a.yandex-team.ru/arc_vcs/testenv/stream_processor/src/main/java/engine/DefaultTaskEnqueuerRegistry.java?rev=92a7679ecb4beaa3cca3febd8ad3ef750a0d4e80#L370).
* Если GC работает, и место заканчивается в результате стабильного роста данных на протяжении длительного срока, то можно попробовать добавить места в настройках кластера или добавить шард в случае sp-stable-sharded.

Если данные занимают 80% и более, то можно попробовать запустить переналивку хостов (операция "ресинхронизировать"). Это гарантировано очистит все устаревшие данные из коллекций MongoDB, так что диски станут заполнен на 40%.

**\*replication_lag** - В случае слишком большого лага база не сможет опреплицироваться стандартными средствами, в таком случае автоматика примерно через 3 часа перенальет хост (чаще всего это происходит **sp-stable-sharded** с обычно этого достаточно).

{% endcut %}

### Мониторинги по экспорту из SP
**Наиболее информативные дашборды**
* [ClickHouse export](https://solomon.yandex-team.ru/?project=ci_sp&cluster=ci_sp_10&service=ci_sp_10&dashboard=ci-clickhouse)

**Список мониторингов**
* [ci_sp-primary_export_latency](https://juggler.yandex-team.ru/check_details/?host=solomon-alert&service=ci_sp-primary_export_latency)
* [ci_sp-secondary_export_latency](https://juggler.yandex-team.ru/check_details/?host=solomon-alert&service=ci_sp-secondary_export_latency)

{% cut "Действия" %}
1. По дашборду **ClickHouse export** понять какой процесс тормозит.
2. Проверить логи соответствующего экспортера на наличие ошибок в контроллере самогона.

**ci_sp-primary_export_latency** - Если тормозит выгрузка в коллекцию **runs**, имеет смысл проверить MDB ClickHouse **ci-clickhouse-stable-runs**.

**ci_sp-secondary_export_latency** - Имеет смысл проверить логи ошибок и MDB ClickHouse **ci-clickhouse-stable-all-runs**.

{% endcut %}

---

## Мониторинги по ClickHouse
**Список мониторингов железных ClickHouse**
* [unispace](https://juggler.yandex-team.ru/check_details/?host=clickhouse&service=unispace)
* [replicas_status](https://juggler.yandex-team.ru/check_details/?host=clickhouse_replicas_status&service=replicas_status)
* [ci_clickhouse_liveness](https://juggler.yandex-team.ru/check_details/?host=solomon-alert&service=ci_clickhouse_liveness)
* [UNREACHABLE](https://juggler.yandex-team.ru/check_details/?host=clickhouse&service=UNREACHABLE)

**Список мониторингов MDB ClickHouse**
* [clickhouse-stable-all-runs*](https://juggler.yandex-team.ru/check_details/?host=solomon-alert&service=clickhouse-stable-all-runs)
* [clickhouse-stable-runs-abs_delay](https://juggler.yandex-team.ru/check_details/?host=solomon-alert&service=clickhouse-stable-runs-abs_delay)
* [clickhouse-stable-runs-cpu_wait](https://juggler.yandex-team.ru/check_details/?host=solomon-alert&service=clickhouse-stable-runs-cpu_wait)
* [clickhouse-stable-runs-disk_free](https://juggler.yandex-team.ru/check_details/?host=solomon-alert&service=clickhouse-stable-runs-disk_free)

{% cut "Действия на железных ClickHouse" %}
1. Из алерта определяем проблемный хост. Например, на `ci-clickhouse-05.search.yandex.net`
2. Заходим по ssh: `ssh root@ci-clickhouse-05.search.yandex.net`
3. (По ситуации) Логинимся в кликхаус: `clickhouse-client --database testenv`

**unispace** - кончается место на жестких дисках
* Находим размеры таблиц:
  ```sql
  SELECT table, round(sum(bytes) / 1024/1024/1024, 2) as size_gb FROM system.parts WHERE active GROUP BY table ORDER BY size_gb DESC;
  ```
* Если видим, что таблица `query_log_1` занимает много места (50+ Гб), то:
    * Находим дату `select max(event_date) from system.query_log_1 limit 1;`
    * Идём к [@anmakon](https://staff.yandex-team.ru/anmakon), спрашиваем, можно ли дропнуть эту таблицу.

**replicas_status** - отстают реплики.
* Закрыть порт 8123 на отстающих хостах ```sudo iptruler 8123 down ael```.
* Установить какие таблицы отстают через ```curl localhost:8123/replicas_status```.
* Устанавливаем приичины отставания, вероятно придется грепать логи ```/var/log/clickhouse-server/```.
* После устранения отставания открываем порты ```sudo iptruler flush```

**ci_clickhouse_liveness** - кол-во живых хостов по http.
**UNREACHABLE** - недоступны хосты по сети.

{% endcut %}

{% cut "Действия на MDB ClickHouse" %}
1. Из алерта определяем проблемный кластер.
2. Смотрим раздел "Мониторинги" на кластере, при необходимости на хостах.
3. (По ситуации) подключаемся к серверу через clickhouse-client ("Подключиться" на главной странице кластера в MDB), пароль в секрете [ci-clickhouse-mdb](https://yav.yandex-team.ru/secret/sec-01e6y28hjnwqekar7we7br2vmz/explore/versionshttps://yav.yandex-team.ru/secret/sec-01e6y28hjnwqekar7we7br2vmz/explore/versions)

**disk_free** - кончается место на жестких дисках
* Для кластера **clickhouse-stable-all-runs** обычно достаточно почистить старые данные в таблице **all_runs_by_test_id**, оставив данные за полгода.
* Для кластера **clickhouse-stable-runs** вероятно придется добавить еще один шард.

**abs_delay** - отстают реплики.
* Обычно рассасывается само, в ином случае идем в mdb-support.

**cpu_wait** - упираемся в CPU.

{% endcut %}

---

## Мониторинги по CI backend
**Контроллеры CI backend в самогоне**
* [BACKEND STABLE](https://cibackend.n.yandex-team.ru/)

**Наиболее информативные дашборды**
* [CI Backend stable](https://solomon.yandex-team.ru/?project=ci_backend&dashboard=ci_backend_main_dashboard_stable)

**Список мониторингов**
[ci_backend-stable](https://juggler.yandex-team.ru/check_details/?host=solomon-alert&service=ci_backend-stable) - общий агрегат всех ошибок по ручкам ci backend

{% cut "Действия" %}
1. Из алерта определяем какая ручка падает.
2. Смотрим логи на контроллере BACKEND STABLE. Проверяем когда последний раз деплоился сервис.
3. Можно попробовать выключить/включить проблемный хост, часто помогает при проблемах с кешами.

{% endcut %}
