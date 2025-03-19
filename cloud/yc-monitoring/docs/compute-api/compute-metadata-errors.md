[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=compute+metadata+errors), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service=compute-metadata-errors)

## Описание
На данный момент алерт приходит только на код 500.

## Диагностика
- заходим на ноду
- `journalctl -u yc-compute-metadata -p err | grep code=500` - в строке лога об ошибке должны отложиться REQUEST_ID и INSTANCE_ID
- по `journalctl REQUEST_ID=...` получаем полную информацию о запросе
- в случае, когда ошибка возникла в самом сервисе, в общем случае рекомендуется откатить сервис на предыдущую стабильную версию: `apt install --allow-downgrades yc-compute-metadata=...`
- в случае, когда ошибка порождена запросом в сторонний сервис (на данный момент это token service, oslogin service и access service, а также token agent) - сообщить детали дежурным по этому сервису
