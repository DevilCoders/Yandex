[Алерт contrail-vports-super-flow-v2 в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-vports-super-flow-v2)

## Что проверяет

Наличие портов с некорректными метаданными `super-flow-v2`.

)).

## Если загорелось

1. Жёлтым — не опасно, можно прикопать дамп из `/var/lib/yc/yc-contrail-monitor-vrouter` в тикет. 

2. Красным — возможно, сломан датаплей — нужно отключить super-flow-v2 у affected VM 

```
sudo yc-compute-admin network feature-flags network-interface-attachment delete --network-interface-attachment-key $INSTANCEID-0  --zone-id $ZONE --flag ecmp-hash-each-packet --flag super-flow-v2 [--apply]
```