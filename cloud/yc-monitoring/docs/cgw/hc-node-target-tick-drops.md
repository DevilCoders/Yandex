# hc-node-target-tick-drops
[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts/yandexcloud-prod_target_tick_drops_healthcheck_node),
[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dhc-node-target-tick-drops)

## Подробности
- hc-node не успевает проверять таргеты и обновлять статусы таргетов.

## Воздействие
- target'ы не могут поменять здоровье, здоровые таргеты не вводятся в балансировку, больные не выводятся.

## Что делать
1. Посмотреть графики [LoadAverage](https://solomon.cloud.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod&service=sys&l.host=hc-node-*&l.path=%2FProc%2FLoadAverage1min&graph=auto) и [System/IdleTime](https://solomon.cloud.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod&service=sys&l.host=hc-node-*&l.path=%2FSystem%2FIdleTime&graph=auto) и прочие - возможно, кончился CPU/IO. Если так, то надо переналивать hc-node - либо добавлять ещё одну ноду, либо добавлять ресурсы существующим.
1. Почитать логи hc-node на предмет ошибок:
```
➜  ~ pssh logshatter-rc1c-01.svc.cloud.yandex.net
(PROD)elantsev@logshatter-rc1c-01:~$ sudo -i
(PROD) root@logshatter-rc1c-01:~# clickhouse-client
rc1c-su2bacmh5dd9glr3.mdb.yandexcloud.net :) select datetime, hostname, message, error from ylb_prod_stable_logs where date = today() and unit = 'yc-healthcheck-node.service' and error != '' and message not in ('tcp check complete', 'http check complete')
```

## Ссылки
- Родственный аларм: [hc-node-tick-drops](https://docs.yandex-team.ru/yc-monitoring/cgw/hc-node-tick-drops)
