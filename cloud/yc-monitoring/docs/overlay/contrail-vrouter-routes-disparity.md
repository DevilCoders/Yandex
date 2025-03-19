[Алерт contrail-vrouter-routes-disparity в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-vrouter-routes-disparity)

## Что проверяет

Что для маршрутов, пересылаемых через control оба control отдают одинаковые данные

## Если загорелось

- Возможно загорелось вместе с `contrail-control-routes-divergence` - тогда чинить его

- Посмотреть сломанный маршрут через `yc-contrail-introspect-vrouter vrf rt -A -4|-6 $VRF $ROUTE` и сделать выводы о сломанности контрола. Диагностировать сломанный контрол

- Если на control всё нормально, починить сломанные порты через `sudo yc-contrail-tool port reattach --subnet $VRF --backup $VRF.json --delay=5`
