[Алерт contrail-vrouter-routes-interface в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-vrouter-routes-interface)

## Что проверяет

Что для маршрутов для локально доступного интерфейса отдаются корректные данные

## Если загорелось

- Попробовать переанонсировать маршрут, флапнув локальным портом через `sudo ip link set $TAPNAME down && sleep 3 && sudo ip link set $TAPNAME up`
