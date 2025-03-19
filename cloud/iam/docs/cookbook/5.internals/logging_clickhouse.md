## Зачем нужны логи в Clickhouse

Логи до YT доезжают примерно за полтора-два часа. Часто хочется посмотреть результат выполнения только что запущенного
запроса, а искать его в логах непосредственно на машине нельзя из-за недостатка прав и сложности процесса в целом
(нужно знать группу машин, пути к логам и их формат, из-за ограниченности размера дисков логи могут быстро стираться
и т.п.)

## Как это устроено

{% note info %}

Пока у DataTransfer и Logbroker нет поддержки Terraform, конфигурация проводится через UI.

{% endnote %}

### Конфигурация Logbroker

1. В [интерфейсе Logbroker](https://logbroker.cloud.yandex.ru/yc-logbroker/accounts) находим свой аккаунт с логами.
2. Создаем новый ресурс Consumer: Supported Codecs: raw, lzop, gzip, zstd; Limits Mode: wait.
3. В конфигурации Consumer'а создаем новый Read Rule: Topic: топик с логами; Type: All original.
4. В конфигурации Consumer'а добавляем ACL для субъекта ajet5egu65m66slf9tn0 (сервисный аккаунт DataTransfer);
   Permissions: ReadTopic.
5. В конфигурации Topic'а добавляем ACL для субъекта ajet5egu65m66slf9tn0 (сервисный аккаунт DataTransfer);
   Permissions: ReadTopic.

### Конфигурация кластера Clickhouse

{% note info %}

Опытным путем установлено, что для потока логов в 100Мб/сек без репликации хватает кластера s2.small с диском 170Гб и не
хватает s2.micro.

{% endnote %}

{% note info %}

[Кластер с логами прода Облака](https://console.cloud.yandex.ru/folders/b1gpagj693mc3m3krqg3/managed-clickhouse/cluster/c9qm3hq1a1rchepfsokt)
находится в каталоге logs системного облака yc.iam.service-cloud.

{% endnote %}

При создании кластера ставим галочку Управление пользователями через SQL. В качестве сети использована [сеть для
внутренних тулов команды IAM](/).

В кластере создаем таблицу под логи, пример для access_log:
```
CREATE TABLE logs.access_log (
  `request_id` Nullable(String),
  `type` Nullable(String),
  `authority` Nullable(String),
  `app` Nullable(String),
  `request_uri` Nullable(String),
  `unixtime` Nullable(UInt32) CODEC(DoubleDelta, ZSTD),
  `duration` Nullable(Int32),
  `grpc_service` Nullable(String),
  `grpc_method` Nullable(String),
  `grpc_status_code` Nullable(Int8),
  `remote_ip` Nullable(String),
  `user_agent` Nullable(String),
  `remote_calls_count` Nullable(UInt16),
  `remote_calls_duration` Nullable(Float64),
  `request` Nullable(String),
  `response` Nullable(String),
  `subject_info` Nullable(String),
  `_rest` Nullable(String),
  `_timestamp` DateTime,
  `_partition` String,
  `_offset` UInt64 CODEC(Delta, ZSTD),
  `_idx` UInt32 CODEC(DoubleDelta, ZSTD),
  
  INDEX request_id_idx (request_id) TYPE bloom_filter GRANULARITY 3
)
ENGINE = MergeTree
ORDER BY (_timestamp, _partition, _offset, _idx)
TTL _timestamp + INTERVAL 3 HOUR DELETE
SETTINGS index_granularity = 8192
```

Создаем пользователя с правами на запись в эту таблицу.

```
CREATE USER logs IDENTIFIED WITH plaintext_password BY '<password>';
GRANT INSERT ON logs.access_log TO logs;
GRANT SHOW ON logs.access_log TO logs;
```

### Конфигурация Data Transfer

1. Просим у дежурного Data Transfer выдать альфа-флаг для доступа к облачному логброкеру.
2. Создаем эндпоинт-источник с типом LOGBROKER_V2.
3. Указываем кластер yc-logbroker, консьюмер созданный при конфигурации логброкера, топик, на который в консьюмере
   создан Read Rule.
4. Описываем поля в универсальном парсере с форматом данных JSON, так чтобы они совпадали по названию и типам с
   созданной таблицей (поля с префиксами добавятся автоматически).
5. Добавляем колонку, содержащую дату-время. Для Java-логов правило парсинга даты 2006-01-02T15:04:05.999999999Z.
6. Включаем галочку Продолжать работу при превышении по TTL.
7. Создаем эндпоинт-приемник, где выбираем созданный CH-кластер.
8. Включаем в нем переопределение имен таблиц из `<logbroker account name>_<logbroker topic name>` в созданную в кластере
   таблицу (без имени базы в начале).
9. Создаем и активируем новый трансфер, используя созданные приемник и источник.

{% note alert %}

В полях, составляющих первичный ключ, не должно быть NULL'ов.

{% endnote %}

## Настройка clickhouse-client

Устанавливаем пакет clickhouse-client:
```
$ sudo apt install clickhouse-client
```

Создаем файл config-logs.xml следующего содержания (можно положить его в ~/clickhouse-client/):
```
<config>
    <host>rc1a-tx9lu8sjnxlt9srn.mdb.yandexcloud.net</host>
    <port>9440</port>
    <database>logs</database>
    <user>viewer</user>
    <password>...</password>
    <secure>True</secure>
    <format>PrettyNoEscapes</format>
</config>
```

Пароль для учетной записи viewer лежит в [yav](https://yav.yandex-team.ru/secret/sec-01fe890pmjrs07p9482zetacgh/explore/version/head).

Запросы к базе можно делать через clickhouse-client:
```
$ clickhouse-client --config ~/clickhouse-client/config-logs.xml --query "SELECT * FROM logs.access_log WHERE request_id = '<request-id>'" | less -S
```
