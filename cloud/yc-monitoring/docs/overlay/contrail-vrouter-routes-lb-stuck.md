[Алерт contrail-vrouter-routes-lb-stuck в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-vrouter-routes-lb-stuck)

## Что проверяет

Что LB маршруты для инородного таргета (ri-lb-ri.inet to IPv6, ri-lb-ri-ipv6.inet6 to IPv4) в vrouter модуле указывают на Discard NextHop.

## Если загорелось

- Проверить отработал ли аторекавери vrouter-routes-lb-stuck-autorecovery, если нет, то запустить рекавери командой `yc-autorecovery vrouter-routes-lb-stuck`

- Сделать reattach портов в проблемном сабнете через `yc-contrail-tool port reattach --subnet SUBNET_ID --delay 40 --backup SUBNET_ID.bkp`

