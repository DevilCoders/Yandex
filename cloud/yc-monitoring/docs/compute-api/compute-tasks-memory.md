[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=compute-tasks+memory), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-tasks-memory)

## Описание
Оповещает о превышения сервисом compute-tasks порога потребления памяти

## Диагностика
- для профилирования сервис поддерживает интерфейс pprof. Например, `curl [::]:9283/debug/pprof/cmdline`.
