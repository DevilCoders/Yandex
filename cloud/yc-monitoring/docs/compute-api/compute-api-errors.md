[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=alert+on+Compute+api+errors+), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-head-errors)

## Описание
Мониторинг частоты критических ошибок API

## Подробности
Загорается при всплеске ошибок на compute-head'ах. Критическими считаются следующие типы ошибок: `Unknown|DeadlineExceeded|Unimplemented|Internal|Unavailable|DataLoss`

## Диагностика
- Посмотреть на [график](https://grafana.yandex-team.ru/d/qU8QWBFZz/cloud-compute-api?viewPanel=4&orgId=1)
- Проверить свежие операции на голове `sudo yc-compute-tasksctl operations list-completed` на предмет ошибок
- Читать логи по операциям с ошибками `sudo journalctl OPERATION_ID=<id>`

## Ссылки
- [Дашборд в Графане](https://grafana.yandex-team.ru/d/qU8QWBFZz/cloud-compute-api?orgId=1)