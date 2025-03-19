[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=hanging+tasks), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-hanging-tasks)

## Описание
Мониторинг долго исполняемых тасков yc-compute-tasks

## Подробности
Загорается при наличии тасков, исполняющихся дольше ожидаемого.

## Диагностика
- Посмотреть на [график](https://grafana.yandex-team.ru/d/VdSkkchZk/cloud-compute-tasks?orgId=1&from=now-3h&to=now&viewPanel=18)
- Посмотреть список висящих тасков (их ID, тип и ожидаемое время завершения) на голове `sudo yc-compute-tasksctl tasks list-hanging`
- Читать логи по таскам `sudo journalctl TASK_ID=<id>`

## Ссылки
- [Дашборд в Графане](https://grafana.yandex-team.ru/d/VdSkkchZk/cloud-compute-tasks?orgId=1&from=now-3h&to=now)