## Функциональные тесты
Для тестов используется [godog](https://github.com/DATA-DOG/godog). Код тестов лежит [тут](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/mdb-internal-api/functest). Feature-файлы лежат [тут](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/mdb-internal-api/functest/features). Фиче-файлы подключается [тут](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/mdb-internal-api/functest/func_test.go?rev=6155638#L10).

Тесты запускают оба API и MetaDB. MetaDB и старое API запускаются рецептом. Новое API запускается как библиотека теста, что позволяет подставлять моки прямо в процессе реботы. Тесты умеют выбирать на какие ручки в какое API ходить. Тесты имеют общий код, но вызываются из разных директорий, в которых и происходит параметризация внутри `ya.make`.

Тесты запускаются через `ya make -tt`. Можно запустить как все тесты разом из `mdb-internal-api/functest`, так и каждый тест по отдельности зайдя в нужную директорию (`mdb-internal-api/functest/tests`).

### Дебаг тестов
Переменная окружения `GODOG_HOLD_ON` позволяет остановить выполнение теста и не завершать процесс. Возможные значения:
1. `exit` - не завершать процесс после завершения теста.
2. `error` - остановить выполнение теста при первой ошибке и не завершать процесс.

В Аркадии у всех тестов есть таймаут. Если его не отключить, то тест с `GODOG_HOLD_ON` будет убит по таймауту. Отключить таймаут можно во время запуска теста через аргумент `--test-disable-timeout`.

Так же полезно добавить флаги `--test-stdout --test-stderr` - это заставляет тесты выводить свои логи сразу на консоль.

Пример - `GODOG_HOLD_ON=error GODOG_FEATURE_PATHS=/home/sidh/devel/go/src/a.yandex-team.ru/cloud/mdb/mdb-internal-api/functest/features/common/common.feature ya make -tt --test-stdout --test-stderr`

Для дебага приложений на Go используем [delve](https://github.com/go-delve/delve). Для подключения к идущему тесту нужно выполнить `dlv attach <pid>` где `pid` - процесс самого теста.

Также, есть возможность запуска тестов в debug-режиме через IDE, см. раздел [запуск тестов без ya.make](#bystryj-zapusk-testov-bez)

### Запуск части `features`

Переменной окружения `GODOG_FEATURE_PATHS` можно переопределить список запускаемых `features`.  **Все пути должны быть абсолютными**. Можно указывать как конкретные файлы, так и директории (будет вычитывать рекурсивно). Список указывается через `;`.

`GODOG_FEATURE_PATHS=/home/sidh/devel/go/src/a.yandex-team.ru/cloud/mdb/mdb-internal-api/functest/features/common/common.feature ya make -tt`.

Переменной окружения `GODOG_TAGS` - можно запускать только `Scenario` *удовлетворящие* этим `tags`

Запуск тестов с `timetravel`
```
functest $ export GODOG_TAGS=timetravel
functest $ ya make -tt --test-size=medium
```

Запуск тестов имеющих `events` и `delete`
```
functest $ GODOG_TAGS='@events && @delete' ya make -tt --test-size=medium
```

[Примеры выражений для tags](https://a.yandex-team.ru/arc/trunk/arcadia/vendor/github.com/DATA-DOG/godog/features/tags.feature)

**Название теста**| **Модифицирующие ручки**| **Read-only ручки**
--- | --- | ---
`wgrpcrgrpc`| gRPC| gRPC
`wgrpcrrest`| gRPC| REST
`wrestrgrpc`| REST| gRPC
`wrestrrest`| REST| REST

### Быстрый запуск тестов без `ya make`
"Из коробки" запуск тестов стандартными средствами Go невозможен, т.к. тесты ожидают подготовленного окружения,
которое поднимается средствами рецептов. Для нового `internal api`, единственный используемый рецепт - это
[metadb](../../dbaas_metadb). Используемый для поднятия этой базы рецепт можно настроить, чтобы база переживала
запуск тестов, таким образом последующие тесты можно будет запускать без использования рецепта. Это означает две вещи:
* Postgresql server не будет подниматься при каждом запуске тестов, ускоряя процесс
* Отвязавшись от рецептов, появляется возможность использовать нативный `go test`, ускоряя процесс еще больше
* Отказавшись от `ya make`, тесты можно запускать хоть напрямую из `GoLand`.

Чтобы получить описанные выше блага:
* Включаем `persistence` для рецепта `metadb`: [дока](../../recipes/postgresql/README.md)
* Единожды прогоняем `ya make -tt` на любой кусок функциональных тестов; убеждаемся, что `postgresql` все еще запущен
* Устанавливаем переменные окружения, которые обычно выставляются в `ya.make`:
    ```bash
    export MDB_INTERNAL_API_FUNC_TEST_READ_MODE_API=grpc
    export MDB_INTERNAL_API_FUNC_TEST_MODIFY_MODE_API=grpc
    # Any directory
    export DBAAS_INTERNAL_API_RECIPE_TMP_ROOT=/tmp/dbaas-internal-api-tmp
    export _SETUP_GODOG_FEATURE_PATHS=cloud/mdb/mdb-internal-api/functest/features/kafka
    export GODOG_FEATURE_PATHS=/Users/prez/work/arc/arcadia/cloud/mdb/mdb-internal-api/functest/features/kafka/create_cluster.feature
    ```
* В рабочей директории, из которой будем запускать тесты, создаем директорию `logs` (не стал пока прописывать это в коде тестов):
    ```bash
    mkdir -p logs
    ```
* Запускаем тесты!
    ```bash
    $ time ~/.mdb_pg_recipe_persistence/env.sh go test ./... --count=1
    ok   a.yandex-team.ru/cloud/mdb/mdb-internal-api/functest/tests/kafka 2.377s

    real 0m6.684s
    ```

#### Debug на OS X
Для работы отладчика на OS X используется `debugserver` от `LLVM`. Сочетание различных [багов и недоработок](https://youtrack.jetbrains.com/issue/GO-8953) приводит к тому, что при запуске в debug-режиме при возникновении `nil reference`, даже если паника от него будет перехвачена через `recover`, дебаггер останавливает выполнение на этом месте и дальше не идёт.

Чтобы это исправить, нужно обновить CommandLineTools, например, как советуют на [SO](https://stackoverflow.com/questions/34617452/how-to-update-xcode-from-command-line).

### Фичи
Все старые степы работают, но появились новые.

Названия фичей **должны** быть уникальны. Это связано с тем, что все фичи запускаются в рамках одного test suite, и аркадийный CI должен распарсить все тесты входящий в suite.

Степы обращения к API теперь могут иметь префиксы `REST` и `gRPC` - например, `When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/databases/testdb2"` является аналогом `When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.DatabaseService" with data`, но для разных API. Степы с префиксом определяют, для какого типа API они будут работать. То есть, степ с префиксом `REST` никогда не будет вызван, если мы должны протестировать `gRPC` ручку, и наоборот. В старом API для всех степов со словом `gRPC` поставлены ничего не делающие заглушки.

Вызов того или иного API определяется по тому, модифицирующая это ручка или нет. Для `REST` метод `GET` считается читающим, всё остальное - модифицирующим.  Для `gRPC` методы с префиксом `GET` и `LIST` читающие, остальные - модифицирующие.

Если степ обращения API не имеет префикса (написан в старом формате и подразумевает обращение к старому API), то он будет вызван безяапеляционно, не важно какой вариант теста вы запустили. Это необходимо для работы тестов в случаях, когда в новом API еще нет аналога необходимой ручки.

Если функциональность реализована только в gRPC, то вся фича помечается тегом @grpc_api. Такие тесты запускаются только в wgrpcrgrpc. (Ко всем тестам, кроме wrgrpcrgrpc, в теги добавляется "~@grpc_api && $GODOG_TAGS")

Степы проверок результатов учитывают, какой степ действия был вызван до этого (`REST` или `gRPC`). Например, степ `Then we get gRPC response with body` будет `noop`, если до этого было действие через `REST` степ.

Степы следует располагать парами группируя по типам. Например как [тут](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/mdb-internal-api/functest/features/clickhouse/databases.feature?rev=6012257#L56). Сначала пара "REST запрос, REST ответ", потом пара аналогичных запросов к `gRPC`. Так будет проще и удобнее.

Тело `gRPC` запросов и ответов нужно писать в формате `JSON`. Названия ключей и типы должны строго соответствовать тому, что в спеках. Наша отсебятина в старом API не прокатит. Спеки лежат [тут](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/bitbucket/private-api). Для интересующихся, мы используем [этот](https://godoc.org/github.com/golang/protobuf/jsonpb) маршалер `JSON`<->`Protobuf` и `gRPC` рефлексию для определения типов данных в рантайме.

### Проблемы
##### В stderr вываливается исключение вида "TestStartedError: Create Compute Hadoop Cluster"
Причина -несколько фичей имеют одинаковые называния. https://st.yandex-team.ru/DEVTOOLS-6530

##### В stdout пишется что-то типа "не определён степ в godog"
В гошных тестах не определён степ. Т.к. эта ошибка выводится вне каких-либо тестов, то аркадийный CI не может её смапить на тест и никуда не выводит.
