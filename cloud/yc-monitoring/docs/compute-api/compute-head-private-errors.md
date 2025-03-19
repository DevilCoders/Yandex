[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=compute+private+API+errors),
[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-head-private-errors)

## Описание
Оповещает об ошибках в compute private API

## Подробности
В private API ходит сервис compute-node для синхронизации состояния инстансов. Все ручки private API синхронные и внутри ходят в:
 - YDB
 - Access service
 - Disk manager (ручка `assign`)
 - Scheduler (ручка `secondary_allocation`)

Соответственно, ошибки в ответах могут свидетельствовать о проблемах с одним из указанных сервисов, либо о внутренних ошибках.
Некорректные запросы со стороны ноды также могут приводить к возврату ошибок.

## Диагностика
- проверяем на ((https://grafana.yandex-team.ru/d/qU8QWBFZz/cloud-compute-api дашборде)) корреляцию с ошибками клиентов и YDB
- смотрим логи:
  - заходим на голову, указанную в алерте
  - фильтруем логи по времени обработки запроса: `journalctl -u yc-compute-head API_ID=private -p err --since=-10m`. В строке лога об ошибке должны отложиться `REQUEST_ID`
  - по `journalctl REQUEST_ID=...`  получаем полную информацию о запросе
  - смотрим причины ошибок

## Ссылки
- График по ошибкам в private API есть на панели "Private API" ((https://grafana.yandex-team.ru/d/qU8QWBFZz/cloud-compute-api дашборда compute API))
