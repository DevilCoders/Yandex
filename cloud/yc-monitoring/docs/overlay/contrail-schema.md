[Алерт contrail-schema в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-schema)

## Что проверяет

- живость сервиса `contrail-schema` через интроспекцию

- наличие в логе ошибок `Error in reinit`

- корректность JSON-логов

## В активном состоянии работает только на одной голове из 5. На неактивных горит красным: contrail-schema = Can't get status for service (127.0.0.1:8087), это нормально.

## Если загорелось

- `introspect_mon -c contrail-schema`

- `systemctl status contrail-schema`

- логи в `/var/log/contrail/contrail-schema.log`

- `safe-restart --force contrail-schema` — рестарт