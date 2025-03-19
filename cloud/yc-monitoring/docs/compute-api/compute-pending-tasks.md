[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=pending), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-pending-tasks)

## Описание
Мониторинг очереди тасков yc-compute-tasks

## Подробности
Загорается при разрастании очереди тасков на исполнение.

## Диагностика
- Посмотреть на [график](https://grafana.yandex-team.ru/d/VdSkkchZk/cloud-compute-tasks?viewPanel=12&orgId=1)
- Проверить ожидающие таски на голове `sudo yc-compute-tasksctl tasks list-pending` и сравнить список с `list-processing`
- Читать логи по операциям с ошибками `sudo journalctl TASK_ID=<id>`

## Ссылки
- [Дашборд в Графане](https://grafana.yandex-team.ru/d/VdSkkchZk/cloud-compute-tasks?orgId=1)