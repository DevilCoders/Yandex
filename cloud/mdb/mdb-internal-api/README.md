# MDB Internal API

### Структура

##### Общие принципы
Верхнеуровневые директории:
* `cmd` - main'ы всех приложений
* `functest` - функциональные тесты (документация [тут](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/mdb-internal-api/functest/README.md))
* `internal` - детали реализации (это невозможно импортировать другим сервисам)
* `pkg` - все "публичные" пакеты (то что можно импортировать другим сервисам)

Практически весь код следует общему правилу разделения на пакеты - `mypackage` содержит интерфейсы для чего-либо, а `mypackage/myimplementation` одну из реализаций этого интерфейса.

Например, `internal/logsdb` содержит интерфейс к LogsDB, `internal/logsdb/clickhouse` - реализацию этого интерфейса для ClickHouse, а `internal/logsdb/mocks` - моки этого интерфейса.

##### Более детальная структура
* `internal/app.go` - инициализация приложения
* `internal/api` - реализация API
* `internal/api/grpc` - реализация gRPC API
* `internal/api/auth` - аутентификация пользователей
* `internal/logic` - верхнеуровневая (бизнес-)логика сервиса, внутри лежат пакеты-провайдеры логики
* `internal/logsdb` - работа с LogsDB
* `internal/metadb` - работа с MetaDB
* `internal/models` -  внутренние модели
* `pkg/internalapi` - клиент к сервису (удобен для хождения в internal api из других сервисов и для тестов)

### Design

##### Архитектура
В общем случае применяется **Clean Architecture**. Если разделять на слои, то выглядит это _примерно_ так (звёздочка означает рекурсивное включение пакетов во всех поддиректориях):

**Слой**|**Пакеты**
---|---
`1`|`internal/models/*`
`2`|`internal/logic/*`, `internal/logsdb`, `internal/metadb`, `internal/auth`
`3`|`internal/api/*`, `internal/logsdb/<implementation>/*`, `internal/metadb/<implementation>/*`, `internal/auth/<implementation>/*`
`4`|`internal/app.go`

Запрос приходит в обработчик из `internal/api/*`. Обработчик вызывает нужный метод в `internal/logic/*` - там должна быть основная логика обработки (проверка аутентификации, вызовы клиентов к другим сервисам или обращений к хранилищу и т.п.). Непосредственно хождения в БД или работа с сервисами выделена в отдельные пакеты (`internal/metadb/*`, `internal/logsdb/*`, etc). `internal/app.go` связывает всё это вместе через `Dependency Injection`.

Несколько правил вытекающих из описаного выше (которые сами по себе следуют из Clean Architecture):
1. Весь `protocol-specific` код должен быть в `internal/api/<protocol>`. Это значит, что только в этих пакетах можно импортировать `gRPC` пакеты или код сгенерированый из `protobuf` спецификаций. Очевидное исключение - `internal/app.go`, который отвечает за DI.
2. Весь `database-specific` и `externalservice-specific` код должен быть в соответствующих пакетах (`internal/metadb/*`, `internal/logsdb/*`, etc). Пакеты могут предоставлять интерфейсы для каких-то специфичных действий, например начала транзакции, но эти интерфейсы должны быть максимально database-agnostic (к прмиеру, там не может быть типа из `pgx`, т.к. это уже деталь реализации).

##### Ошибки
В общем случае используем [xerrors](https://a.yandex-team.ru/arc/trunk/arcadia/library/go/core/xerrors).

Для gRPC [используется](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/internal/grpcutil/interceptors/chain.go) [конвертер](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/internal/grpcutil/grpcerr/errors.go) ошибок. Примерная логика:
1. Если в стэке ошибок есть [semerr](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/internal/semerr) ошибка, то она используется для кода и текста gRPC ошибки. Используется самая верхняя `semerr` ошибка (ищется через `xerrors.As`).
2. Если в конфиге `mdb-internal-api` значение `expose_error_details` выставлено в `true`, то в details gRPC ошибки будет выведена полная информация об ошибке (`%+v` при форматировании) - все вложеные ошибки, все стэкфреймы и т.п.. Данную опцию следует применять **ТОЛЬКО** для внутренней инсталяции. По умолчанию она выставлена в `false`.
3. Иначе будет `Unknown` код и `Unknown error` сообщение.

Общий подход к [обработке ошибок в API Облака][yc-errors].
Вид возвращаемой ошибки зависит от контекста ([раздел "Статус-коды"][yc-errors]).

Примеры для нашего `API`.

> Возвращать подвиды `FAILED_PRECONDITION` — `NOT_FOUND`, `ALREADY_EXISTS` , `OUT_OF_RANGE` , `PERMISSION_DENIED` — следует только, если ошибка относится к основной сущности, обрабатываемой методом сервиса.

- `*.ClusterService/ListLogs` для несуществующего кластера - это `NOT_FOUND`
- `*.UserService/List` для несуществуещего кластера - это `FAILED_PRECONDITION`

>  Если запрос некорректный независимо от состояния сервиса и его зависимостей, следует возвращать `INVALID_ARGUMENT` ; иначе — `FAILED_PRECONDITION`

- `*.postgresql.v1.UserService/Add` в случае выхода за `max_connections` - это `FAILED_PRECONDITION`, а не `INVALID_ARGUMENT`, тк это зависит от 'состоянения' кластера

##### Добавление новых proto-спек
Чтобы в `a.yandex-team.ru/cloud/mdb/mdb-internal-api` команда `ya make --add-result=pb.go` начала генерировать код по нужной вам спеке, этот код необходимо начать использовать. Например, чтобы генерировать код по спеке лежащей в `a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1` нужно заимпортировать пакет `a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/mdb/kafka/v1`. Спеки генерируются **только** для импортируемых пакетов, никакой рекурсии внутрь директорий.

Заходим в директорию c нужной вам спекой и выполняем `ya make --add-result=pb.go`. Добавляем в `mdb-internal-api` код, использующий любую часть этой спеки. Теперь `ya make --add-result=pb.go` для `mdb-internal-api` будет генерировать подключеные спеки.

### Тестирование

##### Тесты
Код написан так, что все модули приложения изолированы друг от друга интерфейсами. Это позволяет написать тест на любой модуль используя моки нужных интерфейсов.

Рекомендуется писать следующие тесты в порядке приоритета:
1. Юнит-тесты на отдельные методы.
2. Тесты на отдельные функции модулей (на алгоритмы, на функции имеющие какую-то логику и т.п.).
3. Тесты на модули, если в них много логики и мало перекладывания моделек.
4. Функциональные тесты на API в целом.

Не рекомендуется писать тесты на:
1. Ручки gRPC - в большинстве случаев это просто перекладывание из protobuf во внутренние модели и обратно. Если хочется, то лучше написать тест на конвертеры.

##### Запуск mdb-internal-api локально
1. `make genconfig` - это сгенерирует конфиг `mdb-internal-api.yaml`.
2. Из `/srv/pillar/private/dbaas/mdb_internal_api_<installation>.sls` берём адреса и пароль metadb, а так же адреса и пароль logsdb и пишем их в соответствующие поля в сгенерированом конфиге.
3. Меняем значения `metadb.sslrootcert`, `logsdb.ca_file` и `access_service.capath` на правильные.
4. Возможно, меняет значения `expose_error_details`, `logsdb.debug` и `logic.s3_bucket_prefix`.
5. `make run`.

Как минимум для `porto-test` это должно работать из коробки, без дополнительных дырок или чего-то еще.
