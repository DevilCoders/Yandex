[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=compute-head+memory), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-head-memory)

## Описание
Оповещает о превышения сервисом compute-head порога потребления памяти

## Диагностика
- для профилирования сервис поддерживает интерфейс pprof. Например, `curl [::]:9083/debug/pprof/cmdline`.
