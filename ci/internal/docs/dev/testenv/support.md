## Немного описания
См. [Передача знаний по TestEnv](https://wiki.yandex-team.ru/testenvironment/testenv-knowledge-transfer/)

## Поиск проблем
В первую очередь нужно смотреть в рассылку "testenv-errors@yandex-team.ru" - там можно найти трассировку упавших задач. 
Также можно посмотреть на упавшие задачи ядра в engine task status (скачать их логи можно YQL скриптом [отсюда](https://a.yandex-team.ru/arcadia/ci/docs/dev/testenv/logs.md)):

process_db - основная задача. Добавляет тест резалты для новых ревизий, переключает ресурсы и много чего другого. Если не работает вообще ничего - вероятно проблема где-то в ней.
add_new_shard_revisions - добавление новых ревизий в базу.
create_auxiliary_checks - создание мануал ранов.

Работа с SB задачами и их статусами:
process_finished_test_task
process_database_executing_tasks
update_test_result_and_diff_statuses


## Не обновляются ресурсы:
Посмотреть ошибки таски process_db. Наиболее распространенная ошибка - разъезжаются интервалы ресурсов после force_update. Например:

```
/testenv/engine/testenv/core/engine/database.py in get_global_resources_by_revision_beta(self=<testenv.core.engine.database.DatabaseImpl object>, test_class=<Name=MIDDLESEARCH_INFO_REQUESTS>, revision=8316612, return_unformatted_data=False)
    939             names_2_ids = {name_2_id[0]: name_2_id[1] for name_2_id in query.all()}
    940             logging.info('[resource for execute] Names 2 Ids: %s', names_2_ids)
=>  941             assert len(names_2_ids) == len(resource_names)
    942             return names_2_ids
    943 
builtin len = <built-in function len>, names_2_ids = {u'DYNAMIC_MODELS_ARCHIVE': 2228307184, u'DYNAMIC_MODELS_ARCHIVE_BASE': 2228307096, u'MIDDLESEARCH_CONFIG': 2236248873, u'MIDDLESEARCH_DATA': 2237351272, u'SEARCH_CONFIG_FOR_BASESEARCH1': 1012012869, u'SEARCH_CONFIG_FOR_BASESEARCH2': 1012012869, u'SEARCH_DATABASE_FOR_BASESEARCH1': 996978427, u'SEARCH_DATABASE_FOR_BASESEARCH2': 996978427, u'STABLE_BASESEARCH_EXECUTABLE': 2209190916}, resource_names = ['MIDDLESEARCH_CONFIG', 'MIDDLESEARCH_DATA', 'DYNAMIC_MODELS_ARCHIVE', 'SEARCH_DATABASE_FOR_BASESEARCH1', 'SEARCH_DATABASE_FOR_BASESEARCH2', 'SEARCH_CONFIG_FOR_BASESEARCH1', 'SEARCH_CONFIG_FOR_BASESEARCH2', 'PLAIN_TEXT_QUERIES_FOR_MIDDLESEARCH', 'DYNAMIC_MODELS_ARCHIVE_BASE', 'STABLE_BASESEARCH_EXECUTABLE'] 
```
Это говорит о том, что TE не может найти ресурс для запуска MIDDLESEARCH_INFO_REQUESTS на ревизии 8316612.
Просим пользователя указать нужный интервал ресурса для ручного проставления в MySQL:
подключаемся к custom шарду MySQL, делаем запрос вида 
```
UPDATE DATABASE_NAME_resources SET revision_from=1234 WHERE resource_id=1234
```
Заметьте, что дефисы в названиях проектов становятся нижними подчеркиваниями, а для некоторых особо старых проектов их названия в бд не совпадают с названиями в UI - например ws-middle-trunk в MySQL называется web_middle_trunk.

## Ошибки при процессинге интервала бинпоиском:
Принудительное закрытие интервала:
В custom шарде нас интересует таблица DATABASENAME_checked_intervals.
```
UPDATE DATABASENAME_checked_intervals SET is_checked=1 WHERE test_name=OUR_PROBLEM_TEST_NAME;
```
Если хочется не только закрыть текущий интервал, но и переместить указатель на последнюю отработавшую ревизию, нужно сделать insert с нужной ревизией:
```
INSERT INTO DATABASENAME_checked_intervals VALUES("OUR_PROBLEM_TEST_NAME", 1234567, 1);
```


## Как сменить owner'а у Large-тестов (на другую sandbox-группу)

1. Ставим клиент:
    ```
    sudo apt-get update
    apt-get install mysql-client
    ```
2. Выполняем шаги [отсюда](https://yc.yandex-team.ru/folders/fooo2f1maha60gvok8ml/managed-mysql/cluster/mdbn1pq5ulbhhmabtgea), [скрин](https://jing.yandex-team.ru/files/albazh/2021-06-11_14-57-36.png). Мастер-хост со временем может меняться. Данный сниппет всегда подскажет текущий мастер.
3. Пароль из ключа `mysql_autocheck_master_password`: [ссылка](https://yav.yandex-team.ru/secret/sec-01dw2n63s9zkkw4wpzxg15z784/explore/version/ver-01enj9wxbwdw6sqda6rqycfvk3)
4. Сами запросы:
    ```sql
    select count(*) from autocheck_trunk_tests where name REGEXP 'FAT_[^/]*MAPS/.*';
    select task_owner, name from autocheck_trunk_tests where name REGEXP 'FAT_[^/]*MAPS/.*';

    # Вероятно, стоит делать с доп условием на task_owner = 'AUTOCHECK-FAT' (зависит от тикета)
    # select count(*) from autocheck_trunk_tests where name REGEXP 'FAT_[^/]*MAPS/.*' and task_owner = 'AUTOCHECK-FAT';
    # select task_owner, name from autocheck_trunk_tests where name REGEXP 'FAT_[^/]*MAPS/.*' and task_owner = 'AUTOCHECK-FAT';
    ```
5. Проверяем по `name`, что не выбралось чего-то лишнего
6. Дальше update:
    ```sql
    set autocommit=0;
    Start transaction;
    update autocheck_trunk_tests set task_owner = 'MAPS-CI' where name REGEXP 'FAT_[^/]*MAPS/.*';

    # или с доп условием на task_owner
    # update autocheck_trunk_tests set task_owner = 'MAPS-CI' where name REGEXP 'FAT_[^/]*MAPS/.*' and task_owner = 'AUTOCHECK-FAT';
    commit;
    ```
7. Проверить можно так:
    ```sql
    select task_owner, name from autocheck_trunk_tests where name REGEXP 'FAT_[^/]*MAPS/.*';
    ```


## Заканчивается место в системном шарде (алерт system_shard_unispace)

https://yc.yandex-team.ru/folders/fooo2f1maha60gvok8ml/managed-mysql/cluster/mdb4ti8a84ct89f4l27a?section=monitoring

Нужно посмотреть на место командой `show table status`.

Скорее всего, распухла таблица `testenv_tasks`. В этом случае делаем следующее:
1) `truncate table testenv_tasks;`
2) `optimize table testenv_tasks;`
3) Перезапускаем все **engine**-ы (нужно сделать обязательно, чтобы TE перезапустил все свои задачи). См. ниже

В течение нескольких часов место на дисках освободится.
Проверить можно выполнив `ANALYZE NO_WRITE_TO_BINLOG TABLE testenv_tasks;` и посмотрев `show table status`.
Также см. https://st.yandex-team.ru/MDBSUPPORT-4354


## Как рестартовать engine-ы
1) Зайти в https://deploy.yandex-team.ru/stages/testenv-prod/config/du-engine/box-engine/wl-engine
2) Отредактировать конфиг:
2.1) Найти переменную **RESTART** и увеличить её значение на единицу (1 -> 2, 2 -> 3 и т.д.)
2.2) Выполнить **Update**
   

## Как рестартовать web-интерфейсы
Отредактирование конфиг аналогично engine-у, только конфиги будут (лучше применять в обоих):
1) https://deploy.yandex-team.ru/stages/testenv-prod/config/du-webserver/box-webserver/wl-ci.in.yandex-team.ru
2) https://deploy.yandex-team.ru/stages/testenv-prod/config/du-webserver/box-webserver/wl-te.in.yandex-team.ru


## CPU throttling CUSTOM шарда
1) SHOW PROCESSLIST; (ищем зависшие запросы)
2) Например зависли запросы на test_diffs в базе arcadia-uids-trunk:
 - ставим start_revision джобов на сравнительно новую ревизию (через интерфейс testenv (вкладка tests -> edit test))
 - удаляем test_diffs и test_results до этой ревизии (```DELETE FROM arcadia_uids_trunk_test_diffs WHERE revision1 < 8407690 OR revision2 < 8407690```)
3) Убиваем старые зависшие запросы
