[Алерт contrail-vrouter-routes-default в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-vrouter-routes-default)

## Что проверяет

Что дефолтный маршрут, который должен идти через один из типов клаудгейтов присутствует и является туннелем (ECMP-группой туннелей)

## Если загорелось

- На обычный сабнет с IPv6-CIDR (в проверке горит `inet6`) — проверить что в API есть import_rt,
уточнить почему сеть завели без import_rt у владельцев сети

- На `*:ri-lb-ri`/`*ri-lb-ri-ipv6` - уточнить у /duty lb, почему нет анонса

- На fip bucket (например, `public@ru-central1-a`) - уточнить у /duty cgw, почему нет анонса

При немассовых загораниях (например на одном хосте) и при ошибке disparity (`route announces from controls differ`)
следовать по инструкции [contrail-vrouter-routes-disparity](https://docs.yandex-team.ru/yc-monitoring/overlay/contrail-vrouter-routes-disparity).
