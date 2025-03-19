[Алерт contrail-control в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-control)

## Что проверяет

Живость сервиса `contrail-control` через интроспекцию: подключение к `ifmap`, `discovery`.

## Если загорелось

- `introspect_mon -c contrail-control` — внутренняя диагностика контрейла. Недоступность collector игнорируем

- `safe-restart --force contrail-control` — рестарт

- логи в `/var/log/contrail/contrail-control.log`

- если недоступны ifmap/discovery, то смотрим их статусы и логи