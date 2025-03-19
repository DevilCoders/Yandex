[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=API+latency), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-head-latency)

## Описание
Оповещает о долгих ответах API

## Диагностика
- заходим на голову, указанную в алерте
- фильтруем логи по времени обработки запроса: `journalctl -u yc-compute-head --since=-10m | grep "rt=[1-9][0-9\.]*s"`. В строке лога об ошибке должны отложиться `REQUEST_ID`
- по `journalctl REQUEST_ID=...`  получаем полную информацию о запросе
- смотрим, где тормозили, жалуемся виновным
