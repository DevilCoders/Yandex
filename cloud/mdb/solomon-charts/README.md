# solomon-charts

В этом репозитории хранятся конфиги графиков и дашбордов для системы мониторинга Solomon.
Для работы скрипта синхронизации необходимо иметь OAuth-токен, который для обоих продов можно получить [по ссылке](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=1c0c37b3488143ff8ce570adb66b9dfa)

Для некоторых окружений нужно получить iam-token:
`yc iam create-token`. Для GPN токен получать от своего юзера, так как у него есть права на изменение настроек.
## Концепция

Файлы:
- `out` - директория с генерированными сущностями Solomon, контент не коммитим
- `solomon` - скрипты для генерации сущностей их загрузки
- `templates` - шаблоны сущностей
- `solomon.json` - конфиг для рендера и загрузки сущностей
- `solomon.py` - скрипт для создания и обновления сущностей из `out` в Solomon.

Директории `alerts`, `dashboards`, `graphs`, `notifcation_channels` и файлы `update.py`, `config.json` используются для поддерживаемого, но устаревшего подхода. Новые сущности нужно создавать в директории `templates`.

### Общий подход

Отрендеренная сущность в `./out` = шаблон сущности из `./templates` + набор атрибутов окружения из `./solomon.json`

Шаблоны следует размещать в `./templates/{entity_kind}/{area}`. Файл шаблона должен иметь маску `[-a-z0-9]+\.j2`.
`entity_kind` - тип (dashboards, alerts, graphs, etc.), `area` - название фичи или сервиса, некоторый домен.
Имя файла на идентификатор в Solomon или название не влияет.
В шаблон можно инклудить файлы с маской файла `*.t.js`. Сделано для выражений алертов или графиков.
Чтобы заинклудить нужно написать конструкцию вида `{% include {entity_kind}/{area}/{template_name} %}`. То есть
указать путь до включаемого файла без его расширения.
Базо-специфичные шаблоны стоит хранить в `./templates/{entity_kind}/{db}`.


### Пример: как добавить шаблон алерта и раскатить по окужениям

Есть несколько развернутых одинаковых демонов или библиотек в разных окружениях, которые отправляют один и тот же сигнал под разными сервисами
и даже в разных окружениях на разные инсталляции Solomon. Как сделать с одним шаблоном по шагам:

1. Так как выкатываем по разным окружениям и проектам Solomon, то окружения описываем в `./solomon.json`.
2. Если окружения нет, то создаем в секции `.envs`.
3. В настройках окружения обязательно дожны быть:
    * `project_id` - проект Solomon
    * `id_prefix` - префикс идентификатора сущности: `mdb-prod|mdb-test`.
4. Как сделать алерт для множества демонов под разными сервисами в одном окружении: в `./solomon.json` в секции `.opts` добавить секцию со списком
этих сервисов, в качестве примера можно посмотреть на `.opts.db`. При рендере шаблона скрипт вычитывает идентификаторы из его `id` поля и
строит продукт их комбинаций. Опираясь от получившегося продукта из наборов идентификаторов скрипт будет последовательно
рендерить сущности сохраняя в `./out/{env_name}/{area}/` в файлы с названием их идентификатов с расширением `json`.
5. Описываем программу алерта в `./templates/{entity_kind}/{area}/{cool-alert}.t.js`
6. В expression вставляем инклуд: `{% include {entity_kind}/{area}/{cool-alert} %}`
7. Последовательно для всех `env_name`: `python ./solomon.py render -s ./templates/ -t ./out -e {env_name} -k alerts`
8. `export SOLOMON_OAUTH_TOKEN_env=***`, `export SOLOMON_IAM_TOKEN_env=(yc iam create-token)` или кладем в файл `~/tokens/solomon.json` токен `{ "{env_name}":"***" }`(только для OAuth)
9. Последовательно `python ./solomon.py upload --env {env_name} --kind alerts`.


### Нюансы и детали реализации

* Можно пользоваться только `solomon.py`.
* `solomon.py render` - рендер сущностей из `./templates` с использованием Jinja2 и `./scripts/config.json`
* `solomon.py upload` - создание обновление сущностей в Solomon из `./out`.
* Контекст Jinja создает из конфига окружения `solomon.json` `.envs.your_env`, `.opts`, `.db_ctx` и передает параметры напрямую,
то есть можно к ним обращаться через такое же название переменной.
* Для того чтобы передать набор атрибутов по ключу, следует использовать в `.opts` словарь из словарей, пример - `db_ctx`.

* Нюанс #0: чтобы не конфликтовать с jinja синаксисом  Solomon для идентификаторов используется конструкция `<< [_a-z]{1,55} >>`.
* Нюанс #1: `render` сам ищет идентификаторы в темплейтах и пытается разрезолвить их в конфиге
* Нюанс #2: списковые и словарные переменные следует использовать в id шаблона, так скрипт поймет, что надо генерировать
несколько сущностей на 1 шаблон. При нахождении там несколькоих списков или словарей генерируется продукт от них (декартовое
произведение).
* Нюанс #3: `db_ctxs` - переменные, которые будут добавлены в контекст при нахождении шаблона `<< db >>`, используется для jinja-if'ов.
Со списками в них не тестировалась
* Нюанс #4: На словарь скрипт генерирует набор сущностей с (key, value), чтобы обратиться к значению, то к названию словаря
следует приписать `_val`, пример: `<< health_status >>` - рендерит ключ из записи словаря, `<< health_status_val >>` -
рендерит значение из записи словаря.
* Нюанс #5: Не надо коммитить `./out` директорию. Всегда можно перегенерить.
* Нюанс #6: Если надо избежать рендера каких-то сущностей для некоторых окружений, можно создать в `area` директории `index.json`
с примерно таким содержимым (секция `exclude` не обязательная):

```json
{
    "include":[
        {
            "env_name": "porto",
            "files": ["dead-in-walle.j2", "invalid-in-walle.j2"]
        }
    ],
    "exclude":[
        {
            "env_name": "compute",
            "files": ["auto-spi-check.j2"]
        }
    ]
}
```

То есть в этой директории будут включаться в рендер только те два файла если в `env_name` будет подстрока `porto`.
Если нужно исключить все файлы из `area` для не `porto` окружений:

```json
{
    "include":[
        {
            "env_name": "porto",
            "files": "*"
        }
    ]
}
```

Фильтров может быть несколько.
При одновременном использовании include и exclude, последний имеет приоритет, а также работает только с тем списком, который получен в результате обработки `include`.


Чтобы избежать лишних обновлений и случайного затирания, используются следующие правила для рендера из шаблонов в сущности:
1. Иерархия сущностей: `environment` -> `entity_kind` -> `area`
2. Именование сущностей (`id`):
    * общий шаблон: `<< id_prefix >>-area-<< your_list >>-your-postfix`
    * `id_prefix` - идентификатор MDB и контура (`prod`, `preprod`, `test`), пример: `mdb-prod`. Внутри solomon.y-t.ru
    очевидно, что MDB не будет там держать сущности и сигналы и `compute-prod`, следовательно про compute нет смысла и писать.
    * `area` или `service` - к чему принадлежит сущность, примеры: `dbm`, `metadb`, `health`.
    * ваш собственный суффикс.
    * не более 55 символов в ID, ограничение Solomon.
    * пример: id:`<< id_prefix >>-health-<< db >>-clusters-alive`.


### TODO:
 - [x] Добавить возможность инклудить expressions из смежных j2-файлов.
 - [ ] Добавить поддержку версионирования, чтобы не перетирать.

## Deprecated
Обновление для внутреннего прода:
```
TOKEN=*** python update.py --project-name internal-mdb --env <env>
, где <env> - test, prod
```
Обновление для внешнего прода:
```
ENDPOINT='https://solomon.cloud.yandex-team.ru/api/v2' TOKEN=*** python update.py --project-name yandexcloud --env <env>
, где <env> - preprod, prod
```
Если пишет 403, возможно нет прав, [смотреть тут](https://solomon.cloud.yandex-team.ru/admin/projects/yandexcloud)

В ```alerts/*-program.json``` лежит программа на [Язык аналитических запросов Solomon](https://wiki.yandex-team.ru/solomon/userguide/el/), система alerts описана [тут](https://wiki.yandex-team.ru/solomon/userguide/alerting/)

Подробное описание мониторинга MDB: [https://wiki.yandex-team.ru/mdb/internal/mdb-monitoring/](https://wiki.yandex-team.ru/mdb/internal/mdb-monitoring/)
В файле config.json описываются наборы графиков и дашбордов для каждого из проектов, а пример запуска скрипта выглядит так:
```bash
schizophrenia-o2:solomon-charts schizophrenia$ TOKEN=<OAuth token> ./update.py internal-mdb
dashboards/mdb-cluster-clickhouse.json skipped, no difference
dashboards/mdb-cluster-postgres.json skipped, no difference
dashboards/mdb-cluster-mongodb.json created
dashboards/mdb-porto-instance.json created
dashboards/mdb-porto-cluster.json created
graphs/mdb-porto-instance-cpu.json created
graphs/mdb-porto-instance-memory.json skipped, no difference
graphs/mdb-porto-instance-memory-faults.json skipped, no difference
graphs/mdb-porto-instance-network.json skipped, no difference
graphs/mdb-porto-instance-network-packets.json created
graphs/mdb-porto-instance-disk.json skipped, no difference
graphs/mdb-porto-cluster-cpu.json skipped, no difference
graphs/mdb-porto-cluster-memory.json created
graphs/mdb-porto-cluster-memory-faults.json skipped, no difference
graphs/mdb-porto-cluster-network.json created
graphs/mdb-porto-cluster-network-packets.json created
graphs/mdb-porto-cluster-disk.json skipped, no difference
```

Установка:
```
$ pyenv virtualenv solomon
$ eval "$(pyenv init -)"
$ pyenv activate solomon
(solomon) $ pip install jinja2 click requests
$
```
