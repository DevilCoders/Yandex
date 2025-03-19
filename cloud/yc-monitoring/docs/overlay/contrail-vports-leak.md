[Алерт contrail-vports-leak в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-vports-leak)

## Что проверяет

Что количество портов, воткнутых в vrouter, не превышает разумные пределы (300).

## Если загорелось

Проверить, сколько интерфейсов действительно есть в compute:
```
sudo yc-compute-node-admin network interfaces list
```

- Если количество интерфейсов в compute больше лимита, править мониторинг.

- Если меньше, искать потерянные интерфейсы в `/var/lib/contrail*/ports/` и удалять их,
после чего сделать `yc-contrail-tool port detach` на их uuid.

