# Утилита piper-control

На хостах пайпера установлена утилита [piper-control](https://a.yandex-team.ru/arc_vcs/cloud/billing/go/deploy/packer/piper/files/piper-control),
позволяющая быстрым способом получить информацию о состоянии процесса или перезапустить контейнер.

Утилита должна запускаться из-под root или sudo.

Доступны следующие команды:
## piper-control inspect
Выполняет `docker inspect` для контейнера пайпера, выводит информацию о контейнере.
Наиболее полезная информация — подключенные volume'ы, до них реально достучаться в файловой системе хоста.

## piper-control restart
Перезапускает контейнер пайпера.

## piper-control shell
Запускает оболочку командной строки внутри контейнера пайпера.

## piper-control stats
Общение с ручками stats сервера, алиас к курлу /stats.
Если запустить без дополнительных параметров, то будут перечислены подпути, какие есть.

Команда имеет параметры:
### piper-control stats metrics
Выводит текущие значения метрик в формате Prometheus.

### piper-control stats ping
Запускает HealthCheck'и и выводит результаты.
Удобно смотреть вывод вместе с `jq`:

`piper-control stats ping | jq`

Можно также вывести информацию по какой-то конкретной проверке: `piper-control stats ping/<name>`.

Например: `piper-control stats ping/iam`.

### piper-control stats juggler
Выводит результаты HealthCheck'ов в формате для juggler'а.

### piper-control stats stats
Позволяет получить статистику по разным провайдерам статистики.

```shell
# piper-control stats stats/resharder-lbreader:yc/preprod/billing-mk8s-masters:0 | jq
{
  "Status": "running",
  "InflyMessages": 0,
  "ReadMessages": 0,
  "HandleCalls": 0,
  "Handled": 0,
  "HandleFailures": 0,
  "HandleCancellations": 0,
  "Locks": 390,
  "ActiveLocks": 0,
  "LastEventAt": "2022-02-16T08:34:25.431539773Z",
  "Restarts": 250,
  "Suspends": 10,
  "ReaderStats": {
    "MemUsage": 0,
    "InflightCount": 0,
    "WaitAckCount": 0,
    "BytesExtracted": 0,
    "BytesRead": 0,
    "SessionID": "/yc/preprod/rt-resharder-pollster_246_191338_13502069156211818978"
  }
}

# piper-control stats stats/ydb | jq
{
  "DB": {
    "MaxOpenConnections": 64,
    "OpenConnections": 2,
    "InUse": 0,
    "Idle": 2,
    "WaitCount": 0,
    "WaitDuration": 0,
    "MaxIdleClosed": 0,
    "MaxIdleTimeClosed": 0,
    "MaxLifetimeClosed": 343
  },
  "Direct": {
    "Idle": 0,
    "Index": 3,
    "WaitQ": 0,
    "MinSize": 10,
    "MaxSize": 8,
    "CreateInProgress": 0,
    "Ready": 0,
    "BusyCheck": 0
  }
}
```

Список доступных провайдеров можно получить так:
```shell
# piper-control stats stats | jq
{
  "Providers": [
    "resharder-lbreader:yc/preprod/billing-mk8s-masters:0",
    "resharder-lbreader:yc.billing.service-cloud/billing-kms-api:0",
    "resharder-lbreader:yc.billing.service-cloud/billing-cdn:0",
    "resharder-lbreader:yc/preprod/billing-datalens-sessions:0",
    "resharder-lbreader:yc.billing.service-cloud/billing-mdb-instance:0",
    "resharder-lbreader:yc-pre/billing-compute-instance:0",
    "iam-meta",
    "resharder-lbreader:yc.billing.service-cloud/billing-sqs-requests:0",
    "resharder-lbreader:yc/preprod/billing-mkt-product-usage:0",
    "resharder-lbreader:yc/preprod/billing-nlb-traffic:0",
    "resharder-lbreader:yc/preprod/billing-ydbcp-cluster:0",
    "resharder-lbreader:yc.billing.service-cloud/billing-object-storage:0",
    "tvm-2001059",
    "resharder-lbreader:yc-pre/billing-ai-requests:0",
    "resharder-lbreader:yc/preprod/billing-iot-traffic:0",
    "resharder-lbreader:yc/preprod/billing-compute-image-v2:0",
    "resharder-lbreader:yc/preprod/billing-compute-snapshot-v2:0",
    "resharder-lbreader:yc.billing.service-cloud/billing-object-requests:0",
    "resharder-lbreader:yc.billing.service-cloud/billing-dns-zones:0",
    "resharder-lbreader:yc/preprod/billing-ydb-serverless:0",
    "resharder-lbreader:yc.billing.service-cloud/billing-ml-platform:0",
    "resharder-lbreader:yc-pre/billing-sdn-fip-v2:0",
    "resharder-lbreader:yc/preprod/billing-monitoring:0",
    "resharder-lbreader:yc.billing.service-cloud/billing-subscriptions:0",
    "resharder-lbreader:yc/preprod/billing-nlb-balancer-v2:0",
    "resharder-lbreader:yc/preprod/billing-tracker:0",
    "resharder-lbreader:yc.billing.service-cloud/billing-mk8s-masters:0",
    "resharder-lbreader:yc-pre/billing-ml-platform:0",
    "resharder-lbreader:yc/preprod/billing-xmlsearch-requests:0",
    "resharder-lbreader:yc-mdb-pre/billing-mdb-instance:0",
    "resharder-lbreader:yc/preprod/billing-serverless-functions:0",
    "resharder-lbreader:yc/preprod/billing-compute-hostgroup:0",
    "resharder-lbreader:yc/preprod/billing-ms-sql-report:0",
    "resharder-lbreader:yc.billing.service-cloud/billing-sdn-traffic-nfc:0",
    "ydb",
    "resharder-lbreader:yc-pre/billing-api-gateway:0",
    "resharder-lbreader:yc.billing.service-cloud/billing-alb-balancers:0",
    "resharder-lbreader:yc.billing.service-cloud/billing-mdb-mssql:0"
  ]
}
```

## piper-control profile

Способ получить различные профиллировочные данные.

см. [https://pkg.go.dev/runtime/pprof](https://h.yandex-team.ru?https://pkg.go.dev/runtime/pprof)

Пример вызова: `piper-control profile allocs?seconds=10 > allocs.prof`
