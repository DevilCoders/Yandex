[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=compute+metadata+latency), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service=compute-metadata-latency)

## Описание
Оповещает о медленных ответах сервиса метаданных

## Диагностика
- заходим на ноду
- `journalctl -u yc-compute-metadata --since=-10m | grep "rt=[1-9][0-9\.]*s"`  - фильтруем логи по времени обработки запроса - (учитываем, что данное время включает long polling - обычно 30 секунд - возможны попадания лишних строк, можно дополнительно отфильтровать `grep -v "rt=30"` ). В строке лога об ошибке должны отложиться `REQUEST_ID` и `INSTANCE_ID`
- по `journalctl REQUEST_ID=...`  получаем полную информацию о запросе
- в случае, когда ошибка возникла в самом сервисе, в общем случае рекомендуется откатить сервис на предыдущую стабильную версию: `apt install --allow-downgrades yc-compute-metadata=...` 
- в случае, когда ошибка порождена запросом в сторонний сервис (на данный момент это token service, oslogin service и access service, а также token agent) - сообщить детали дежурным по этому сервису
