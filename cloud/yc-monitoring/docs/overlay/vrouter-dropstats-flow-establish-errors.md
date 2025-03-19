[Алерт vrouter-dropstats-flow-establish-errors в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dvrouter-dropstats-flow-establish-errors)

## Что проверяет

Ошибки при установке flow: либо же превышены лимиты или залип flow

## Если загорелось

- Проверить на графике compute node: был ли высоким flow active и кто порождает больше всего flow

  - Возможно это DDoS, триальный скане — подробнее [на вики](https://wiki.yandex-team.ru/cloud/devel/sdn/duty/ddos-detection/) и у `/duty security`

  - Опционально — [включить super-flow-v2](https://wiki.yandex-team.ru/users/sklyaus/super-flows-shop/#superflowsv2)

- (для `ds_flow_queue_limit_exceeded`) Проверить залипшие flow через `yc-contrail-introspect-vrouter flows hold`

- [как интерпретировать dropstats & short flows](https://wiki.yandex-team.ru/cloud/devel/sdn/dropstats/)