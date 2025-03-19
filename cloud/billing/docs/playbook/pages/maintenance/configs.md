# Конфигурирование piper

## Основные принципы
piper позволяет комплексно управлять конфигурацией путем последовательной загрузки из нескольких источников.
При этом разные источники могут быть опциональными и зависеть от окружения развертывания.

Источники конфигукрации разделяются на 2 типа:
- **Структурированные данные**
  Загружаются как данные, сериализованные в YAML. Позволяют определять весь спектр настроек, в том числе с динамическими ключами (например, map источников resharder)
- **KV данные**
  Загружаются как набор key-value пар и служат для переопределения уже загруженных структурированных данных. С помощью kv невозможно добавить "динамический ключ" конфигурации

Загрузка по типам источников производится следующим образом:
- Сначала в структуру конфигурации производится последовательная десериарилизация yaml. При этом следующие значения добавляются через библиотеку объединения структур для корректного объединения динамических ключей
- По загруженной структуре вычисляются "пути" к каждому значения конфигурации. Например:
   ```
   resharder:
     source:
       My/pretty.source:
         installation: prod
   ```
  будет иметь ключ `resharder.source.My/pretty.source.installation`. Т.е. вся иерархия ключей yaml собирается через '.'.
- По вычисленным путям в текущей конфигурации производится поиск в загруженных KV источниках и если обнаруживается полное совпадение, то значение переопределяется.

## Последовательность загрузки источников конфигурации
Загрузка локальных источников, позволяет загружать любые значения:
1. yaml файлы, переданные в ключах `--config` при запуске процесса (поддерживают wildcard, надо следить чтобы bash не развернул до вызова - экранировать)
2. env переменные по полному соответсвию "пути" к значению и по преобразованию в upper case и замене '-' на '_' в пути

Загрузка сетевых источников:
1. Загрузка структурированной конфигурации из lockbox (требует чтобы в уже загруженном конфиге были определены ключи `lockbox.*`).
2. Загрузка kv информации из lockbox
3. Загрузка kv информации из YDB

## lockbox
Загрузка из lockbox возможна только при запуске на VM облака.

Если включена загрузка в фалах конфигурации, то производится следующая последовательность действий:
- в метаданных VM производится поиск ключа `lockbox-secret-id`
- из lockbox загружается актуальная версия секрета
- если в секрете есть ключ `yaml`, то он используется как структурированные данные
- из остальных ключей выбираются все с префиксом "piper." и используются как KV конфиг

Из этого источника можно загрузить вcе части конфигурации, кроме управляющих загрузкой данных из lockbox

## YDB
Загрузка из YDB производится как KV конфиг из таблицы `utility/context` с префиксом "piper."
Все загруженные данные применяются к уже загруженному ранее конфигу.

Из этого источника нельзя переопределять базовые настройки (уровень логирования, подключение к YDB и т.д.).

## Файлы конфигурации
Файлы конфигурации располагаются в папке [piper/configuration](https://a.yandex-team.ru/arc_vcs/cloud/billing/go/piper/configuration).
Там же есть Readme-файл с подробным описанием структуры конфигурации и полей.

### Окружения
На машинах, в файле `/etc/yc/installation` находится название среды, конфиги для которой будут применены при запуске.
В папке [piper/configuration/configs](https://a.yandex-team.ru/arc_vcs/cloud/billing/go/piper/configuration/configs) заготовлены конфиги для разных окружений.
Есть при этом две специальные папки:
- common — эти конфиги будут применены ко всем средам
- default — эти конфиги будут использованы, если нет файла `/etc/yc/installation` или он содержит некорректное значение (не соответствующее ни одной из подпапок в [piper/configuration/configs](https://a.yandex-team.ru/arc_vcs/cloud/billing/go/piper/configuration/configs)).

## Подключение конфигов к образу
До запуска основного контейнера пайпера происходит запуск контейнера `piper-config` (описан в секции `initContainers` [pod-файла](https://a.yandex-team.ru/arc_vcs/cloud/billing/go/deploy/packer/piper/files/piper.pod.tmpl)).
В этом контейнере выполняется [скрипт](https://a.yandex-team.ru/arc_vcs/cloud/billing/go/piper/configuration/scripts/entrypoint.sh), который, опираясь на значение в файле `/etc/yc/installation` хоста, собирает в одной папке yaml-файлы с конфигурацией для данного окружения.

Эта папка в итоге подключается как том `/configs` к основному контейнеру пайпера.

### /local-config {#local-config}
Также к контейнеру пайпера подключается в `/local-config` папка хоста `/etc/yc/billing/local-config` (по умолчанию — пустая). Это даёт возможность сохранить там временные переопределеления параметров.
{% note info %}

После изменения локальных конфигов нужно зайти на машины и перезапустить piper.
[{#T}](maintenance.md#svmlogin) | [Утилита piper-control](piper-control.md)

{% endnote %}

## Процесс загрузки конфигурации
Конфиги загружаются через `https://github.com/heetch/confita`.
В терминах `confita` источники данных для конфигурации называется `backend`.
Файлы конфигурации последовательно загружаются через `yaml-backend`'ы, и с помощью `github.com/imdario/mergo` вливаются в одну структуру. Таким образом, загруженные позже файлы переопределяют значения параметров, загруженных раньше.

{% note info %}

Команда `piper` запускается в контейнере с параметрами `-c /config/piper/*.yaml -c /local-config/*.yaml`, это и даёт возможность преопределения через [{#T}](configs.md#local-config).

{% endnote %}

## show-config
Есть возможность увидеть текущий конфиг. Для этого надо зайти в консоль контейнера и запустить `piper` с параметром `show-config`:
```
# piper-control shell
root@piper-vla1:/# /usr/bin/piper show-config -c /config/piper/*.yaml

log:
  level: info
  paths:
  - stderr
trace:
  queue_size: 2048
  local_agent_hostport: ""
tls:
  ca_path: /etc/ssl/certs/ca-certificates.crt
ydb:
  address: ydb-billing.cloud-preprod.yandex.net:2136
  database: /pre-prod_global/billing
  root: hardware/default/billing/
  auth:
    type: iam-metadata
  connect_timeout: "5"
  max_connections: 64
  max_idle_connections: 64
  max_direct_connections: 8
  conn_max_lifetime: "600"
  installations:
    uniq:
      address: ydb-billing.cloud-preprod.yandex.net:2136
      database: /pre-prod_global/billing
      root: hardware/default/billing/
      auth:
        type: iam-metadata
      connect_timeout: "5"
      max_connections: 64
      max_idle_connections: 64
      max_direct_connections: 8
      conn_max_lifetime: "600"
tvm:
  client_id: "2000508"
status_server:
  port: 9741
resource_manager:
  endpoint: rm.private-api.cloud-preprod.yandex.net:4284
  auth:
    type: iam-metadata
team_integration:
  endpoint: ""
  auth:
    type: jwt
unified_agent:
  solomon_metrics_port: 0
  health_check_port: 0
logbroker_installations:
  lbk:
    host: vla.logbroker.yandex.net
    port: 2135
    database: /Root
    disable_tls: "true"
    auth:
      type: tvm
      tvm_destination: 2001059
    resharder_consumer: /yc/preprod/rt-resharder-pollster
  lbkx:
    host: lbkx.logbroker.yandex.net
    port: 2136
    database: /Root
    auth:
      type: tvm
      tvm_destination: 2001059
    resharder_consumer: /yc/preprod/rt-resharder-pollster
  yc_preprod:
    host: lb.cc8035oc71oh9um52mv3.ydb.mdb.cloud-preprod.yandex.net
    port: 2135
    database: /pre-prod_global/aoeb66ftj1tbt1b2eimn/cc8035oc71oh9um52mv3
    auth:
      type: iam-metadata
    resharder_consumer: /yc.billing.service-cloud/rt-resharder-pollster
clickhouse:
  database: ""
  port: 9000
  auth: {}
  max_connections: 4
  max_idle_connections: 4
  conn_max_lifetime: "600"
  shards: []
resharder:
  disabled: "false"
  enable_team_integration: "false"
  enable_deduplication: "false"
  enable_e2e_reporting: "false"
  sink:
    logbroker:
      enabled: "true"
      installation: yc_preprod
      topic: /yc.billing.service-cloud/rt/billing-resharder-output
      partitions: 10
      route: source_metrics
      max_parallel: 32
      split_size: 50MiB
      compression:
        disabled: "false"
        level: 0
    logbroker_errors:
      enabled: "true"
      installation: yc_preprod
      topic: /yc.billing.service-cloud/rt/billing-resharder-errors
      partitions: 1
      route: invalid_metrics
      max_parallel: 32
      split_size: 50MiB
      compression:
        disabled: "false"
        level: 0
    ydb_errors:
      enabled: "true"
  source: {}
  metrics_grace: "43200"
  write_limit: 500000
dump:
  disabled: "false"
  sink:
    ydb_errors:
      enabled: "true"
    ch_errors:
      enabled: "true"
  source: {}
```
