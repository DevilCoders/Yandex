# Метрики тестов

Метрики тестов поставляются в общедоступный кликхаус и доступны через [YQL](https://yql.yandex-team.ru/)
```
use ci_public;
```

## Схема таблицы `autocheck.metrics`

```
    `branch` String CODEC(LZ4),
    `path` String CODEC(LZ4),
    `test_name` String CODEC(LZ4),
    `subtest_name` String CODEC(LZ4),
    `toolchain` String CODEC(LZ4),
    `metric_name` String CODEC(LZ4),
    `date` DateTime,
    `revision` UInt64,
    `result_type` String CODEC(LZ4),
    `value` Float64,
    `test_status` String CODEC(LZ4),
    `check_id` UInt64,
    `suite_id` UInt64 CODEC(Delta(8), LZ4),
    `test_id` UInt64 CODEC(Delta(8), LZ4)

```

```
        PARTITION BY toMonth(date)
        ORDER BY (branch, path, test_name, subtest_name, toolchain, metric_name, date, revision)
        TTL date + toIntervalDay(180)
```


## Примеры

[Пример запроса в YQL](https://yql.yandex-team.ru/Operations/Ymq2qbq3kwZYoT9yTBl7UWgFTS1-2kth9EbESosQ8o4=)
