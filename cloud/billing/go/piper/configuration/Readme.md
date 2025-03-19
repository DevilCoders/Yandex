# Конфигурация

Тут располагаются файлы конфигурации для всех сред при деплое piper.
Конкретная конфигурация определяется или из переменной окружения `INSTALLATION` или из примонтированного файла `/installation`

После определения инсталляции в каталог `/config` в контейнере копируется содержимое соотвтетвующего каталога из `/config-sources` (в образ копируется из `./configs`)

## Структура конфигурации среды

Для каждой среды структура конфигурации представляется следующей:
 - `jaeger.yaml` - конфигурация jaeger-agent
 - `unified-agent-conf.d/*.yaml` - конфигурация unified-agent
 - `piper/*.yaml` - конфигурация сервиса piper

## Конфигурация jaeger

Для разных сред должна различаться только настройками хоста репортера. Остальное можно оставлять как есть, для `piper` критично наличие `jaeger-compact` процессора и согласованность порта с его настройками.

## Конфигурация unified-agent

Документация по конфигурации: https://logbroker.yandex-team.ru/docs/unified_agent/configuration

Для разных сред надо актуализировать вот эту часть конфига записи логов:
```(yaml)
output:
  plugin: logbroker
  config:
    endpoint: <logbroker installation endpoint>
    port: <logbroker installation port>
    topic: <logs topic>
    database: <logbroker installation database>
    codec: gzip
    iam: # auth for installation
        cloud_meta: {}
    use_ssl: {}
```

## Конфигурация piper

Основные секции конфигурации:
- lockbox - настройки загрузки конфигов из lockbox
- log - настройки логирования
- trace - настройки распределенной трассировки (jaeger)
- tls - настройки tls (используютя далее в подключениях)
- ydb - настройки подключения к ydb
- iam_meta - настройки получения токенов iam из метаданных виртуальной машины
- jwt - настройки получения токенов iam через jwt
- tvm - настройки tvm аутентификации
- status_server - настройки http сервера для получения статистики работы сервиса
- resource_manager - настройки сервиса менеджера ресурсов IAM
- team_integration - настройки сервиса team integration
- logbroker_installations - настройки используемых инсталляций logbroker
- resharder - обработчики resharder
- dump - обработчик записи данных из топика в базы

### Описание обработчиков resharder
```(yaml)
logbroker_installation:
  cloud:
    host: lb.etn03iai600jur7pipla.ydb.mdb.yandexcloud.net
    port: 2135
    database: /global/b1gvcqr959dbmi1jltep/etn03iai600jur7pipla
    auth:
        type: iam-metadata
resharder:
  sink:
    logbroker:
      installation: cloud
      topic: yc.billing.service-cloud/resharder-output
      partitions: 50
    logbroker_errors:
      installation: cloud
      topic: yc.billing.service-cloud/resharder-errors
      partitions: 25
  source:
    some-incoming-topic:
      logbroker:
        installation: cloud
        consumer: rt-resharder-polster
        topic: some-incoming-topic
        max_messages: 1000
        max_size: 50mib
        lag: 288h
        batch_limit: 1000
        batch_timeout: 15s
      parallel: 2
      handler: general
      params:
        chunk_size: 50mib
        metric_lifetime: 288h
```

### Описание обработчиков dumper
```(yaml)
logbroker_installation:
  cloud:
    host: lb.etn03iai600jur7pipla.ydb.mdb.yandexcloud.net
    port: 2135
    database: /global/b1gvcqr959dbmi1jltep/etn03iai600jur7pipla
    auth:
        type: iam-metadata
dump:
  source:
    resharder-errors:
        logbroker:
            installation: cloud
            consumer: rt-resharder-errors-polster
            topic: yc.billing.service-cloud/resharder-errors
            max_messages: 1000
            max_size: 50mib
            lag: 288h
            batch_limit: 1000
            batch_timeout: 15s
        handler: dumpErrors:ydb
