[Алерт contrail-api в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-api)

## Что проверяет

- живость сервиса `contrail-api`: подключение к `ifmap`, `rabbitmq`, `zookeeper`, `contrail-discovery`, `cassandra`

- сообщения `RabbitMQ is connected to non-local endpoint` в логе

- что возвращается `200 OK` на запрос к `/global-system-configs`

## Если загорелось

- `introspect_mon -c contrail-api` — внутренняя диагностика контрейла. Недоступность collector игнорируем

- `safe-restart --force contrail-api` — рестарт, вместе с `contrail-api` рестартанёт `ifmap`

- логи в `/var/log/contrail/contrail-api.log`

- если недоступны ifmap/rabbitmq/zookeeper/discovery/cassandra, то смотрим их статусы и логи