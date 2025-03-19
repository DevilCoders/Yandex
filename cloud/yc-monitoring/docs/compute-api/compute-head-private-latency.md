[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=compute+private+API+latency),
[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-head-private-latency)

## Описание
Оповещает о медленных ответах compute private API

## Подробности
В private API ходит сервис compute-node для синхронизации состояния инстансов. Все ручки private API синхронные и внутри ходят в:
 - YDB
 - Access service
 - Disk manager (ручка `assign`)
 - Scheduler (ручка `secondary_allocation`)

Соответственно, большие задержки ответов могут свидетельствовать о проблемах с одним из указанных сервисов, либо о внутренних ошибках.

## Диагностика
- проверяем на ((https://grafana.yandex-team.ru/d/qU8QWBFZz/cloud-compute-api дашборде)) корреляцию с ошибками/задержками внешних сервисов
- смотрим логи:
  - заходим на голову, указанную в алерте
  - фильтруем логи по времени обработки запроса: `journalctl -u yc-compute-head API_ID=private --since=-10m | grep "rt=[1-9][0-9\.]*s"`. В строке лога об ошибке должны отложиться `REQUEST_ID`
  - по `journalctl REQUEST_ID=...`  получаем полную информацию о запросе
  - смотрим, где тормозили, жалуемся виновным

## Ссылки
- График по latency в private API есть на панели "Private API" ((https://grafana.yandex-team.ru/d/qU8QWBFZz/cloud-compute-api дашборда compute API))
