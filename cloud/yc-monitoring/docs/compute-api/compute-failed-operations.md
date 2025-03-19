[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=compute+failed+op), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-failed-operations)

## Описание
Мониторинг частоты падающих операций yc-compute-tasks

## Подробности
Загорается при наличии падающих с критической ошибкой операций в 5-минутном окне. Критическими считаются следующие типы ошибок: `Unknown|DeadlineExceeded|Unimplemented|Internal|Unavailable|DataLoss`

## Диагностика
- Посмотреть на [график](https://grafana.yandex-team.ru/d/VdSkkchZk/cloud-compute-tasks?viewPanel=2&orgId=1&from=now-3h&to=now)
- Проверить свежие операции на голове `sudo yc-compute-tasksctl operations list-processing` на предмет ошибок
- Читать логи по операциям с ошибками `sudo journalctl OPERATION_ID=<id>`

## Ссылки
- [Дашборд в Графане](https://grafana.yandex-team.ru/d/VdSkkchZk/cloud-compute-tasks?orgId=1&from=now-3h&to=now)