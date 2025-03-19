[Алерт oct-cassandra-read-latency в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Doct-cassandra-read-latency)

## Что проверяет

Что 99.9-ый перцентиль времени ответа на запросы чтения из кассандры не превышает одной секунды для прода и двух секунд для препрода.

Незвонящий.

## Если загорелось

- выполните [чистку tombstones](https://wiki.yandex-team.ru/cloud/devel/sdn/duty/cassandra-tombstones-accumulation/)

- если не помогло, проанализируйте LA на машинах, кол-во запросов в кассандру (может их стало резко больше?), потребление кассандрой CPU,

- если зашли в тупик, позовите [simonov-d](https://staff.yandex-team.ru/simonov-d)